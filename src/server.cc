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

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "ateles.grpc.pb.h"
#include "worker.h"

using ateles::Ateles;
using ateles::Doc;
using ateles::MapContext;
using ateles::MapContextInfo;
using ateles::MapResult;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using std::chrono::system_clock;

class AtelesImpl final : public Ateles::Service {
  public:
    explicit AtelesImpl() {}

    Status CreateMapContext(ServerContext* context,
        const MapContextInfo* ctx_info,
        MapContext* ctx) override;

    Status MapDocs(ServerContext* context,
        ServerReaderWriter<MapResult, Doc>* stream) override;

  private:
      Worker::Ptr get_worker(std::string& ref);

    std::mutex _worker_lock;
    std::map<std::string, ateles::Worker::Ptr> _workers;
};

Status
CreateMapContext(ServerContext* context,
    const MapContextInfo* ctx_info,
    MapContext* ctx)
{
    std::unique_lock<std::mutex> lock(this->_worker_lock);

    std::string name = ctx_info->name();
    auto iter = this->_workers.find(name);
    if(iter != this->_workers.end()) {
        return Status(
            StatusCode::ALREADY_EXISTS, "This context already exists");
    }

    auto worker = ateles::Worker::create();

    auto fut = worker->set_lib(ctx_info->lib());
    fut.wait();
    Result result = fut.get();

    if(result->first) {
        return Status(StatusCode::INVALID_ARGUMENT, "Invalid library");
    }

    for(int i = 0; i < ctx_info->map_funs_size(); i++) {
        fut = worker->add_fun(ctx_info->map_funs(i));
        fut.wait();
        result = fut.get();

        if(result->first) {
            return Status(StatusCode::INVALID_ARGUMENT,
                "Invalid map function: " + ctx_info->map_funs(i));
        }
    }

    this->_workers[name] = worker;
    ctx->set_ref(name);
    return Status::OK;
}

Status
MapDocs(ServerContext* context, ServerReaderWriter<MapResult, Doc>* stream)
{
    Doc doc;
    while(stream->Read(&doc)) {
        auto worker = this->get_worker(doc.ref());
        if(!worker) {
            return Status(StatusCode::NOT_FOUND, "Missing context for ref");
        }

        std::string results;
        std::future<Result> fut = worker->map_doc(doc.body(), results);
        fut.wait();
        Result result = fut.get();

        MapResult r;
        r.set_ref(doc.ref());
        r.set_results(results);
        stream->Write(r);
    }
    return Status::OK;
}


Worker::Ptr
AtelesImpl::get_worker(std::string& ref)
{
    std::unique_lock<std::mutex> lock(_lock);

    auto iter = this->_workers.find(ref);
    if(iter == this->_workers.end()) {
        return Worker::Ptr();
    }

    return iter->second;
}


void
RunServer()
{
    std::string server_address("0.0.0.0:50051");
    AtelesImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Listening at: " << server_address << std::endl;
    server->Wait();
}

int
main(int argc, char** argv)
{
    JS_Init();
    // Docs say we have to create at least one JSContext
    // in a single threaded manner. So here we are.
    JS_NewContext(8L * 1024 * 1024);

    RunServer();

    return 0;
}