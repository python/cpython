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
    sub    $8, %rsp
    call    *%rcx
    add    $8, %rsp
    ret
#endif // __x86_64__
#if defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
    // ARM64 little endian, 64bit ABI
    // generate with aarch64-linux-gnu-gcc 12.1
#if defined(__ARM_FEATURE_PAC_DEFAULT) && (__ARM_FEATURE_PAC_DEFAULT & 1) == 1 || \
    defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1
    hint 25
#endif
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    blr     x3
    ldp     x29, x30, [sp], 16
#if defined(__ARM_FEATURE_PAC_DEFAULT) && (__ARM_FEATURE_PAC_DEFAULT & 1) == 1 || \
    defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1
    hint 29
#endif
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
#if defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
#if defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1 || \
  defined(__ARM_FEATURE_PAC_DEFAULT) && (__ARM_FEATURE_PAC_DEFAULT & 1) == 1
    .pushsection    .note.gnu.property, "a"
    .align  3
    .word   2f - 1f
    .word   4f - 3f
    .word   5            /* NT_GNU_PROPERTY_TYPE_0 */
1:  .asciz  "GNU"

2:  .align  3
3:  .word   0xc0000000   /* type: GNU_PROPERTY_AARCH64_FEATURE_1_AND */
    .word   6f - 5f      /* size */
5:  .word   3            /* value: GNU_PROPERTY_AARCH64_FEATURE_1_BTI */

6:  .align  3
4:  .popsection
#endif
#endif
