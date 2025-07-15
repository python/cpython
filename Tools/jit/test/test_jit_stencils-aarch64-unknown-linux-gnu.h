void
emit_shim(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
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
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000000:  R_AARCH64_ADR_GOT_PAGE       _JIT_OPARG
    // 4: f9400108      ldr     x8, [x8]
    // 0000000000000004:  R_AARCH64_LD64_GOT_LO12_NC   _JIT_OPARG
    // 8: 72003d1f      tst     w8, #0xffff
    // c: 54000040      b.eq    0x14 <_JIT_ENTRY+0x14>
    // 10: 14000000      b       0x10 <_JIT_ENTRY+0x10>
    // 0000000000000010:  R_AARCH64_JUMP26     _JIT_JUMP_TARGET
    const unsigned char code_body[20] = {
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x1f, 0x3d, 0x00, 0x72, 0x40, 0x00, 0x00, 0x54,
        0x00, 0x00, 0x00, 0x14,
    };
    // 0: OPARG
    patch_64(data + 0x0, instruction->oparg);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_33rx(code + 0x0, (uintptr_t)data);
    patch_aarch64_26r(code + 0x10, state->instruction_starts[instruction->jump_target]);
}

void
emit_2(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000000:  R_AARCH64_ADR_GOT_PAGE       _JIT_OPARG
    // 4: f9400108      ldr     x8, [x8]
    // 0000000000000004:  R_AARCH64_LD64_GOT_LO12_NC   _JIT_OPARG
    // 8: 72003d1f      tst     w8, #0xffff
    // c: 54000040      b.eq    0x14 <_JIT_ENTRY+0x14>
    // 10: 14000000      b       0x10 <_JIT_ENTRY+0x10>
    // 0000000000000010:  R_AARCH64_JUMP26     _JIT_ERROR_TARGET
    const unsigned char code_body[20] = {
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x1f, 0x3d, 0x00, 0x72, 0x40, 0x00, 0x00, 0x54,
        0x00, 0x00, 0x00, 0x14,
    };
    // 0: OPARG
    patch_64(data + 0x0, instruction->oparg);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_33rx(code + 0x0, (uintptr_t)data);
    patch_aarch64_26r(code + 0x10, state->instruction_starts[instruction->error_target]);
}

void
emit_3(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000000:  R_AARCH64_ADR_GOT_PAGE       _JIT_TARGET
    // 4: 90000009      adrp    x9, 0x0 <_JIT_ENTRY>
    // 0000000000000004:  R_AARCH64_ADR_GOT_PAGE       _JIT_OPERAND0
    // 8: f9400108      ldr     x8, [x8]
    // 0000000000000008:  R_AARCH64_LD64_GOT_LO12_NC   _JIT_TARGET
    // c: f9400129      ldr     x9, [x9]
    // 000000000000000c:  R_AARCH64_LD64_GOT_LO12_NC   _JIT_OPERAND0
    // 10: f9008adf      str     xzr, [x22, #0x110]
    // 14: f9002295      str     x21, [x20, #0x40]
    // 18: 8b284120      add     x0, x9, w8, uxtw
    // 1c: d65f03c0      ret
    const unsigned char code_body[32] = {
        0x08, 0x00, 0x00, 0x90, 0x09, 0x00, 0x00, 0x90,
        0x08, 0x01, 0x40, 0xf9, 0x29, 0x01, 0x40, 0xf9,
        0xdf, 0x8a, 0x00, 0xf9, 0x95, 0x22, 0x00, 0xf9,
        0x20, 0x41, 0x28, 0x8b, 0xc0, 0x03, 0x5f, 0xd6,
    };
    // 0: TARGET
    // 8: OPERAND0
    patch_64(data + 0x0, instruction->target);
    patch_64(data + 0x8, instruction->operand0);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_21rx(code + 0x0, (uintptr_t)data);
    patch_aarch64_21rx(code + 0x4, (uintptr_t)data + 0x8);
    patch_aarch64_12x(code + 0x8, (uintptr_t)data);
    patch_aarch64_12x(code + 0xc, (uintptr_t)data + 0x8);
}

void
emit_4(
    unsigned char *code, unsigned char *data, _PyExecutorObject *executor,
    const _PyUOpInstruction *instruction, jit_state *state)
{
    // 0000000000000000 <_JIT_ENTRY>:
    // 0: 90000008      adrp    x8, 0x0 <_JIT_ENTRY>
    // 0000000000000000:  R_AARCH64_ADR_GOT_PAGE       _JIT_OPERAND1
    // 4: f9400108      ldr     x8, [x8]
    // 0000000000000004:  R_AARCH64_LD64_GOT_LO12_NC   _JIT_OPERAND1
    // 8: f9403d00      ldr     x0, [x8, #0x78]
    // c: f9008ac8      str     x8, [x22, #0x110]
    // 10: d61f0000      br      x0
    const unsigned char code_body[20] = {
        0x08, 0x00, 0x00, 0x90, 0x08, 0x01, 0x40, 0xf9,
        0x00, 0x3d, 0x40, 0xf9, 0xc8, 0x8a, 0x00, 0xf9,
        0x00, 0x00, 0x1f, 0xd6,
    };
    // 0: OPERAND1
    patch_64(data + 0x0, instruction->operand1);
    memcpy(code, code_body, sizeof(code_body));
    patch_aarch64_33rx(code + 0x0, (uintptr_t)data);
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
    [1] = {emit_1, 20, 8, {0}},
    [2] = {emit_2, 20, 8, {0}},
    [3] = {emit_3, 32, 16, {0}},
    [4] = {emit_4, 20, 8, {0}},
};

static const void * const symbols_map[1] = {
    0
};
