void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 6db63bef      stp     d15, d14, [sp, #-0xa0]!
    // 4: 6d0133ed      stp     d13, d12, [sp, #0x10]
    // 8: 6d022beb      stp     d11, d10, [sp, #0x20]
    // c: 6d0323e9      stp     d9, d8, [sp, #0x30]
    // 10: a9046ffc      stp     x28, x27, [sp, #0x40]
    // 14: a90567fa      stp     x26, x25, [sp, #0x50]
    // 18: a9065ff8      stp     x24, x23, [sp, #0x60]
    // 1c: a90757f6      stp     x22, x21, [sp, #0x70]
    // 20: a9084ff4      stp     x20, x19, [sp, #0x80]
    // 24: a9097bfd      stp     x29, x30, [sp, #0x90]
    // 28: 910243fd      add     x29, sp, #0x90
    // 2c: aa0003f4      mov     x20, x0
    // 30: aa0103f5      mov     x21, x1
    // 34: aa0203f6      mov     x22, x2
    // 38: 9400000c      bl      0x68 <ltmp0+0x68>
    // 3c: a9497bfd      ldp     x29, x30, [sp, #0x90]
    // 40: a9484ff4      ldp     x20, x19, [sp, #0x80]
    // 44: a94757f6      ldp     x22, x21, [sp, #0x70]
    // 48: a9465ff8      ldp     x24, x23, [sp, #0x60]
    // 4c: a94567fa      ldp     x26, x25, [sp, #0x50]
    // 50: a9446ffc      ldp     x28, x27, [sp, #0x40]
    // 54: 6d4323e9      ldp     d9, d8, [sp, #0x30]
    // 58: 6d422beb      ldp     d11, d10, [sp, #0x20]
    // 5c: 6d4133ed      ldp     d13, d12, [sp, #0x10]
    // 60: 6cca3bef      ldp     d15, d14, [sp], #0xa0
    // 64: d65f03c0      ret
    const unsigned char code_body[104] = {
        0xef, 0x3b, 0xb6, 0x6d, 0xed, 0x33, 0x01, 0x6d,
        0xeb, 0x2b, 0x02, 0x6d, 0xe9, 0x23, 0x03, 0x6d,
        0xfc, 0x6f, 0x04, 0xa9, 0xfa, 0x67, 0x05, 0xa9,
        0xf8, 0x5f, 0x06, 0xa9, 0xf6, 0x57, 0x07, 0xa9,
        0xf4, 0x4f, 0x08, 0xa9, 0xfd, 0x7b, 0x09, 0xa9,
        0xfd, 0x43, 0x02, 0x91, 0xf4, 0x03, 0x00, 0xaa,
        0xf5, 0x03, 0x01, 0xaa, 0xf6, 0x03, 0x02, 0xaa,
        0x0c, 0x00, 0x00, 0x94, 0xfd, 0x7b, 0x49, 0xa9,
        0xf4, 0x4f, 0x48, 0xa9, 0xf6, 0x57, 0x47, 0xa9,
        0xf8, 0x5f, 0x46, 0xa9, 0xfa, 0x67, 0x45, 0xa9,
        0xfc, 0x6f, 0x44, 0xa9, 0xe9, 0x23, 0x43, 0x6d,
        0xeb, 0x2b, 0x42, 0x6d, 0xed, 0x33, 0x41, 0x6d,
        0xef, 0x3b, 0xca, 0x6c, 0xc0, 0x03, 0x5f, 0xd6,
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
    // 0: a9bf7bfd      stp     x29, x30, [sp, #-0x10]!
    // 4: 910003fd      mov     x29, sp
    // 8: 90000008      adrp    x8, 0x0 <ltmp0>
    // 0000000000000008:  ARM64_RELOC_GOT_LOAD_PAGE21  _sausage
    // c: f9400108      ldr     x8, [x8]
    // 000000000000000c:  ARM64_RELOC_GOT_LOAD_PAGEOFF12       _sausage
    // 10: 39400108      ldrb    w8, [x8]
    // 14: 36000068      tbz     w8, #0x0, 0x20 <ltmp0+0x20>
    // 18: 94000000      bl      0x18 <ltmp0+0x18>
    // 0000000000000018:  ARM64_RELOC_BRANCH26 _order_eggs_sausage_and_bacon
    // 1c: 14000002      b       0x24 <ltmp0+0x24>
    // 20: 94000000      bl      0x20 <ltmp0+0x20>
    // 0000000000000020:  ARM64_RELOC_BRANCH26 _order_eggs_and_bacon
    // 24: 90000008      adrp    x8, 0x0 <ltmp0>
    // 0000000000000024:  ARM64_RELOC_GOT_LOAD_PAGE21  _spammed
    // 28: f9400108      ldr     x8, [x8]
    // 0000000000000028:  ARM64_RELOC_GOT_LOAD_PAGEOFF12       _spammed
    // 2c: 3900011f      strb    wzr, [x8]
    // 30: a8c17bfd      ldp     x29, x30, [sp], #0x10
    const unsigned char code_body[52] = {
        0xfd, 0x7b, 0xbf, 0xa9, 0xfd, 0x03, 0x00, 0x91,
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x08, 0x01, 0x40, 0x39, 0x68, 0x00, 0x00, 0x36,
        0x00, 0x00, 0x00, 0x94, 0x02, 0x00, 0x00, 0x14,
        0x00, 0x00, 0x00, 0x94, 0x08, 0x00, 0x00, 0x90,
        0x08, 0x01, 0x40, 0xf9, 0x1f, 0x01, 0x00, 0x39,
        0xfd, 0x7b, 0xc1, 0xa8,
    };
    // 0: &spammed+0x0
    // 8: &sausage+0x0
    patch_64(data + 0x0, (uintptr_t)&spammed);
    patch_64(data + 0x8, (uintptr_t)&sausage);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_33rx(code + 0x8, (uintptr_t)data + 0x8);
    patch_aarch64_trampoline(code + 0x18, 0x1, state);
    patch_aarch64_trampoline(code + 0x20, 0x0, state);
    patch_aarch64_33rx(code + 0x24, (uintptr_t)data);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 90000008      adrp    x8, 0x0 <ltmp0>
    // 0000000000000000:  ARM64_RELOC_GOT_LOAD_PAGE21  _spam
    // 4: f9400108      ldr     x8, [x8]
    // 0000000000000004:  ARM64_RELOC_GOT_LOAD_PAGEOFF12       _spam
    // 8: 39400108      ldrb    w8, [x8]
    // c: 7100051f      cmp     w8, #0x1
    // 10: 54000041      b.ne    0x18 <ltmp0+0x18>
    // 14: 14000000      b       0x14 <ltmp0+0x14>
    // 0000000000000014:  ARM64_RELOC_BRANCH26 __JIT_ERROR_TARGET
    const unsigned char code_body[24] = {
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x08, 0x01, 0x40, 0x39, 0x1f, 0x05, 0x00, 0x71,
        0x41, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x14,
    };
    // 0: &spam+0x0
    patch_64(data + 0x0, (uintptr_t)&spam);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_33rx(code + 0x0, (uintptr_t)data);
    patch_aarch64_26r(code + 0x14, state->instruction_starts[instruction->error_target]);
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

static const StencilGroup shim = {emit_shim, 104, 0, {0}};

static const StencilGroup stencil_groups[MAX_UOP_ID + 1] = {
    [0] = {emit_0, 0, 0, {0}},
    [1] = {emit_1, 52, 16, {0x03}},
    [2] = {emit_2, 24, 8, {0}},
};

static const void * const symbols_map[2] = {
    [0] = &order_eggs_and_bacon,
    [1] = &order_eggs_sausage_and_bacon,
};
