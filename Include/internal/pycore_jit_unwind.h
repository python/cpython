#ifndef Py_INTERNAL_JIT_UNWIND_H
#define Py_INTERNAL_JIT_UNWIND_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>
#include <stdint.h>

/*
 * Compiler-emitted CFI for the shim region (GDB path only).
 *
 * Captured at build time by Tools/jit from the shim's compiled .eh_frame
 * so the runtime CIE/FDE can describe whatever prologue the compiler
 * chose, without hand-rolling DWARF. Executors pass NULL and fall back
 * to the invariant-based steady-state rule that the CIE emits by hand.
 *
 * The struct is defined unconditionally so jit_record_code() in Python/jit.c
 * has a valid pointer type on every platform — callers on non-(Linux+ELF)
 * always pass NULL, matching jit_record_code()'s internal #ifdef.
 */
typedef struct {
    const uint8_t *cie_init_cfi;
    size_t         cie_init_cfi_size;
    const uint8_t *fde_cfi;
    size_t         fde_cfi_size;
    uint32_t       code_align;
    int32_t        data_align;
    uint32_t       ra_column;
} _PyJitUnwind_ShimCfi;

#if defined(PY_HAVE_PERF_TRAMPOLINE) || (defined(__linux__) && defined(__ELF__))

/* DWARF exception-handling pointer encodings shared by JIT unwind users. */
enum {
    DWRF_EH_PE_absptr = 0x00,
    DWRF_EH_PE_omit = 0xff,

    /* Data type encodings */
    DWRF_EH_PE_uleb128 = 0x01,
    DWRF_EH_PE_udata2 = 0x02,
    DWRF_EH_PE_udata4 = 0x03,
    DWRF_EH_PE_udata8 = 0x04,
    DWRF_EH_PE_sleb128 = 0x09,
    DWRF_EH_PE_sdata2 = 0x0a,
    DWRF_EH_PE_sdata4 = 0x0b,
    DWRF_EH_PE_sdata8 = 0x0c,
    DWRF_EH_PE_signed = 0x08,

    /* Reference type encodings */
    DWRF_EH_PE_pcrel = 0x10,
    DWRF_EH_PE_textrel = 0x20,
    DWRF_EH_PE_datarel = 0x30,
    DWRF_EH_PE_funcrel = 0x40,
    DWRF_EH_PE_aligned = 0x50,
    DWRF_EH_PE_indirect = 0x80
};

/* Return the size of the generated .eh_frame data for the given encoding. */
size_t _PyJitUnwind_EhFrameSize(int absolute_addr,
                                const _PyJitUnwind_ShimCfi *shim_cfi);

/*
 * Build DWARF .eh_frame data for JIT code; returns size written or 0 on error.
 * absolute_addr selects the FDE address encoding:
 * - 0: PC-relative offsets (perf jitdump synthesized DSO).
 * - nonzero: absolute addresses (GDB JIT in-memory ELF).
 *
 * shim_cfi selects which JIT region the CFI describes (GDB path only):
 * - NULL: executor trace; steady-state rule in the CIE applies at every PC.
 * - non-NULL: compile_shim() output; the captured CIE/FDE CFI bytes are
 *            spliced in so unwinding is valid at every PC in the shim.
 */
size_t _PyJitUnwind_BuildEhFrame(uint8_t *buffer, size_t buffer_size,
                                 const void *code_addr, size_t code_size,
                                 int absolute_addr,
                                 const _PyJitUnwind_ShimCfi *shim_cfi);

void *_PyJitUnwind_GdbRegisterCode(const void *code_addr,
                                  size_t code_size,
                                  const char *entry,
                                  const char *filename,
                                  const _PyJitUnwind_ShimCfi *shim_cfi);

void _PyJitUnwind_GdbUnregisterCode(void *handle);

#endif  // defined(PY_HAVE_PERF_TRAMPOLINE) || (defined(__linux__) && defined(__ELF__))

#endif  // Py_INTERNAL_JIT_UNWIND_H
