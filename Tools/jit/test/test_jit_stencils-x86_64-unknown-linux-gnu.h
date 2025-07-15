void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
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
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000002:  R_X86_64_64  _JIT_OPARG
    // a: 66 85 c0                      testw   %ax, %ax
    // d: 0f 85 00 00 00 00             jne     0x13 <_JIT_ENTRY+0x13>
    // 000000000000000f:  R_X86_64_PLT32       _JIT_JUMP_TARGET-0x4
    const unsigned char code_body[19] = {
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x66, 0x85, 0xc0, 0x0f, 0x85,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x2, instruction->oparg);
    patch_32r(code + 0xf, state->instruction_starts[instruction->jump_target] + -0x4);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000002:  R_X86_64_64  _JIT_OPARG
    // a: 66 85 c0                      testw   %ax, %ax
    // d: 0f 85 00 00 00 00             jne     0x13 <_JIT_ENTRY+0x13>
    // 000000000000000f:  R_X86_64_PLT32       _JIT_ERROR_TARGET-0x4
    const unsigned char code_body[19] = {
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x66, 0x85, 0xc0, 0x0f, 0x85,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x2, instruction->oparg);
    patch_32r(code + 0xf, state->instruction_starts[instruction->error_target] + -0x4);
}

void
emit_3(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 49 c7 86 10 01 00 00 00 00 00 00      movq    $0x0, 0x110(%r14)
    // b: 4d 89 6c 24 40                movq    %r13, 0x40(%r12)
    // 10: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000012:  R_X86_64_64  _JIT_TARGET
    // 1a: 89 c1                         movl    %eax, %ecx
    // 1c: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 000000000000001e:  R_X86_64_64  _JIT_OPERAND0
    // 26: 48 01 c8                      addq    %rcx, %rax
    // 29: c3                            retq
    const unsigned char code_body[42] = {
        0x49, 0xc7, 0x86, 0x10, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x4d, 0x89, 0x6c, 0x24, 0x40,
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x89, 0xc1, 0x48, 0xb8, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x01,
        0xc8, 0xc3,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x12, instruction->target);
    patch_64(code + 0x1e, instruction->operand0);
}

void
emit_4(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000002:  R_X86_64_64  _JIT_OPERAND1
    // a: 49 89 86 10 01 00 00          movq    %rax, 0x110(%r14)
    // 11: 48 b8 00 00 00 00 00 00 00 00 movabsq $0x0, %rax
    // 0000000000000013:  R_X86_64_64  _JIT_OPERAND1+0x78
    // 1b: 48 8b 00                      movq    (%rax), %rax
    // 1e: ff e0                         jmpq    *%rax
    const unsigned char code_body[32] = {
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x49, 0x89, 0x86, 0x10, 0x01, 0x00,
        0x00, 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x48, 0x8b, 0x00, 0xff, 0xe0,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_64(code + 0x2, instruction->operand1);
    patch_64(code + 0x13, instruction->operand1 + 0x78);
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
    [1] = {emit_1, 19, 0, {0}},
    [2] = {emit_2, 19, 0, {0}},
    [3] = {emit_3, 42, 0, {0}},
    [4] = {emit_4, 32, 0, {0}},
};

static const void * const symbols_map[1] = {
    0
};
