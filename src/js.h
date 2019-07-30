#ifndef ATELES_JSUTIL_H
#    define ATELES_JSUTIL_H

#    include "js/Initialization.h"
#    include "jsapi.h"

namespace ateles
{
class Context {
  public:
    typedef std::shared_ptr<Context> Ptr;

    ~Context();

    Ptr create() { return std::make_shared<Context>(); }

    JSContext* _cx;
    JS::RootedObject* _conv_global;
    JS::RootedObject* _map_global;

  private:
    explicit Context();
};

bool task_set_lib(const std::string& lib, Context* cx);

std::string format_string(JSContext* cx, JS::HandleString str);
std::string format_value(JSContext* cx, JS::HandleValue val);
std::string format_exception(JSContext* cx, JS::HandleValue exc);
bool load_script(JSContext* cx, std::string name, std::string source);

}  // namespace ateles