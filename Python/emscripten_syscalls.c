#include "emscripten.h"

// If we're running in node, report the UID of the user in the native system as
// the UID of the user. Since the nodefs will report the uid correctly, if we
// don't make getuid report it correctly too we'll see some permission errors.
// Normally __syscall_getuid32 is a stub that always returns 0 but it is
// defined with weak linkage so we can override it.
EM_JS(int, __syscall_getuid32_js, (void), {
    // If we're in node and we can, report the native uid
    if (ENVIRONMENT_IS_NODE) {
        return process.getuid();
    }
    // Fall back to the stub case of returning 0.
    return 0;
})

int __syscall_getuid32(void) {
    return __syscall_getuid32_js();
}

EM_JS(int, __syscall_umask_js, (int mask), {
    // If we're in node and we can, call native process.umask()
    if (ENVIRONMENT_IS_NODE) {
        try {
            return process.umask(mask);
        } catch(e) {
            // oops...
            // NodeJS docs: "In Worker threads, process.umask(mask) will throw an exception."
            // umask docs: "This system call always succeeds"
            return 0;
        }
    }
    // Fall back to the stub case of returning 0.
    return 0;
})

int __syscall_umask(int mask) {
    return __syscall_umask_js(mask);
}

#include <wasi/api.h>
#include <errno.h>
#undef errno

// Variant of EM_JS that does C preprocessor substitution on the body
#define EM_JS_MACROS(ret, func_name, args, body...)                            \
  EM_JS(ret, func_name, args, body)

EM_JS_MACROS(void, _emscripten_promising_main_js, (void), {
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
    const origResolveGlobalSymbol = resolveGlobalSymbol;
    if (!Module.onExit && process?.exit) {
        Module.onExit = (code) => process.exit(code);
    }
    // * wrap the main symbol with WebAssembly.promising,
    // * call exit_with_live_runtime() to prevent emscripten from shutting down
    //   the runtime before the promise resolves,
    // * call onExit / process.exit ourselves, since exit_with_live_runtime()
    //   prevented Emscripten from calling it normally.
    resolveGlobalSymbol = function (name, direct = false) {
        const orig = origResolveGlobalSymbol(name, direct);
        if (name === "main") {
            const main = WebAssembly.promising(orig.sym);
            orig.sym = (...args) => {
                (async () => {
                    const ret = await main(...args);
                    process?.exit?.(ret);
                })();
                _emscripten_exit_with_live_runtime();
            };
        }
        return orig;
    };
})

__attribute__((constructor)) void _emscripten_promising_main(void) {
    _emscripten_promising_main_js();
}


#define IOVEC_T_BUF_OFFSET 0
#define IOVEC_T_BUF_LEN_OFFSET 4
#define IOVEC_T_SIZE 8
_Static_assert(offsetof(__wasi_iovec_t, buf) == IOVEC_T_BUF_OFFSET,
               "Unexpected __wasi_iovec_t layout");
_Static_assert(offsetof(__wasi_iovec_t, buf_len) == IOVEC_T_BUF_LEN_OFFSET,
               "Unexpected __wasi_iovec_t layout");
_Static_assert(sizeof(__wasi_iovec_t) == IOVEC_T_SIZE,
               "Unexpected __wasi_iovec_t layout");

// If the stream has a readAsync handler, read to buffer defined in iovs, write
// number of bytes read to *nread, and return a promise that resolves to the
// errno. Otherwise, return null.
EM_JS_MACROS(__externref_t, __maybe_fd_read_async, (
    __wasi_fd_t fd,
    const __wasi_iovec_t *iovs,
    size_t iovcnt,
    __wasi_size_t *nread
), {
    if (!WebAssembly.promising) {
        return null;
    }
    var stream;
    try {
        stream = SYSCALLS.getStreamFromFD(fd);
    } catch (e) {
        // If the fd was already closed or never existed, getStreamFromFD()
        // raises. We'll let fd_read_orig() handle setting errno.
        return null;
    }
    if (!stream.stream_ops.readAsync) {
        // Not an async device. Fall back to __wasi_fd_read_orig().
        return null;
    }
    return (async () => {
        // This is the same as libwasi.js fd_read() and doReadv() except we use
        // readAsync and we await it.
        // https://github.com/emscripten-core/emscripten/blob/4.0.11/src/lib/libwasi.js?plain=1#L331
        // https://github.com/emscripten-core/emscripten/blob/4.0.11/src/lib/libwasi.js?plain=1#L197
        try {
            var ret = 0;
            for (var i = 0; i < iovcnt; i++) {
                var ptr = HEAP32[(iovs + IOVEC_T_BUF_OFFSET)/4];
                var len = HEAP32[(iovs + IOVEC_T_BUF_LEN_OFFSET)/4];
                iovs += IOVEC_T_SIZE;
                var curr = await stream.stream_ops.readAsync(stream, HEAP8, ptr, len);
                if (curr < 0) return -1;
                ret += curr;
                if (curr < len) break; // nothing more to read
            }
            HEAP32[nread/4] = ret;
            return 0;
        } catch (e) {
            if (e.name !== 'ErrnoError') {
                throw e;
            }
            return e.errno;
        }
    })();
};
);

// Bind original fd_read syscall to __wasi_fd_read_orig().
__wasi_errno_t __wasi_fd_read_orig(__wasi_fd_t fd, const __wasi_iovec_t *iovs,
                                   size_t iovs_len, __wasi_size_t *nread)
    __attribute__((__import_module__("wasi_snapshot_preview1"),
                   __import_name__("fd_read"), __warn_unused_result__));

// Take a promise that resolves to __wasi_errno_t and suspend until it resolves,
// get the output.
EM_JS(__wasi_errno_t, __block_for_errno, (__externref_t p), {
    return p;
}
if (WebAssembly.Suspending) {
    __block_for_errno = new WebAssembly.Suspending(__block_for_errno);
}
)

// Replacement for fd_read syscall. Call __maybe_fd_read_async. If it returned
// null, delegate back to __wasi_fd_read_orig. Otherwise, use __block_for_errno
// to get the result.
__wasi_errno_t __wasi_fd_read(__wasi_fd_t fd, const __wasi_iovec_t *iovs,
                              size_t iovs_len, __wasi_size_t *nread) {
  __externref_t p = __maybe_fd_read_async(fd, iovs, iovs_len, nread);
  if (__builtin_wasm_ref_is_null_extern(p)) {
    return __wasi_fd_read_orig(fd, iovs, iovs_len, nread);
  }
  __wasi_errno_t res = __block_for_errno(p);
  return res;
}
