#ifndef Py_CORE_JIT_UNWIND_H
#define Py_CORE_JIT_UNWIND_H

#ifdef PY_HAVE_PERF_TRAMPOLINE

#include <stddef.h>

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

void _PyJitUnwind_GdbRegisterCode(const void *code_addr,
                                  unsigned int code_size,
                                  const char *entry,
                                  const char *filename);

#endif  // PY_HAVE_PERF_TRAMPOLINE

#endif  // Py_CORE_JIT_UNWIND_H
