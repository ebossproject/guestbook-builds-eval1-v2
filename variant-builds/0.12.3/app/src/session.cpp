#include <cstring>
#include <sstream>

#include <crow/logging.h>

#include "session.hpp"

#include "assert.hpp"
#include "base64.hpp"

std::string maybe_mangle(std::string &candidate);
std::string maybe_demangle(std::string &candidate);

Session::key_t get_key();
Session::field_map_t deserialize_map(std::string serialized);

Session::Session(std::string cookie_contents) {
    if (0 == cookie_contents.size()) {
        return;
    }

    nonce_t nonce;

    maybe_string maybe_dec_cookie_contents = base64::decode(cookie_contents);

    if (!maybe_dec_cookie_contents.has_value()) {
        CROW_LOG_ERROR << "Failed to decode session cookie contents";
        return;
    }

    std::string dec_cookie_contents = maybe_dec_cookie_contents.value();

    if (dec_cookie_contents.size() < nonce.size()) {
        CROW_LOG_ERROR << "Invalid session cookie contents";
        return;
    }


    std::memcpy(nonce.data(), dec_cookie_contents.data(), nonce.size());
    std::string ciphertext = dec_cookie_contents.substr(nonce.size());

    Session::key_t key = get_key();

    std::string plaintext;
    plaintext.resize(ciphertext.size() - crypto_secretbox_MACBYTES);

    int got = crypto_secretbox_open_easy(
        (unsigned char *)plaintext.data(),
        (const unsigned char *)ciphertext.data(),
        ciphertext.size(),
        nonce.data(),
        key.data()
    );

    if (0 != got) {
        CROW_LOG_ERROR << "Failed to decrypt session cookie contents";
        return;
    }

    fields = deserialize_map(plaintext);
}

void Session::put_field(std::string key, std::string value) {
    fields[maybe_mangle(key)] = maybe_mangle(value);
}

void Session::delete_field(std::string key) {
    fields.erase(maybe_mangle(key));
}

bool Session::has_field(std::string key) {
    return fields.find(maybe_mangle(key)) != fields.end();
}

maybe_string Session::get_field(std::string key) {
    auto got = fields.find(maybe_mangle(key));

    if (got == fields.end()) {
        return std::nullopt;
    }

    return {maybe_demangle(got->second)};
}

unsigned char escape = 0x1b;
unsigned char record_separator = 0x1e;
unsigned char unit_separator = 0x1f;

// mangle keys and values with record or unit separators

std::string definitely_mangle(std::string &candidate);
std::string maybe_mangle(std::string &candidate) {
    for (auto &c : candidate) {
        if (c == escape || c == record_separator || c == unit_separator) {
            return definitely_mangle(candidate);
        }
    }

    return candidate;
}
std::string definitely_mangle(std::string &candidate) {
    std::string mangled = base64::encode(candidate);
    mangled.insert(0, 1, escape);

    return mangled;
}

std::string maybe_demangle(std::string &candidate) {
    if (candidate[0] != escape) {
        return candidate;
    }

    maybe_string maybe_demangled = base64::decode(candidate.substr(1));

    if (!maybe_demangled.has_value()) {
        CROW_LOG_ERROR << "Failed to decode mangled field";
        return "";
    }

    return maybe_demangled.value();
}

// map serde
Session::field_map_t deserialize_map(std::string serialized) {
    Session::field_map_t deserialized;

    size_t start = 0;
    size_t end = serialized.find(record_separator);

    while (end != std::string::npos) {
        std::string record = serialized.substr(start, end - start);

        size_t record_end = record.find(unit_separator);

        if (record_end == std::string::npos) {
            CROW_LOG_ERROR << "Invalid record in session cookie contents";
            return {};
        }

        std::string key = record.substr(0, record_end);
        std::string value = record.substr(record_end + 1);

        deserialized[key] = value;

        start = end + 1;
        end = serialized.find(record_separator, start);
    }

    return deserialized;
}

std::string Session::serialize() {
    if (fields.empty()) {
        return "";
    }

    std::stringstream payload;

    for (auto &field : fields) {
        payload << field.first 
            << unit_separator 
            << field.second 
            << record_separator;
    }

    std::string payload_s = payload.str();

    nonce_t nonce;
    randombytes_buf(nonce.data(), nonce.size());

    std::string ciphertext;
    ciphertext.resize(payload_s.size() + crypto_secretbox_MACBYTES);

    int got = crypto_secretbox_easy(
        (unsigned char *)ciphertext.data(),
        (const unsigned char *)payload_s.data(),
        payload_s.size(),
        nonce.data(),
        get_key().data()
    );

    assert_zero(got);

    std::string packed;
    packed.resize(nonce.size() + ciphertext.size());
    std::memcpy(packed.data(), nonce.data(), nonce.size());
    std::memcpy(packed.data() + nonce.size(), ciphertext.data(), ciphertext.size());

    return base64::encode(packed);
}

// key stuff
Session::key_t key;
bool did_key_init = false;

Session::key_t init_random_key() {
    CROW_LOG_INFO << "Couldn't find key in `SESSION_KEY` env, using random key";

    crypto_secretbox_keygen(key.data());

    did_key_init = true;

    return key;
}

Session::key_t init_key_from_env(std::string env_key) {
    if (env_key.size() != crypto_secretbox_KEYBYTES * 2) {
        CROW_LOG_CRITICAL << "Invalid key length in `SESSION_KEY` env, using random key";
        return init_random_key();
    }

    for (size_t i = 0; i < crypto_secretbox_KEYBYTES; i++) {
        char byte[3] = {env_key[i * 2], env_key[i * 2 + 1], '\0'};
        key[i] = std::stoi(byte, nullptr, 16);
    }

    did_key_init = true;

    return key;
}

Session::key_t init_key() {
    if (const char *env_key = std::getenv("SESSION_KEY")) {
        return init_key_from_env(std::string{env_key});
    }

    return init_random_key();
}

Session::key_t get_key() {
    if (did_key_init) return key;

    return init_key();
}
