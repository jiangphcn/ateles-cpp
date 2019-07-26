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
#include <sstream>

#include "js/Conversions.h"

#include "ateles_map.h"
#include "escodegen.h"
#include "esprima.h"
#include "rewrite_anon_fun.h"


enum class PrintErrorKind { Error, Warning, StrictWarning, Note };


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

std::string
format_string(JSContext* cx, JS::HandleString str)
{
    std::string buf = "\"";

    JS::UniqueChars chars(JS_EncodeStringToUTF8(cx, str));
    if(!chars) {
        JS_ClearPendingException(cx);
        return "[invalid string]";
    }

    buf += chars.get();
    buf += '"';

    return buf;
}

std::string
format_value(JSContext* cx, JS::HandleValue val)
{
    JS::RootedString str(cx);

    if(val.isString()) {
        str = val.toString();
        return format_string(cx, str);
    }

    str = JS::ToString(cx, val);

    if(!str) {
        JS_ClearPendingException(cx);
        str = JS_ValueToSource(cx, val);
    }

    if(!str) {
        JS_ClearPendingException(cx);
        if(val.isObject()) {
            const JSClass* klass = JS_GetClass(&val.toObject());
            if(klass) {
                str = JS_NewStringCopyZ(cx, klass->name);
            } else {
                return "[uknown object]";
            }
        } else {
            return "[unknown non-object]";
        }
    }

    if(!str) {
        JS_ClearPendingException(cx);
        return "[invalid class]";
    }

    JS::UniqueChars bytes(JS_EncodeStringToUTF8(cx, str));
    if(!bytes) {
        JS_ClearPendingException(cx);
        return "[invalid string]";
    }

    return bytes.get();
}


std::string
format_exception(JSContext* cx, JS::HandleValue exc)
{
    if(!exc.isObject()) {
        return "Exception is not an object";
    }

    JS::RootedObject exc_obj(cx, &exc.toObject());
    JSErrorReport* report = JS_ErrorFromException(cx, exc_obj);

    if(!report) {
        return format_value(cx, exc);
    }

    std::ostringstream prefix;
    if(report->filename) {
        prefix << report->filename << ':';
    }

    if(report->lineno) {
        prefix << report->lineno << ':' << report->column << ' ';
    }

    PrintErrorKind kind = PrintErrorKind::Error;
    if(JSREPORT_IS_WARNING(report->flags)) {
        if(JSREPORT_IS_STRICT(report->flags)) {
            kind = PrintErrorKind::StrictWarning;
        } else {
            kind = PrintErrorKind::Warning;
        }
    }

    if(kind != PrintErrorKind::Error) {
        const char* kindPrefix = nullptr;
        switch(kind) {
            case PrintErrorKind::Error:
                MOZ_CRASH("unreachable");
            case PrintErrorKind::Warning:
                kindPrefix = "warning";
                break;
            case PrintErrorKind::StrictWarning:
                kindPrefix = "strict warning";
                break;
            case PrintErrorKind::Note:
                kindPrefix = "note";
                break;
        }

        prefix << kindPrefix << ": ";
    }

    prefix << std::endl << report->message().c_str();

    return prefix.str();
}


bool
load_script(JSContext* ctx, const char* name, unsigned char* source, size_t len)
{
    JS::RootedValue rval(ctx);
    JS::CompileOptions opts(ctx);
    opts.setFileAndLine(name, 1);
    if(!JS::Evaluate(ctx, opts, (char*) source, len, &rval)) {
        JS::RootedValue exc(ctx);
        if(!JS_GetPendingException(ctx, &exc)) {
            fprintf(stderr, "Unknown error evaluating script: %s\n", name);
        } else {
            JS_ClearPendingException(ctx);
            fprintf(stderr, "Error evaluating script: %s\n", format_exception(ctx, exc).c_str());
        }
        return false;
    }
    return true;
}



static bool
print_fun(JSContext* cx, unsigned argc, JS::Value* vp)
{
    JS::CallArgs args = CallArgsFromVp(argc, vp);

    for(int i = 0; i < args.length(); i++) {
        fprintf(stderr, "> %s\n", format_value(cx, args[i]).c_str());
    }

    return true;
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

        load_script(this->_ctx, "esprima.js", esprima_data, esprima_len);
        load_script(this->_ctx, "escodegen.js", escodegen_data, escodegen_len);
        load_script(this->_ctx,
            "rewrite_anon_fun.js",
            rewrite_anon_fun_data,
            rewrite_anon_fun_len);

        JS::HandleObject covn_global_obj(this->_conv_global);
        if(!JS_DefineFunction(this->_ctx, covn_global_obj, "print", print_fun, 0, 0)) {
            return false;
        }

    }

    {
        // Scope to the map compartment
        JSAutoCompartment ac(this->_ctx, *this->_map_global);
        JS_InitStandardClasses(this->_ctx, *this->_map_global);

        load_script(
            this->_ctx, "ateles_map.js", ateles_map_data, ateles_map_len);

        JS::HandleObject map_global_obj(this->_map_global);
        if(!JS_DefineFunction(this->_ctx, map_global_obj, "print", print_fun, 0, 0)) {
            return false;
        }
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

    {
        JSAutoCompartment ac(this->_ctx, *this->_conv_global);

        JSString* fun_src =
            JS_NewStringCopyN(this->_ctx, source.data(), source.size());
        if(!fun_src) {
            ret = "error_creating_conv_string";
            goto error;
        }

        JS::HandleObject global_handle(this->_conv_global);
        JS::RootedValue func(this->_ctx);
        if(!JS_GetProperty(this->_ctx, global_handle, "rewrite_anon_fun", &func)) {
            ret = "failed_to_get_rewrite_func";
            goto error;
        }

        JS::AutoValueArray<1> argv(this->_ctx);
        argv[0].setString(fun_src);
        JS::RootedValue rval(this->_ctx);

        if(!JS_CallFunctionValue(this->_ctx, NULL, func, argv, &rval)) {
            JS::RootedValue exc(this->_ctx);
            if(!JS_GetPendingException(this->_ctx, &exc)) {
                ret = "unknown_error_converting_function";
            } else {
                JS_ClearPendingException(this->_ctx);
                ret = this->format_exception(exc);
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
               JS_ClearPendingException(this->_ctx);
               ret = this->format_exception(exc);
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
JSMapContext::format_string(JS::HandleString str)
{
    return ::format_string(this->_ctx, str);
}

std::string
JSMapContext::format_value(JS::HandleValue val)
{
    return ::format_value(this->_ctx, val);
}


std::string
JSMapContext::format_exception(JS::HandleValue exc)
{
    return ::format_exception(this->_ctx, exc);
}

};  // namespace ateles