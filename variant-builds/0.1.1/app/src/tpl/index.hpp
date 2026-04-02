#pragma once

#include "base.hpp"

#include "../db.hpp"

namespace tpl {
    class Index : Base {
        public:
        Index(Db& _db) : db(_db) {}

        std::string csrf_token;
        Db &db;

        crow::response render() override;
    };
    
}
