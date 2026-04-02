#include "db.hpp"

#include "assert.hpp"

#include <crow/logging.h>

static_assert(0 == SQLITE_OK, "really expected SQLITE_OK to be zero");

Db::Db() {
    assert_zero(sqlite3_open(":memory:", &db));
    assert_zero(sqlite3_exec(db, 
    "CREATE TABLE guests (\n"
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "  name TEXT NOT NULL,\n"
    "  email TEXT NOT NULL,\n"
    "  message TEXT NOT NULL,\n"
    "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
    ")",
    nullptr, nullptr, nullptr));

}

Db::~Db() {
    assert_zero(sqlite3_close(db));
}

DbStatement Db::prepare(const char *sql) {
    CROW_LOG_INFO << "preparing " << sql;
    return DbStatement(db, sql);
}

std::optional<DbStatement> Db::try_prepare(const char *sql) {
    CROW_LOG_INFO << "trying to prepare " << sql;
    sqlite3_stmt *stmt;
    int got = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (got != SQLITE_OK) {
        CROW_LOG_ERROR << "failed to prepare " << sql;
        return std::nullopt;
    }
    CROW_LOG_INFO << "did prepare " << std::hex << stmt;
    return {DbStatement{stmt}};
}

DbStatement::DbStatement(sqlite3 *db, const char *sql) {
    assert_zero(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr));
}

void DbStatement::bind(int index, const std::string& value) {
    CROW_LOG_INFO << "binding in " << std::hex << this;
    assert_zero(sqlite3_bind_text(stmt, index, value.c_str(), value.size(), SQLITE_TRANSIENT));
}

void DbStatement::with_row(std::function<void()> f) {
    CROW_LOG_INFO << "stepping " << std::hex << this;
    while (true) {
        CROW_LOG_INFO << sqlite3_expanded_sql(stmt) << "\nstep " << std::hex << stmt;
        int rc = sqlite3_step(stmt);
        switch (rc) {
            case SQLITE_DONE:
                return;
            case SQLITE_ROW:
                f();
                break;
            default:
                CROW_LOG_ERROR << "sqlite3_step failed";
                return;
        }
    }
}

DbStatement::~DbStatement() {
    if (stmt) {
        CROW_LOG_INFO << "finalizing " << std::hex << stmt << " for " << this;
        assert_zero(sqlite3_finalize(stmt));
    }
}
