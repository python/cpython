    .text
#ifdef __x86_64__
#if defined(__APPLE__)
    .globl	__Py_trampoline_func_start
__Py_trampoline_func_start:
#else
    .globl	_Py_trampoline_func_start
_Py_trampoline_func_start:
#endif
    .cfi_startproc
#if defined(__CET__) && (__CET__ & 1)
    endbr64
#endif
    push   %rbp
    .cfi_def_cfa_offset 16
    .cfi_offset %rbp, -16
    mov    %rsp, %rbp
    .cfi_def_cfa_register %rbp
    call   *%rcx
    pop    %rbp
    .cfi_def_cfa %rsp, 8
    ret
    .cfi_endproc
#if defined(__APPLE__)
    .globl	__Py_trampoline_func_end
__Py_trampoline_func_end:
#else
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits
#endif
# Note for indicating the assembly code supports CET
#if defined(__CET__) && (__CET__ & 1)
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
#endif
#endif // __x86_64__
