void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 6db63bef      stp     d15, d14, [sp, #-0xa0]!
    // 4: a90857f6      stp     x22, x21, [sp, #0x80]
    // 8: aa0103f5      mov     x21, x1
    // c: aa0203f6      mov     x22, x2
    // 10: a9094ff4      stp     x20, x19, [sp, #0x90]
    // 14: aa0003f4      mov     x20, x0
    // 18: 6d0133ed      stp     d13, d12, [sp, #0x10]
    // 1c: 6d022beb      stp     d11, d10, [sp, #0x20]
    // 20: 6d0323e9      stp     d9, d8, [sp, #0x30]
    // 24: a9047bfd      stp     x29, x30, [sp, #0x40]
    // 28: 910103fd      add     x29, sp, #0x40
    // 2c: a9056ffc      stp     x28, x27, [sp, #0x50]
    // 30: a90667fa      stp     x26, x25, [sp, #0x60]
    // 34: a9075ff8      stp     x24, x23, [sp, #0x70]
    // 38: 9400000c      bl      0x68 <_JIT_ENTRY+0x68>
    // 3c: a9494ff4      ldp     x20, x19, [sp, #0x90]
    // 40: a94857f6      ldp     x22, x21, [sp, #0x80]
    // 44: a9475ff8      ldp     x24, x23, [sp, #0x70]
    // 48: a94667fa      ldp     x26, x25, [sp, #0x60]
    // 4c: a9456ffc      ldp     x28, x27, [sp, #0x50]
    // 50: a9447bfd      ldp     x29, x30, [sp, #0x40]
    // 54: 6d4323e9      ldp     d9, d8, [sp, #0x30]
    // 58: 6d422beb      ldp     d11, d10, [sp, #0x20]
    // 5c: 6d4133ed      ldp     d13, d12, [sp, #0x10]
    // 60: 6cca3bef      ldp     d15, d14, [sp], #0xa0
    // 64: d65f03c0      ret
    const unsigned char code_body[104] = {
        0xef, 0x3b, 0xb6, 0x6d, 0xf6, 0x57, 0x08, 0xa9,
        0xf5, 0x03, 0x01, 0xaa, 0xf6, 0x03, 0x02, 0xaa,
        0xf4, 0x4f, 0x09, 0xa9, 0xf4, 0x03, 0x00, 0xaa,
        0xed, 0x33, 0x01, 0x6d, 0xeb, 0x2b, 0x02, 0x6d,
        0xe9, 0x23, 0x03, 0x6d, 0xfd, 0x7b, 0x04, 0xa9,
        0xfd, 0x03, 0x01, 0x91, 0xfc, 0x6f, 0x05, 0xa9,
        0xfa, 0x67, 0x06, 0xa9, 0xf8, 0x5f, 0x07, 0xa9,
        0x0c, 0x00, 0x00, 0x94, 0xf4, 0x4f, 0x49, 0xa9,
        0xf6, 0x57, 0x48, 0xa9, 0xf8, 0x5f, 0x47, 0xa9,
        0xfa, 0x67, 0x46, 0xa9, 0xfc, 0x6f, 0x45, 0xa9,
        0xfd, 0x7b, 0x44, 0xa9, 0xe9, 0x23, 0x43, 0x6d,
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
    // 4: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000004:  R_AARCH64_ADR_GOT_PAGE       sausage
    // 8: 910003fd      mov     x29, sp
    // c: f9400108      ldr     x8, [x8]
    // 000000000000000c:  R_AARCH64_LD64_GOT_LO12_NC   sausage
    // 10: 39400108      ldrb    w8, [x8]
    // 14: 36000088      tbz     w8, #0x0, 0x24 <_JIT_ENTRY+0x24>
    // 18: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000018:  R_AARCH64_ADR_GOT_PAGE       order_eggs_sausage_and_bacon
    // 1c: f9400108      ldr     x8, [x8]
    // 000000000000001c:  R_AARCH64_LD64_GOT_LO12_NC   order_eggs_sausage_and_bacon
    // 20: 14000003      b       0x2c <_JIT_ENTRY+0x2c>
    // 24: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000024:  R_AARCH64_ADR_GOT_PAGE       order_eggs_and_bacon
    // 28: f9400108      ldr     x8, [x8]
    // 0000000000000028:  R_AARCH64_LD64_GOT_LO12_NC   order_eggs_and_bacon
    // 2c: d63f0100      blr     x8
    // 30: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000030:  R_AARCH64_ADR_GOT_PAGE       spammed
    // 34: f9400108      ldr     x8, [x8]
    // 0000000000000034:  R_AARCH64_LD64_GOT_LO12_NC   spammed
    // 38: 3900011f      strb    wzr, [x8]
    // 3c: a8c17bfd      ldp     x29, x30, [sp], #0x10
    const unsigned char code_body[64] = {
        0xfd, 0x7b, 0xbf, 0xa9, 0x08, 0x00, 0x00, 0x90,
        0xfd, 0x03, 0x00, 0x91, 0x08, 0x01, 0x40, 0xf9,
        0x08, 0x01, 0x40, 0x39, 0x88, 0x00, 0x00, 0x36,
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x03, 0x00, 0x00, 0x14, 0x08, 0x00, 0x00, 0x90,
        0x08, 0x01, 0x40, 0xf9, 0x00, 0x01, 0x3f, 0xd6,
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x1f, 0x01, 0x00, 0x39, 0xfd, 0x7b, 0xc1, 0xa8,
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
    patch_aarch64_21rx(code + 0x4, (uintptr_t)data);
    patch_aarch64_12x(code + 0xc, (uintptr_t)data);
    patch_aarch64_33rx(code + 0x18, (uintptr_t)data + 0x8);
    patch_aarch64_33rx(code + 0x24, (uintptr_t)data + 0x10);
    patch_aarch64_33rx(code + 0x30, (uintptr_t)data + 0x18);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000000:  R_AARCH64_ADR_GOT_PAGE       spam
    // 4: f9400108      ldr     x8, [x8]
    // 0000000000000004:  R_AARCH64_LD64_GOT_LO12_NC   spam
    // 8: 39400108      ldrb    w8, [x8]
    // c: 7100051f      cmp     w8, #0x1
    // 10: 54000041      b.ne    0x18 <_JIT_ENTRY+0x18>
    // 14: 14000000      b       0x14 <_JIT_ENTRY+0x14>
    // 0000000000000014:  R_AARCH64_JUMP26     _JIT_ERROR_TARGET
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
    [1] = {emit_1, 64, 32, {0}},
    [2] = {emit_2, 24, 8, {0}},
};

static const void * const symbols_map[1] = {
    0
};
