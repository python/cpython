void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 55                            pushq   %rbp
    // 1: 48 89 e5                      movq    %rsp, %rbp
    // 4: 41 57                         pushq   %r15
    // 6: 41 56                         pushq   %r14
    // 8: 41 55                         pushq   %r13
    // a: 41 54                         pushq   %r12
    // c: 53                            pushq   %rbx
    // d: 50                            pushq   %rax
    // e: 49 89 fc                      movq    %rdi, %r12
    // 11: 49 89 f5                      movq    %rsi, %r13
    // 14: 49 89 d6                      movq    %rdx, %r14
    // 17: e8 0f 00 00 00                callq   0x2b <__JIT_ENTRY+0x2b>
    // 1c: 48 83 c4 08                   addq    $0x8, %rsp
    // 20: 5b                            popq    %rbx
    // 21: 41 5c                         popq    %r12
    // 23: 41 5d                         popq    %r13
    // 25: 41 5e                         popq    %r14
    // 27: 41 5f                         popq    %r15
    // 29: 5d                            popq    %rbp
    // 2a: c3                            retq
    const unsigned char code_body[43] = {
        0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x41, 0x56,
        0x41, 0x55, 0x41, 0x54, 0x53, 0x50, 0x49, 0x89,
        0xfc, 0x49, 0x89, 0xf5, 0x49, 0x89, 0xd6, 0xe8,
        0x0f, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4, 0x08,
        0x5b, 0x41, 0x5c, 0x41, 0x5d, 0x41, 0x5e, 0x41,
        0x5f, 0x5d, 0xc3,
    };
    memcpy(code, code_body, sizeof(code_body));
}

void
emit_0(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 55                            pushq   %rbp
    // 1: 48 89 e5                      movq    %rsp, %rbp
    // 4: 5d                            popq    %rbp
    const unsigned char code_body[5] = {
        0x55, 0x48, 0x89, 0xe5, 0x5d,
    };
    memcpy(code, code_body, sizeof(code_body));
}

void
emit_1(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 55                            pushq   %rbp
    // 1: 48 89 e5                      movq    %rsp, %rbp
    // 4: 48 8b 05 00 00 00 00          movq    (%rip), %rax            ## 0xb <__JIT_ENTRY+0xb>
    // 0000000000000007:  X86_64_RELOC_GOT_LOAD        _sausage@GOTPCREL
    // b: 80 38 00                      cmpb    $0x0, (%rax)
    // e: 74 08                         je      0x18 <__JIT_ENTRY+0x18>
    // 10: ff 15 00 00 00 00             callq   *(%rip)                 ## 0x16 <__JIT_ENTRY+0x16>
    // 0000000000000012:  X86_64_RELOC_GOT     _order_eggs_sausage_and_bacon@GOTPCREL
    // 16: eb 06                         jmp     0x1e <__JIT_ENTRY+0x1e>
    // 18: ff 15 00 00 00 00             callq   *(%rip)                 ## 0x1e <__JIT_ENTRY+0x1e>
    // 000000000000001a:  X86_64_RELOC_GOT     _order_eggs_and_bacon@GOTPCREL
    // 1e: 48 8b 05 00 00 00 00          movq    (%rip), %rax            ## 0x25 <__JIT_ENTRY+0x25>
    // 0000000000000021:  X86_64_RELOC_GOT_LOAD        _spammed@GOTPCREL
    // 25: c6 00 00                      movb    $0x0, (%rax)
    // 28: 5d                            popq    %rbp
    const unsigned char code_body[41] = {
        0x55, 0x48, 0x89, 0xe5, 0x48, 0x8b, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x38, 0x00, 0x74, 0x08,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x06,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b,
        0x05, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00,
        0x5d,
    };
    // 0: &spammed+0x0
    // 8: &order_eggs_and_bacon+0x0
    // 10: &order_eggs_sausage_and_bacon+0x0
    // 18: &sausage+0x0
    patch_64(data + 0x0, (uintptr_t)&spammed);
    patch_64(data + 0x8, (uintptr_t)&order_eggs_and_bacon);
    patch_64(data + 0x10, (uintptr_t)&order_eggs_sausage_and_bacon);
    patch_64(data + 0x18, (uintptr_t)&sausage);
    memcpy(code, code_body, sizeof(code_body));
    patch_x86_64_32rx(code + 0x7, (uintptr_t)data + 0x14);
    patch_x86_64_32rx(code + 0x12, (uintptr_t)data + 0xc);
    patch_x86_64_32rx(code + 0x1a, (uintptr_t)data + 0x4);
    patch_x86_64_32rx(code + 0x21, (uintptr_t)data + -0x4);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 55                            pushq   %rbp
    // 1: 48 89 e5                      movq    %rsp, %rbp
    // 4: 48 8b 05 00 00 00 00          movq    (%rip), %rax            ## 0xb <__JIT_ENTRY+0xb>
    // 0000000000000007:  X86_64_RELOC_GOT_LOAD        _spam@GOTPCREL
    // b: 80 38 01                      cmpb    $0x1, (%rax)
    // e: 75 06                         jne     0x16 <__JIT_ENTRY+0x16>
    // 10: 5d                            popq    %rbp
    // 11: e9 00 00 00 00                jmp     0x16 <__JIT_ENTRY+0x16>
    // 0000000000000012:  X86_64_RELOC_BRANCH  __JIT_ERROR_TARGET
    // 16: 5d                            popq    %rbp
    const unsigned char code_body[23] = {
        0x55, 0x48, 0x89, 0xe5, 0x48, 0x8b, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x38, 0x01, 0x75, 0x06,
        0x5d, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x5d,
    };
    // 0: &spam+0x0
    patch_64(data + 0x0, (uintptr_t)&spam);
    memcpy(code, code_body, sizeof(code_body));
    patch_x86_64_32rx(code + 0x7, (uintptr_t)data + -0x4);
    patch_32r(code + 0x12, state->instruction_starts[instruction->error_target] + -0x4);
}

static_assert(SYMBOL_MASK_WORDS >= 1, "SYMBOL_MASK_WORDS too small");

typedef struct {
    void (*emit)(
        unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
        const _PyUOpInstruction *instruction, jit_state *state);
    size_t code_size;
    size_t data_size;
    symbol_mask trampoline_mask;
} StencilGroup;

static const StencilGroup shim = {emit_shim, 43, 0, {0}};

static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {
    [0] = {emit_0, 5, 0, {0}},
    [1] = {emit_1, 41, 32, {0}},
    [2] = {emit_2, 23, 8, {0}},
};

static const void * const symbols_map[1] = {
    0
};
