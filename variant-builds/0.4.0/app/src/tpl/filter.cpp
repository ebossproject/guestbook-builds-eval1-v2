#include "filter.hpp"

#include "../assert.hpp"

#include <cctype>
#include <functional>
#include <optional>
#include <set>
#include <vector>

#include <crow/json.h>
#include <crow/logging.h>
#include <crow/mustache.h>

using namespace tpl;

namespace
{
    using bind_list = std::vector<std::string>;

    std::optional<crow::json::wvalue> select_into_json(Db &db, const char *sql, const bind_list binds)
    {
        std::optional<DbStatement> maybe_stmt = db.try_prepare(sql);
        if (!maybe_stmt.has_value())
        {
            return std::nullopt;
        }
        DbStatement stmt = maybe_stmt.value();

        assert(sqlite3_column_count(stmt.stmt) == 4);

        for (size_t i = 0; i < binds.size(); i++)
        {
            stmt.bind(i + 1, binds[i]);
        }

        std::vector<crow::json::wvalue> result;

        stmt.with_row([&result, &stmt]()
                      { result.push_back({{{"name", std::string((const char *)sqlite3_column_text(stmt.stmt, 0))},
                                           {"email", std::string((const char *)sqlite3_column_text(stmt.stmt, 1))},
                                           {"message", std::string((const char *)sqlite3_column_text(stmt.stmt, 2))},
                                           {"created_at", std::string((const char *)sqlite3_column_text(stmt.stmt, 3))}}}); });

        CROW_LOG_INFO << "Loaded " << result.size() << " filtered guestbook entries";

        return {result};
    }

    const std::string and_bit = " AND\n";
}


std::set<std::string> allowable_columns = {"name", "email", "message", "created_at"};

::crow::response Filter::render()
{
    auto t = crow::mustache::load("filter.html.mustache");

    std::string sql = "SELECT \n"
                      "\tname, email, message, created_at\n"
                      "\tFROM guests\n";

    bind_list binds;

    if (where.size() > 0)
    {
        sql += "\tWHERE\n";
        for (auto &pair : where)
        {
            if (allowable_columns.find(pair.first) == allowable_columns.end())
            {
                CROW_LOG_WARNING << "got invalid filter column " << pair.first;
                continue;
            }
            sql += "\t\t" + pair.first + " = ? " + and_bit;
            binds.push_back(pair.second);
        }
        sql.erase(sql.size() - and_bit.size(), and_bit.size());
        sql.push_back('\n');
    }

    if (order_by.size() > 0)
    {
        bool did_set_order_clause = false;
        sql += "\tORDER BY\n";
        for (auto &pair : order_by)
        {
            std::string dir = pair.second;
            if (allowable_columns.find(pair.first) == allowable_columns.end())
            {
                CROW_LOG_WARNING << "got invalid sort column " << pair.first;
                continue;
            }
            CROW_LOG_INFO << "ordering by " << pair.first << " " << dir;
            sql += "\t\t" + pair.first + " " + dir + ",\n";
            did_set_order_clause = true;
        }
        if (did_set_order_clause)
        {
            sql.pop_back(); // newline
            sql.pop_back(); // comma
            sql.push_back('\n');
        }
    }

    crow::mustache::context ctx;

    auto maybe_guestbook = select_into_json(db, sql.c_str(), binds);
    if (!maybe_guestbook.has_value())
    {
        std::string oopsie = "failed to query:\n" + sql;
        return {400, oopsie};
    }

    ctx["sql"] = sql;
    ctx["guestbook"] = std::move(maybe_guestbook.value());
    return {t.render(ctx)};
}
