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

#include "ateles_map.h"
#include "escodegen.h"
#include "esprima.h"
#include "rewrite_anon_fun.h"

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

bool
load_script(JSContext* ctx, const char* name, unsigned char* source, size_t len)
{
    JS::RootedValue rval(ctx);
    JS::CompileOptions opts(ctx);
    opts.setFileAndLine(name, 1);
    return JS::Evaluate(ctx, opts, (char*) source, len, &rval);
}

namespace ateles
{
JSMapContext::JSMapContext(std::string lib) : _lib(lib) {}

bool
JSMapContext::init()
{
    this->_ctx = JS_NewContext(8L * 1024 * 1024);
    if(!this->_ctx) {
        return false;
    }

    if(!JS::InitSelfHostedCode(this->_ctx)) {
        return false;
    }

    JSAutoRequest ar(this->_ctx);

    JS::CompartmentOptions options;
    this->_conv_global = new JS::RootedObject(this->_ctx,
        JS_NewGlobalObject(this->_ctx,
            &global_class,
            nullptr,
            JS::FireOnNewGlobalHook,
            options));

    if(!this->_conv_global) {
        return false;
    }

    this->_map_global = new JS::RootedObject(this->_ctx,
        JS_NewGlobalObject(this->_ctx,
            &global_class,
            nullptr,
            JS::FireOnNewGlobalHook,
            options));

    if(!this->_map_global) {
        return false;
    }

    {
        // Scope to the conv compartment
        JSAutoCompartment ac(this->_ctx, *this->_conv_global);
        JS_InitStandardClasses(this->_ctx, *this->_conv_global);

        load_script(this->_ctx, "escodegen.js", escodegen_data, escodegen_len);
        load_script(this->_ctx, "esprima.js", esprima_data, esprima_len);
        load_script(this->_ctx,
            "rewrite_anon_fun.js",
            rewrite_anon_fun_data,
            rewrite_anon_fun_len);
    }

    {
        // Scope to the map compartment
        JSAutoCompartment ac(this->_ctx, *this->_map_global);
        JS_InitStandardClasses(this->_ctx, *this->_map_global);

        load_script(
            this->_ctx, "ateles_map.js", ateles_map_data, ateles_map_len);
    }

    return true;
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
};  // namespace ateles