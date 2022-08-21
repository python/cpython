    .text
    .globl	_Py_trampoline_func_start
_Py_trampoline_func_start:
#ifdef __x86_64__
    push   %rbp
    mov    %rsp,%rbp
    mov    %rdi,%rax
    mov    %rsi,%rdi
    mov    %rdx,%rsi
    mov    %ecx,%edx
    call   *%rax
    pop    %rbp
    ret
#endif // __x86_64__
#if defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
    // ARM64 little endian, 64bit ABI
    // generate with aarch64-linux-gnu-gcc 12.1
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    mov     x4, x0
    mov     x0, x1
    mov     x1, x2
    mov     w2, w3
    blr     x4
    ldp     x29, x30, [sp], 16
    ret
#endif
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits
