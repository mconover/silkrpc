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

#include "engine_api.hpp"

#include <string>
#include <utility>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <catch2/catch.hpp>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <silkworm/common/base.hpp>

#include <silkworm/silkrpc/core/rawdb/chain.hpp>
#include <silkworm/silkrpc/ethdb/transaction_database.hpp>
#include <silkworm/silkrpc/ethdb/tables.hpp>
#include <silkworm/silkrpc/http/methods.hpp>
#include <silkworm/silkrpc/json/types.hpp>

namespace silkrpc::commands {

using Catch::Matchers::Message;
using evmc::literals::operator""_bytes32;

namespace {
class BackEndMock : public ethbackend::BackEnd {
public:
    MOCK_METHOD((boost::asio::awaitable<evmc::address>), etherbase, ());
    MOCK_METHOD((boost::asio::awaitable<uint64_t>), protocol_version, ());
    MOCK_METHOD((boost::asio::awaitable<uint64_t>), net_version, ());
    MOCK_METHOD((boost::asio::awaitable<std::string>), client_version, ());
    MOCK_METHOD((boost::asio::awaitable<uint64_t>), net_peer_count, ());
    MOCK_METHOD((boost::asio::awaitable<ExecutionPayload>), engine_get_payload_v1, (uint64_t));
    MOCK_METHOD((boost::asio::awaitable<PayloadStatus>), engine_new_payload_v1, (ExecutionPayload));
    MOCK_METHOD((boost::asio::awaitable<ForkChoiceUpdatedReply>), engine_forkchoice_updated_v1, (ForkChoiceUpdatedRequest));
    MOCK_METHOD((boost::asio::awaitable<std::vector<NodeInfo>>), engine_node_info, ());
};

class MockCursor : public ethdb::Cursor {
public:
    uint32_t cursor_id() const override { return 0; }

    MOCK_METHOD((boost::asio::awaitable<void>), open_cursor, (const std::string&, bool));
    MOCK_METHOD((boost::asio::awaitable<KeyValue>), seek, (silkworm::ByteView));
    MOCK_METHOD((boost::asio::awaitable<KeyValue>), seek_exact, (silkworm::ByteView));
    MOCK_METHOD((boost::asio::awaitable<KeyValue>), next, ());
    MOCK_METHOD((boost::asio::awaitable<void>), close_cursor, ());
};

//! This dummy transaction just gives you the same cursor over and over again.
class DummyTransaction : public ethdb::Transaction {
public:
    explicit DummyTransaction(std::shared_ptr<ethdb::Cursor> cursor) : cursor_(cursor) {}

    uint64_t tx_id() const override { return 0; }

    boost::asio::awaitable<void> open() override { co_return; }

    boost::asio::awaitable<std::shared_ptr<ethdb::Cursor>> cursor(const std::string& /*table*/) override {
        co_return cursor_;
    }

    boost::asio::awaitable<std::shared_ptr<ethdb::CursorDupSort>> cursor_dup_sort(const std::string& /*table*/) override {
        co_return nullptr;
    }

    boost::asio::awaitable<void> close() override { co_return; }

private:
    std::shared_ptr<ethdb::Cursor> cursor_;
};

//! This dummy database acts as a factory for dummy transactions using the same cursor.
class DummyDatabase : public ethdb::Database {
public:
    explicit DummyDatabase(std::shared_ptr<ethdb::Cursor> cursor) : cursor_(cursor) {}

    boost::asio::awaitable<std::unique_ptr<ethdb::Transaction>> begin() override {
        co_return std::make_unique<DummyTransaction>(cursor_);
    }
private:
    ethdb::Transaction* transaction_;
    std::shared_ptr<ethdb::Cursor> cursor_;
};

} // namespace

class EngineRpcApiTest : public EngineRpcApi{
public:
    explicit EngineRpcApiTest(std::unique_ptr<ethdb::Database>& database, std::unique_ptr<ethbackend::BackEnd>& backend): EngineRpcApi(database, backend) {}

