#pragma once

#include "base.hpp"

#include "../db.hpp"

#include <unordered_map>

namespace tpl {

    class Filter : Base {
        public:
        using query_dict = std::unordered_map<std::string, std::string>;

        Filter(Db& _db, query_dict where, query_dict order_by) : db(_db), where(where), order_by(order_by) {}

        Db &db;
        query_dict where;
        query_dict order_by;

        crow::response render() override;
    };
}