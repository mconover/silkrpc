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
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <string>

#include "header.hpp"

namespace silkrpc::http::method {

constexpr const char* k_web3_clientVersion{"web3_clientVersion"};
constexpr const char* k_web3_sha3{"web3_sha3"};

constexpr const char* k_net_listening{"net_listening"};
constexpr const char* k_net_peerCount{"net_peerCount"};
constexpr const char* k_net_version{"net_version"};

constexpr const char* k_eth_blockNumber{"eth_blockNumber"};
constexpr const char* k_eth_chainId{"eth_chainId"};
constexpr const char* k_eth_protocolVersion{"eth_protocolVersion"};
constexpr const char* k_eth_syncing{"eth_syncing"};
constexpr const char* k_eth_gasPrice{"eth_gasPrice"};
constexpr const char* k_eth_getUncleByBlockHashAndIndex{"eth_getUncleByBlockHashAndIndex"};
constexpr const char* k_eth_getUncleByBlockNumberAndIndex{"eth_getUncleByBlockNumberAndIndex"};
constexpr const char* k_eth_getUncleCountByBlockHash{"eth_getUncleCountByBlockHash"};
constexpr const char* k_eth_getUncleCountByBlockNumber{"eth_getUncleCountByBlockNumber"};
constexpr const char* k_eth_getTransactionByHash{"eth_getTransactionByHash"};
constexpr const char* k_eth_getTransactionByBlockHashAndIndex{"eth_getTransactionByBlockHashAndIndex"};
constexpr const char* k_eth_getRawTransactionByHash{"eth_getRawTransactionByHash"};
constexpr const char* k_eth_getRawTransactionByBlockHashAndIndex{"eth_getRawTransactionByBlockHashAndIndex"};
constexpr const char* k_eth_getRawTransactionByBlockNumberAndIndex{"eth_getRawTransactionByBlockNumberAndIndex"};
constexpr const char* k_eth_getTransactionByBlockNumberAndIndex{"eth_getTransactionByBlockNumberAndIndex"};
constexpr const char* k_eth_getTransactionReceipt{"eth_getTransactionReceipt"};
constexpr const char* k_eth_estimateGas{"eth_estimateGas"};
constexpr const char* k_eth_getBalance{"eth_getBalance"};
constexpr const char* k_eth_getCode{"eth_getCode"};
constexpr const char* k_eth_getTransactionCount{"eth_getTransactionCount"};
constexpr const char* k_eth_getStorageAt{"eth_getStorageAt"};
constexpr const char* k_eth_call{"eth_call"};
constexpr const char* k_eth_callBundle{"eth_callBundle"};
constexpr const char* k_eth_createAccessList{"eth_createAccessList"};
constexpr const char* k_eth_newFilter{"eth_newFilter"};
constexpr const char* k_eth_newBlockFilter{"eth_newBlockFilter"};
constexpr const char* k_eth_newPendingTransactionFilter{"eth_newPendingTransactionFilter"};
constexpr const char* k_eth_getFilterChanges{"eth_getFilterChanges"};
constexpr const char* k_eth_uninstallFilter{"eth_uninstallFilter"};
constexpr const char* k_eth_getLogs{"eth_getLogs"};
constexpr const char* k_eth_sendRawTransaction{"eth_sendRawTransaction"};
constexpr const char* k_eth_sendTransaction{"eth_sendTransaction"};
constexpr const char* k_eth_signTransaction{"eth_signTransaction"};
constexpr const char* k_eth_getProof{"eth_getProof"};
constexpr const char* k_eth_mining{"eth_mining"};
constexpr const char* k_eth_coinbase{"eth_coinbase"};
constexpr const char* k_eth_hashrate{"eth_hashrate"};
constexpr const char* k_eth_submitHashrate{"eth_submitHashrate"};
constexpr const char* k_eth_getWork{"eth_getWork"};
constexpr const char* k_eth_submitWork{"eth_submitWork"};
constexpr const char* k_eth_subscribe{"eth_subscribe"};
constexpr const char* k_eth_unsubscribe{"eth_unsubscribe"};
constexpr const char* k_eth_getBlockByHash{"eth_getBlockByHash"};
constexpr const char* k_eth_getBlockTransactionCountByHash{"eth_getBlockTransactionCountByHash"};
constexpr const char* k_eth_getBlockByNumber{"eth_getBlockByNumber"};
constexpr const char* k_eth_getBlockTransactionCountByNumber{"eth_getBlockTransactionCountByNumber"};
constexpr const char* k_eth_getBlockReceipts{"eth_getBlockReceipts"};

constexpr const char* k_debug_accountRange{"debug_accountRange"};
constexpr const char* k_debug_getModifiedAccountsByNumber{"debug_getModifiedAccountsByNumber"};
constexpr const char* k_debug_getModifiedAccountsByHash{"debug_getModifiedAccountsByHash"};
constexpr const char* k_debug_storageRangeAt{"debug_storageRangeAt"};
constexpr const char* k_debug_traceTransaction{"debug_traceTransaction"};
constexpr const char* k_debug_traceCall{"debug_traceCall"};
constexpr const char* k_debug_traceBlockByNumber{"debug_traceBlockByNumber"};
constexpr const char* k_debug_traceBlockByHash{"debug_traceBlockByHash"};

constexpr const char* k_trace_call{"trace_call"};
constexpr const char* k_trace_callMany{"trace_callMany"};
constexpr const char* k_trace_rawTransaction{"trace_rawTransaction"};
constexpr const char* k_trace_replayBlockTransactions{"trace_replayBlockTransactions"};
constexpr const char* k_trace_replayTransaction{"trace_replayTransaction"};
constexpr const char* k_trace_block{"trace_block"};
constexpr const char* k_trace_filter{"trace_filter"};
constexpr const char* k_trace_get{"trace_get"};
constexpr const char* k_trace_transaction{"trace_transaction"};

constexpr const char* k_erigon_getBlockByTimestamp{"erigon_getBlockByTimestamp"};
constexpr const char* k_erigon_getHeaderByHash{"erigon_getHeaderByHash"};
constexpr const char* k_erigon_getHeaderByNumber{"erigon_getHeaderByNumber"};
constexpr const char* k_erigon_getLogsByHash{"erigon_getLogsByHash"};
constexpr const char* k_erigon_forks{"erigon_forks"};
constexpr const char* k_erigon_watchTheBurn{"erigon_watchTheBurn"};
constexpr const char* k_erigon_cumulative_chain_traffic{"erigon_cumulativeChainTraffic"};
constexpr const char* k_erigon_blockNumber{"erigon_blockNumber"};
constexpr const char* k_erigon_nodeInfo{"erigon_nodeInfo"};

constexpr const char* k_parity_getBlockReceipts{"parity_getBlockReceipts"};
constexpr const char* k_parity_listStorageKeys{"parity_listStorageKeys"};

constexpr const char* k_engine_getPayloadV1{"engine_getPayloadV1"};
constexpr const char* k_engine_newPayloadV1{"engine_newPayloadV1"};
constexpr const char* k_engine_forkchoiceUpdatedV1{"engine_forkchoiceUpdatedV1"};
constexpr const char* k_engine_exchangeTransitionConfiguration{"engine_exchangeTransitionConfigurationV1"};

constexpr const char* k_txpool_status{"txpool_status"};
constexpr const char* k_txpool_content{"txpool_content"};

constexpr const char* k_ots_getApiLevel{"ots_getApiLevel"};
constexpr const char* k_ots_hasCode{"ots_hasCode"};

} // namespace silkrpc::http::method

