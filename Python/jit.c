#ifdef _Py_JIT

#include "Python.h"

#include "pycore_abstract.h"
#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_optimizer.h"
#include "pycore_pyerrors.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_jit.h"

#include "jit_stencils.h"

// Memory management stuff: ////////////////////////////////////////////////////

#ifndef MS_WINDOWS
    #include <sys/mman.h>
#endif

static size_t
get_page_size(void)
{
#ifdef MS_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

static void
jit_warn(const char *message)
{
#ifdef MS_WINDOWS
    int hint = GetLastError();
#else
    int hint = errno;
#endif
    PyErr_WarnFormat(PyExc_RuntimeWarning, 0, "JIT %s (%d)", message, hint);
}

static char *
jit_alloc(size_t size)
{
    assert(size);
    assert(size % get_page_size() == 0);
#ifdef MS_WINDOWS
    int flags = MEM_COMMIT | MEM_RESERVE;
    char *memory = VirtualAlloc(NULL, size, flags, PAGE_READWRITE);
    int failed = memory == NULL;
#else
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    char *memory = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    int failed = memory == MAP_FAILED;
#endif
    if (failed) {
        jit_warn("unable to allocate memory");
        return NULL;
    }
    return memory;
}

static void
jit_free(char *memory, size_t size)
{
    assert(size);
    assert(size % get_page_size() == 0);
#ifdef MS_WINDOWS
    int failed = !VirtualFree(memory, 0, MEM_RELEASE);
#else
    int failed = munmap(memory, size);
#endif
    if (failed) {
        jit_warn("unable to free memory");
    }
}

static int
mark_executable(char *memory, size_t size)
{
    if (size == 0) {
        return 0;
    }
    assert(size % get_page_size() == 0);
    // Do NOT ever leave the memory writable! Also, don't forget to flush the
    // i-cache (I cannot begin to tell you how horrible that is to debug):
#ifdef MS_WINDOWS
    if (!FlushInstructionCache(GetCurrentProcess(), memory, size)) {
        jit_warn("unable to flush instruction cache");
        return -1;
    }
    int old;
    int failed = !VirtualProtect(memory, size, PAGE_EXECUTE_READ, &old);
#else
    __builtin___clear_cache((char *)memory, (char *)memory + size);
    int failed = mprotect(memory, size, PROT_EXEC | PROT_READ);
#endif
    if (failed) {
        jit_warn("unable to protect executable memory");
        return -1;
    }
    return 0;
}

static int
mark_readable(char *memory, size_t size)
{
    if (size == 0) {
        return 0;
    }
    assert(size % get_page_size() == 0);
#ifdef MS_WINDOWS
    DWORD old;
    int failed = !VirtualProtect(memory, size, PAGE_READONLY, &old);
#else
    int failed = mprotect(memory, size, PROT_READ);
#endif
    if (failed) {
        jit_warn("unable to protect readable memory");
        return -1;
    }
    return 0;
}

// JIT compiler stuff: /////////////////////////////////////////////////////////

// Warning! AArch64 requires you to get your hands dirty. These are your gloves:

// value[i : i + n]
static uint32_t
get_bits(uint64_t value, uint8_t i, uint8_t n)
{
    assert(n <= 32);
    return (value >> i) & ((1ULL << n) - 1);
}

// *loc[i : i + n] = value[j : j + n]
static void
set_bits(uint32_t *loc, uint8_t i, uint8_t n, uint64_t value, uint8_t j)
{
    assert(i + n <= 32);
    // Clear the bits we're about to patch:
    *loc &= ~(((1ULL << n) - 1) << i);
    assert(get_bits(*loc, i, n) == 0);
    // Patch the bits:
    *loc |= get_bits(value, j, n) << i;
    assert(get_bits(*loc, i, n) == get_bits(value, j, n));
}

// See https://developer.arm.com/documentation/ddi0602/2023-09/Base-Instructions
// for instruction encodings:
#define IS_AARCH64_ADD_OR_SUB(I) (((I) & 0x11C00000) == 0x11000000)
#define IS_AARCH64_ADRP(I)       (((I) & 0x9F000000) == 0x90000000)
#define IS_AARCH64_BRANCH(I)     (((I) & 0x7C000000) == 0x14000000)
#define IS_AARCH64_LDR_OR_STR(I) (((I) & 0x3B000000) == 0x39000000)
#define IS_AARCH64_MOV(I)        (((I) & 0x9F800000) == 0x92800000)

// LLD is an awesome reference for how to perform relocations... just keep in
// mind that Tools/jit/build.py does some filtering and preprocessing for us!
// Here's a good place to start for each platform:
// - aarch64-apple-darwin:
//   - https://github.com/llvm/llvm-project/blob/main/lld/MachO/Arch/ARM64Common.cpp
//   - https://github.com/llvm/llvm-project/blob/main/lld/MachO/Arch/ARM64Common.h
// - aarch64-unknown-linux-gnu:
//   - https://github.com/llvm/llvm-project/blob/main/lld/ELF/Arch/AArch64.cpp
// - i686-pc-windows-msvc:
//   - https://github.com/llvm/llvm-project/blob/main/lld/COFF/Chunks.cpp
// - x86_64-apple-darwin:
//   - https://github.com/llvm/llvm-project/blob/main/lld/MachO/Arch/X86_64.cpp
// - x86_64-pc-windows-msvc:
//   - https://github.com/llvm/llvm-project/blob/main/lld/COFF/Chunks.cpp
// - x86_64-unknown-linux-gnu:
//   - https://github.com/llvm/llvm-project/blob/main/lld/ELF/Arch/X86_64.cpp
static void
patch(char *base, const Hole *hole, uint64_t *patches)
{
    void *location = base + hole->offset;
    uint64_t value = patches[hole->value] + (uint64_t)hole->symbol + hole->addend;
    uint32_t *loc32 = (uint32_t *)location;
    uint64_t *loc64 = (uint64_t *)location;
    switch (hole->kind) {
        case HoleKind_IMAGE_REL_I386_DIR32:
            // 32-bit absolute address.
            // Check that we're not out of range of 32 unsigned bits:
            assert(value < (1ULL << 32));
            *loc32 = (uint32_t)value;
            return;
        case HoleKind_ARM64_RELOC_UNSIGNED:
        case HoleKind_IMAGE_REL_AMD64_ADDR64:
        case HoleKind_R_AARCH64_ABS64:
        case HoleKind_X86_64_RELOC_UNSIGNED:
        case HoleKind_R_X86_64_64:
            // 64-bit absolute address.
            *loc64 = value;
            return;
        case HoleKind_R_AARCH64_CALL26:
        case HoleKind_R_AARCH64_JUMP26:
            // 28-bit relative branch.
            assert(IS_AARCH64_BRANCH(*loc32));
            value -= (uint64_t)location;
            // Check that we're not out of range of 28 signed bits:
            assert((int64_t)value >= -(1 << 27));
            assert((int64_t)value < (1 << 27));
            // Since instructions are 4-byte aligned, only use 26 bits:
            assert(get_bits(value, 0, 2) == 0);
            set_bits(loc32, 0, 26, value, 2);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G0_NC:
            // 16-bit low part of an absolute address.
            assert(IS_AARCH64_MOV(*loc32));
            // Check the implicit shift (this is "part 0 of 3"):
            assert(get_bits(*loc32, 21, 2) == 0);
            set_bits(loc32, 5, 16, value, 0);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G1_NC:
            // 16-bit middle-low part of an absolute address.
            assert(IS_AARCH64_MOV(*loc32));
            // Check the implicit shift (this is "part 1 of 3"):
            assert(get_bits(*loc32, 21, 2) == 1);
            set_bits(loc32, 5, 16, value, 16);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G2_NC:
            // 16-bit middle-high part of an absolute address.
            assert(IS_AARCH64_MOV(*loc32));
            // Check the implicit shift (this is "part 2 of 3"):
            assert(get_bits(*loc32, 21, 2) == 2);
            set_bits(loc32, 5, 16, value, 32);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G3:
            // 16-bit high part of an absolute address.
            assert(IS_AARCH64_MOV(*loc32));
            // Check the implicit shift (this is "part 3 of 3"):
            assert(get_bits(*loc32, 21, 2) == 3);
            set_bits(loc32, 5, 16, value, 48);
            return;
        case HoleKind_ARM64_RELOC_GOT_LOAD_PAGE21:
            // 21-bit count of pages between this page and an absolute address's
            // page... I know, I know, it's weird. Pairs nicely with
            // ARM64_RELOC_GOT_LOAD_PAGEOFF12 (below).
            assert(IS_AARCH64_ADRP(*loc32));
            // Number of pages between this page and the value's page:
            value = (value >> 12) - ((uint64_t)location >> 12);
            // Check that we're not out of range of 21 signed bits:
            assert((int64_t)value >= -(1 << 20));
            assert((int64_t)value < (1 << 20));
            // value[0:2] goes in loc[29:31]:
            set_bits(loc32, 29, 2, value, 0);
            // value[2:21] goes in loc[5:26]:
            set_bits(loc32, 5, 19, value, 2);
            return;
        case HoleKind_ARM64_RELOC_GOT_LOAD_PAGEOFF12:
            // 12-bit low part of an absolute address. Pairs nicely with
            // ARM64_RELOC_GOT_LOAD_PAGE21 (above).
            assert(IS_AARCH64_LDR_OR_STR(*loc32) || IS_AARCH64_ADD_OR_SUB(*loc32));
            // There might be an implicit shift encoded in the instruction:
            uint8_t shift = 0;
            if (IS_AARCH64_LDR_OR_STR(*loc32)) {
                shift = (uint8_t)get_bits(*loc32, 30, 2);
                // If both of these are set, the shift is supposed to be 4.
                // That's pretty weird, and it's never actually been observed...
                assert(get_bits(*loc32, 23, 1) == 0 || get_bits(*loc32, 26, 1) == 0);
            }
            value = get_bits(value, 0, 12);
            assert(get_bits(value, 0, shift) == 0);
            set_bits(loc32, 10, 12, value, shift);
            return;
    }
    Py_UNREACHABLE();
}

static void
copy_and_patch(char *base, const Stencil *stencil, uint64_t *patches)
{
    memcpy(base, stencil->body, stencil->body_size);
    for (uint64_t i = 0; i < stencil->holes_size; i++) {
        patch(base, &stencil->holes[i], patches);
    }
}

static void
emit(const StencilGroup *stencil_group, uint64_t patches[])
{
    char *data = (char *)patches[HoleValue_DATA];
    copy_and_patch(data, &stencil_group->data, patches);
    char *text = (char *)patches[HoleValue_TEXT];
    copy_and_patch(text, &stencil_group->text, patches);
}

// This becomes the executor's execute member, and handles some setup/teardown:
static _Py_CODEUNIT *
execute(_PyExecutorObject *executor, _PyInterpreterFrame *frame,
        PyObject **stack_pointer)
{
    PyThreadState *tstate = PyThreadState_Get();
    assert(PyObject_TypeCheck(executor, &_PyUOpExecutor_Type));
    jit_func jitted = ((_PyUOpExecutorObject *)executor)->jit_code;
    _Py_CODEUNIT *next_instr = jitted(frame, stack_pointer, tstate);
    Py_DECREF(executor);
    return next_instr;
}

// Compiles executor in-place. Don't forget to call _PyJIT_Free later!
int
_PyJIT_Compile(_PyUOpExecutorObject *executor)
{
    // Loop once to find the total compiled size:
    size_t text_size = 0;
    size_t data_size = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(executor); i++) {
        _PyUOpInstruction *instruction = &executor->trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        text_size += stencil_group->text.body_size;
        data_size += stencil_group->data.body_size;
    }
    // Round up to the nearest page (text and data need separate pages):
    size_t page_size = get_page_size();
    assert((page_size & (page_size - 1)) == 0);
    text_size += page_size - (text_size & (page_size - 1));
    data_size += page_size - (data_size & (page_size - 1));
    char *memory = jit_alloc(text_size + data_size);
    if (memory == NULL) {
        goto fail;
    }
    // Loop again to emit the code:
    char *text = memory;
    char *data = memory + text_size;
    for (Py_ssize_t i = 0; i < Py_SIZE(executor); i++) {
        _PyUOpInstruction *instruction = &executor->trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        // Think of patches as a dictionary mapping HoleValue to uint64_t:
        uint64_t patches[] = GET_PATCHES();
        patches[HoleValue_CONTINUE] = (uint64_t)text + stencil_group->text.body_size;
        patches[HoleValue_CURRENT_EXECUTOR] = (uint64_t)executor;
        patches[HoleValue_OPARG] = instruction->oparg;
        patches[HoleValue_OPERAND] = instruction->operand;
        patches[HoleValue_TARGET] = instruction->target;
        patches[HoleValue_DATA] = (uint64_t)data;
        patches[HoleValue_TEXT] = (uint64_t)text;
        patches[HoleValue_TOP] = (uint64_t)memory;
        patches[HoleValue_ZERO] = 0;
        emit(stencil_group, patches);
        text += stencil_group->text.body_size;
        data += stencil_group->data.body_size;
    }
    if (mark_executable(memory, text_size) ||
        mark_readable(memory + text_size, data_size))
    {
        jit_free(memory, text_size + data_size);
        goto fail;
    }
    executor->base.execute = execute;
    executor->jit_code = memory;
    executor->jit_size = text_size + data_size;
    return 1;
fail:
    return PyErr_Occurred() ? -1 : 0;
}

void
_PyJIT_Free(_PyUOpExecutorObject *executor)
{
    char *memory = (char *)executor->jit_code;
    size_t size = executor->jit_size;
    if (memory) {
        executor->jit_code = NULL;
        executor->jit_size = 0;
        jit_free(memory, size);
    }
}

#endif  // _Py_JIT
