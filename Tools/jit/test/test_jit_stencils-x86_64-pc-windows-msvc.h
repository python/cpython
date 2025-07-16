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
    // 8: 56                            pushq   %rsi
    // 9: 57                            pushq   %rdi
    // a: 53                            pushq   %rbx
    // b: 48 81 ec a0 00 00 00          subq    $0xa0, %rsp
    // 12: 44 0f 29 bc 24 90 00 00 00    movaps  %xmm15, 0x90(%rsp)
    // 1b: 44 0f 29 b4 24 80 00 00 00    movaps  %xmm14, 0x80(%rsp)
    // 24: 44 0f 29 6c 24 70             movaps  %xmm13, 0x70(%rsp)
    // 2a: 44 0f 29 64 24 60             movaps  %xmm12, 0x60(%rsp)
    // 30: 44 0f 29 5c 24 50             movaps  %xmm11, 0x50(%rsp)
    // 36: 44 0f 29 54 24 40             movaps  %xmm10, 0x40(%rsp)
    // 3c: 44 0f 29 4c 24 30             movaps  %xmm9, 0x30(%rsp)
    // 42: 44 0f 29 44 24 20             movaps  %xmm8, 0x20(%rsp)
    // 48: 0f 29 7c 24 10                movaps  %xmm7, 0x10(%rsp)
    // 4d: 0f 29 34 24                   movaps  %xmm6, (%rsp)
    // 51: 49 89 cc                      movq    %rcx, %r12
    // 54: 49 89 d5                      movq    %rdx, %r13
    // 57: 4d 89 c6                      movq    %r8, %r14
    // 5a: e8 52 00 00 00                callq   0xb1 <_JIT_ENTRY+0xb1>
    // 5f: 0f 28 34 24                   movaps  (%rsp), %xmm6
    // 63: 0f 28 7c 24 10                movaps  0x10(%rsp), %xmm7
    // 68: 44 0f 28 44 24 20             movaps  0x20(%rsp), %xmm8
    // 6e: 44 0f 28 4c 24 30             movaps  0x30(%rsp), %xmm9
    // 74: 44 0f 28 54 24 40             movaps  0x40(%rsp), %xmm10
    // 7a: 44 0f 28 5c 24 50             movaps  0x50(%rsp), %xmm11
    // 80: 44 0f 28 64 24 60             movaps  0x60(%rsp), %xmm12
    // 86: 44 0f 28 6c 24 70             movaps  0x70(%rsp), %xmm13
    // 8c: 44 0f 28 b4 24 80 00 00 00    movaps  0x80(%rsp), %xmm14
    // 95: 44 0f 28 bc 24 90 00 00 00    movaps  0x90(%rsp), %xmm15
    // 9e: 48 81 c4 a0 00 00 00          addq    $0xa0, %rsp
    // a5: 5b                            popq    %rbx
    // a6: 5f                            popq    %rdi
    // a7: 5e                            popq    %rsi
    // a8: 41 5c                         popq    %r12
    // aa: 41 5d                         popq    %r13
    // ac: 41 5e                         popq    %r14
    // ae: 41 5f                         popq    %r15
    // b0: c3                            retq
    const unsigned char code_body[177] = {
        0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
        0x56, 0x57, 0x53, 0x48, 0x81, 0xec, 0xa0, 0x00,
        0x00, 0x00, 0x44, 0x0f, 0x29, 0xbc, 0x24, 0x90,
        0x00, 0x00, 0x00, 0x44, 0x0f, 0x29, 0xb4, 0x24,
        0x80, 0x00, 0x00, 0x00, 0x44, 0x0f, 0x29, 0x6c,
        0x24, 0x70, 0x44, 0x0f, 0x29, 0x64, 0x24, 0x60,
        0x44, 0x0f, 0x29, 0x5c, 0x24, 0x50, 0x44, 0x0f,
        0x29, 0x54, 0x24, 0x40, 0x44, 0x0f, 0x29, 0x4c,
        0x24, 0x30, 0x44, 0x0f, 0x29, 0x44, 0x24, 0x20,
        0x0f, 0x29, 0x7c, 0x24, 0x10, 0x0f, 0x29, 0x34,
        0x24, 0x49, 0x89, 0xcc, 0x49, 0x89, 0xd5, 0x4d,
        0x89, 0xc6, 0xe8, 0x52, 0x00, 0x00, 0x00, 0x0f,
        0x28, 0x34, 0x24, 0x0f, 0x28, 0x7c, 0x24, 0x10,
        0x44, 0x0f, 0x28, 0x44, 0x24, 0x20, 0x44, 0x0f,
        0x28, 0x4c, 0x24, 0x30, 0x44, 0x0f, 0x28, 0x54,
        0x24, 0x40, 0x44, 0x0f, 0x28, 0x5c, 0x24, 0x50,
        0x44, 0x0f, 0x28, 0x64, 0x24, 0x60, 0x44, 0x0f,
        0x28, 0x6c, 0x24, 0x70, 0x44, 0x0f, 0x28, 0xb4,
        0x24, 0x80, 0x00, 0x00, 0x00, 0x44, 0x0f, 0x28,
        0xbc, 0x24, 0x90, 0x00, 0x00, 0x00, 0x48, 0x81,
        0xc4, 0xa0, 0x00, 0x00, 0x00, 0x5b, 0x5f, 0x5e,
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
    // 0: 48 83 ec 28                   subq    $0x28, %rsp
    // 4: 48 8b 05 00 00 00 00          movq    (%rip), %rax            # 0xb <_JIT_ENTRY+0xb>
    // 0000000000000007:  IMAGE_REL_AMD64_REL32        __imp_sausage
    // b: 80 38 00                      cmpb    $0x0, (%rax)
    // e: 74 08                         je      0x18 <_JIT_ENTRY+0x18>
    // 10: ff 15 00 00 00 00             callq   *(%rip)                 # 0x16 <_JIT_ENTRY+0x16>
    // 0000000000000012:  IMAGE_REL_AMD64_REL32        __imp_order_eggs_sausage_and_bacon
    // 16: eb 06                         jmp     0x1e <_JIT_ENTRY+0x1e>
    // 18: ff 15 00 00 00 00             callq   *(%rip)                 # 0x1e <_JIT_ENTRY+0x1e>
    // 000000000000001a:  IMAGE_REL_AMD64_REL32        __imp_order_eggs_and_bacon
    // 1e: 48 8b 05 00 00 00 00          movq    (%rip), %rax            # 0x25 <_JIT_ENTRY+0x25>
    // 0000000000000021:  IMAGE_REL_AMD64_REL32        __imp_spammed
    // 25: c6 00 00                      movb    $0x0, (%rax)
    // 28: 48 83 c4 28                   addq    $0x28, %rsp
    const unsigned char code_body[44] = {
        0x48, 0x83, 0xec, 0x28, 0x48, 0x8b, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x80, 0x38, 0x00, 0x74, 0x08,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x06,
        0xff, 0x15, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b,
        0x05, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00,
        0x48, 0x83, 0xc4, 0x28,
    };
    // 0: &sausage+0x0
    // 8: &order_eggs_sausage_and_bacon+0x0
    // 10: &order_eggs_and_bacon+0x0
    // 18: &spammed+0x0
    patch_64(data + 0x0, (uintptr_t)&sausage);
    patch_64(data + 0x8, (uintptr_t)&order_eggs_sausage_and_bacon);
    patch_64(data + 0x10, (uintptr_t)&order_eggs_and_bacon);
    patch_64(data + 0x18, (uintptr_t)&spammed);
    memcpy(code, code_body, sizeof(code_body));
    patch_x86_64_32rx(code + 0x7, (uintptr_t)data + -0x4);
    patch_x86_64_32rx(code + 0x12, (uintptr_t)data + 0x4);
    patch_x86_64_32rx(code + 0x1a, (uintptr_t)data + 0xc);
    patch_x86_64_32rx(code + 0x21, (uintptr_t)data + 0x14);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 48 8b 05 00 00 00 00          movq    (%rip), %rax            # 0x7 <_JIT_ENTRY+0x7>
    // 0000000000000003:  IMAGE_REL_AMD64_REL32        __imp_spam
    // 7: 80 38 01                      cmpb    $0x1, (%rax)
    // a: 0f 84 00 00 00 00             je      0x10 <_JIT_ENTRY+0x10>
    // 000000000000000c:  IMAGE_REL_AMD64_REL32        _JIT_ERROR_TARGET
    const unsigned char code_body[16] = {
        0x48, 0x8b, 0x05, 0x00, 0x00, 0x00, 0x00, 0x80,
        0x38, 0x01, 0x0f, 0x84,
    };
    // 0: &spam+0x0
    patch_64(data + 0x0, (uintptr_t)&spam);
    memcpy(code, code_body, sizeof(code_body));
    patch_x86_64_32rx(code + 0x3, (uintptr_t)data + -0x4);
    patch_x86_64_32rx(code + 0xc, state->instruction_starts[instruction->error_target] + -0x4);
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

static const StencilGroup shim = {emit_shim, 177, 0, {0}};

static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {
    [0] = {emit_0, 0, 0, {0}},
    [1] = {emit_1, 44, 32, {0}},
    [2] = {emit_2, 16, 8, {0}},
};

static const void * const symbols_map[1] = {
    0
};
