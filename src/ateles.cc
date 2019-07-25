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

#include "js/Conversions.h"

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

std::string
JSMapContext::add_fun(std::string source)
{
    JSAutoRequest ar(this->_ctx);

    // TODO: Turn this into a std::unique_ptr and use
    // return instead of goto error
    size_t len = 0;
    char* converted = NULL;
    std::string ret = "";

    fprintf(stderr, "\r\n\r\nOHAI\r\n\r\n");

    {
        JSAutoCompartment ac(this->_ctx, *this->_conv_global);

        JSString* fun_src =
            JS_NewStringCopyN(this->_ctx, source.data(), source.size());
        if(!fun_src) {
            ret = "error_creating_conv_string";
            goto error;
        }
        JS::Value fun_src_val;
        fun_src_val.setString(fun_src);

        JS::HandleObject global_handle(this->_conv_global);
        JS::RootedValue func(this->_ctx);
        if(!JS_GetProperty(this->_ctx, global_handle, "rewrite_anon_fun", &func)) {
            ret = "failed_to_get_rewrite_func";
            goto error;
        }

        JS::CallArgs args = JS::CallArgsFromVp(1, &fun_src_val);
        JS::RootedValue rval(this->_ctx);
        if(!JS_CallFunctionValue(this->_ctx, NULL, func, args, &rval)) {
            JS::RootedValue exc(this->_ctx);
            if(!JS_GetPendingException(this->_ctx, &exc)) {
                ret = "unknown_error_converting_function";
            } else {
                JS::HandleValue exc_handle(exc);
                ret = this->format_exception(exc_handle);
            }
            goto error;
        }

        JSString* rstr = rval.toString();
        JSFlatString* flat_str = JS_FlattenString(this->_ctx, rstr);
        if(!flat_str) {
            ret = "error_flattening_string";
            goto error;
        }

        len = JS::GetDeflatedUTF8StringLength(flat_str);
        converted = (char*) JS_malloc(this->_ctx, len);
        if(!converted) {
            ret = "error_allocating_buffer";
            goto error;
        }

        JS::DeflateStringToUTF8Buffer(
            flat_str, mozilla::RangedPtr<char>(converted, len));
    }

    {
        JSAutoCompartment ac(this->_ctx, *this->_map_global);
        JSString* fun_src = JS_NewStringCopyN(this->_ctx, converted, len);
        if(!fun_src) {
            ret = "error_creating_fun_string";
            goto error;
        }
        JS::Value fun_src_val;
        fun_src_val.setString(fun_src);

        JS::HandleObject this_obj(this->_map_global);
        JS::CallArgs args = JS::CallArgsFromVp(1, &fun_src_val);
        JS::RootedValue rval(this->_ctx);
        if(!JS::Call(this->_ctx,
               this_obj,
               "add_fun",
               args,
               &rval)) {
           JS::RootedValue exc(this->_ctx);
           if(!JS_GetPendingException(this->_ctx, &exc)) {
               ret = "unknown_error_adding_function";
           } else {
               JS::HandleValue exc_handle(exc);
               ret = this->format_exception(exc_handle);
           }
           goto error;
        }
    }

    assert(converted != NULL);
    JS_free(this->_ctx, converted);

    return ret;

error:
    if(converted != NULL) {
        JS_free(this->_ctx, converted);
    }

    return ret;
}

std::vector<std::string>
JSMapContext::map_doc(std::string doc)
{
    return std::vector<std::string>();
}

std::string
JSMapContext::format_exception(JS::HandleValue exc)
{
    JSString* str = JS::ToString(this->_ctx, exc);

    JSFlatString* flat_str = JS_FlattenString(this->_ctx, str);
    if(!flat_str) {
        return "error_flattening_string";
    }

    size_t len = JS::GetDeflatedUTF8StringLength(flat_str);
    char* converted = (char*) JS_malloc(this->_ctx, len);
    if(!converted) {
        return "error_allocating_buffer";
    }

    JS::DeflateStringToUTF8Buffer(
        flat_str, mozilla::RangedPtr<char>(converted, len));

    return std::string(converted, len);
}



};  // namespace ateles