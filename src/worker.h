#ifndef ATELES_H
#define ATELES_H

#include <function>
#include <string>
#include <vector>

#include "js.h"

namespace ateles
{
class Worker {
  public:
    typedef std::shared_ptr<Worker> Ptr;
    typedef std::pair<std::string, std::string> Result;

    static Ptr create() { return std::make_shared<Worker>(); }

    std::future<Result> set_lib(const std::string& lib);
    std::future<Result> add_fun(const std::string& source);
    std::future<Result> map_doc(const std::string& doc);

  private:
    explicit Worker();

    void run();

    std::function<bool(Context*)> get_task();

    sts::unique_ptr<std::thread> _thread;

    std::mutex _task_lock;
    std::condition_variable _task_cv;
    std::queue<std::function<bool(Context*)>> _tasks;
    bool _alive;
};

}  // namespace ateles

#endif  // included worker.h