    using EngineRpcApi::handle_engine_get_payload_v1;
    using EngineRpcApi::handle_engine_new_payload_v1;
    using EngineRpcApi::handle_engine_forkchoice_updated_v1;
    using EngineRpcApi::handle_engine_exchange_transition_configuration_v1;
};

using testing::InvokeWithoutArgs;

static silkworm::Bytes kBlockHash(32, '\0');
static silkworm::Bytes kChainConfig{*silkworm::from_hex(
        "7b22436861696e4e616d65223a22676f65726c69222c"
        "22636861696e4964223a352c"
        "22636f6e73656e737573223a22636c69717565222c"
        "22686f6d657374656164426c6f636b223a302c"
        "2264616f466f726b537570706f7274223a747275652c"
        "22656970313530426c6f636b223a302c"
        "2265697031353048617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22656970313535426c6f636b223a302c"
        "2262797a616e7469756d426c6f636b223a302c"
        "22636f6e7374616e74696e6f706c65426c6f636b223a302c"
        "2270657465727362757267426c6f636b223a302c"
        "22697374616e62756c426c6f636b223a313536313635312c"
        "226265726c696e426c6f636b223a343436303634342c"
        "226c6f6e646f6e426c6f636b223a353036323630352c"
        "227465726d696e616c546f74616c446966666963756c7479223a31303739303030302c"
        "227465726d696e616c546f74616c446966666963756c7479506173736564223a747275652c"
        "227465726d696e616c426c6f636b48617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22636c69717565223a7b22706572696f64223a31352c"
        "2265706f6368223a33303030307d7d")};
static silkworm::Bytes kChainConfigNoTerminalTotalDifficulty{*silkworm::from_hex(
        "7b22436861696e4e616d65223a22676f65726c69222c"
        "22636861696e4964223a352c"
        "22636f6e73656e737573223a22636c69717565222c"
        "22686f6d657374656164426c6f636b223a302c"
        "2264616f466f726b537570706f7274223a747275652c"
        "22656970313530426c6f636b223a302c"
        "2265697031353048617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22656970313535426c6f636b223a302c"
        "2262797a616e7469756d426c6f636b223a302c"
        "22636f6e7374616e74696e6f706c65426c6f636b223a302c"
        "2270657465727362757267426c6f636b223a302c"
        "22697374616e62756c426c6f636b223a313536313635312c"
        "226265726c696e426c6f636b223a343436303634342c"
        "226c6f6e646f6e426c6f636b223a353036323630352c"
        "227465726d696e616c546f74616c446966666963756c7479506173736564223a747275652c"
        "227465726d696e616c426c6f636b48617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22636c69717565223a7b22706572696f64223a31352c"
        "2265706f6368223a33303030307d7d")};
static silkworm::Bytes kChainConfigNoTerminalBlockHash{*silkworm::from_hex(
        "7b22436861696e4e616d65223a22676f65726c69222c"
        "22636861696e4964223a352c"
        "22636f6e73656e737573223a22636c69717565222c"
        "22686f6d657374656164426c6f636b223a302c"
        "2264616f466f726b537570706f7274223a747275652c"
        "22656970313530426c6f636b223a302c"
        "2265697031353048617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22656970313535426c6f636b223a302c"
        "2262797a616e7469756d426c6f636b223a302c"
        "22636f6e7374616e74696e6f706c65426c6f636b223a302c"
        "2270657465727362757267426c6f636b223a302c"
        "22697374616e62756c426c6f636b223a313536313635312c"
        "226265726c696e426c6f636b223a343436303634342c"
        "226c6f6e646f6e426c6f636b223a353036323630352c"
        "227465726d696e616c546f74616c446966666963756c7479223a31303739303030302c"
        "227465726d696e616c546f74616c446966666963756c7479506173736564223a747275652c"
        "22636c69717565223a7b22706572696f64223a31352c"
        "2265706f6368223a33303030307d7d")};
static silkworm::Bytes kChainConfigNoTerminalBlockNumber{*silkworm::from_hex(
        "7b22436861696e4e616d65223a22676f65726c69222c"
        "22636861696e4964223a352c"
        "22636f6e73656e737573223a22636c69717565222c"
        "22686f6d657374656164426c6f636b223a302c"
        "2264616f466f726b537570706f7274223a747275652c"
        "22656970313530426c6f636b223a302c"
        "2265697031353048617368223a22307830303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030222c"
        "22656970313535426c6f636b223a302c"
        "2262797a616e7469756d426c6f636b223a302c"
        "22636f6e7374616e74696e6f706c65426c6f636b223a302c"
        "2270657465727362757267426c6f636b223a302c"
        "22697374616e62756c426c6f636b223a313536313635312c"
        "226265726c696e426c6f636b223a343436303634342c"
        "226c6f6e646f6e426c6f636b223a353036323630352c"
        "227465726d696e616c546f74616c446966666963756c7479223a31303739303030302c"
        "227465726d696e616c546f74616c446966666963756c7479506173736564223a747275652c"
        "227465726d696e616c426c6f636b48617368223a223078"
        "30303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030303030227d")};

TEST_CASE("handle_engine_get_payload_v1 succeeds if request is expected payload", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock backend;
    EXPECT_CALL(backend, engine_get_payload_v1(1)).WillOnce(InvokeWithoutArgs(
        []() -> boost::asio::awaitable<ExecutionPayload> {
            co_return ExecutionPayload{1};
        }
    ));

    std::unique_ptr<ethbackend::BackEnd> backend_ptr(&backend);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_getPayloadV1",
        "params":["0x0000000000000001"]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    std::unique_ptr<ethdb::Database> database;
    // Initialise components
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();


    CHECK(reply ==  R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":{
            "baseFeePerGas":"0x0",
            "blockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "blockNumber":"0x1",
            "extraData":"0x",
            "gasLimit":"0x0",
            "gasUsed":"0x0",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "prevRandao": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "receiptsRoot": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "stateRoot": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "feeRecipient":"0x0000000000000000000000000000000000000000",
            "timestamp":"0x0",
            "transactions":null
         }
    })"_json);

    cp.stop();

    cp.stop();
    cp.join();
    backend_ptr.release();
}

