#include "Python.h"

#ifdef _Py_JIT

#include "pycore_abstract.h"
#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pyerrors.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_uops.h"
#include "pycore_jit.h"

#include "ceval_macros.h"
#include "jit_stencils.h"

#ifndef MS_WINDOWS
    #include <sys/mman.h>
#endif

static uint64_t page_size;

static bool
is_page_aligned(void *p)
{
    return ((uint64_t)p & (page_size - 1)) == 0;
}

static char *
alloc(uint64_t pages)
{
    assert(pages);
    uint64_t size = pages * page_size;
    char *memory;
#ifdef MS_WINDOWS
    memory = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (memory == NULL) {
        const char *w = "JIT unable to allocate memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        return NULL;
    }
#else
    memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory == MAP_FAILED) {
        const char *w = "JIT unable to allocate memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        return NULL;
    }
#endif
    assert(is_page_aligned(memory));
    return memory;
}

static int
mark_executable(char *memory, uint64_t pages)
{
    assert(is_page_aligned(memory));
    if (pages == 0) {
        return 0;
    }
    uint64_t size = pages * page_size;
#ifdef MS_WINDOWS
    if (!FlushInstructionCache(GetCurrentProcess(), memory, size)) {
        const char *w = "JIT unable to flush instruction cache (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        return -1;
    }
    DWORD old;
    if (!VirtualProtect(memory, size, PAGE_EXECUTE, &old)) {
        const char *w = "JIT unable to protect executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        return -1;
    }
#else
    __builtin___clear_cache((char *)memory, (char *)memory + size);
    if (mprotect(memory, size, PROT_EXEC)) {
        const char *w = "JIT unable to protect executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        return -1;
    }
#endif
    return 0;
}

static int
mark_readable(char *memory, uint64_t pages)
{
    assert(is_page_aligned(memory));
    if (pages == 0) {
        return 0;
    }
    uint64_t size = pages * page_size;
#ifdef MS_WINDOWS
    DWORD old;
    if (!VirtualProtect(memory, size, PAGE_READONLY, &old)) {
        const char *w = "JIT unable to protect readable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        return -1;
    }
#else
    if (mprotect(memory, size, PROT_READ)) {
        const char *w = "JIT unable to protect readable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        return -1;
    }
#endif
    return 0;
}

static uint64_t
size_to_pages(uint64_t size)
{
    return (size + page_size - 1) / page_size;
}

// static char *emit_trampoline(uint64_t where, char **trampolines);

static bool
fits_in_signed_bits(uint64_t value, int bits)
{
    int64_t v = (int64_t)value;
    return (v >= -(1LL << (bits - 1))) && (v < (1LL << (bits - 1)));
}

// static uint64_t
// potential_trampoline_size(const StencilGroup *stencil_group)
// {
//     return stencil_group->text.holes_size * trampoline_stencil_group.text.body_size;
// }

