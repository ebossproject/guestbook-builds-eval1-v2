#include <crow/logging.h>

#include "base64.hpp"

#include "assert.hpp"


const int base64_variant = sodium_base64_VARIANT_URLSAFE_NO_PADDING;

std::string base64::encode(const std::string &data) {
    std::string encoded;
    
    // size includes a trailing null 
    // c.f. https://doc.libsodium.org/helpers#base64-encoding-decoding
    encoded.resize(sodium_base64_ENCODED_LEN(
        data.size(),
        base64_variant
    ));

    char* got = sodium_bin2base64(
        encoded.data(),
        encoded.size(),
        (const unsigned char *)data.data(),
        data.size(),
        base64_variant
    );

    assert(got == encoded.data());

    // validate and then remove trailing null
    assert(0 == encoded.back());
    encoded.resize(encoded.size() - 1);

    return encoded;
}

maybe_string base64::decode(const std::string &encoded) {
    std::string decoded;

    size_t decoded_len;
    decoded.resize(encoded.size());

    int got = sodium_base642bin(
        (unsigned char *)decoded.data(),
        decoded.size(),
        encoded.data(),
        encoded.size(),
        nullptr,
        &decoded_len,
        nullptr,
        base64_variant
    );

    if (0 != got) {
        CROW_LOG_INFO
            << "Failed to decode base64, got " << got 
            << " after " << decoded_len 
            << " bytes from " << encoded;
        CROW_LOG_INFO
            << "decoded.size() " << decoded.size()
            << " encoded.size() " << encoded.size();
        return std::nullopt;
    }

    decoded.resize(decoded_len);

    return decoded;
}
