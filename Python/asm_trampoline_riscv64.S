    .text
    .globl	_Py_trampoline_func_start
_Py_trampoline_func_start:
    addi    sp,sp,-16
    sd      ra,8(sp)
    jalr    a3
    ld      ra,8(sp)
    addi    sp,sp,16
    jr      ra
    .globl	_Py_trampoline_func_end
_Py_trampoline_func_end:
    .section        .note.GNU-stack,"",@progbits
