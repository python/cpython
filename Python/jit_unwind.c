/*
 * Python JIT - DWARF .eh_frame builder
 *
 * This file contains the DWARF CFI generator used to build .eh_frame
 * data for JIT code (perf jitdump and other unwinders).
 */

#include "Python.h"
#include "pycore_jit_unwind.h"

#ifdef PY_HAVE_PERF_TRAMPOLINE

#if defined(__linux__)
#  include <elf.h>
#endif
#include <stdio.h>
#include <string.h>

// =============================================================================
//                              DWARF CONSTANTS
// =============================================================================

/*
 * DWARF (Debug With Arbitrary Record Formats) constants
 *
 * DWARF is a debugging data format used to provide stack unwinding information.
 * These constants define the various encoding types and opcodes used in
 * DWARF Call Frame Information (CFI) records.
 */

/* DWARF Call Frame Information version */
#define DWRF_CIE_VERSION 1

/* DWARF CFA (Call Frame Address) opcodes */
enum {
    DWRF_CFA_nop = 0x0,                    // No operation
    DWRF_CFA_offset_extended = 0x5,        // Extended offset instruction
    DWRF_CFA_def_cfa = 0xc,               // Define CFA rule
    DWRF_CFA_def_cfa_register = 0xd,      // Define CFA register
    DWRF_CFA_def_cfa_offset = 0xe,        // Define CFA offset
    DWRF_CFA_offset_extended_sf = 0x11,   // Extended signed offset
    DWRF_CFA_advance_loc = 0x40,          // Advance location counter
    DWRF_CFA_offset = 0x80,               // Simple offset instruction
    DWRF_CFA_restore = 0xc0               // Restore register
};

/* DWARF Exception Handling pointer encodings */
enum {
    DWRF_EH_PE_absptr = 0x00,             // Absolute pointer
    DWRF_EH_PE_omit = 0xff,               // Omitted value

    /* Data type encodings */
    DWRF_EH_PE_uleb128 = 0x01,            // Unsigned LEB128
    DWRF_EH_PE_udata2 = 0x02,             // Unsigned 2-byte
    DWRF_EH_PE_udata4 = 0x03,             // Unsigned 4-byte
    DWRF_EH_PE_udata8 = 0x04,             // Unsigned 8-byte
    DWRF_EH_PE_sleb128 = 0x09,            // Signed LEB128
    DWRF_EH_PE_sdata2 = 0x0a,             // Signed 2-byte
    DWRF_EH_PE_sdata4 = 0x0b,             // Signed 4-byte
    DWRF_EH_PE_sdata8 = 0x0c,             // Signed 8-byte
    DWRF_EH_PE_signed = 0x08,             // Signed flag

    /* Reference type encodings */
    DWRF_EH_PE_pcrel = 0x10,              // PC-relative
    DWRF_EH_PE_textrel = 0x20,            // Text-relative
    DWRF_EH_PE_datarel = 0x30,            // Data-relative
    DWRF_EH_PE_funcrel = 0x40,            // Function-relative
    DWRF_EH_PE_aligned = 0x50,            // Aligned
    DWRF_EH_PE_indirect = 0x80            // Indirect
};

/*
 * Architecture-specific DWARF register numbers
 *
 * These constants define the register numbering scheme used by DWARF
 * for each supported architecture. The numbers must match the ABI
 * specification for proper stack unwinding.
 */
enum {
#ifdef __x86_64__
    /* x86_64 register numbering (note: order is defined by x86_64 ABI) */
    DWRF_REG_AX,    // RAX
    DWRF_REG_DX,    // RDX
    DWRF_REG_CX,    // RCX
    DWRF_REG_BX,    // RBX
    DWRF_REG_SI,    // RSI
    DWRF_REG_DI,    // RDI
    DWRF_REG_BP,    // RBP
    DWRF_REG_SP,    // RSP
    DWRF_REG_8,     // R8
    DWRF_REG_9,     // R9
    DWRF_REG_10,    // R10
    DWRF_REG_11,    // R11
    DWRF_REG_12,    // R12
    DWRF_REG_13,    // R13
    DWRF_REG_14,    // R14
    DWRF_REG_15,    // R15
    DWRF_REG_RA,    // Return address (RIP)
#elif defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
    /* AArch64 register numbering */
    DWRF_REG_FP = 29,  // Frame Pointer
    DWRF_REG_RA = 30,  // Link register (return address)
    DWRF_REG_SP = 31,  // Stack pointer
#else
#    error "Unsupported target architecture"
#endif
};

// =============================================================================
//                              ELF OBJECT CONTEXT
// =============================================================================

/*
 * Context for building ELF/DWARF structures
 *
 * This structure maintains state while constructing DWARF unwind information.
 * It acts as a simple buffer manager with pointers to track current position
 * and important landmarks within the buffer.
 */
