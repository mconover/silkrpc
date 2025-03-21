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

#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include <evmc/evmc.hpp>
#include <intx/intx.hpp>

namespace silkrpc {

struct ChainTraffic {
    intx::uint<256> cumulative_gas_used;
    uint64_t cumulative_transactions_count;
};


} // namespace silkrpc

