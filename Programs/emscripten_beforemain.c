/* Emscripten setup for when Python is the program. Linked into the
 * interpreter only, never into libpython.
 *
 * Runs main() under WebAssembly.promising so that libpython can suspend the
 * wasm stack in a syscall (see Python/emscripten_syscalls.c).
 */

#include <emscripten.h>

EM_JS(void, _PyEmscripten_BeforeMain_js, (void), {
    if (!WebAssembly.promising) {
        // No stack switching support =(
        return;
    }
    if (ENVIRONMENT_IS_NODE && !Module.onExit) {
        Module.onExit = (code) => process.exit(code);
    }
    // promising() needs the raw export; _main may be a JS wrapper around it.
    const main = WebAssembly.promising(wasmExports.__main_argc_argv);
    _main = (...args) => {
        Module.Py_EmscriptenStackSwitching = true;
        // Exit the way callMain() would have, once main() is actually done.
        main(...args).then((ret) => exitJS(ret, true)).catch(handleException);
        // Unwind to callMain() without letting it exit: main() is still
        // running on the promising stack.
        throw "unwind";
    };
    // callMain() takes the entry point from _main, or from wasmImports.main
    // when linked with -sMAIN_MODULE.
    if ("main" in wasmImports) {
        wasmImports.main = _main;
    }
})

EM_JS_DEPS(_PyEmscripten_BeforeMain, "$exitJS,$handleException");

__attribute__((constructor)) void
_PyEmscripten_BeforeMain(void)
{
    _PyEmscripten_BeforeMain_js();
}
