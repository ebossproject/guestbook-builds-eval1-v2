#pragma once

#include "base.hpp"

namespace tpl {
    class Info : Base {
        public:
        crow::response render() override;
    };
}