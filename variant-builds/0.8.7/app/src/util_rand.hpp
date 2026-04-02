#pragma once

namespace util_rand {
    int rand_die();
    int rand_byte();
    int rand_lc();
    int rand_len();

    unsigned int rand_lt(unsigned int max);

    std::string rand_word();
    std::string rand_long_word();
    std::string rand_blob();
}
