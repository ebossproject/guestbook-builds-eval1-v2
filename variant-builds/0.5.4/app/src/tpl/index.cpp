#include "index.hpp"

#include "../assert.hpp"

#include <functional>
#include <vector>

#include <crow/json.h>
#include <crow/logging.h>
#include <crow/mustache.h>

using namespace tpl;

namespace
{
    crow::json::wvalue select_into_json(Db &db)
    {
        DbStatement stmt = db.prepare(
            "SELECT \n"
            "  name, email, message, created_at\n"
            "  FROM guests\n"
            "  ORDER BY id DESC");

        assert(sqlite3_column_count(stmt.stmt) == 4);

        std::vector<crow::json::wvalue> result;

        stmt.with_row([&result, &stmt]()
                      { result.push_back({{{"name", std::string((const char *)sqlite3_column_text(stmt.stmt, 0))},
                                           {"email", std::string((const char *)sqlite3_column_text(stmt.stmt, 1))},
                                           {"message", std::string((const char *)sqlite3_column_text(stmt.stmt, 2))},
                                           {"created_at", std::string((const char *)sqlite3_column_text(stmt.stmt, 3))}}}); });

        CROW_LOG_INFO << "Loaded " << result.size() << " guestbook entries";

        return {result};
    }
}
::crow::response Index::render()
{
    auto t = crow::mustache::load("index.html.mustache");

    crow::mustache::context ctx;
    ctx["csrf_token"] = csrf_token;
    ctx["guestbook"] = select_into_json(db);
    return {t.render(ctx)};
}