typedef struct ELFObjectContext {
    uint8_t* p;            // Current write position in buffer
    uint8_t* startp;       // Start of buffer (for offset calculations)
    uint8_t* eh_frame_p;   // Start of EH frame data (for relative offsets)
    uint8_t* fde_p;        // Start of FDE data (for PC-relative calculations)
    uintptr_t code_addr;   // Address of the code section
    size_t code_size;      // Size of the code section
} ELFObjectContext;

// =============================================================================
//                              DWARF GENERATION UTILITIES
// =============================================================================

/*
 * Append a null-terminated string to the ELF context buffer.
 *
 * Args:
 *   ctx: ELF object context
 *   str: String to append (must be null-terminated)
 *
 * Returns: Offset from start of buffer where string was written
 */
static uint32_t elfctx_append_string(ELFObjectContext* ctx, const char* str) {
    uint8_t* p = ctx->p;
    uint32_t ofs = (uint32_t)(p - ctx->startp);

    /* Copy string including null terminator */
    do {
        *p++ = (uint8_t)*str;
    } while (*str++);

    ctx->p = p;
    return ofs;
}

/*
 * Append a SLEB128 (Signed Little Endian Base 128) value
 *
 * SLEB128 is a variable-length encoding used extensively in DWARF.
 * It efficiently encodes small numbers in fewer bytes.
 *
 * Args:
 *   ctx: ELF object context
 *   v: Signed value to encode
 */
static void elfctx_append_sleb128(ELFObjectContext* ctx, int32_t v) {
    uint8_t* p = ctx->p;

    /* Encode 7 bits at a time, with continuation bit in MSB */
    for (; (uint32_t)(v + 0x40) >= 0x80; v >>= 7) {
        *p++ = (uint8_t)((v & 0x7f) | 0x80);  // Set continuation bit
    }
    *p++ = (uint8_t)(v & 0x7f);  // Final byte without continuation bit

    ctx->p = p;
}

/*
 * Append a ULEB128 (Unsigned Little Endian Base 128) value
 *
 * Similar to SLEB128 but for unsigned values.
 *
 * Args:
 *   ctx: ELF object context
 *   v: Unsigned value to encode
 */
static void elfctx_append_uleb128(ELFObjectContext* ctx, uint32_t v) {
    uint8_t* p = ctx->p;

    /* Encode 7 bits at a time, with continuation bit in MSB */
    for (; v >= 0x80; v >>= 7) {
        *p++ = (char)((v & 0x7f) | 0x80);  // Set continuation bit
    }
    *p++ = (char)v;  // Final byte without continuation bit

    ctx->p = p;
}

/*
 * Macros for generating DWARF structures
 *
 * These macros provide a convenient way to write various data types
 * to the DWARF buffer while automatically advancing the pointer.
 */
#define DWRF_U8(x) (*p++ = (x))                                    // Write unsigned 8-bit
#define DWRF_I8(x) (*(int8_t*)p = (x), p++)                       // Write signed 8-bit
#define DWRF_U16(x) (*(uint16_t*)p = (x), p += 2)                 // Write unsigned 16-bit
#define DWRF_U32(x) (*(uint32_t*)p = (x), p += 4)                 // Write unsigned 32-bit
#define DWRF_ADDR(x) (*(uintptr_t*)p = (x), p += sizeof(uintptr_t)) // Write address
#define DWRF_UV(x) (ctx->p = p, elfctx_append_uleb128(ctx, (x)), p = ctx->p) // Write ULEB128
#define DWRF_SV(x) (ctx->p = p, elfctx_append_sleb128(ctx, (x)), p = ctx->p) // Write SLEB128
#define DWRF_STR(str) (ctx->p = p, elfctx_append_string(ctx, (str)), p = ctx->p) // Write string

/* Align to specified boundary with NOP instructions */
#define DWRF_ALIGNNOP(s)                                          \
    while ((uintptr_t)p & ((s)-1)) {                              \
        *p++ = DWRF_CFA_nop;                                       \
    }

/* Write a DWARF section with automatic size calculation */
#define DWRF_SECTION(name, stmt)                                  \
    {                                                             \
        uint32_t* szp_##name = (uint32_t*)p;                      \
        p += 4;                                                   \
        stmt;                                                     \
        *szp_##name = (uint32_t)((p - (uint8_t*)szp_##name) - 4); \
    }

// =============================================================================
//                              DWARF EH FRAME GENERATION
// =============================================================================

static void elf_init_ehframe(ELFObjectContext* ctx, int absolute_addr);

size_t
_PyJitUnwind_EhFrameSize(int absolute_addr)
{
    /* The .eh_frame we emit is small and bounded; keep a generous buffer. */
    uint8_t scratch[512];
    _Static_assert(sizeof(scratch) >= 256,
                   "scratch buffer may be too small for elf_init_ehframe");
    ELFObjectContext ctx;
    ctx.code_size = 1;
    ctx.code_addr = 0;
    ctx.startp = ctx.p = scratch;
    ctx.eh_frame_p = NULL;
    ctx.fde_p = NULL;
    /* Generate once into scratch to learn the required size. */
    elf_init_ehframe(&ctx, absolute_addr);
    ptrdiff_t size = ctx.p - ctx.startp;
    assert(size <= (ptrdiff_t)sizeof(scratch));
    return (size_t)size;
}

