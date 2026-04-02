#pragma once

#include <functional>
#include <optional>
#include <string>

#include "sqlite3.h"

class DbStatement;

class Db {
public:
    Db();
    ~Db();

    DbStatement prepare(const char *sql);
    std::optional<DbStatement> try_prepare(const char *sql);
    
    sqlite3 *db;
};

class DbStatement {
    friend class Db;

    public:
    DbStatement(DbStatement& other) {
        stmt = other.stmt;
        other.stmt = nullptr;
    }
    DbStatement(DbStatement&& other) {
        stmt = other.stmt;
        other.stmt = nullptr;
    };

    ~DbStatement();
    sqlite3_stmt *stmt;

    void with_row(std::function<void()> f);

    void bind(int index, const std::string& value);

    private:
    DbStatement(sqlite3 *db, const char *sql);
    DbStatement(sqlite3_stmt *stmt) : stmt(stmt) {}

    // int column_count = -1;
};

