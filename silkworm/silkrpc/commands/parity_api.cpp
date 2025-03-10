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

#include "parity_api.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include <silkworm/common/util.hpp>

#include <silkworm/silkrpc/common/constants.hpp>
#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/common/util.hpp>
#include <silkworm/silkrpc/core/blocks.hpp>
#include <silkworm/silkrpc/core/cached_chain.hpp>
#include <silkworm/silkrpc/core/receipts.hpp>
#include <silkworm/silkrpc/core/state_reader.hpp>
#include <silkworm/silkrpc/core/rawdb/chain.hpp>
#include <silkworm/silkrpc/ethdb/transaction_database.hpp>
#include <silkworm/silkrpc/ethdb/tables.hpp>
#include <silkworm/silkrpc/json/types.hpp>
#include <silkworm/silkrpc/types/log.hpp>
#include <silkworm/silkrpc/types/receipt.hpp>

namespace silkrpc::commands {

// https://eth.wiki/json-rpc/API#parity_getblockreceipts
boost::asio::awaitable<void> ParityRpcApi::handle_parity_get_block_receipts(const nlohmann::json& request, nlohmann::json& reply) {
    auto params = request["params"];
    if (params.size() != 1) {
        auto error_msg = "invalid parity_getBlockReceipts params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto block_id = params[0].get<std::string>();
    SILKRPC_DEBUG << "block_id: " << block_id << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};

        const auto block_number = co_await core::get_block_number(block_id, tx_database);
        const auto block_with_hash = co_await core::read_block_by_number(*context_.block_cache(), tx_database, block_number);
        auto receipts{co_await core::get_receipts(tx_database, block_with_hash)};
        SILKRPC_INFO << "#receipts: " << receipts.size() << "\n";

        const auto block{block_with_hash.block};
        for (size_t i{0}; i < block.transactions.size(); i++) {
            receipts[i].effective_gas_price = block.transactions[i].effective_gas_price(block.header.base_fee_per_gas.value_or(0));
        }

        reply = make_json_content(request["id"], receipts);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_WARN << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}

// TODO(canepat) will raise error until Silkrpc implements Erigon2u1 https://github.com/ledgerwatch/erigon-lib/pull/559
boost::asio::awaitable<void> ParityRpcApi::handle_parity_list_storage_keys(const nlohmann::json& request, nlohmann::json& reply) {
    const auto params = request["params"];
    if (params.size() < 2) {
        auto error_msg = "invalid parity_listStorageKeys params: " + params.dump();
        SILKRPC_ERROR << error_msg << "\n";
        reply = make_json_error(request["id"], 100, error_msg);
        co_return;
    }
    const auto address = params[0].get<evmc::address>();
    const auto quantity = params[1].get<uint64_t>();
    std::optional<silkworm::Bytes> offset = std::nullopt;
    if (params.size() >= 3 && !params[2].is_null()) {
        offset = std::make_optional(params[2].get<silkworm::Bytes>());
    }
    std::string block_id = core::kLatestBlockId;
    if (params.size() >= 4) {
        block_id = params[3].get<std::string>();
    }

    SILKRPC_DEBUG << "address: 0x" << silkworm::to_hex(address)
        << " quantity: " << quantity
        << " offset: 0x" << (offset ? silkworm::to_hex(offset.value()) : silkworm::to_hex(silkworm::Bytes{})) << "\n";

    auto tx = co_await database_->begin();

    try {
        ethdb::TransactionDatabase tx_database{*tx};
        silkrpc::StateReader state_reader{tx_database};

        const auto block_number = co_await silkrpc::core::get_block_number(block_id, tx_database);
        SILKRPC_DEBUG << "read account with address: " << silkworm::to_hex(address) << " block number: " << block_number << "\n";
        std::optional<silkworm::Account> account = co_await state_reader.read_account(address, block_number);
        if (!account) throw std::domain_error{"account not found"};

        silkworm::Bytes seek_bytes = silkworm::db::storage_prefix(full_view(address), account->incarnation);
        const auto cursor = co_await tx->cursor_dup_sort(db::table::kPlainState);
        SILKRPC_TRACE << "ParityRpcApi::handle_parity_list_storage_keys cursor id: " << cursor->cursor_id() << "\n";
        silkworm::Bytes seek_val = offset ? offset.value() : silkworm::Bytes{};

        std::vector<evmc::bytes32> keys;
        auto v = co_await cursor->seek_both(seek_bytes, seek_val);
        // We look for keys until we have the quantity we want or the key is invalid/empty
        while (v.size() >= silkworm::kHashLength && keys.size() != quantity) {
            auto value = silkworm::bytes32_from_hex(silkworm::to_hex(v));
            keys.push_back(value);
            const auto kv_pair = co_await cursor->next_dup();

            if (kv_pair.key != seek_bytes) {
                break;
            }
            v = kv_pair.value;
        }
        reply = make_json_content(request["id"], keys);
    } catch (const std::invalid_argument& iv) {
        SILKRPC_WARN << "invalid_argument: " << iv.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_content(request["id"], {});
    } catch (const std::exception& e) {
        SILKRPC_ERROR << "exception: " << e.what() << " processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, e.what());
    } catch (...) {
        SILKRPC_ERROR << "unexpected exception processing request: " << request.dump() << "\n";
        reply = make_json_error(request["id"], 100, "unexpected exception");
    }

    co_await tx->close(); // RAII not (yet) available with coroutines
    co_return;
}
} // namespace silkrpc::commands