TEST_CASE("handle_engine_get_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_getPayloadV1",
        "params":[]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(new BackEndMock);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_get_payload_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"invalid engine_getPayloadV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);

    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_new_payload_v1 succeeds if request is expected payload status", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock backend;
    EXPECT_CALL(backend, engine_new_payload_v1(testing::_)).WillOnce(InvokeWithoutArgs(
        []() -> boost::asio::awaitable<PayloadStatus> {
            co_return PayloadStatus{
                .status = "INVALID",
                .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
                .validation_error = "some error"
            };
        }
    ));

    std::unique_ptr<ethbackend::BackEnd> backend_ptr(&backend);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_newPayloadV1",
        "params":[{
            "parentHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
            "feeRecipient":"0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b",
            "stateRoot":"0xca3149fa9e37db08d1cd49c9061db1002ef1cd58db2210f2115c8c989b2bdf45",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "prevRandao":"0x0000000000000000000000000000000000000000000000000000000000000001",
            "blockNumber":"0x1",
            "gasLimit":"0x1c9c380",
            "gasUsed":"0x0",
            "timestamp":"0x5",
            "extraData":"0x",
            "baseFeePerGas":"0x7",
            "blockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858",
            "transactions":["0xf92ebdeab45d368f6354e8c5a8ac586c"]
        }]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    std::unique_ptr<ethdb::Database> database;
    // Initialise components
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_new_payload_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "id":1,
        "jsonrpc":"2.0",
        "result": {
               "latestValidHash":"0x0000000000000000000000000000000000000000000000000000000000000040",
               "status":"INVALID",
               "validationError":"some error"
        }
    })"_json);

    cp.stop();
    cp.join();
    backend_ptr.release();
}

