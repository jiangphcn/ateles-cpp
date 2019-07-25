#ifndef ATELES_H
#define ATELES_H

#include <string>
#include <vector>

#include "js/Initialization.h"
#include "jsapi.h"

namespace ateles
{
class JSMapContext {
  public:
    typedef std::shared_ptr<JSMapContext> Ptr;

    static Ptr create(std::string lib)
    {
        Ptr ret(new JSMapContext(lib));

        if(ret->init()) {
            return ret;
        }

        return Ptr();
    }

    bool add_fun(std::string source);
    std::vector<std::string> map_doc(std::string doc);

  private:
    explicit JSMapContext(std::string lib);

    bool init();

    JSContext* _ctx;
    JS::RootedObject* _conv_global;
    JS::RootedObject* _map_global;
    std::string _lib;
};

}  // namespace ateles

#endif  // included ateles.h