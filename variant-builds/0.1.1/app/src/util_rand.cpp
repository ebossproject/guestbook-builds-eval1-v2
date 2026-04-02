#include <random>

#include "util_rand.hpp"
std::random_device rd;
std::mt19937 gen(rd());

std::uniform_int_distribution<> die_dist(1, 6);
std::uniform_int_distribution<> byte_dist(0, 255);
std::uniform_int_distribution<> lc_dist(97, 122);
std::uniform_int_distribution<> len_dist(10, 100);

using namespace util_rand;

int util_rand::rand_die() {
    return die_dist(gen);
}
int util_rand::rand_byte() {
    return byte_dist(gen);
}
int util_rand::rand_lc() {
    return lc_dist(gen);
}
int util_rand::rand_len() {
    return len_dist(gen);
}

std::string util_rand::rand_word() {
    int len = rand_len();
    std::string word;
    word.reserve(len);
    for (int i = 0; i < len; i++) {
        word.push_back(rand_lc());
    }
    return word;
}

std::string util_rand::rand_long_word() {
    int len = 64;
    std::string word;
    word.reserve(len);
    for (int i = 0; i < len; i++) {
        word.push_back(rand_lc());
    }
    return word;
}

std::string util_rand::rand_blob() {
    int len = rand_len();
    std::string blob;
    blob.reserve(len);
    for (int i = 0; i < len; i++) {
        blob.push_back(rand_byte());
    }
    return blob;
}

unsigned int util_rand::rand_lt(unsigned int max) {
    std::uniform_int_distribution<unsigned int> dist(0, max);
    return dist(gen);
}
 