TEST_CASE("handle_engine_new_payload_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_newPayloadV1",
        "params":[]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(new BackEndMock);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_new_payload_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"invalid engine_newPayloadV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);

    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 succeeds only with forkchoiceState", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock *backend = new BackEndMock;
    EXPECT_CALL(*backend, engine_forkchoice_updated_v1(testing::_)).WillOnce(InvokeWithoutArgs(
        []() -> boost::asio::awaitable<ForkChoiceUpdatedReply> {
            co_return ForkChoiceUpdatedReply{
                .payload_status = PayloadStatus{
                    .status = "INVALID",
                    .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
                    .validation_error = "some error"
                },
                .payload_id = std::nullopt
            };
        }
    ));

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedv1",
        "params":[
            {
                "headBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "safeBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "finalizedBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a"
            }
        ]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(backend);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();
    CHECK(reply ==  R"({
        "jsonrpc":"2.0",
        "id":1,
        "result": {
           "payloadStatus":{
               "latestValidHash":"0x0000000000000000000000000000000000000000000000000000000000000040",
               "status":"INVALID",
               "validationError":"some error"
           }
        }
    })"_json);
    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 succeeds with both params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock *backend = new BackEndMock;
    EXPECT_CALL(*backend, engine_forkchoice_updated_v1(testing::_)).WillOnce(InvokeWithoutArgs(
        []() -> boost::asio::awaitable<ForkChoiceUpdatedReply> {
            co_return ForkChoiceUpdatedReply{
                .payload_status = PayloadStatus{
                    .status = "INVALID",
                    .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
                    .validation_error = "some error"
                },
                .payload_id = std::nullopt
            };
        }
    ));

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedv1",
        "params":[
            {
                "headBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "safeBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "finalizedBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a"
            },
            {
                "timestamp":"0x1",
                "prevRandao":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "feeRecipient":"0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b"
            }
        ]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(backend);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();
    CHECK(reply ==  R"({
        "jsonrpc":"2.0",
        "id":1,
        "result": {
           "payloadStatus":{
               "latestValidHash":"0x0000000000000000000000000000000000000000000000000000000000000040",
               "status":"INVALID",
               "validationError":"some error"
           }
        }
    })"_json);
    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 succeeds with both params and second set to null", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock *backend = new BackEndMock;
    EXPECT_CALL(*backend, engine_forkchoice_updated_v1(testing::_)).WillOnce(InvokeWithoutArgs(
        []() -> boost::asio::awaitable<ForkChoiceUpdatedReply> {
            co_return ForkChoiceUpdatedReply{
                .payload_status = PayloadStatus{
                    .status = "INVALID",
                    .latest_valid_hash = 0x0000000000000000000000000000000000000000000000000000000000000040_bytes32,
                    .validation_error = "some error"
                },
                .payload_id = std::nullopt
            };
        }
    ));

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedv1",
        "params":[
            {
                "headBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "safeBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "finalizedBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a"
            },
            null
        ]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(backend);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();
    CHECK(reply ==  R"({
        "jsonrpc":"2.0",
        "id":1,
        "result": {
           "payloadStatus":{
               "latestValidHash":"0x0000000000000000000000000000000000000000000000000000000000000040",
               "status":"INVALID",
               "validationError":"some error"
           }
        }
    })"_json);
    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 fails with invalid amount of params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedV1",
        "params":[]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(new BackEndMock);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"invalid engine_forkchoiceUpdatedV1 params: []"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);

    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 fails with empty finalized block hash", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock *backend = new BackEndMock();
    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedv1",
        "params":[
            {
                "headBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "safeBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "finalizedBlockHash":"0x0000000000000000000000000000000000000000000000000000000000000000"
            }
        ]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(backend);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();
    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"finalized block hash is empty"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_forkchoice_updated_v1 fails with empty safe block hash", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    BackEndMock *backend = new BackEndMock();
    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_forkchoiceUpdatedv1",
        "params":[
            {
                "headBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a",
                "safeBlockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
                "finalizedBlockHash":"0x3b8fb240d288781d4aac94d3fd16809ee413bc99294a085798a589dae51ddd4a"
            }
        ]
    })"_json;
    // Initialize contex pool
    ContextPool cp{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    cp.start();
    // Initialise components
    std::unique_ptr<ethbackend::BackEnd> backend_ptr(backend);
    std::unique_ptr<ethdb::Database> database;
    EngineRpcApiTest rpc(database, backend_ptr);

    // spawn routine
    auto result{boost::asio::co_spawn(cp.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_forkchoice_updated_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();
    CHECK(reply ==  R"({
        "error":{
            "code":100,
            "message":"safe block hash is empty"
        },
        "id":1,
        "jsonrpc":"2.0" 
    })"_json);
    cp.stop();
    cp.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 succeeds if EL configurations has the same request configuration", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    const silkworm::Bytes block_number{*silkworm::from_hex("0000000000000000")};
    const silkworm::ByteView block_key {block_number};
    EXPECT_CALL(*mock_cursor, seek_exact(block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kBlockHash};
        }
    ));

    const silkworm::Bytes genesis_block_hash{*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")};
    const silkworm::ByteView genesis_block_key {genesis_block_hash};
    EXPECT_CALL(*mock_cursor, seek_exact(genesis_block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kChainConfig};
        }
    ));

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[{
            "terminalTotalDifficulty":"0xa4a470",
            "terminalBlockHash":"0x0000000000000000000000000000000000000000000000000000000000000000_bytes32",
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":{
            "terminalBlockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "terminalBlockNumber": "0x0",
            "terminalTotalDifficulty": "0xa4a470"
        }
    })"_json);

    context_pool.stop();
    context_pool.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 succeeds and default terminal block number to zero if chain config doesn't specify it", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    const silkworm::Bytes block_number{*silkworm::from_hex("0000000000000000")};
    const silkworm::ByteView block_key {block_number};
    EXPECT_CALL(*mock_cursor, seek_exact(block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kBlockHash};
        }
    ));

    const silkworm::Bytes genesis_block_hash{*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")};
    const silkworm::ByteView genesis_block_key {genesis_block_hash};
    EXPECT_CALL(*mock_cursor, seek_exact(genesis_block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kChainConfigNoTerminalBlockNumber};
        }
    ));

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[{
            "terminalTotalDifficulty":"0xa4a470",
            "terminalBlockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply ==  R"({
        "id":1,
        "jsonrpc":"2.0",
        "result":{
            "terminalBlockHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "terminalBlockNumber": "0x0",
            "terminalTotalDifficulty": "0xa4a470"
        }
    })"_json);

    context_pool.stop();
    context_pool.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if incorrect terminal total difficulty", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    const silkworm::Bytes block_number{*silkworm::from_hex("0000000000000000")};
    const silkworm::ByteView block_key {block_number};
    EXPECT_CALL(*mock_cursor, seek_exact(block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kBlockHash};
        }
    ));

    const silkworm::Bytes genesis_block_hash{*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")};
    const silkworm::ByteView genesis_block_key {genesis_block_hash};
    EXPECT_CALL(*mock_cursor, seek_exact(genesis_block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kChainConfig};
        }
    ));

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[{
            "terminalTotalDifficulty":"0xf4242",
            "terminalBlockHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"incorrect terminal total difficulty"
            },
            "id":1,
            "jsonrpc":"2.0"
        })"_json);
    context_pool.stop();
    context_pool.join();
}


