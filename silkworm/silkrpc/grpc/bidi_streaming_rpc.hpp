/*
    Copyright 2022 The Silkrpc Authors

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

#include <memory>
#include <optional>
#include <system_error>
#include <utility>

#include <silkworm/silkrpc/config.hpp>

#include <agrpc/rpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/experimental/append.hpp>
#include <grpcpp/grpcpp.h>

#include <silkworm/silkrpc/common/log.hpp>
#include <silkworm/silkrpc/grpc/error.hpp>
#include <silkworm/silkrpc/grpc/util.hpp>

namespace silkrpc {

namespace detail {
struct ReadDoneTag {
};
} // namespace detail

template<auto Rpc>
class BidiStreamingRpc;

template<
    typename Stub,
    typename Request,
    template<typename, typename> typename Responder,
    typename Reply,
    std::unique_ptr<Responder<Request, Reply>>(Stub::*PrepareAsync)(grpc::ClientContext*, grpc::CompletionQueue*)
>
class BidiStreamingRpc<PrepareAsync> {
private:
     struct ReadNext {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op, bool ok) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ok): " << this << " START\n";
            if (ok) {
                SILKRPC_DEBUG << "BidiStreamingRpc::ReadNext(op, ok): rw=" << self_.reader_writer_.get() << " before read\n";
                agrpc::read(self_.reader_writer_, self_.reply_,
                    boost::asio::bind_executor(self_.grpc_context_, boost::asio::experimental::append(std::move(op), detail::ReadDoneTag{})));
                SILKRPC_DEBUG << "BidiStreamingRpc::ReadNext(op, ok): rw=" << self_.reader_writer_.get() << " after read\n";
            } else {
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, bool ok, detail::ReadDoneTag) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ok, ReadDoneTag): " << this << " ok=" << ok << "\n";
            if (ok) {
                op.complete({}, self_.reply_);
            } else {
                self_.finish(std::move(op));
            }
        }

        template<typename Op>
        void operator()(Op& op, const boost::system::error_code& ec) {
            SILKRPC_TRACE << "BidiStreamingRpc::ReadNext(op, ec): " << this << " ec=" << ec << "\n";
            op.complete(ec, self_.reply_);
        }
    };

    struct RequestAndRead : ReadNext {
        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::RequestAndRead::initiate rw=" << this->self_.reader_writer_.get() << " START\n";
            agrpc::request(PrepareAsync, this->self_.stub_, this->self_.context_, this->self_.reader_writer_,
                boost::asio::bind_executor(this->self_.grpc_context_, std::move(op)));
            SILKRPC_TRACE << "BidiStreamingRpc::RequestAndRead::initiate rw=" << this->self_.reader_writer_.get() << " END\n";
        }

        using ReadNext::operator();
    };

    struct WriteAndRead : ReadNext {
        const Request& request;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::WriteAndRead::initiate " << this << "\n";
            if (this->self_.reader_writer_) {
                agrpc::write(this->self_.reader_writer_, request, boost::asio::bind_executor(this->self_.grpc_context_, std::move(op)));
            } else {
                op.complete(make_error_code(grpc::StatusCode::INTERNAL, "agrpc::write called before agrpc::request"), this->self_.reply_);
            }
        }

        using ReadNext::operator();
    };

    struct WritesDoneAndFinish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            if (self_.status_) {
                SILKRPC_DEBUG << "BidiStreamingRpc::WritesDoneAndFinish " << this << " already finished\n";
                if (self_.status_->ok()) {
                    op.complete({});
                } else {
                    op.complete(make_error_code(self_.status_->error_code(), self_.status_->error_message()));
                }
                return;
            }
            SILKRPC_TRACE << "BidiStreamingRpc::WritesDoneAndFinish::initiate " << this << "\n";
            if (self_.reader_writer_) {
                agrpc::writes_done(self_.reader_writer_, boost::asio::bind_executor(self_.grpc_context_, std::move(op)));
            } else {
                op.complete(make_error_code(grpc::StatusCode::INTERNAL, "agrpc::writes_done called before agrpc::request"));
            }
        }

        template<typename Op>
        void operator()(Op& op, bool ok) {
            SILKRPC_TRACE << "BidiStreamingRpc::WritesDoneAndFinish::completed " << this << " ok=" << ok << "\n";
            self_.finish(std::move(op));
        }

        template<typename Op>
        void operator()(Op& op, const boost::system::error_code& ec) {
            op.complete(ec);
        }
    };

    struct Finish {
        BidiStreamingRpc& self_;

        template<typename Op>
        void operator()(Op& op) {
            SILKRPC_TRACE << "BidiStreamingRpc::Finish::initiate " << this << "\n";
            self_.status_ = std::make_optional<grpc::Status>();
            agrpc::finish(self_.reader_writer_, *self_.status_, boost::asio::bind_executor(self_.grpc_context_, std::move(op)));
        }

        template<typename Op>
        void operator()(Op& op, bool ok) {
            // Check Finish result to treat any unknown error as such (strict)
            if (!ok) {
                self_.status_ = grpc::Status{grpc::StatusCode::UNKNOWN, "unknown error in finish"};
            }

            SILKRPC_DEBUG << "BidiStreamingRpc::Finish::completed ok=" << ok << " " << *self_.status_ << "\n";
            if (self_.status_->ok()) {
                op.complete({});
            } else {
                op.complete(make_error_code(self_.status_->error_code(), self_.status_->error_message()));
            }
        }
    };

public:
    explicit BidiStreamingRpc(Stub& stub, agrpc::GrpcContext& grpc_context)
        : stub_(stub), grpc_context_(grpc_context) {}

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto request_and_read(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply&)>(RequestAndRead{*this}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto write_and_read(const Request& request, CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code, Reply&)>(WriteAndRead{*this, request}, token);
    }

    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto writes_done_and_finish(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(WritesDoneAndFinish{*this}, token);
    }

    auto get_executor() const noexcept {
        return grpc_context_.get_executor();
    }

private:
    template<typename CompletionToken = agrpc::DefaultCompletionToken>
    auto finish(CompletionToken&& token = {}) {
        return boost::asio::async_compose<CompletionToken, void(boost::system::error_code)>(Finish{*this}, token);
    }

    Stub& stub_;
    agrpc::GrpcContext& grpc_context_;
    grpc::ClientContext context_;
    std::unique_ptr<Responder<Request, Reply>> reader_writer_;
    Reply reply_;
    std::optional<grpc::Status> status_;
};

} // namespace silkrpc

