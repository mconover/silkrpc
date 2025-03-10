/*
   Copyright 2021 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "filter.cpp" // NOLINT(build/include)

#include <sstream>

#include <catch2/catch.hpp>

#include <silkworm/silkrpc/common/util.hpp>

namespace silkrpc {

using evmc::literals::operator""_address, evmc::literals::operator""_bytes32;

TEST_CASE("write null filter addresses to ostream", "[silkrpc][types][filter]") {
    std::optional<FilterAddresses> addresses{std::nullopt};
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "null");
}

TEST_CASE("write 0-sized filter addresses to ostream", "[silkrpc][types][filter]") {
    FilterAddresses addresses{};
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[]");
}

TEST_CASE("write 1-sized filter addresses to ostream", "[silkrpc][types][filter]") {
    FilterAddresses addresses{0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address};
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[0x6090a6e47849629b7245dfa1ca21d94cd15878ef]");
}

TEST_CASE("write 2-sized filter addresses to ostream", "[silkrpc][types][filter]") {
    FilterAddresses addresses{
        0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address,
        0x702a999710cfd011b475505335d4f437d8132fae_address
    };
    std::ostringstream oss;
    oss << addresses;
    CHECK(oss.str() == "[0x6090a6e47849629b7245dfa1ca21d94cd15878ef 0x702a999710cfd011b475505335d4f437d8132fae]");
}

TEST_CASE("write 0-sized filter subtopics to ostream", "[silkrpc][types][filter]") {
    FilterSubTopics subtopics{};
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == "[]");
}

TEST_CASE("write 1-sized filter subtopics to ostream", "[silkrpc][types][filter]") {
    FilterSubTopics subtopics{0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32};
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == "[0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c]");
}

TEST_CASE("write 2-sized filter subtopics to ostream", "[silkrpc][types][filter]") {
    FilterSubTopics subtopics{
        0x0000000000000000000000000000000000000000000000000000000000000000_bytes32,
        0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32
    };
    std::ostringstream oss;
    oss << subtopics;
    CHECK(oss.str() == R"([0x0000000000000000000000000000000000000000000000000000000000000000 0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c])");
}

TEST_CASE("write null filter topics to ostream", "[silkrpc][types][filter]") {
    std::optional<FilterTopics> topics{std::nullopt};
    std::ostringstream oss;
    oss << topics;
    CHECK(oss.str() == "null");
}

TEST_CASE("write 0-sized filter topics to ostream", "[silkrpc][types][filter]") {
    FilterTopics topics{};
    std::ostringstream oss;
    oss << topics;
    CHECK(oss.str() == "[]");
}

TEST_CASE("write 1-sized filter topics to ostream", "[silkrpc][types][filter]") {
    FilterTopics topics{
        {0x0000000000000000000000000000000000000000000000000000000000000000_bytes32},
        {0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32},
    };
    std::ostringstream oss;
    oss << topics;
    CHECK(oss.str() == "[[0x0000000000000000000000000000000000000000000000000000000000000000] [0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c]]");
}

TEST_CASE("write filter to ostream", "[silkrpc][types][filter]") {
    Filter filter{
        "0",
        "10000000",
        FilterAddresses{0x6090a6e47849629b7245dfa1ca21d94cd15878ef_address},
        FilterTopics{
            {0x0000000000000000000000000000000000000000000000000000000000000000_bytes32},
            {0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c_bytes32},
        },
    };
    std::ostringstream oss;
    oss << filter;
    CHECK(oss.str() == "from_block: 0 to_block: 10000000 address: "
        "[0x6090a6e47849629b7245dfa1ca21d94cd15878ef] topics: [[0x0000000000000000000000000000000000000000000000000000000000000000] "
        "[0x374f3a049e006f36f6cf91b02a3b0ee16c858af2f75858733eb0e927b5b7126c]] block_hash: null");
}

} // namespace silkrpc
