/*
    Copyright 2020-2022 The Silkrpc Authors

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

#include "cached_database.hpp"

#include <memory>

#include <silkworm/silkrpc/core/blocks.hpp>
#include <silkworm/silkrpc/ethdb/tables.hpp>

namespace silkrpc::ethdb::kv {

CachedDatabase::CachedDatabase(const BlockNumberOrHash& block_id, Transaction& txn, kv::StateCache& state_cache)
    : block_id_(block_id), txn_(txn), state_cache_{state_cache}, txn_database_{txn_} {}

boost::asio::awaitable<KeyValue> CachedDatabase::get(const std::string& table, const silkworm::ByteView& key) const {
    co_return co_await txn_database_.get(table, key);
}

boost::asio::awaitable<silkworm::Bytes> CachedDatabase::get_one(const std::string& table, const silkworm::ByteView& key) const {
    // Just PlainState and Code tables are present in state cache
    if (table == db::table::kPlainState) {
        std::shared_ptr<kv::StateView> view = state_cache_.get_view(txn_);
        if (view != nullptr) {
            // TODO(canepat) remove key copy changing DatabaseReader interface
            const auto value = co_await view->get(silkworm::Bytes{key.data(), key.size()});
            co_return value ? *value : silkworm::Bytes{};
        }
    } else if (table == db::table::kCode) {
        std::shared_ptr<kv::StateView> view = state_cache_.get_view(txn_);
        if (view != nullptr) {
            // TODO(canepat) remove key copy changing DatabaseReader interface
            const auto value = co_await view->get_code(silkworm::Bytes{key.data(), key.size()});
            co_return value ? *value : silkworm::Bytes{};
        }
    }

    // Simply use transaction-based remote database as fallback
    co_return co_await txn_database_.get_one(table, key);
}

boost::asio::awaitable<std::optional<silkworm::Bytes>> CachedDatabase::get_both_range(const std::string& table,
                                                                                      const silkworm::ByteView& key,
                                                                                      const silkworm::ByteView& subkey) const {
    co_return co_await txn_database_.get_both_range(table, key, subkey);
}

boost::asio::awaitable<void> CachedDatabase::walk(const std::string& table, const silkworm::ByteView& start_key, uint32_t fixed_bits,
                                                  core::rawdb::Walker w) const {
    co_await txn_database_.walk(table, start_key, fixed_bits, w);
}

boost::asio::awaitable<void> CachedDatabase::for_prefix(const std::string& table, const silkworm::ByteView& prefix,
                                                        core::rawdb::Walker w) const {
    co_await txn_database_.for_prefix(table, prefix, w);
}

}  // namespace silkrpc::ethdb::kv
