#ifndef Py_INTERNAL_JIT_UNWIND_H
#define Py_INTERNAL_JIT_UNWIND_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>
#include <stdint.h>

#if defined(_Py_JIT) && defined(__linux__) && defined(__ELF__)
#  define PY_HAVE_JIT_GDB_UNWIND
#  if defined(HAVE_EXECINFO_H) && defined(HAVE_BACKTRACE) && \
          defined(HAVE_LIBGCC_EH_FRAME_REGISTRATION)
#    define PY_HAVE_JIT_GNU_BACKTRACE_UNWIND
#  endif
#endif

#if defined(PY_HAVE_PERF_TRAMPOLINE) \
    || defined(PY_HAVE_JIT_GDB_UNWIND) \
    || defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)

#if defined(PY_HAVE_JIT_GDB_UNWIND)
extern PyMutex _Py_jit_debug_mutex;
#endif

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
size_t _PyJitUnwind_EhFrameSize(int absolute_addr);

/*
 * Build DWARF .eh_frame data for JIT code; returns size written or 0 on error.
 * absolute_addr selects the FDE address encoding:
 * - 0: PC-relative offsets (perf jitdump synthesized DSO).
 * - nonzero: absolute addresses (GDB JIT in-memory ELF).
 */
size_t _PyJitUnwind_BuildEhFrame(uint8_t *buffer, size_t buffer_size,
                                 const void *code_addr, size_t code_size,
                                 int absolute_addr);

void *_PyJitUnwind_GdbRegisterCode(const void *code_addr,
                                  size_t code_size,
                                  const char *entry,
                                  const char *filename);

void _PyJitUnwind_GdbUnregisterCode(void *handle);

#if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
void *_PyJitUnwind_GnuBacktraceRegisterCode(const void *code_addr,
                                            size_t code_size);

void _PyJitUnwind_GnuBacktraceUnregisterCode(void *handle);
#endif

#endif  // JIT unwind support

#endif  // Py_INTERNAL_JIT_UNWIND_H
