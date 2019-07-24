#ifndef ATELES_H
#define ATELES_H

#include <string>
#include <vector>

#include "jsapi.h"
#include "js/Initialization.h"

namespace ateles
{
class JSMapContext {
  public:
    typedef std::shared_ptr<JSMapContext> Ptr;

    static Ptr create(std::string lib) { return Ptr(new JSMapContext(lib)); }

    bool add_fun(std::string source);
    std::vector<std::string> map_doc(std::string doc);

  private:
    explicit JSMapContext(std::string lib);

    JSContext* _ctx;
    JS::RootedObject* _global_obj;
    std::string _lib;
};

}  // namespace ateles

#endif  // included ateles.h