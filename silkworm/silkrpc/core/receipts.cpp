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

#include "receipts.hpp"

#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/core/rawdb/chain.hpp>

namespace silkrpc::core {

boost::asio::awaitable<Receipts> get_receipts(const core::rawdb::DatabaseReader& db_reader, const silkworm::BlockWithHash& block_with_hash) {
    const auto cached_receipts = co_await core::rawdb::read_receipts(db_reader, block_with_hash);
    if (!cached_receipts.empty()) {
        co_return cached_receipts;
    }

    // If not already present, retrieve receipts by executing transactions
    //auto block = co_await core::rawdb::read_block(db_reader, hash, number);
    // TODO(canepat): implement
    SILKRPC_WARN << "retrieve receipts by executing transactions NOT YET IMPLEMENTED\n";
    co_return Receipts{};
}

} // namespace silkrpc::core
