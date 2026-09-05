/* Emscripten setup for when Python is the program. Linked into the
 * interpreter only, never into libpython.
 */

#include "pyconfig.h"

#include <emscripten.h>
#include <errno.h>

// Variant of EM_JS that does C preprocessor substitution on the body
#define EM_JS_MACROS(ret, func_name, args, body...)                            \
  EM_JS(ret, func_name, args, body)

#ifdef Py_EMSCRIPTEN_DYNAMIC_LINKING
#define _Py_EM_WRAP_MAIN                                                       \
    const origResolveGlobalSymbol = resolveGlobalSymbol;                       \
    resolveGlobalSymbol = function (name, direct = false) {                    \
        const orig = origResolveGlobalSymbol(name, direct);                    \
        if (name === "main") {                                                 \
            orig.sym = _PyEM_promising(orig.sym);                              \
        }                                                                      \
        return orig;                                                           \
    };
#else
// wasmExports["main"], not _main: promising() rejects the export wrapper.
#define _Py_EM_WRAP_MAIN                                                       \
    _main = _PyEM_promising(wasmExports["main"]);
#endif

EM_JS_MACROS(void, _PyEmscripten_BeforeMain_js, (void), {
    // Define FS.createAsyncInputDevice(), This is quite similar to
    // FS.createDevice() defined here:
    // https://github.com/emscripten-core/emscripten/blob/4.0.11/src/lib/libfs.js?plain=1#L1642
    // but instead of returning one byte at a time, the input() function should
    // return a Uint8Array. This makes the handler code simpler, the
    // `createAsyncInputDevice` simpler, and everything faster.
    FS.createAsyncInputDevice = function(parent, name, input) {
        parent = typeof parent == 'string' ? parent : FS.getPath(parent);
        var path = PATH.join2(parent, name);
        var mode = FS_getMode(true, false);
        FS.createDevice.major ||= 64;
        var dev = FS.makedev(FS.createDevice.major++, 0);
        async function getDataBuf() {
            var buf;
            try {
                buf = await input();
            } catch (e) {
                throw new FS.ErrnoError(EIO);
            }
            if (!buf?.byteLength) {
                throw new FS.ErrnoError(EAGAIN);
            }
            ops._dataBuf = buf;
        }

        var ops = {
            _dataBuf: new Uint8Array(0),
            open(stream) {
                stream.seekable = false;
            },
            async readAsync(stream, buffer, offset, length, pos /* ignored */) {
                buffer = buffer.subarray(offset, offset + length);
                if (!ops._dataBuf.byteLength) {
                    await getDataBuf();
                }
                var toRead = Math.min(ops._dataBuf.byteLength, buffer.byteLength);
                buffer.subarray(0, toRead).set(ops._dataBuf);
                buffer = buffer.subarray(toRead);
                ops._dataBuf = ops._dataBuf.subarray(toRead);
                if (toRead) {
                    stream.node.atime = Date.now();
                }
                return toRead;
            },
        };
        FS.registerDevice(dev, ops);
        return FS.mkdev(path, mode, dev);
    };
    if (!WebAssembly.promising) {
        // No stack switching support =(
        return;
    }
    if (ENVIRONMENT_IS_NODE && !Module.onExit) {
        Module.onExit = (code) => process.exit(code);
    }
    _Py_EM_WRAP_MAIN
    // main() now runs on a promising stack, so libpython may suspend in a
    // syscall.
    Module.Py_EmscriptenStackSwitching = true;
}
// * wrap the entry point with WebAssembly.promising,
// * call exit_with_live_runtime() to prevent emscripten from shutting down
//   the runtime before the promise resolves,
// * call onExit / process.exit ourselves, since exit_with_live_runtime()
//   prevented Emscripten from calling it normally.
function _PyEM_promising(orig) {
    const main = WebAssembly.promising(orig);
    return (...args) => {
        (async () => {
            const ret = await main(...args);
            Module.onExit?.(ret);
        })();
        _emscripten_exit_with_live_runtime();
    };
}
)

#ifdef Py_EMSCRIPTEN_DYNAMIC_LINKING
EM_JS_DEPS(_PyEmscripten_BeforeMain,
           "$FS,$PATH,$FS_getMode,$resolveGlobalSymbol,"
           "emscripten_exit_with_live_runtime");
#else
EM_JS_DEPS(_PyEmscripten_BeforeMain,
           "$FS,$PATH,$FS_getMode,emscripten_exit_with_live_runtime");
#endif

__attribute__((constructor)) void _PyEmscripten_BeforeMain(void) {
    _PyEmscripten_BeforeMain_js();
}
