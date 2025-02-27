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

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <silkworm/silkrpc/config.hpp> // NOLINT(build/include_order)

#include <boost/asio/awaitable.hpp>
#include <silkworm/chain/config.hpp>
#include <silkworm/common/util.hpp>
#include <silkworm/types/block.hpp>
#include <silkworm/types/transaction.hpp>

#include <silkworm/silkrpc/core/blocks.hpp>
#include <silkworm/silkrpc/core/rawdb/accessors.hpp>

namespace silkrpc {

const intx::uint256 kWei = 1;
const intx::uint256 kGWei = 1E9;

const intx::uint256 kDefaultPrice = 0;
const intx::uint256 kDefaultMaxPrice = 500 * kGWei;
const intx::uint256 kDefaultMinPrice = 2 * kWei;

const std::uint8_t kCheckBlocks = 20;
const std::uint8_t kSamples = 3;
const std::uint8_t kMaxSamples = kCheckBlocks * kSamples;
const std::uint8_t kPercentile = 60;

typedef std::function<boost::asio::awaitable<silkworm::BlockWithHash>(uint64_t)> BlockProvider;

class GasPriceOracle {
public:
    explicit GasPriceOracle(const BlockProvider& block_provider) : block_provider_(block_provider) {}
    virtual ~GasPriceOracle() {}

    GasPriceOracle(const GasPriceOracle&) = delete;
    GasPriceOracle& operator=(const GasPriceOracle&) = delete;

    boost::asio::awaitable<intx::uint256> suggested_price(uint64_t block_number);

private:
    boost::asio::awaitable<void> load_block_prices(uint64_t block_number, uint64_t limit, std::vector<intx::uint256>& tx_prices);

    const BlockProvider& block_provider_;
};

} // namespace silkrpc

