void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 41 57                         pushq   %r15
    // 2: 41 56                         pushq   %r14
    // 4: 41 55                         pushq   %r13
    // 6: 41 54                         pushq   %r12
    // 8: 53                            pushq   %rbx
    // 9: 49 89 fc                      movq    %rdi, %r12
    // c: 49 89 f5                      movq    %rsi, %r13
    // f: 49 89 d6                      movq    %rdx, %r14
    // 12: e8 0a 00 00 00                callq   0x21 <_JIT_ENTRY+0x21>
    // 17: 5b                            popq    %rbx
    // 18: 41 5c                         popq    %r12
    // 1a: 41 5d                         popq    %r13
    // 1c: 41 5e                         popq    %r14
    // 1e: 41 5f                         popq    %r15
    // 20: c3                            retq
    const unsigned char code_body[33] = {
        0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
        0x53, 0x49, 0x89, 0xfc, 0x49, 0x89, 0xf5, 0x49,
        0x89, 0xd6, 0xe8, 0x0a, 0x00, 0x00, 0x00, 0x5b,
        0x41, 0x5c, 0x41, 0x5d, 0x41, 0x5e, 0x41, 0x5f,
        0xc3,
    };
    memcpy(code, code_body, sizeof(code_body));
}

void
emit_0(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
}

void
emit_1(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 50                            pushq   %rax
    // 1: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000003:  R_X86_64_64  sausage
    // b: 80 38 00                      cmpb    $0x0, (%rax)
    // e: 74 08                         je      0x18 <_JIT_ENTRY+0x18>
    // 10: ff 15 00 00 00 00             callq   *(%rip)                 # 0x16 <_JIT_ENTRY+0x16>
    // 0000000000000012:  R_X86_64_GOTPCRELX   order_eggs_sausage_and_bacon-0x4
    // 16: eb 06                         jmp     0x1e <_JIT_ENTRY+0x1e>
    // 18: ff 15 00 00 00 00             callq   *(%rip)                 # 0x1e <_JIT_ENTRY+0x1e>
    // 000000000000001a:  R_X86_64_GOTPCRELX   order_eggs_and_bacon-0x4
    // 1e: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000020:  R_X86_64_64  spammed
    // 28: c6 00 00                      movb    $0x0, (%rax)
    // 2b: 58                            popq    %rax
    const unsigned char code_body[44] = {
        0x50, 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x38, 0x00, 0x74, 0x08,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x06,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0x48, 0xb8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xc6, 0x00, 0x00, 0x58,
    };
    // 0: &order_eggs_sausage_and_bacon+0x0
    // 8: &order_eggs_and_bacon+0x0
    patch_64(data + 0x0, (uintptr_t)&order_eggs_sausage_and_bacon);
    patch_64(data + 0x8, (uintptr_t)&order_eggs_and_bacon);
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x3, (uintptr_t)&sausage);
    patch_x86_64_32rx(code + 0x12, (uintptr_t)data + -0x4);
    patch_x86_64_32rx(code + 0x1a, (uintptr_t)data + 0x4);
    patch_64(code + 0x20, (uintptr_t)&spammed);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000002:  R_X86_64_64  spam
    // a: 80 38 01                      cmpb    $0x1, (%rax)
    // d: 0f 84 00 00 00 00             je      0x13 <_JIT_ENTRY+0x13>
    // 000000000000000f:  R_X86_64_PLT32       _JIT_ERROR_TARGET-0x4
    const unsigned char code_body[19] = {
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x38, 0x01, 0x0f, 0x84,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x2, (uintptr_t)&spam);
    patch_32r(code + 0xf, state->instruction_starts[instruction->error_target] + -0x4);
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

static const StencilGroup shim = {emit_shim, 33, 0, {0}};

static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {
    [0] = {emit_0, 0, 0, {0}},
    [1] = {emit_1, 44, 16, {0}},
    [2] = {emit_2, 19, 0, {0}},
};

static const void * const symbols_map[1] = {
    0
};
