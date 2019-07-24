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
#include "ateles.h"

#include <cstdlib>
#include <iostream>

static JSClassOps global_ops = {nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JS_GlobalObjectTraceHook};

/* The class of the global object. */
static JSClass global_class = {"global", JSCLASS_GLOBAL_FLAGS, &global_ops};

namespace ateles
{
JSMapContext::JSMapContext(std::string lib)
{
    this->_lib = lib;

    this->_ctx = JS_NewContext(8L * 1024 * 1024);
    if(!this->_ctx) {
        return;
    }

    if(!JS::InitSelfHostedCode(this->_ctx)) {
        return;
    }

    JSAutoRequest ar(this->_ctx);

    JS::CompartmentOptions options;
    this->_global_obj = new JS::RootedObject(this->_ctx,
        JS_NewGlobalObject(this->_ctx, &global_class, nullptr, JS::FireOnNewGlobalHook, options));

    if(!this->_global_obj) {
        return;
    }

    {
        JSAutoCompartment ac(this->_ctx, *this->_global_obj);
        JS_InitStandardClasses(this->_ctx, *this->_global_obj);
    }
}

bool
JSMapContext::add_fun(std::string source)
{
    return false;
}

std::vector<std::string>
JSMapContext::map_doc(std::string doc)
{
    return std::vector<std::string>();
}
}
;