/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <csignal>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "ateles.grpc.pb.h"
#include "lru.h"
#include "worker.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using std::chrono::system_clock;

namespace ateles
{
class AtelesImpl final : public Ateles::Service {
  public:
    explicit AtelesImpl() : _workers(50) {}

    Status CreateContext(ServerContext* cx,
        const CreateContextRequest* req,
        CreateContextResponse* resp) override;

    Status AddMapFuns(ServerContext* cx,
        const AddMapFunsRequest* req,
        AddMapFunsResponse* resp) override;

    Status MapDocs(ServerContext* cx,
        ServerReaderWriter<MapDocsResponse, MapDocsRequest>* stream) override;

  private:
    Worker::Ptr get_worker(const std::string& ref);

    std::mutex _worker_lock;
    LRU<std::string, Worker::Ptr> _workers;
};

Status
AtelesImpl::CreateContext(ServerContext* cx,
    const CreateContextRequest* req,
    CreateContextResponse* resp)
{
    std::unique_lock<std::mutex> lock(this->_worker_lock);

    std::string cxid = req->context_id();
    if(this->_workers.get(cxid)) {
        return Status(
            StatusCode::ALREADY_EXISTS, "The given context_id already exists.");
    }

    try {
        this->_workers.put(cxid, Worker::create());
    } catch(AtelesError& err) {
        return Status(err.code(), err.what());
    }

    return Status::OK;
}

Status
AtelesImpl::AddMapFuns(ServerContext* cx,
    const AddMapFunsRequest* req,
    AddMapFunsResponse* resp)
{
    auto worker = this->get_worker(req->context_id());
    if(!worker) {
        return Status(
            StatusCode::NOT_FOUND, "The given context_id does not exist.");
    }

    JSFuture futures[req->map_funs_size() + 1];

    futures[0] = worker->set_lib(req->lib());
    for(int i = 0; i < req->map_funs_size(); i++) {
        futures[i + 1] = worker->add_map_fun(req->map_funs(i));
    }

    try {
        for(int i = 0; i < req->map_funs_size() + 1; i++) {
            futures[i].get();
        }
    } catch(AtelesError& err) {
        return Status(err.code(), err.what());
    }

    return Status::OK;
}

Status
AtelesImpl::MapDocs(ServerContext* context,
    ServerReaderWriter<MapDocsResponse, MapDocsRequest>* stream)
{
    MapDocsRequest req;
    while(stream->Read(&req)) {
        auto worker = this->get_worker(req.context_id());
        if(!worker) {
            return Status(
                StatusCode::NOT_FOUND, "The given context_id does not exist.");
        }

        MapDocsResponse resp;

        try {
            auto f = worker->map_doc(req.doc());
            f.wait();
            resp.set_ok(true);
            resp.set_map_id(req.map_id());
            resp.set_result(f.get());
        } catch(AtelesError err) {
            resp.set_ok(false);
            resp.set_map_id(std::to_string(err.code()));
            resp.set_result(err.what());
        }

        stream->Write(resp);
    }
    return Status::OK;
}

Worker::Ptr
AtelesImpl::get_worker(const std::string& cxid)
{
    std::unique_lock<std::mutex> lock(_worker_lock);
    return this->_workers.get(cxid);
}

}  // namespace ateles

void
RunServer()
{
    std::string server_address("0.0.0.0:50051");
    ateles::AtelesImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Listening at: " << server_address << std::endl;
    server->Wait();
}

void
exit_cleanly(int signum)
{
    exit(1);
}

int
main(int argc, char** argv)
{
    std::signal(SIGINT, exit_cleanly);

    JS_Init();
    // Docs say we have to create at least one JSContext
    // in a single threaded manner. So here we are.
    JS_NewContext(8L * 1024 * 1024);

    RunServer();

    return 0;
}