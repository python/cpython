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
#if defined(__CET__) && (__CET__ & 1)
    endbr64
#endif
    push   %rbp
    mov    %rsp, %rbp
    call   *%rcx
    pop    %rbp
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
#ifdef __riscv
    addi    sp,sp,-16
    sd      ra,8(sp)
    jalr    a3
    ld      ra,8(sp)
    addi    sp,sp,16
    jr      ra
#endif
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits
# Note for indicating the assembly code supports CET
#if defined(__x86_64__) && defined(__CET__) && (__CET__ & 1)
    .section    .note.gnu.property,"a"
    .align 8
    .long    1f - 0f
    .long    4f - 1f
    .long    5
0:
    .string  "GNU"
1:
    .align 8
    .long    0xc0000002
    .long    3f - 2f
2:
    .long    0x3
3:
    .align 8
4:
#endif // __x86_64__