TEST_CASE("handle_engine_transition_configuration_v1 fails if execution layer does not have terminal total difficulty", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    const silkworm::Bytes block_number{*silkworm::from_hex("0000000000000000")};
    const silkworm::ByteView block_key {block_number};
    EXPECT_CALL(*mock_cursor, seek_exact(block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kBlockHash};
        }
    ));

    const silkworm::Bytes genesis_block_hash{*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")};
    const silkworm::ByteView genesis_block_key {genesis_block_hash};
    EXPECT_CALL(*mock_cursor, seek_exact(genesis_block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kChainConfigNoTerminalTotalDifficulty};
        }
    ));

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[{
            "terminalTotalDifficulty":"0xf4240",
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858",
            "terminalBlockNumber":"0x0"
        }]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"execution layer does not have terminal total difficulty"
            },
            "id":1,
            "jsonrpc":"2.0"
        })"_json);
    context_pool.stop();
    context_pool.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if consensus layer sends block number different from zero", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    const silkworm::Bytes block_number{*silkworm::from_hex("0000000000000000")};
    const silkworm::ByteView block_key {block_number};
    EXPECT_CALL(*mock_cursor, seek_exact(block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kBlockHash};
        }
    ));

    const silkworm::Bytes genesis_block_hash{*silkworm::from_hex("0000000000000000000000000000000000000000000000000000000000000000")};
    const silkworm::ByteView genesis_block_key {genesis_block_hash};
    EXPECT_CALL(*mock_cursor, seek_exact(genesis_block_key)).WillOnce(InvokeWithoutArgs(
        [&]() -> boost::asio::awaitable<KeyValue> {
            co_return KeyValue{silkworm::Bytes{}, kChainConfig};
        }
    ));

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[{
            "terminalTotalDifficulty":"0xf4240",
            "terminalBlockHash":"0x3559e851470f6e7bbed1db474980683e8c315bfce99b2a6ef47c057c04de7858",
            "terminalBlockNumber":"0x1"
        }]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"consensus layer terminal block number is not zero"
            },
            "id":1,
            "jsonrpc":"2.0"
        })"_json);
    context_pool.stop();
    context_pool.join();
}

TEST_CASE("handle_engine_transition_configuration_v1 fails if incorrect params", "[silkrpc][engine_api]") {
    SILKRPC_LOG_VERBOSITY(LogLevel::None);

    silkrpc::ContextPool context_pool{1, []() { return grpc::CreateChannel("localhost", grpc::InsecureChannelCredentials()); }};
    context_pool.start();

    std::shared_ptr<MockCursor> mock_cursor = std::make_shared<MockCursor>();

    std::unique_ptr<ethdb::Database> database_ptr = std::make_unique<DummyDatabase>(mock_cursor);
    std::unique_ptr<ethbackend::BackEnd> backend_ptr;
    EngineRpcApiTest rpc(database_ptr, backend_ptr);

    nlohmann::json reply;
    nlohmann::json request = R"({
        "jsonrpc":"2.0",
        "id":1,
        "method":"engine_transitionConfigurationV1",
        "params":[]
    })"_json;

    auto result{boost::asio::co_spawn(context_pool.next_io_context(), [&rpc, &reply, &request]() {
        return rpc.handle_engine_exchange_transition_configuration_v1(
            request,
            reply
        );
    }, boost::asio::use_future)};
    result.get();

    CHECK(reply == R"({
        "error":{
            "code":100,
            "message":"invalid engine_exchangeTransitionConfigurationV1 params: []"
            },
            "id":1,
            "jsonrpc":"2.0"
        })"_json);
    context_pool.stop();
    context_pool.join();
}
} // namespace silkrpc::commands
