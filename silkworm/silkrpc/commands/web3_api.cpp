/*
    Copyright 2020 The Silkrpc Authors

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

#include "web3_api.hpp"

#include <string>

#include <silkworm/common/util.hpp>

#include <silkworm/silkrpc/common/constants.hpp>
#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/common/util.hpp>
#include <silkworm/silkrpc/json/types.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#web3_clientversion
boost::asio::awaitable<void> Web3RpcApi::handle_web3_client_version(const nlohmann::json& request, nlohmann::json& reply) {
    try {
        const auto client_version = co_await backend_->client_version();
        reply = make_json_content(request["id"], client_version);
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], -32000, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }
    co_return;
}

// https://eth.wiki/json-rpc/API#web3_sha3
boost::asio::awaitable<void> Web3RpcApi::handle_web3_sha3(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid web3_sha3 params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto input_string = params[0].get<std::string>();
    const auto optional_input_bytes = silkworm::from_hex(input_string);
    if (!optional_input_bytes) {
        auto error_msg = "invalid input: " + input_string;
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    auto eth_hash = hash_of(optional_input_bytes.value());
    const auto output = "0x" + silkworm::to_hex({eth_hash.bytes, silkworm::kHashLength});
    reply = make_json_content(request["id"], output);
    co_return;
}

} // namespace silkrpc::commands