static void
patch(char *base, const Hole *hole, uint64_t *patches)
{
    char *location = base + hole->offset;
    uint64_t value = patches[hole->value] + (uint64_t)hole->symbol + hole->addend;
    uint32_t *addr = (uint32_t *)location;
    switch (hole->kind) {
        case HoleKind_IMAGE_REL_I386_DIR32:
            *addr = (uint32_t)value;
            return;
        case HoleKind_IMAGE_REL_AMD64_REL32:
        case HoleKind_IMAGE_REL_I386_REL32:
        case HoleKind_R_X86_64_PC32:  // XXX
        case HoleKind_R_X86_64_PLT32:
        case HoleKind_X86_64_RELOC_BRANCH:
        case HoleKind_X86_64_RELOC_GOT:
        case HoleKind_X86_64_RELOC_GOT_LOAD:
            // if (!fits_in_signed_bits(value - (uint64_t)location, 32)) {
            //     value = (uint64_t)emit_trampoline(value + 4, trampolines) - 4;  // XXX
            // }
            value -= (uint64_t)location;
            assert(fits_in_signed_bits(value, 32));
            *addr = (uint32_t)value;
            return;
        case HoleKind_ARM64_RELOC_UNSIGNED:
        case HoleKind_IMAGE_REL_AMD64_ADDR64:
        case HoleKind_R_AARCH64_ABS64:
        case HoleKind_R_X86_64_64:
        case HoleKind_X86_64_RELOC_UNSIGNED:
            *(uint64_t *)addr = value;
            return;
        case HoleKind_ARM64_RELOC_GOT_LOAD_PAGE21:
            value = ((value >> 12) << 12) - (((uint64_t)location >> 12) << 12);
            assert((*addr & 0x9F000000) == 0x90000000);
            assert((value & 0xFFF) == 0);
            uint32_t lo = (value << 17) & 0x60000000;
            uint32_t hi = (value >> 9) & 0x00FFFFE0;
            *addr = (*addr & 0x9F00001F) | hi | lo;
            return;
        case HoleKind_R_AARCH64_CALL26:
        case HoleKind_R_AARCH64_JUMP26:
            // if (!fits_in_signed_bits(value - (uint64_t)location, 28)) {
            //     value = (uint64_t)emit_trampoline(value, trampolines);
            // }
            value -= (uint64_t)location;
            assert(((*addr & 0xFC000000) == 0x14000000) ||
                   ((*addr & 0xFC000000) == 0x94000000));
            assert((value & 0x3) == 0);
            assert(fits_in_signed_bits(value, 28));
            *addr = (*addr & 0xFC000000) | ((uint32_t)(value >> 2) & 0x03FFFFFF);
            return;
        case HoleKind_ARM64_RELOC_GOT_LOAD_PAGEOFF12:
            value &= (1 << 12) - 1;
            assert(((*addr & 0x3B000000) == 0x39000000) ||
                   ((*addr & 0x11C00000) == 0x11000000));
            int shift = 0;
            if ((*addr & 0x3B000000) == 0x39000000) {
                shift = ((*addr >> 30) & 0x3);
                if (shift == 0 && (*addr & 0x04800000) == 0x04800000) {
                    shift = 4;
                }
            }
            assert(((value & ((1 << shift) - 1)) == 0));
            *addr = (*addr & 0xFFC003FF) | ((uint32_t)((value >> shift) << 10) & 0x003FFC00);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G0_NC:
            assert(((*addr >> 21) & 0x3) == 0);
            *addr = (*addr & 0xFFE0001F) | (((value >>  0) & 0xFFFF) << 5);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G1_NC:
            assert(((*addr >> 21) & 0x3) == 1);
            *addr = (*addr & 0xFFE0001F) | (((value >> 16) & 0xFFFF) << 5);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G2_NC:
            assert(((*addr >> 21) & 0x3) == 2);
            *addr = (*addr & 0xFFE0001F) | (((value >> 32) & 0xFFFF) << 5);
            return;
        case HoleKind_R_AARCH64_MOVW_UABS_G3:
            assert(((*addr >> 21) & 0x3) == 3);
            *addr = (*addr & 0xFFE0001F) | (((value >> 48) & 0xFFFF) << 5);
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

// static char *
// emit_trampoline(uint64_t where, char **trampolines)
// {
//     assert(trampolines && *trampolines);
//     const StencilGroup *stencil_group = &trampoline_stencil_group;
//     assert(stencil_group->data.body_size == 0);
//     uint64_t patches[] = GET_PATCHES();
//     patches[HoleValue_CONTINUE] = where;
//     patches[HoleValue_TEXT] = (uint64_t)*trampolines;
//     patches[HoleValue_ZERO] = 0;
//     emit(stencil_group, patches, NULL);
//     char *trampoline = *trampolines;
//     *trampolines += stencil_group->text.body_size;
//     return trampoline;
// }

static void
initialize_jit(void)
{
    if (page_size) {
        return;
    }
#ifdef MS_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = si.dwPageSize;
#else
    page_size = sysconf(_SC_PAGESIZE);
#endif
    assert(page_size);
    assert((page_size & (page_size - 1)) == 0);
}

// The world's smallest compiler?
_PyJITFunction
_PyJIT_CompileTrace(_PyUOpExecutorObject *executor, _PyUOpInstruction *trace, int size)
{
    initialize_jit();
    // First, loop over everything once to find the total compiled size:
    // uint64_t trampoline_size = potential_trampoline_size(&wrapper_stencil_group);
    uint64_t text_size = wrapper_stencil_group.text.body_size;
    uint64_t data_size = wrapper_stencil_group.data.body_size;
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        // trampoline_size += potential_trampoline_size(stencil_group);
        text_size += stencil_group->text.body_size;
        data_size += stencil_group->data.body_size;
        assert(stencil_group->text.body_size);
    };
    uint64_t text_pages = size_to_pages(text_size);
    uint64_t data_pages = size_to_pages(data_size);
    char *text = alloc(text_pages + data_pages);
    if (text == NULL) {
        return NULL;
    }
    char *data = text + text_pages * page_size;
    // char *head_trampolines = text + text_size;
    char *head_text = text;
    char *head_data = data;
    // First, the wrapper:
    const StencilGroup *stencil_group = &wrapper_stencil_group;
    uint64_t patches[] = GET_PATCHES();
    patches[HoleValue_CONTINUE] = (uint64_t)head_text + stencil_group->text.body_size;
    patches[HoleValue_DATA] = (uint64_t)head_data;
    patches[HoleValue_TEXT] = (uint64_t)head_text;
    patches[HoleValue_ZERO] = 0;
    emit(stencil_group, patches);
    head_text += stencil_group->text.body_size;
    head_data += stencil_group->data.body_size;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        uint64_t patches[] = GET_PATCHES();
        patches[HoleValue_CONTINUE] = (uint64_t)head_text + stencil_group->text.body_size;
        patches[HoleValue_CURRENT_EXECUTOR] = (uint64_t)executor;
        patches[HoleValue_OPARG] = instruction->oparg;
        patches[HoleValue_OPERAND] = instruction->operand;
        patches[HoleValue_TARGET] = instruction->target;
        patches[HoleValue_DATA] = (uint64_t)head_data;
        patches[HoleValue_TEXT] = (uint64_t)head_text;
        patches[HoleValue_TOP] = (uint64_t)text + wrapper_stencil_group.text.body_size;
        patches[HoleValue_ZERO] = 0;
        emit(stencil_group, patches);
        head_text += stencil_group->text.body_size;
        head_data += stencil_group->data.body_size;
    };
    if (mark_executable(text, text_pages) || mark_readable(data, data_pages)) {
        return NULL;
    }
    // Wow, done already?
    assert(head_text == text + text_size);
    assert(head_data == data + data_size);
    return (_PyJITFunction)text;
}

#endif