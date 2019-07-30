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

namespace ateles
{
Worker::Worker()::_task_lock(), _tasks(), _alive(true)
{
    this->_thread = std::make_unique<std::thread>(&Worker::run, this);
}

std::future<Result>
Worker::set_lib(const std::string& lib)
{
    std::unique_lock<std::mutex> lock(this->_task_lock);
    this->_tasks.push(std::bind(task_set_lib, lib));
}


void
Worker::run()
{
    auto cx = Context::create(),

         if(!cx)
    {
        // Probably need to figure out a better
        // synchronous thread startup
        std::unique_lock<std::mutex> lock(this->_task_lock);
        this->_alive = false;
        return;
    }

    bool go = true;
    while(go) {
        auto task = this->get_task();
        go = task(cx.get());
    }

    std::unique_lock<std::mutex> lock(this->_task_lock);
    this->_alive = false;
}

std::function<bool(Context*)>
Worker::get_task()
{
    std::unique_lock<std::mutex> lock(this->_task_lock);
    this->_task_cv.wait(lock, [] { return !this->_tasks.empty(); });

    if(this->_tasks.empty()) {
        continue;
    }

    auto task = this->_tasks.front();
    this->_tasks.pop();
    return task;
}
