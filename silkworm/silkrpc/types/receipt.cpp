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

#include "receipt.hpp"

#include <iomanip>

#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/common/util.hpp>
#include <silkworm/types/bloom.cpp> // NOLINT(build/include) m3_2048 not exported

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, const Receipt& r) {
    out << " block_hash: " << r.block_hash;
    out << " block_number: " << r.block_number;
    out << " contract_address: " << r.contract_address;
    out << " cumulative_gas_used: " << r.cumulative_gas_used;
    if (r.from) {
        out << " from: " << silkworm::to_hex(*r.from);
    } else {
        out << " from: null";
    }
    out << " gas_used: " << r.gas_used;
    out << " #logs: " << r.logs.size();
    auto bloom_view = full_view(r.bloom);
    out << " bloom: " << silkworm::to_hex(bloom_view);
    out << " success: " << r.success;
    if (r.to) {
        out << " to: " << silkworm::to_hex(*r.to);
    } else {
        out << " to: null";
    }
    out << " tx_hash: " << r.tx_hash;
    out << " tx_index: " << r.tx_index;
    if (r.type) {
        out << " type: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*r.type) << std::dec;
    } else {
        out << " type: null";
    }

    return out;
}

silkworm::Bloom bloom_from_logs(const Logs& logs) {
    SILKRPC_TRACE << "bloom_from_logs #logs: " << logs.size() << "\n";
    silkworm::Bloom bloom{};
    for (auto const& log : logs) {
        silkworm::m3_2048(bloom, full_view(log.address));
        for (const auto& topic : log.topics) {
            silkworm::m3_2048(bloom, full_view(topic));
        }
    }
    SILKRPC_TRACE << "bloom_from_logs bloom: " << silkworm::to_hex(full_view(bloom)) << "\n";
    return bloom;
}

} // namespace silkrpc
