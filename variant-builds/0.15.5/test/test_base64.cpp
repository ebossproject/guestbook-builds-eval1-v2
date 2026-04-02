#include <string>

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include "base64.hpp"
#include "util_rand.hpp"
#include "maybe_string.hpp"

using namespace util_rand;

Test(base64, fixed_text) {
    std::string given_plaintext = "asdf\n";
    std::string given_encoded = "YXNkZgo";
   
    std::string got_encoded = base64::encode(given_plaintext);
    cr_assert(eq(str, given_encoded.c_str(), got_encoded.c_str()));

    maybe_string maybe_got_plaintext = base64::decode(given_encoded);
    cr_assert(maybe_got_plaintext.has_value());

    std::string got_plaintext = maybe_got_plaintext.value();
    cr_assert(eq(str, given_plaintext.c_str(), got_plaintext.c_str()));
}

Test(base64, touch_fuzzy) {
    unsigned int count = 1000;

    for (unsigned int i = 0; i < count; i++) {
        std::string given_plaintext = rand_word();

        std::string got_encoded = base64::encode(given_plaintext);

        cr_assert(gt(got_encoded.size(), given_plaintext.size()));

        maybe_string maybe_got_plaintext = base64::decode(got_encoded);
        cr_assert(maybe_got_plaintext.has_value(), "failed to decode \"%s\" from plaintext \"%s\"", 
            got_encoded.c_str(),
            given_plaintext.c_str());
        std::string got_plaintext = maybe_got_plaintext.value();
        cr_assert(eq(str, given_plaintext.c_str(), got_plaintext.c_str()));
    }
    
}

Test(base64, get_dizzy) {
    unsigned int count = 1000;

    for (unsigned int i = 0; i < count; i++) {
        std::string given_plaintext = rand_blob();

        std::string got_encoded = base64::encode(given_plaintext);

        cr_assert(gt(got_encoded.size(), given_plaintext.size()));

        maybe_string maybe_got_plaintext = base64::decode(got_encoded);
        cr_assert(maybe_got_plaintext.has_value(), "failed to decode \"%s\" from plaintext \"%s\"", 
            got_encoded.c_str(),
            given_plaintext.c_str());
        std::string got_plaintext = maybe_got_plaintext.value();
        cr_assert(eq(str, given_plaintext.c_str(), got_plaintext.c_str()));
    }
    
}