    .text
    .globl	_Py_trampoline_func_start
# The following assembly is equivalent to:
# PyObject *
# trampoline(PyThreadState *ts, _PyInterpreterFrame *f,
#            int throwflag, py_evaluator evaluator)
# {
#     return evaluator(ts, f, throwflag);
# }
_Py_trampoline_func_start:
#ifdef __x86_64__
    sub    $8, %rsp
    call    *%rcx
    add    $8, %rsp
    ret
#endif // __x86_64__
#if defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
    // ARM64 little endian, 64bit ABI
    // generate with aarch64-linux-gnu-gcc 12.1
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    blr     x3
    ldp     x29, x30, [sp], 16
    ret
#endif
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits
