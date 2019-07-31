
#include "js.h"

#include <sstream>

#include "ateles_map.h"
#include "errors.h"
#include "escodegen.h"
#include "esprima.h"
#include "js/Conversions.h"
#include "rewrite_anon_fun.h"

namespace ateles
{
enum class PrintErrorKind
{
    Error,
    Warning,
    StrictWarning,
    Note
};

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

static bool
print_fun(JSContext* cx, unsigned argc, JS::Value* vp)
{
    JS::CallArgs args = CallArgsFromVp(argc, vp);

    for(int i = 0; i < args.length(); i++) {
        fprintf(stderr, "> %s\n", format_value(cx, args[i]).c_str());
    }

    return true;
}

JSContext*
create_context()
{
    JSContext* cx = JS_NewContext(8L * 1024 * 1024);
    if(!cx) {
        throw AtelesError("Error creating JSContext");
    }
    if(!JS::InitSelfHostedCode(cx)) {
        throw AtelesError("Error initializing self hosted code.");
    }
    return cx;
}

Context::Context() : _cx(create_context(), JS_DestroyContext)
{
    JSAutoRequest ar(this->_cx.get());

    JS::CompartmentOptions options;
    this->_conv_global = new JS::RootedObject(this->_cx.get(),
        JS_NewGlobalObject(this->_cx.get(),
            &global_class,
            nullptr,
            JS::FireOnNewGlobalHook,
            options));

    if(!this->_conv_global) {
        throw AtelesError("Error creating converter global object.");
    }

    this->_global = new JS::RootedObject(this->_cx.get(),
        JS_NewGlobalObject(this->_cx.get(),
            &global_class,
            nullptr,
            JS::FireOnNewGlobalHook,
            options));

    if(!this->_global) {
        throw AtelesError("Error creating context global object.");
    }

    {
        // Scope to the conv compartment
        JSAutoCompartment ac(this->_cx.get(), *this->_conv_global);
        JS_InitStandardClasses(this->_cx.get(), *this->_conv_global);

        load_script(this->_cx.get(),
            "esprima.js",
            std::string((char*) esprima_data, esprima_len));
        load_script(this->_cx.get(),
            "escodegen.js",
            std::string((char*) escodegen_data, escodegen_len));
        load_script(this->_cx.get(),
            "rewrite_anon_fun.js",
            std::string((char*) rewrite_anon_fun_data, rewrite_anon_fun_len));
    }

    {
        // Scope to the map compartment
        JSAutoCompartment ac(this->_cx.get(), *this->_global);
        JS_InitStandardClasses(this->_cx.get(), *this->_global);

        load_script(this->_cx.get(),
            "ateles_map.js",
            std::string((char*) ateles_map_data, ateles_map_len));

        JS::HandleObject global_obj(this->_global);
        if(!JS_DefineFunction(
               this->_cx.get(), global_obj, "print", print_fun, 1, 0)) {
            throw AtelesError("Error installing print function.");
        }
    }
}

std::string
Context::set_lib(const std::string& lib)
{
    return "";
}

std::string
Context::add_map_fun(const std::string& source)
{
    std::string conv = this->transpile(source);

    JSAutoRequest ar(this->_cx.get());
    JSAutoCompartment ac(this->_cx.get(), *this->_global);

    JS::HandleObject this_obj(this->_global);

    JSString* fun_src =
        JS_NewStringCopyN(this->_cx.get(), conv.c_str(), conv.size());
    if(!fun_src) {
        throw AtelesResourceExhaustedError(
            "Error creating JavaScript string object.");
    }

    JS::AutoValueArray<1> argv(this->_cx.get());
    argv[0].setString(fun_src);

    JS::RootedValue rval(this->_cx.get());

    if(!JS::Call(this->_cx.get(), this_obj, "add_fun", argv, &rval)) {
        JS::RootedValue exc(this->_cx.get());
        if(!JS_GetPendingException(this->_cx.get(), &exc)) {
            throw AtelesError("Unknown error when adding map function.");
        } else {
            JS_ClearPendingException(this->_cx.get());
            throw AtelesInvalidArgumentError("Error adding map function: "
                + format_exception(this->_cx.get(), exc));
        }
    }

    return "";
}

std::string
Context::map_doc(const std::string& doc)
{
    JSAutoRequest ar(this->_cx.get());
    JSAutoCompartment ac(this->_cx.get(), *this->_global);

    JS::HandleObject this_obj(this->_global);

    JSString* js_doc =
        JS_NewStringCopyN(this->_cx.get(), doc.c_str(), doc.size());
    if(!js_doc) {
        throw AtelesResourceExhaustedError(
            "Error creating JavaScript string object.");
    }

    JS::AutoValueArray<1> argv(this->_cx.get());
    argv[0].setString(js_doc);

    JS::RootedValue rval(this->_cx.get());

    if(!JS::Call(this->_cx.get(), this_obj, "map_doc", argv, &rval)) {
        JS::RootedValue exc(this->_cx.get());
        if(!JS_GetPendingException(this->_cx.get(), &exc)) {
            throw AtelesError("Unknown mapping document.");
        } else {
            JS_ClearPendingException(this->_cx.get());
            throw AtelesInvalidArgumentError("Error mapping document: "
                + format_exception(this->_cx.get(), exc));
        }
    }

    return js_to_string(this->_cx.get(), rval);
}

std::string
Context::transpile(const std::string& source)
{
    JSAutoRequest ar(this->_cx.get());
    JSAutoCompartment ac(this->_cx.get(), *this->_conv_global);

    JSString* fun_src =
        JS_NewStringCopyN(this->_cx.get(), source.data(), source.size());
    if(!fun_src) {
        throw AtelesResourceExhaustedError("Error creating JavaScript string.");
    }

    JS::HandleObject this_obj(this->_conv_global);

    JS::AutoValueArray<1> argv(this->_cx.get());
    argv[0].setString(fun_src);
    JS::RootedValue rval(this->_cx.get());

    if(!JS::Call(this->_cx.get(), this_obj, "rewrite_anon_fun", argv, &rval)) {
        JS::RootedValue exc(this->_cx.get());
        if(!JS_GetPendingException(this->_cx.get(), &exc)) {
            throw AtelesError(
                "Unknown error converting anonymous JavaScript function.");
        } else {
            JS_ClearPendingException(this->_cx.get());
            throw AtelesInvalidArgumentError("Invalid JavaScript function: "
                + format_exception(this->_cx.get(), exc));
        }
    }

    return js_to_string(this->_cx.get(), rval);
}

std::string
js_to_string(JSContext* cx, JS::HandleValue val)
{
    JS::RootedString sval(cx);
    sval = val.toString();

    JS::UniqueChars chars(JS_EncodeStringToUTF8(cx, sval));
    if(!chars) {
        JS_ClearPendingException(cx);
        throw AtelesInvalidArgumentError("Error converting value to string.");
    }

    return chars.get();
}

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

void
load_script(JSContext* cx, std::string name, std::string source)
{
    JS::RootedValue rval(cx);
    JS::CompileOptions opts(cx);
    opts.setFileAndLine(name.c_str(), 1);
    if(!JS::Evaluate(cx, opts, source.c_str(), source.size(), &rval)) {
        JS::RootedValue exc(cx);
        if(!JS_GetPendingException(cx, &exc)) {
            throw AtelesError("Unknown error evaluating script: " + name);
        } else {
            JS_ClearPendingException(cx);
            throw AtelesError(
                "Error evaluating script: " + format_exception(cx, exc));
        }
    }
}

}  // namespace ateles