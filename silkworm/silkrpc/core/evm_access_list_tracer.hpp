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

#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <silkworm/execution/evm.hpp>
#pragma GCC diagnostic pop
#include <silkworm/state/intra_block_state.hpp>

#include <silkworm/silkrpc/concurrency/context_pool.hpp>
#include <silkworm/silkrpc/core/rawdb/accessors.hpp>
#include <silkworm/silkrpc/types/block.hpp>
#include <silkworm/silkrpc/types/call.hpp>
#include <silkworm/silkrpc/types/transaction.hpp>

namespace silkrpc {

class AccessListTracer : public silkworm::EvmTracer {
public:
    explicit AccessListTracer(const evmc::address& from, const evmc::address& to): from_{from}, to_{to} {
    }
    AccessListTracer(const AccessListTracer&) = delete;
    AccessListTracer& operator=(const AccessListTracer&) = delete;

    const AccessList& get_access_list() { return access_list_; }

    void on_execution_start(evmc_revision rev, const evmc_message& msg, evmone::bytes_view code) noexcept override;
    void on_instruction_start(uint32_t pc, const intx::uint256 *stack_top, const int stack_height,
            const evmone::ExecutionState& execution_state, const silkworm::IntraBlockState& intra_block_state) noexcept override;
    void on_execution_end(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {}
    void on_precompiled_run(const evmc_result& result, int64_t gas, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_reward_granted(const silkworm::CallResult& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {};
    void on_creation_completed(const evmc_result& result, const silkworm::IntraBlockState& intra_block_state) noexcept override {};



    void reset_access_list() {access_list_.clear();}
    static void dump(const std::string& str, const AccessList& acl);
    static bool compare(const AccessList& acl1, const AccessList& acl2);

private:
    inline bool exclude(const evmc::address& address);
    inline evmc::address address_from_hex_string(const std::string& s);
    inline bool is_storage_opcode(const std::string & opcode_name);
    inline bool is_contract_opcode(const std::string & opcode_name);
    inline bool is_call_opcode(const std::string & opcode_name);

    void add_storage(const evmc::address& address, const evmc::bytes32& storage);
    void add_address(const evmc::address& address);


    AccessList access_list_;
    evmc::address from_;
    evmc::address to_;

    const char* const* opcode_names_ = nullptr;
};

inline bool operator!=(const AccessList& acl1, const AccessList& acl2) {
    return AccessListTracer::compare(acl1, acl2) == false;
}

inline bool operator==(const AccessList& acl1, const AccessList& acl2) {
    return AccessListTracer::compare(acl1, acl2);
}


} // namespace silkrpc