size_t
_PyJitUnwind_BuildEhFrame(uint8_t *buffer, size_t buffer_size,
                        const void *code_addr, size_t code_size,
                        int absolute_addr)
{
    if (buffer == NULL || code_addr == NULL || code_size == 0) {
        return 0;
    }
    /* Generate the frame twice: once to size-check, once to write. */
    size_t required = _PyJitUnwind_EhFrameSize(absolute_addr);
    if (required == 0 || required > buffer_size) {
        return 0;
    }
    ELFObjectContext ctx;
    ctx.code_size = code_size;
    ctx.code_addr = (uintptr_t)code_addr;
    ctx.startp = ctx.p = buffer;
    ctx.eh_frame_p = NULL;
    ctx.fde_p = NULL;
    elf_init_ehframe(&ctx, absolute_addr);
    size_t written = (size_t)(ctx.p - ctx.startp);
    /* The frame size is independent of code_addr/code_size (fixed-width fields). */
    assert(written == required);
    return written;
}

/*
 * Generate a minimal .eh_frame for a single JIT code region.
 *
 * The .eh_frame section contains Call Frame Information (CFI) that describes
 * how to unwind the stack at any point in the code. This is essential for
 * unwinding through JIT-generated code.
 *
 * The generated data contains:
 * 1. A CIE (Common Information Entry) describing the calling convention.
 * 2. An FDE (Frame Description Entry) describing how to unwind the JIT frame.
 *
 * The caller selects the FDE address encoding through absolute_addr:
 * - 0: emit PC-relative addresses for perf's synthesized DSO layout.
 * - 1: emit absolute addresses for the GDB JIT in-memory ELF.
 */
