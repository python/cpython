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
