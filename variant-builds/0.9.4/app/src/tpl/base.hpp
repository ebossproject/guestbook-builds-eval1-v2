#pragma once

#include <string>

#include <crow/http_response.h>

namespace tpl {
    class Base {
        public:
        virtual crow::response render() = 0;
    };
}