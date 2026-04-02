#include <sstream>

#include <sys/types.h>

#include <crow.h>
#include <crow/middlewares/cookie_parser.h>

#include "db.hpp"
#include "session.hpp"
#include "template_finder.hpp"
#include "util_rand.hpp"

#include "tpl/index.hpp"
#include "tpl/info.hpp"
#include "tpl/filter.hpp"

std::string getenv(const char* name, const char* default_value) {
    if (const char *env = std::getenv(name)) {
        return std::string{env};
    }

    return std::string{default_value};
}

int main() {
    crow::App<crow::CookieParser> app;

    Db db;

    std::string session_cookie_name = getenv("SESSION_COOKIE_NAME", "guestbook_session");

    std::string template_path = template_finder::find_template_path();
    crow::mustache::set_global_base(template_path);

    CROW_LOG_INFO << "main is thread " << gettid();

    CROW_ROUTE(app, "/")([&](const crow::request& req){
        CROW_LOG_INFO << "route running in thread " << gettid();

        auto& ctx = app.get_context<crow::CookieParser>(req);
        Session session{ctx.get_cookie(session_cookie_name)};

        std::string csrf_token = util_rand::rand_long_word();
        session.put_field("csrf_token", csrf_token);
        ctx.set_cookie(session_cookie_name, session.serialize())
            .httponly();

        tpl::Index tpl = {db};
        tpl.csrf_token = csrf_token;

        return tpl.render();
    });


    CROW_ROUTE(app, "/filter")([&](const crow::request& req) {
        auto query_params = req.url_params;

        tpl::Filter::query_dict where = query_params.get_dict("where");
        tpl::Filter::query_dict order_by = query_params.get_dict("order_by");

        tpl::Filter tpl = {db, where, order_by};
        
        return tpl.render();
    });

    CROW_ROUTE(app, "/sign")
        .methods("POST"_method)
        ([&db, &app, &session_cookie_name](const crow::request& req, crow::response& resp){
            auto& ctx = app.get_context<crow::CookieParser>(req);
            Session session{ctx.get_cookie(session_cookie_name)};

            if (!session.has_field("csrf_token")) {
                resp.code = 403;
                resp.end();
                return;
            }

            if (req.body.size() > 4096) {
                resp.code = 400;
                resp.end();
                return;
            }

            auto params = req.get_body_params();

            std::string csrf_token = params.get("csrf_token");
            if (session.get_field("csrf_token").value() != csrf_token) {
                resp.code = 403;
                resp.end();
                return;
            }

            std::string name = params.get("name");
            std::string email = params.get("email");
            std::string message = params.get("message");

            DbStatement stmt = db.prepare("INSERT INTO guests (name, email, message) VALUES (?, ?, ?)");
            sqlite3_bind_text(stmt.stmt, 1, name.c_str(), name.size(), SQLITE_STATIC);
            sqlite3_bind_text(stmt.stmt, 2, email.c_str(), email.size(), SQLITE_STATIC);
            sqlite3_bind_text(stmt.stmt, 3, message.c_str(), message.size(), SQLITE_STATIC);

            int rc = sqlite3_step(stmt.stmt);
            if (rc != SQLITE_DONE) {
                CROW_LOG_ERROR << "error inserting guest: " << sqlite3_errmsg(db.db);
                resp.code = 500;
                resp.end();
                return;
            }

            session.delete_field("csrf_token");

            session.put_field("signed_guestbook_name", name);

            ctx.set_cookie(session_cookie_name, session.serialize())
                .httponly();

            resp.add_header("Location", "/");

            resp.code = 303;
            resp.end();
        });

    CROW_ROUTE(app, "/schema")([&db](const crow::request& req, crow::response& resp){
        std::ostringstream buf;
        buf << "<!doctype html><html><head><title>Schema</title></head><body>";

        DbStatement stmt = db.prepare("SELECT name, type, sql FROM sqlite_master");

        buf << "<table><thead><tr><th>name</th><th>type</th><th>sql</th></tr></thead><tbody>";

        bool more_rows = true;

        while(more_rows) {
            int rc = sqlite3_step(stmt.stmt);
            switch (rc) {
                case SQLITE_DONE:
                    more_rows = false;
                    break;
                case SQLITE_ROW:
                    buf << "<tr><td>" << sqlite3_column_text(stmt.stmt, 0) << "</td>";
                    buf << "<td>" << sqlite3_column_text(stmt.stmt, 1) << "</td>";
                    buf << "<td><pre>" << sqlite3_column_text(stmt.stmt, 2) << "</pre></td></tr>";
                    break;
                case SQLITE_BUSY:
                    CROW_LOG_ERROR << "busy stepping schema query";
                    more_rows = false;
                    break;
                case SQLITE_ERROR:
                    CROW_LOG_ERROR << "error stepping schema query: " << sqlite3_errmsg(db.db);
                    more_rows = false;
                    break;
                case SQLITE_MISUSE:
                    CROW_LOG_ERROR << "misuse stepping schema query: " << sqlite3_errmsg(db.db);
                    more_rows = false;
                    break;
            }
        }

        buf << "</tbody></table></body></html>";

        resp.set_header("Content-Type", "text/html");

        resp.write(buf.str());
        resp.end();
    });

    app.port(8080).run();
}
