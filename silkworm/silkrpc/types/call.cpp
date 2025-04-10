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

#include "call.hpp"

#include <string>

#include <silkworm/common/util.hpp>
#include <silkworm/silkrpc/common/util.hpp>
#include <silkworm/common/endian.hpp>

namespace silkrpc {

static std::string optional_uint256_to_string(std::optional<intx::uint256> u) {
    return silkworm::to_hex(silkworm::endian::to_big_compact(u.value_or(intx::uint256{})));
}

std::ostream& operator<<(std::ostream& out, const Call& call) {
    out << "from: " << call.from.value_or(evmc::address{}) << " "
    << "to: " << call.to.value_or(evmc::address{}) << " "
    << "gas: " << call.gas.value_or(0) << " "
    << "gas_price: " << optional_uint256_to_string(call.gas_price.value_or(intx::uint256{})) << " "
    << "max_priority_fee_per_gas: " << optional_uint256_to_string(call.max_priority_fee_per_gas.value_or(intx::uint256{})) << " "
    << "max_fee_per_gas: " << optional_uint256_to_string(call.max_fee_per_gas.value_or(intx::uint256{})) << " "
    << "value: " << optional_uint256_to_string(call.value.value_or(intx::uint256{})) << " "
    << "data: " << silkworm::to_hex(call.data.value_or(silkworm::Bytes{}));
    return out;
}

} // namespace silkrpc
