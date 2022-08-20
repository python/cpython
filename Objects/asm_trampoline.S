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
#ifdef __aarch64__
    TODO
#endif
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits

