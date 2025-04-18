/*
    Copyright 2022 The Silkrpc Authors

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

#include "stream.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <boost/asio/write.hpp>

namespace json {

static std::uint8_t kObjectOpen = 1;
static std::uint8_t kArrayOpen = 2;
static std::uint8_t kFieldWritten = 3;
static std::uint8_t kEntryWritten = 4;

static std::string kOpenBrace{"{"}; // NOLINT(runtime/string)
static std::string kCloseBrace{"}"}; // NOLINT(runtime/string)
static std::string kOpenBracket{"["}; // NOLINT(runtime/string)
static std::string kCloseBracket{"]"}; // NOLINT(runtime/string)
static std::string kFieldSeparator{","}; // NOLINT(runtime/string)

void Stream::open_object() {
    bool isEntry = !stack_.empty() && (stack_.top() == kArrayOpen || stack_.top() == kEntryWritten);
    if (isEntry) {
        if (stack_.top() != kEntryWritten) {
            stack_.push(kEntryWritten);
        } else {
            writer_.write(kFieldSeparator);
        }
    }
    writer_.write(kOpenBrace);
    stack_.push(kObjectOpen);
}

void Stream::close_object() {
    if (!stack_.empty() && stack_.top() == kFieldWritten) {
        stack_.pop();
    }
    stack_.pop();
    writer_.write(kCloseBrace);
}

void Stream::open_array() {
    writer_.write(kOpenBracket);
    stack_.push(kArrayOpen);
}

void Stream::close_array() {
    if (!stack_.empty() && (stack_.top() == kEntryWritten || stack_.top() == kFieldWritten)) {
        stack_.pop();
    }
    stack_.pop();
    writer_.write(kCloseBracket);
}

void Stream::write_json(const nlohmann::json& json) {
    bool isEntry = !stack_.empty() && (stack_.top() == kArrayOpen || stack_.top() == kEntryWritten);
    if (isEntry) {
        if (stack_.top() != kEntryWritten) {
            stack_.push(kEntryWritten);
        } else {
            writer_.write(kFieldSeparator);
        }
    }

    const auto content = json.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);
    writer_.write(content);
}

void Stream::write_field(const std::string& name) {
    ensure_separator();

    write_string(name);
    writer_.write(":");
}

void Stream::write_field(const std::string& name, const nlohmann::json& value) {
    ensure_separator();

    const auto content = value.dump(/*indent=*/-1, /*indent_char=*/' ', /*ensure_ascii=*/false, nlohmann::json::error_handler_t::replace);

    write_string(name);
    writer_.write(":");
    writer_.write(content);
}

void Stream::write_string(const std::string& str) {
   writer_.write("\"" + str + "\"");
}

void Stream::ensure_separator() {
    if (!stack_.empty()) {
        if (stack_.top() != kFieldWritten) {
            stack_.push(kFieldWritten);
        } else {
            writer_.write(kFieldSeparator);
        }
    }
}

} // namespace json
