#ifndef ATELES_JS_H
#define ATELES_JS_H

#include <future>

#include "js/Initialization.h"
#include "jsapi.h"

namespace ateles
{
typedef std::promise<std::string> JSPromise;
typedef std::future<std::string> JSFuture;

class Context {
  public:
    explicit Context();

    std::string set_lib(const std::string& lib);
    std::string add_map_fun(const std::string& source);
    std::string map_doc(const std::string& doc);

  private:
    std::string transpile(const std::string& source);

    std::unique_ptr<JSContext, void (*)(JSContext*)> _cx;
    JS::RootedObject* _conv_global;
    JS::RootedObject* _global;
};

std::string js_to_string(JSContext* cx, JS::HandleValue val);
std::string format_string(JSContext* cx, JS::HandleString str);
std::string format_value(JSContext* cx, JS::HandleValue val);
std::string format_exception(JSContext* cx, JS::HandleValue exc);
void load_script(JSContext* cx, std::string name, std::string source);

}  // namespace ateles

#endif  // included js.h