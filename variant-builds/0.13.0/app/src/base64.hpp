#pragma once

#include <optional>
#include <string>

#include <sodium.h>

#include "maybe_string.hpp"

namespace base64 {
    std::string encode(const std::string &data);

    maybe_string decode(const std::string &encoded);
}