static void elf_init_ehframe(ELFObjectContext* ctx, int absolute_addr) {
    int fde_ptr_enc = absolute_addr
        ? DWRF_EH_PE_absptr
        : (DWRF_EH_PE_pcrel | DWRF_EH_PE_sdata4);
    uint8_t* p = ctx->p;
    uint8_t* framep = p;  // Remember start of frame data

    /*
    * DWARF Unwind Table for Trampoline Function
    *
    * This section defines DWARF Call Frame Information (CFI) using encoded macros
    * like `DWRF_U8`, `DWRF_UV`, and `DWRF_SECTION` to describe how the trampoline function
    * preserves and restores registers. This is used by profiling tools (e.g., `perf`)
    * and debuggers for stack unwinding in JIT-compiled code.
    *
    * -------------------------------------------------
    * TO REGENERATE THIS TABLE FROM GCC OBJECTS:
    * -------------------------------------------------
    *
    * 1. Create a trampoline source file (e.g., `trampoline.c`):
    *
    *      #include <Python.h>
    *      typedef PyObject* (*py_evaluator)(void*, void*, int);
    *      PyObject* trampoline(void *ts, void *f, int throwflag, py_evaluator evaluator) {
    *          return evaluator(ts, f, throwflag);
    *      }
    *
    * 2. Compile to an object file with frame pointer preservation:
    *
    *      gcc trampoline.c -I. -I./Include -O2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -c
    *
    * 3. Extract DWARF unwind info from the object file:
    *
    *      readelf -w trampoline.o
    *
    *    Example output from `.eh_frame`:
    *
    *      00000000 CIE
    *        Version:               1
    *        Augmentation:          "zR"
    *        Code alignment factor: 4
    *        Data alignment factor: -8
    *        Return address column: 30
    *        DW_CFA_def_cfa: r31 (sp) ofs 0
    *
    *      00000014 FDE cie=00000000 pc=0..14
    *        DW_CFA_advance_loc: 4
    *        DW_CFA_def_cfa_offset: 16
    *        DW_CFA_offset: r29 at cfa-16
    *        DW_CFA_offset: r30 at cfa-8
    *        DW_CFA_advance_loc: 12
    *        DW_CFA_restore: r30
    *        DW_CFA_restore: r29
    *        DW_CFA_def_cfa_offset: 0
    *
    * -- These values can be verified by comparing with `readelf -w` or `llvm-dwarfdump --eh-frame`.
    *
    * ----------------------------------
    * HOW TO TRANSLATE TO DWRF_* MACROS:
    * ----------------------------------
    *
    * After compiling your trampoline with:
    *
    *     gcc trampoline.c -I. -I./Include -O2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -c
    *
    * run:
    *
    *     readelf -w trampoline.o
    *
    * to inspect the generated `.eh_frame` data. You will see two main components:
    *
    *     1. A CIE (Common Information Entry): shared configuration used by all FDEs.
    *     2. An FDE (Frame Description Entry): function-specific unwind instructions.
    *
    * ---------------------
    * Translating the CIE:
    * ---------------------
    * From `readelf -w`, you might see:
    *
    *   00000000 0000000000000010 00000000 CIE
    *     Version:               1
    *     Augmentation:          "zR"
    *     Code alignment factor: 4
    *     Data alignment factor: -8
    *     Return address column: 30
    *     Augmentation data:     1b
    *     DW_CFA_def_cfa: r31 (sp) ofs 0
    *
    * Map this to:
    *
    *     DWRF_SECTION(CIE,
    *         DWRF_U32(0);                             // CIE ID (always 0 for CIEs)
    *         DWRF_U8(DWRF_CIE_VERSION);              // Version: 1
    *         DWRF_STR("zR");                         // Augmentation string "zR"
    *         DWRF_UV(4);                             // Code alignment factor = 4
    *         DWRF_SV(-8);                            // Data alignment factor = -8
    *         DWRF_U8(DWRF_REG_RA);                   // Return address register (e.g., x30 = 30)
    *         DWRF_UV(1);                             // Augmentation data length = 1
    *         DWRF_U8(DWRF_EH_PE_pcrel | DWRF_EH_PE_sdata4); // Encoding for FDE pointers
    *
    *         DWRF_U8(DWRF_CFA_def_cfa);              // DW_CFA_def_cfa
    *         DWRF_UV(DWRF_REG_SP);                   // Register: SP (r31)
    *         DWRF_UV(0);                             // Offset = 0
    *
    *         DWRF_ALIGNNOP(sizeof(uintptr_t));       // Align to pointer size boundary
    *     )
    *
    * Notes:
    *   - Use `DWRF_UV` for unsigned LEB128, `DWRF_SV` for signed LEB128.
    *   - `DWRF_REG_RA` and `DWRF_REG_SP` are architecture-defined constants.
    *
    * ---------------------
    * Translating the FDE:
    * ---------------------
    * From `readelf -w`:
    *
    *   00000014 0000000000000020 00000018 FDE cie=00000000 pc=0000000000000000..0000000000000014
    *     DW_CFA_advance_loc: 4
    *     DW_CFA_def_cfa_offset: 16
    *     DW_CFA_offset: r29 at cfa-16
    *     DW_CFA_offset: r30 at cfa-8
    *     DW_CFA_advance_loc: 12
    *     DW_CFA_restore: r30
    *     DW_CFA_restore: r29
    *     DW_CFA_def_cfa_offset: 0
    *
    * Map the FDE header and instructions to:
    *
    *     DWRF_SECTION(FDE,
    *         DWRF_U32((uint32_t)(p - framep));       // Offset to CIE (relative from here)
    *         DWRF_U32(pc_relative_offset);           // PC-relative location of the code (calculated dynamically)
    *         DWRF_U32(ctx->code_size);               // Code range covered by this FDE
    *         DWRF_U8(0);                             // Augmentation data length (none)
    *
    *         DWRF_U8(DWRF_CFA_advance_loc | 1);      // Advance location by 1 unit (1 * 4 = 4 bytes)
    *         DWRF_U8(DWRF_CFA_def_cfa_offset);       // CFA = SP + 16
    *         DWRF_UV(16);
    *
    *         DWRF_U8(DWRF_CFA_offset | DWRF_REG_FP); // Save x29 (frame pointer)
    *         DWRF_UV(2);                             // At offset 2 * 8 = 16 bytes
    *
    *         DWRF_U8(DWRF_CFA_offset | DWRF_REG_RA); // Save x30 (return address)
    *         DWRF_UV(1);                             // At offset 1 * 8 = 8 bytes
    *
    *         DWRF_U8(DWRF_CFA_advance_loc | 3);      // Advance location by 3 units (3 * 4 = 12 bytes)
    *
    *         DWRF_U8(DWRF_CFA_offset | DWRF_REG_RA); // Restore x30
    *         DWRF_U8(DWRF_CFA_offset | DWRF_REG_FP); // Restore x29
    *
    *         DWRF_U8(DWRF_CFA_def_cfa_offset);       // CFA = SP
    *         DWRF_UV(0);
    *     )
    *
    * To regenerate:
    *   1. Get the `code alignment factor`, `data alignment factor`, and `RA column` from the CIE.
    *   2. Note the range of the function from the FDE's `pc=...` line and map it to the JIT code as
    *      the code is in a different address space every time.
    *   3. For each `DW_CFA_*` entry, use the corresponding `DWRF_*` macro:
    *        - `DW_CFA_def_cfa_offset`     → DWRF_U8(DWRF_CFA_def_cfa_offset), DWRF_UV(value)
    *        - `DW_CFA_offset: rX`         → DWRF_U8(DWRF_CFA_offset | reg), DWRF_UV(offset)
    *        - `DW_CFA_restore: rX`        → DWRF_U8(DWRF_CFA_offset | reg) // restore is same as reusing offset
    *        - `DW_CFA_advance_loc: N`     → DWRF_U8(DWRF_CFA_advance_loc | (N / code_alignment_factor))
    *   4. Use `DWRF_REG_FP`, `DWRF_REG_RA`, etc., for register numbers.
    *   5. Use `sizeof(uintptr_t)` (typically 8) for pointer size calculations and alignment.
    */

    /*
     * Emit DWARF EH CIE (Common Information Entry)
     *
     * The CIE describes the calling conventions and basic unwinding rules
     * that apply to all functions in this compilation unit.
     */
    DWRF_SECTION(CIE,
        DWRF_U32(0);                           // CIE ID (0 indicates this is a CIE)
        DWRF_U8(DWRF_CIE_VERSION);            // CIE version (1)
        DWRF_STR("zR");                       // Augmentation string ("zR" = has LSDA)
#ifdef __x86_64__
        DWRF_UV(1);                           // Code alignment factor (x86_64: 1 byte)
#elif defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
        DWRF_UV(4);                           // Code alignment factor (AArch64: 4 bytes per instruction)
#endif
        DWRF_SV(-(int64_t)sizeof(uintptr_t)); // Data alignment factor (negative)
        DWRF_U8(DWRF_REG_RA);                 // Return address register number
        DWRF_UV(1);                           // Augmentation data length
        DWRF_U8(fde_ptr_enc);                 // FDE pointer encoding

        /* Initial CFI instructions - describe default calling convention */
#ifdef __x86_64__
        /* x86_64 initial CFI state */
        DWRF_U8(DWRF_CFA_def_cfa);            // Define CFA (Call Frame Address)
        DWRF_UV(DWRF_REG_SP);                 // CFA = SP register
        DWRF_UV(sizeof(uintptr_t));           // CFA = SP + pointer_size
        DWRF_U8(DWRF_CFA_offset|DWRF_REG_RA); // Return address is saved
        DWRF_UV(1);                           // At offset 1 from CFA
#elif defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
        /* AArch64 initial CFI state */
        DWRF_U8(DWRF_CFA_def_cfa);            // Define CFA (Call Frame Address)
        DWRF_UV(DWRF_REG_SP);                 // CFA = SP register
        DWRF_UV(0);                           // CFA = SP + 0 (AArch64 starts with offset 0)
        // No initial register saves in AArch64 CIE
#endif
        DWRF_ALIGNNOP(sizeof(uintptr_t));     // Align to pointer boundary
    )

    ctx->eh_frame_p = p;  // Remember start of FDE data

    /*
     * Emit DWARF EH FDE (Frame Description Entry)
     *
     * The FDE describes unwinding information specific to this function.
     * It references the CIE and provides function-specific CFI instructions.
     *
     * The PC-relative offset is calculated after the entire EH frame is built
     * to ensure accurate positioning relative to the synthesized DSO layout.
     */
    DWRF_SECTION(FDE,
        DWRF_U32((uint32_t)(p - framep));     // Offset to CIE (backwards reference)
        if (absolute_addr) {
            DWRF_ADDR(ctx->code_addr);        // Absolute code start
            DWRF_ADDR((uintptr_t)ctx->code_size); // Code range covered
        }
        else {
            /*
             * In perf jitdump mode (absolute_addr == 0), the FDE PC field is
             * encoded PC-relative and points back to code_start. For the GDB
             * JIT interface we reuse the same generator with absolute_addr == 1;
             * the EH frame is then carried in a .eh_frame section of an
             * in-memory ELF (no EhFrameHeader).
             */
            ctx->fde_p = p;                   // Remember where PC offset field is located for later calculation
            DWRF_U32(0);                      // Placeholder for PC-relative offset (calculated at end of elf_init_ehframe)
            DWRF_U32(ctx->code_size);         // Address range covered by this FDE (code length)
        }
        DWRF_U8(0);                           // Augmentation data length (none)

        /*
         * Architecture-specific CFI instructions
         *
         * These instructions describe how registers are saved and restored
         * during function calls. Each architecture has different calling
         * conventions and register usage patterns.
         */
#ifdef __x86_64__
        /* x86_64 calling convention unwinding rules with frame pointer */
#  if defined(__CET__) && (__CET__ & 1)
        DWRF_U8(DWRF_CFA_advance_loc | 4);    // Advance past endbr64 (4 bytes)
#  endif
        DWRF_U8(DWRF_CFA_advance_loc | 1);    // Advance past push %rbp (1 byte)
        DWRF_U8(DWRF_CFA_def_cfa_offset);     // def_cfa_offset 16
        DWRF_UV(16);                          // New offset: SP + 16
        DWRF_U8(DWRF_CFA_offset | DWRF_REG_BP); // offset r6 at cfa-16
        DWRF_UV(2);                           // Offset factor: 2 * 8 = 16 bytes
        DWRF_U8(DWRF_CFA_advance_loc | 3);    // Advance past mov %rsp,%rbp (3 bytes)
        DWRF_U8(DWRF_CFA_def_cfa_register);   // def_cfa_register r6
        DWRF_UV(DWRF_REG_BP);                 // Use base pointer register
        DWRF_U8(DWRF_CFA_advance_loc | 3);    // Advance past call *%rcx (2 bytes) + pop %rbp (1 byte) = 3
        DWRF_U8(DWRF_CFA_def_cfa);            // def_cfa r7 ofs 8
        DWRF_UV(DWRF_REG_SP);                 // Use stack pointer register
        DWRF_UV(8);                           // New offset: SP + 8
#elif defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
        /* AArch64 calling convention unwinding rules */
        DWRF_U8(DWRF_CFA_advance_loc | 1);        // Advance by 1 instruction (4 bytes)
        DWRF_U8(DWRF_CFA_def_cfa_offset);         // CFA = SP + 16
        DWRF_UV(16);                              // Stack pointer moved by 16 bytes
        DWRF_U8(DWRF_CFA_offset | DWRF_REG_FP);   // x29 (frame pointer) saved
        DWRF_UV(2);                               // At CFA-16 (2 * 8 = 16 bytes from CFA)
        DWRF_U8(DWRF_CFA_offset | DWRF_REG_RA);   // x30 (link register) saved
        DWRF_UV(1);                               // At CFA-8 (1 * 8 = 8 bytes from CFA)
        DWRF_U8(DWRF_CFA_advance_loc | 3);        // Advance by 3 instructions (12 bytes)
        DWRF_U8(DWRF_CFA_restore | DWRF_REG_RA);  // Restore x30 - NO DWRF_UV() after this!
        DWRF_U8(DWRF_CFA_restore | DWRF_REG_FP);  // Restore x29 - NO DWRF_UV() after this!
        DWRF_U8(DWRF_CFA_def_cfa_offset);         // CFA = SP + 0 (stack restored)
        DWRF_UV(0);                               // Back to original stack position

        DWRF_U8(DWRF_CFA_def_cfa_register);       // CFA = FP (x29)
        DWRF_UV(DWRF_REG_FP);
        DWRF_U8(DWRF_CFA_def_cfa_offset);         // CFA = FP + 16
        DWRF_UV(16);
        DWRF_U8(DWRF_CFA_offset | DWRF_REG_FP);   // x29 saved
        DWRF_UV(2);
        DWRF_U8(DWRF_CFA_offset | DWRF_REG_RA);   // x30 saved
        DWRF_UV(1);

#else
#    error "Unsupported target architecture"
#endif

        DWRF_ALIGNNOP(sizeof(uintptr_t));     // Align to pointer boundary
    )

    ctx->p = p;  // Update context pointer to end of generated data

    /* Calculate and update the PC-relative offset in the FDE
     *
     * When perf processes the jitdump, it creates a synthesized DSO with this layout:
     *
     *     Synthesized DSO Memory Layout:
     *     ┌─────────────────────────────────────────────────────────────┐ < code_start
     *     │                        Code Section                         │
     *     │                    (round_up(code_size, 8) bytes)           │
     *     ├─────────────────────────────────────────────────────────────┤ < start of EH frame data
     *     │                      EH Frame Data                          │
     *     │  ┌─────────────────────────────────────────────────────┐    │
     *     │  │                 CIE data                            │    │
     *     │  └─────────────────────────────────────────────────────┘    │
     *     │  ┌─────────────────────────────────────────────────────┐    │
     *     │  │ FDE Header:                                         │    │
     *     │  │   - CIE offset (4 bytes)                            │    │
     *     │  │   - PC offset (4 bytes) <─ fde_offset_in_frame ─────┼────┼─> points to code_start
     *     │  │   - address range (4 bytes)                         │    │   (this specific field)
     *     │  │ CFI Instructions...                                 │    │
     *     │  └─────────────────────────────────────────────────────┘    │
     *     ├─────────────────────────────────────────────────────────────┤ < reference_point
     *     │                    EhFrameHeader                            │
     *     │                 (navigation metadata)                       │
     *     └─────────────────────────────────────────────────────────────┘
     *
     * The PC offset field in the FDE must contain the distance from itself to code_start:
     *
     *   distance = code_start - fde_pc_field
     *
     * Where:
     *   fde_pc_field_location = reference_point - eh_frame_size + fde_offset_in_frame
     *   code_start_location = reference_point - eh_frame_size - round_up(code_size, 8)
     *
     * Therefore:
     *   distance = code_start_location - fde_pc_field_location
     *            = (ref - eh_frame_size - rounded_code_size) - (ref - eh_frame_size + fde_offset_in_frame)
     *            = -rounded_code_size - fde_offset_in_frame
     *            = -(round_up(code_size, 8) + fde_offset_in_frame)
     *
     * Note: fde_offset_in_frame is the offset from EH frame start to the PC offset field.
     *
     */
    if (!absolute_addr) {
        int32_t rounded_code_size =
            (int32_t)_Py_SIZE_ROUND_UP(ctx->code_size, 8);
        int32_t fde_offset_in_frame = (int32_t)(ctx->fde_p - framep);
        *(int32_t *)ctx->fde_p = -(rounded_code_size + fde_offset_in_frame);
    }
}

