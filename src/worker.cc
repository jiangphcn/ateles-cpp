// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#include "worker.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>

#include "errors.h"
#include "worker.h"

using namespace std::placeholders;

namespace ateles
{
Worker::Worker() : _task_lock(), _tasks()
{
    std::promise<bool> initedp;
    this->_inited = initedp.get_future();

    this->_thread =
        std::make_unique<std::thread>(&Worker::run, this, std::move(initedp));
}

void
Worker::exit()
{
    JSFuture f = this->add_task(task_exit);
    f.wait();
}

JSFuture
Worker::set_lib(const std::string& lib)
{
    return this->add_task(std::bind(task_set_lib, _1, lib));
}

JSFuture
Worker::add_map_fun(const std::string& source)
{
    return this->add_task(std::bind(task_add_map_fun, _1, source));
}

JSFuture
Worker::map_doc(const std::string& doc)
{
    return this->add_task(std::bind(task_map_doc, _1, doc));
}

void
Worker::run(std::promise<bool> inited)
{
    auto cx = std::make_unique<Context>();

    if(!cx) {
        inited.set_value_at_thread_exit(false);
        return;
    }
    inited.set_value(true);

    bool go = true;
    Task::Ptr task;
    while(go) {
        if(!this->get_task(task)) {
            continue;
        }
        go = (*task)(cx.get());
    }
}

JSFuture
Worker::add_task(Task::TaskCallBack cb)
{
    Task::Ptr task = Task::create(cb);
    JSFuture ret = task->get_future();
    std::unique_lock<std::mutex> lock(this->_task_lock);
    this->_tasks.push(std::move(task));
    this->_task_cv.notify_one();
    return ret;
}

bool
Worker::get_task(Task::Ptr& task)
{
    std::unique_lock<std::mutex> lock(this->_task_lock);
    while(this->_tasks.empty()) {
        this->_task_cv.wait(lock);
    }

    if(this->_tasks.empty()) {
        return false;
    }

    task = std::move(this->_tasks.front());
    this->_tasks.pop();
    return true;
}

std::string
task_set_lib(Context* cx, const std::string& lib)
{
    return cx->set_lib(lib);
}

std::string
task_add_map_fun(Context* cx, const std::string& source)
{
    return cx->add_map_fun(source);
}

std::string
task_map_doc(Context* cx, const std::string& doc)
{
    return cx->map_doc(doc);
}

std::string
task_exit(Context* cx)
{
    throw AtelesExit();
}

}  // namespace ateles