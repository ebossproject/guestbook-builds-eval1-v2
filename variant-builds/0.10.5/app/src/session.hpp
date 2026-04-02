#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>

#include <crow/logging.h>
#include <sodium.h>

#include "maybe_string.hpp"

class Session {
    public:
    using nonce_t = std::array<unsigned char, crypto_secretbox_NONCEBYTES>;
    using key_t = std::array<unsigned char, crypto_secretbox_KEYBYTES>;
    using field_map_t = std::map<std::string, std::string>;

    Session(std::string cookie_contents);

    void put_field(std::string key, std::string value);
    void delete_field(std::string key);
    bool has_field(std::string key);
    maybe_string get_field(std::string key);

    std::string serialize();
    
    private:
    field_map_t fields;
};