#if defined(__linux__) && defined(__ELF__)
enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN = 1,
    JIT_UNREGISTER_FN = 2,
};

struct jit_code_entry {
    struct jit_code_entry *next;
    struct jit_code_entry *prev;
    const char *symfile_addr;
    uint64_t symfile_size;
};

struct jit_descriptor {
    uint32_t version;
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

static volatile struct jit_descriptor __jit_debug_descriptor = {
    1, JIT_NOACTION, NULL, NULL
};

static void __attribute__((noinline)) __jit_debug_register_code(void)
{
    /* Keep this call visible to debuggers and not optimized away. */
    (void)__jit_debug_descriptor.action_flag;
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" ::: "memory");
#endif
}

static uint16_t
gdb_jit_machine_id(void)
{
    /* Map the current target to ELF e_machine; return 0 to skip registration. */
#if defined(__x86_64__) || defined(_M_X64)
    return EM_X86_64;
#elif defined(__aarch64__) && !defined(__ILP32__)
    return EM_AARCH64;
#else
    return 0;
#endif
}

static void
gdb_jit_register_code(
    const void *code_addr,
    unsigned int code_size,
    const char *symname,
    const uint8_t *eh_frame,
    size_t eh_frame_size
)
{
    /*
     * Build a minimal in-memory ELF for GDB's JIT interface and link it into
     * __jit_debug_descriptor so debuggers can resolve JIT code.
     */
    if (code_addr == NULL || code_size == 0 || symname == NULL) {
        return;
    }

    const uint16_t machine = gdb_jit_machine_id();
    if (machine == 0) {
        return;
    }

    enum {
        SH_NULL = 0,
        SH_TEXT,
        SH_EH_FRAME,
        SH_SHSTRTAB,
        SH_STRTAB,
        SH_SYMTAB,
        SH_NUM,
    };
    static const char shstrtab[] =
        "\0.text\0.eh_frame\0.shstrtab\0.strtab\0.symtab";
    _Static_assert(sizeof(shstrtab) ==
        1 + sizeof(".text") + sizeof(".eh_frame") +
            sizeof(".shstrtab") + sizeof(".strtab") + sizeof(".symtab"),
        "shstrtab size mismatch");
    const size_t shstrtab_size = sizeof(shstrtab);
    const size_t sh_text = 1;
    const size_t sh_eh_frame = sh_text + sizeof(".text");
    const size_t sh_shstrtab = sh_eh_frame + sizeof(".eh_frame");
    const size_t sh_strtab = sh_shstrtab + sizeof(".shstrtab");
    const size_t sh_symtab = sh_strtab + sizeof(".strtab");
    const size_t text_size = code_size;
    const size_t text_padded = _Py_SIZE_ROUND_UP(text_size, 8);
    const size_t strtab_size = 1 + strlen(symname) + 1;
    const size_t symtab_size = 3 * sizeof(Elf64_Sym);

    size_t offset = sizeof(Elf64_Ehdr);
    offset = _Py_SIZE_ROUND_UP(offset, 16);
    const size_t text_off = offset;
    const size_t eh_off = text_off + text_padded;
    offset = eh_off + eh_frame_size;
    const size_t shstr_off = offset;
    offset += shstrtab_size;
    const size_t str_off = offset;
    offset += strtab_size;
    offset = _Py_SIZE_ROUND_UP(offset, sizeof(Elf64_Sym));
    const size_t sym_off = offset;
    offset += symtab_size;
    offset = _Py_SIZE_ROUND_UP(offset, sizeof(Elf64_Shdr));
    const size_t sh_off = offset;

    const size_t shnum = SH_NUM;
    const size_t total_size = sh_off + shnum * sizeof(Elf64_Shdr);
    uint8_t *buf = (uint8_t *)PyMem_RawMalloc(total_size);
    if (buf == NULL) {
        return;
    }
    memset(buf, 0, total_size);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buf;
    memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
    ehdr->e_ident[EI_CLASS] = ELFCLASS64;
    ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr->e_ident[EI_VERSION] = EV_CURRENT;
    ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
    ehdr->e_type = ET_DYN;
    ehdr->e_machine = machine;
    ehdr->e_version = EV_CURRENT;
    ehdr->e_entry = 0;
    ehdr->e_phoff = 0;
    ehdr->e_shoff = sh_off;
    ehdr->e_ehsize = sizeof(Elf64_Ehdr);
    ehdr->e_shentsize = sizeof(Elf64_Shdr);
    ehdr->e_shnum = shnum;
    ehdr->e_shstrndx = SH_SHSTRTAB;

    memcpy(buf + text_off, code_addr, text_size);
    memcpy(buf + eh_off, eh_frame, eh_frame_size);

    char *shstr = (char *)(buf + shstr_off);
    memcpy(shstr, shstrtab, shstrtab_size);

    char *strtab = (char *)(buf + str_off);
    strtab[0] = '\0';
    memcpy(strtab + 1, symname, strlen(symname));
    strtab[strtab_size - 1] = '\0';

    Elf64_Sym *syms = (Elf64_Sym *)(buf + sym_off);
    memset(syms, 0, symtab_size);
    /* Section symbol for .text (local) */
    syms[1].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
    syms[1].st_shndx = 1;
    /* Function symbol */
    syms[2].st_name = 1;
    syms[2].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
    syms[2].st_other = STV_DEFAULT;
    syms[2].st_shndx = 1;
    /* For ET_DYN/ET_EXEC, st_value is the absolute virtual address. */
    syms[2].st_value = (Elf64_Addr)(uintptr_t)code_addr;
    syms[2].st_size = code_size;

    Elf64_Shdr *shdrs = (Elf64_Shdr *)(buf + sh_off);
    memset(shdrs, 0, shnum * sizeof(Elf64_Shdr));

    shdrs[SH_TEXT].sh_name = sh_text;
    shdrs[SH_TEXT].sh_type = SHT_PROGBITS;
    shdrs[SH_TEXT].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdrs[SH_TEXT].sh_addr = (Elf64_Addr)(uintptr_t)code_addr;
    shdrs[SH_TEXT].sh_offset = text_off;
    shdrs[SH_TEXT].sh_size = text_size;
    shdrs[SH_TEXT].sh_addralign = 16;

    shdrs[SH_EH_FRAME].sh_name = sh_eh_frame;
    shdrs[SH_EH_FRAME].sh_type = SHT_PROGBITS;
    shdrs[SH_EH_FRAME].sh_flags = SHF_ALLOC;
    shdrs[SH_EH_FRAME].sh_addr =
        (Elf64_Addr)((uintptr_t)code_addr + text_padded);
    shdrs[SH_EH_FRAME].sh_offset = eh_off;
    shdrs[SH_EH_FRAME].sh_size = eh_frame_size;
    shdrs[SH_EH_FRAME].sh_addralign = 8;

    shdrs[SH_SHSTRTAB].sh_name = sh_shstrtab;
    shdrs[SH_SHSTRTAB].sh_type = SHT_STRTAB;
    shdrs[SH_SHSTRTAB].sh_offset = shstr_off;
    shdrs[SH_SHSTRTAB].sh_size = shstrtab_size;
    shdrs[SH_SHSTRTAB].sh_addralign = 1;

    shdrs[SH_STRTAB].sh_name = sh_strtab;
    shdrs[SH_STRTAB].sh_type = SHT_STRTAB;
    shdrs[SH_STRTAB].sh_offset = str_off;
    shdrs[SH_STRTAB].sh_size = strtab_size;
    shdrs[SH_STRTAB].sh_addralign = 1;

    shdrs[SH_SYMTAB].sh_name = sh_symtab;
    shdrs[SH_SYMTAB].sh_type = SHT_SYMTAB;
    shdrs[SH_SYMTAB].sh_offset = sym_off;
    shdrs[SH_SYMTAB].sh_size = symtab_size;
    shdrs[SH_SYMTAB].sh_link = SH_STRTAB;
    shdrs[SH_SYMTAB].sh_info = 2;
    shdrs[SH_SYMTAB].sh_addralign = 8;
    shdrs[SH_SYMTAB].sh_entsize = sizeof(Elf64_Sym);

    struct jit_code_entry *entry = PyMem_RawMalloc(sizeof(*entry));
    if (entry == NULL) {
        PyMem_RawFree(buf);
        return;
    }
    entry->symfile_addr = (const char *)buf;
    entry->symfile_size = total_size;
    entry->prev = NULL;
    entry->next = __jit_debug_descriptor.first_entry;
    if (entry->next != NULL) {
        entry->next->prev = entry;
    }
    __jit_debug_descriptor.first_entry = entry;
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
    __jit_debug_register_code();
    __jit_debug_descriptor.action_flag = JIT_NOACTION;

}
#endif  // __linux__ && __ELF__

void
_PyJitUnwind_GdbRegisterCode(const void *code_addr,
                           unsigned int code_size,
                           const char *entry,
                           const char *filename)
{
#if defined(__linux__) && defined(__ELF__)
    /* GDB expects a stable symbol name and absolute addresses in .eh_frame. */
    if (entry == NULL) {
        entry = "";
    }
    if (filename == NULL) {
        filename = "";
    }
    size_t name_size = snprintf(NULL, 0, "py::%s:%s", entry, filename) + 1;
    char *name = (char *)PyMem_RawMalloc(name_size);
    if (name == NULL) {
        return;
    }
    snprintf(name, name_size, "py::%s:%s", entry, filename);

    uint8_t buffer[1024];
    size_t eh_frame_size = _PyJitUnwind_BuildEhFrame(
        buffer, sizeof(buffer), code_addr, code_size, 1);
    if (eh_frame_size == 0) {
        PyMem_RawFree(name);
        return;
    }

    gdb_jit_register_code(code_addr, code_size, name,
                          buffer, eh_frame_size);
    PyMem_RawFree(name);
#else
    (void)code_addr;
    (void)code_size;
    (void)entry;
    (void)filename;
#endif
}

#endif  // PY_HAVE_PERF_TRAMPOLINE
