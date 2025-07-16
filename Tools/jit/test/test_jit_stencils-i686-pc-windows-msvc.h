void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
}

void
emit_0(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 00000000 <__JIT_ENTRY>:
    // 0: 8b 44 24 0c                   movl    0xc(%esp), %eax
    // 4: 8b 4c 24 08                   movl    0x8(%esp), %ecx
    // 8: 8b 54 24 04                   movl    0x4(%esp), %edx
    // c: 89 54 24 04                   movl    %edx, 0x4(%esp)
    // 10: 89 4c 24 08                   movl    %ecx, 0x8(%esp)
    // 14: 89 44 24 0c                   movl    %eax, 0xc(%esp)
    const unsigned char code_body[24] = {
        0x8b, 0x44, 0x24, 0x0c, 0x8b, 0x4c, 0x24, 0x08,
        0x8b, 0x54, 0x24, 0x04, 0x89, 0x54, 0x24, 0x04,
        0x89, 0x4c, 0x24, 0x08, 0x89, 0x44, 0x24, 0x0c,
    };
    memcpy(code, code_body, sizeof(code_body));
}

void
emit_1(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 00000000 <__JIT_ENTRY>:
    // 0: 53                            pushl   %ebx
    // 1: 57                            pushl   %edi
    // 2: 56                            pushl   %esi
    // 3: 8b 74 24 18                   movl    0x18(%esp), %esi
    // 7: 8b 7c 24 14                   movl    0x14(%esp), %edi
    // b: 8b 5c 24 10                   movl    0x10(%esp), %ebx
    // f: 80 3d 00 00 00 00 00          cmpb    $0x0, 0x0
    // 00000011:  IMAGE_REL_I386_DIR32 _sausage
    // 16: 74 07                         je      0x1f <__JIT_ENTRY+0x1f>
    // 18: e8 00 00 00 00                calll   0x1d <__JIT_ENTRY+0x1d>
    // 00000019:  IMAGE_REL_I386_REL32 _order_eggs_sausage_and_bacon
    // 1d: eb 05                         jmp     0x24 <__JIT_ENTRY+0x24>
    // 1f: e8 00 00 00 00                calll   0x24 <__JIT_ENTRY+0x24>
    // 00000020:  IMAGE_REL_I386_REL32 _order_eggs_and_bacon
    // 24: c6 05 00 00 00 00 00          movb    $0x0, 0x0
    // 00000026:  IMAGE_REL_I386_DIR32 _spammed
    // 2b: 89 5c 24 10                   movl    %ebx, 0x10(%esp)
    // 2f: 89 7c 24 14                   movl    %edi, 0x14(%esp)
    // 33: 89 74 24 18                   movl    %esi, 0x18(%esp)
    // 37: 5e                            popl    %esi
    // 38: 5f                            popl    %edi
    // 39: 5b                            popl    %ebx
    const unsigned char code_body[58] = {
        0x53, 0x57, 0x56, 0x8b, 0x74, 0x24, 0x18, 0x8b,
        0x7c, 0x24, 0x14, 0x8b, 0x5c, 0x24, 0x10, 0x80,
        0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x07,
        0xe8, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x05, 0xe8,
        0x00, 0x00, 0x00, 0x00, 0xc6, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x89, 0x5c, 0x24, 0x10, 0x89,
        0x7c, 0x24, 0x14, 0x89, 0x74, 0x24, 0x18, 0x5e,
        0x5f, 0x5b,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_32(code + 0x11, (uintptr_t)&sausage);
    patch_x86_64_32rx(code + 0x19, (uintptr_t)&order_eggs_sausage_and_bacon + -0x4);
    patch_x86_64_32rx(code + 0x20, (uintptr_t)&order_eggs_and_bacon + -0x4);
    patch_32(code + 0x26, (uintptr_t)&spammed);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 00000000 <__JIT_ENTRY>:
    // 0: 8b 54 24 0c                   movl    0xc(%esp), %edx
    // 4: 8b 4c 24 08                   movl    0x8(%esp), %ecx
    // 8: 8b 44 24 04                   movl    0x4(%esp), %eax
    // c: 80 3d 00 00 00 00 01          cmpb    $0x1, 0x0
    // 0000000e:  IMAGE_REL_I386_DIR32 _spam
    // 13: 75 11                         jne     0x26 <__JIT_ENTRY+0x26>
    // 15: 89 54 24 0c                   movl    %edx, 0xc(%esp)
    // 19: 89 4c 24 08                   movl    %ecx, 0x8(%esp)
    // 1d: 89 44 24 04                   movl    %eax, 0x4(%esp)
    // 21: e9 00 00 00 00                jmp     0x26 <__JIT_ENTRY+0x26>
    // 00000022:  IMAGE_REL_I386_REL32 __JIT_ERROR_TARGET
    // 26: 89 54 24 0c                   movl    %edx, 0xc(%esp)
    // 2a: 89 4c 24 08                   movl    %ecx, 0x8(%esp)
    // 2e: 89 44 24 04                   movl    %eax, 0x4(%esp)
    const unsigned char code_body[50] = {
        0x8b, 0x54, 0x24, 0x0c, 0x8b, 0x4c, 0x24, 0x08,
        0x8b, 0x44, 0x24, 0x04, 0x80, 0x3d, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x75, 0x11, 0x89, 0x54, 0x24,
        0x0c, 0x89, 0x4c, 0x24, 0x08, 0x89, 0x44, 0x24,
        0x04, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x89, 0x54,
        0x24, 0x0c, 0x89, 0x4c, 0x24, 0x08, 0x89, 0x44,
        0x24, 0x04,
    };
    memcpy(code, code_body, sizeof(code_body));
    patch_32(code + 0xe, (uintptr_t)&spam);
    patch_x86_64_32rx(code + 0x22, state->instruction_starts[instruction->error_target] + -0x4);
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

static const StencilGroup shim = {emit_shim, 0, 0, {0}};

static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {
    [0] = {emit_0, 24, 0, {0}},
    [1] = {emit_1, 58, 0, {0}},
    [2] = {emit_2, 50, 0, {0}},
};

static const void * const symbols_map[1] = {
    0
};
