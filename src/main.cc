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
#include <cstdlib>
#include <iostream>

#include "js/Initialization.h"
#include "jsapi.h"

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

int
main(int argc, const char* argv[])
{
    JS_Init();

    JSContext* cx = JS_NewContext(8L * 1024 * 1024);
    if(!cx) {
        return 1;
    }

    if(!JS::InitSelfHostedCode(cx)) {
        return 1;
    }

    {
        // Scope for our various stack objects (JSAutoRequest, RootedObject),
        // so they all go out of scope before we JS_DestroyContext.

        JSAutoRequest ar(cx);  // In practice, you would want to exit this any
                               // time you're spinning the event loop

        JS::CompartmentOptions options;
        JS::RootedObject global(cx,
            JS_NewGlobalObject(
                cx, &global_class, nullptr, JS::FireOnNewGlobalHook, options));
        if(!global) {
            return 1;
        }

        JS::RootedValue rval(cx);

        {  // Scope for JSAutoCompartment
            JSAutoCompartment ac(cx, global);
            JS_InitStandardClasses(cx, global);

            const char* script = "'hello'+` world, it is ${new Date()}`";
            const char* filename = "noname";
            int lineno = 1;
            JS::CompileOptions opts(cx);
            opts.setFileAndLine(filename, lineno);
            bool ok = JS::Evaluate(cx, opts, script, strlen(script), &rval);
            if(!ok) {
                return 1;
            }
        }

        JSString* str = rval.toString();
        printf("%s\n", JS_EncodeString(cx, str));
    }

    JS_DestroyContext(cx);
    JS_ShutDown();
    return 0;
}
