
#include <iostream>
#include <vector>

#include "catch2/catch_all.hpp"
#include "crow/query_string.h"

namespace
{
    std::string buildQueryStr(const std::vector<std::pair<std::string, std::string>>& paramList)
    {
        std::string paramsStr{};
        for (const auto& param : paramList)
            paramsStr.append(param.first).append(1, '=').append(param.second).append(1, '&');
        if (!paramsStr.empty())
            paramsStr.resize(paramsStr.size() - 1);
        return paramsStr;
    }
}

TEST_CASE( "empty query params" )
{
    const crow::query_string query_params("");
    const std::vector<std::string> keys = query_params.keys();

    REQUIRE(keys.empty() == true);
}

TEST_CASE( "query string keys" )
{
    const std::vector<std::pair<std::string, std::string>> params {
      {"foo", "bar"}, {"mode", "night"}, {"page", "2"},
      {"tag", "js"}, {"name", "John Smith"}, {"age", "25"},
    };

    const crow::query_string query_params("params?" + buildQueryStr(params));
    const std::vector<std::string> keys = query_params.keys();

    for (const auto& entry: params)
    {
        const bool exist = std::any_of(keys.cbegin(), keys.cend(), [&](const std::string& key) {
            return key == entry.first;});
        REQUIRE(exist == true);
    }
}

TEST_CASE( "query string too long")
{
    std::vector<std::pair<std::string, std::string>> params;
    params.resize(crow::query_string::MAX_KEY_VALUE_PAIRS_COUNT + 2);
    for (size_t i = 0; i < params.size(); ++i)
    {
        params.at(i) = {"k" + std::to_string(i), "v" + std::to_string(i)};
    }

    const std::string build_params = buildQueryStr(params);
    const crow::query_string query_params("params?" + build_params);

    const std::vector<std::string> keys = query_params.keys();
    REQUIRE(keys.size() == params.size());
}

// no brackets
TEST_CASE( "query string arrays" )
{
    const std::string key = "mango";
    const std::vector<const char*> values = {"bort", "biff", "asdf", "bbbb"};
    std::string build_params = "params?";
    for (const auto& value : values)
    {
        build_params.
            append(key).append(1, '=').
            append(value).append(1, '&');
    }

    const crow::query_string query_params(build_params);

    const std::vector<char*> got_values = query_params.get_list(key, false);
    REQUIRE(got_values.size() == values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        const std::string expected_value = values[i];
        const std::string got_value = got_values[i];
        REQUIRE(got_value == expected_value);
    }
}

// with brackets
TEST_CASE( "query string arrays with brackets" )
{
    const std::string key = "mango";
    const std::string key_with_brackets = "mango[]";
    const std::vector<const char*> values = {"bort", "biff", "asdf", "bbbb"};
    std::string build_params = "params?";
    for (const auto& value : values)
    {
        build_params.
            append(key_with_brackets).append(1, '=').
            append(value).append(1, '&');
    }

    const crow::query_string query_params(build_params);

    const std::vector<char*> got_values = query_params.get_list(key);
    REQUIRE(got_values.size() == values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        const std::string expected_value = values[i];
        const std::string got_value = got_values[i];
        REQUIRE(got_value == expected_value);
    }
}

TEST_CASE( "query string map")
{
    const std::string key = "bees";
    const std::map<std::string, std::string> values = {{"bort", "1"}, {"biff", "2"}, {"asdf", "3"}, {"bbbb", "4"}};
    std::string build_params = "params?";
    for (const auto& value : values)
    {
        build_params.
            append(key).append(1, '[').append(value.first).append(1, ']').
            append(1, '=').append(value.second).append(1, '&');
    }

    const crow::query_string query_params(build_params);

    const std::unordered_map<std::string, std::string> got_values = 
        query_params.get_dict(key);
    REQUIRE(got_values.size() == values.size());
    for (const auto& value : values)
    {
        const std::string expected_value = value.second;
        const std::string got_value = got_values.at(value.first);
        REQUIRE(got_value == expected_value);
    }
}
