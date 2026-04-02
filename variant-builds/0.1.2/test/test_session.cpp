#include <set>

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <criterion/logging.h>

#include "base64.hpp"
#include "session.hpp"
#include "maybe_string.hpp"
#include "util_rand.hpp"

using namespace util_rand;

Test(session, fixed_values) {
    Session s = {""};
    s.put_field("key", "value");
    s.put_field("cool", "hat");

    std::string got = s.serialize();

    Session d = {got};

    cr_assert(eq(str, "value", d.get_field("key").value().c_str()));
    cr_assert(eq(str, "hat", d.get_field("cool").value().c_str()));
    cr_assert(not(d.get_field("missing").has_value()));
}

Test(session, touch_fuzzy) {
    unsigned int count = 100;

    for (unsigned int i = 0; i < count; i++) {
        size_t field_count = rand_die();
        Session s = {""};
        Session::field_map_t fields;

        for (size_t j = 0; j < field_count; j++) {
            std::string key = rand_word();
            std::string value = rand_word();

            s.put_field(key, value);
            fields[key] = value;
        }

        std::string got = s.serialize();

        Session d = {got};

        for (auto &field : fields) {
            cr_assert(eq(str, field.second.c_str(), d.get_field(field.first).value().c_str()));
        }
    }
}

Test(session, get_dizzy) {
    unsigned int count = 100;

    for (unsigned int i = 0; i < count; i++) {
        size_t field_count = rand_die();
        Session s = {""};
        Session::field_map_t fields;

        for (size_t j = 0; j < field_count; j++) {
            std::string key = rand_blob();
            std::string value = rand_blob();

            s.put_field(key, value);
            fields[key] = value;
        }

        std::string got = s.serialize();

        Session d = {got};

        for (auto &field : fields) {
            cr_assert(eq(str, field.second.c_str(), d.get_field(field.first).value().c_str()));
        }
    }
}

// buggy
Test(session, get_rekt, .disabled = true) {
    unsigned int count = 1000;

    for (unsigned int i = 0; i < count; i++) {
        size_t field_count = rand_die() + 1;
        Session s = {""};
        Session::field_map_t fields;

        for (size_t j = 0; j < field_count; j++) {
            std::string key = rand_word();
            std::string value = rand_word();

            s.put_field(key, value);
            fields[key] = value;
        }

        std::string got = s.serialize();

        std::string was = got;

        int spot = rand_lt(got.size());
        unsigned char cur, replace;

        int toss = rand_die();
        switch (toss) {
            case 6:
            case 1:
                cur = got[spot];
                do {
                    replace = rand_byte();
                } while (replace == cur);

                got[spot] = replace;
                break;
            case 2:
                got.push_back(rand_byte());
                break;
            case 3:
                got.insert(spot, 1, rand_byte());
                break;
            case 4:
            case 5:
                got.erase(spot);
                break;
        }

        cr_assert(not(eq(str, was.c_str(), got.c_str())), "nothing changed?");

        Session d = {got};

        for (auto &field : fields) {
            cr_assert(not(d.has_field(field.first)), 
                "found %s after toss %d with spot %d size %d and replace %d", 
                field.first.c_str(), toss, spot, got.size(), replace);
        }
    }
}

std::string extract_nonce(std::string serialized_session) {
    std::string deserialized = base64::decode(serialized_session).value();
    return deserialized.substr(0, crypto_secretbox_NONCEBYTES);
}

Test(session, no_nonce_reuse) {
    unsigned int count = 100;

    std::set<std::string> nonces;

    for (unsigned int i = 0; i < count; i++) {
        size_t field_count = rand_die();
        Session s = {""};
        Session::field_map_t fields;

        for (size_t j = 0; j < field_count; j++) {
            std::string key = rand_word();
            std::string value = rand_word();

            s.put_field(key, value);
            fields[key] = value;
        }

        std::string s_nonce = extract_nonce(s.serialize());
        cr_assert(not(nonces.contains(s_nonce)));
        nonces.insert(s_nonce);

        std::string s2_nonce = extract_nonce(s.serialize());
        cr_assert(not(nonces.contains(s2_nonce)));
        nonces.insert(s2_nonce);

        Session d = {s.serialize()};
        std::string d_nonce = extract_nonce(d.serialize());
        cr_assert(not(nonces.contains(d_nonce)));
        nonces.insert(d_nonce);
    }
}
