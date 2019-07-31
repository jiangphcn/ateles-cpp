#ifndef ATELES_WORKER_H
#define ATELES_WORKER_H

#include <functional>
#include <future>
#include <queue>
#include <string>
#include <vector>

#include "errors.h"
#include "js.h"

namespace ateles
{
class Task {
  public:
    typedef std::function<std::string(Context*)> TaskCallBack;
    typedef std::unique_ptr<Task> Ptr;

    explicit Task(TaskCallBack cb) : _cb(cb), _promise() {}

    static Ptr create(TaskCallBack cb) { return std::make_unique<Task>(cb); }

    bool operator()(Context* cx)
    {
        try {
            this->_promise.set_value(this->_cb(cx));
            return true;
        } catch(AtelesExit&) {
            this->_promise.set_value_at_thread_exit("");
            return false;
        } catch(...) {
            this->_promise.set_exception(std::current_exception());
            return true;
        }
    }

    JSFuture get_future() { return this->_promise.get_future(); }

  private:
    TaskCallBack _cb;
    JSPromise _promise;
};

class Worker {
  public:
    typedef std::shared_ptr<Worker> Ptr;
    typedef std::pair<std::string, std::string> Result;

    explicit Worker();

    static Ptr create()
    {
        Ptr ret = std::make_unique<Worker>();
        ret->_inited.wait();
        if(ret->_inited.get()) {
            return ret;
        }
        return Ptr();
    }

    void exit();

    JSFuture set_lib(const std::string& lib);
    JSFuture add_map_fun(const std::string& source);
    JSFuture map_doc(const std::string& doc);

  private:
    void run(std::promise<bool> inited);

    JSFuture add_task(Task::TaskCallBack cb);
    bool get_task(Task::Ptr& task);

    std::unique_ptr<std::thread> _thread;

    std::mutex _task_lock;
    std::condition_variable _task_cv;
    std::queue<Task::Ptr> _tasks;
    std::future<bool> _inited;
};

std::string task_set_lib(Context* cx, const std::string& lib);
std::string task_add_map_fun(Context* cx, const std::string& source);
std::string task_map_doc(Context* cx, const std::string& body);
std::string task_exit(Context* cx);

}  // namespace ateles

#endif  // included worker.h