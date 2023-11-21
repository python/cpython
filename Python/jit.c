#include "Python.h"
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

#define MB (1 << 20)
#define JIT_POOL_SIZE  (128 * MB)

// This next line looks crazy, but it's actually not *that* bad. Yes, we're
// statically allocating a huge empty array in our executable, and mapping
// executable pages inside of it. However, this has a big benefit: we can
// compile our stencils to use the "small" or "medium" code models, since we
// know that all calls (for example, to C-API functions like _PyLong_Add) will
// be less than a relative 32-bit jump away (28 bits on aarch64). If that
// condition didn't hold (for example, if we mmap some memory far away from the
// executable), we would need to use trampolines and/or 64-bit indirect branches
// to extend the range. That's pretty slow and complex, whereas this "just
// works" (though we could certainly switch to a scheme like that without *too*
// much trouble). The OS lazily allocates pages for this array anyways (and it's
// BSS data that's not included in the interpreter executable itself), so it's
// not like we're *actually* making the executable huge at runtime (or on disk):
static unsigned char pool[JIT_POOL_SIZE];
static size_t pool_head;
static size_t page_size;

static int needs_initializing = 1;

static bool
is_page_aligned(void *p)
{
    return (((uintptr_t)p & (page_size - 1)) == 0);
}

static unsigned char *
alloc(size_t pages)
{
    size_t size = pages * page_size;
    if (JIT_POOL_SIZE - page_size < pool_head + size) {
        PyErr_WarnEx(PyExc_RuntimeWarning, "JIT out of memory", 0);
        needs_initializing = -1;
        return NULL;
    }
    unsigned char *memory = pool + pool_head;
    pool_head += size;
    assert(is_page_aligned(pool + pool_head));
    assert(is_page_aligned(memory));
    return memory;
}

static int
mark_executable(unsigned char *memory, size_t pages)
{
    assert(is_page_aligned(memory));
    size_t size = pages * page_size;
    if (size == 0) {
        return 0;
    }
#ifdef MS_WINDOWS
    if (!FlushInstructionCache(GetCurrentProcess(), memory, size)) {
        const char *w = "JIT unable to flush instruction cache (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        needs_initializing = -1;
        return -1;
    }
    DWORD old;
    if (!VirtualProtect(memory, size, PAGE_EXECUTE, &old)) {
        const char *w = "JIT unable to protect executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        needs_initializing = -1;
        return -1;
    }
#else
    __builtin___clear_cache((char *)memory, (char *)memory + size);
    if (mprotect(memory, size, PROT_EXEC)) {
        const char *w = "JIT unable to protect executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        needs_initializing = -1;
        return -1;
    }
#endif
    return 0;
}

static int
mark_readable(unsigned char *memory, size_t pages)
{
    assert(is_page_aligned(memory));
    size_t size = pages * page_size;
    if (size == 0) {
        return 0;
    }
#ifdef MS_WINDOWS
    DWORD old;
    if (!VirtualProtect(memory, size, PAGE_READONLY, &old)) {
        const char *w = "JIT unable to protect readable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, GetLastError());
        needs_initializing = -1;
        return -1;
    }
#else
    if (mprotect(memory, size, PROT_READ)) {
        const char *w = "JIT unable to protect readable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        needs_initializing = -1;
        return -1;
    }
#endif
    return 0;
}

static size_t
size_to_pages(size_t size)
{
    return (size + page_size - 1) / page_size;
}

static void
patch(unsigned char *base, const Hole *hole, uint64_t *patches)
{
    unsigned char *location = base + hole->offset;
    uint64_t value = patches[hole->value] + hole->addend;
    uint32_t *addr = (uint32_t *)location;
    switch (hole->kind) {
        case HoleKind_IMAGE_REL_I386_DIR32:
            *addr = (uint32_t)value;
            return;
        case HoleKind_IMAGE_REL_AMD64_REL32:
        case HoleKind_IMAGE_REL_I386_REL32:
        case HoleKind_R_X86_64_PC32:
        case HoleKind_R_X86_64_PLT32:
        case HoleKind_X86_64_RELOC_BRANCH:
        case HoleKind_X86_64_RELOC_GOT:
        case HoleKind_X86_64_RELOC_GOT_LOAD:
            value -= (uintptr_t)location;
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
            value = ((value >> 12) << 12) - (((uintptr_t)location >> 12) << 12);
            assert((*addr & 0x9F000000) == 0x90000000);
            assert((value & 0xFFF) == 0);
            uint32_t lo = (value << 17) & 0x60000000;
            uint32_t hi = (value >> 9) & 0x00FFFFE0;
            *addr = (*addr & 0x9F00001F) | hi | lo;
            return;
        case HoleKind_R_AARCH64_CALL26:
        case HoleKind_R_AARCH64_JUMP26:
            value -= (uintptr_t)location;
            assert(((*addr & 0xFC000000) == 0x14000000) ||
                   ((*addr & 0xFC000000) == 0x94000000));
            assert((value & 0x3) == 0);
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
copy_and_patch(unsigned char *base, const Stencil *stencil, uint64_t *patches)
{
    memcpy(base, stencil->body, stencil->body_size);
    for (size_t i = 0; i < stencil->holes_size; i++) {
        patch(base, &stencil->holes[i], patches);
    }
}

static void
emit(const StencilGroup *stencil_group, uint64_t patches[])
{
    unsigned char *data = (unsigned char *)(uintptr_t)patches[_JIT_DATA];
    copy_and_patch(data, &stencil_group->data, patches);
    unsigned char *text = (unsigned char *)(uintptr_t)patches[_JIT_BODY];
    copy_and_patch(text, &stencil_group->text, patches);
}

static int
initialize_jit(void)
{
    if (needs_initializing <= 0) {
        return needs_initializing;
    }
    // Keep us from re-entering:
    needs_initializing = -1;
    // Find the page_size:
#ifdef MS_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = si.dwPageSize;
#else
    page_size = sysconf(_SC_PAGESIZE);
#endif
    assert(page_size);
    assert((page_size & (page_size - 1)) == 0);
    // Adjust the pool_head to the next page boundary:
    pool_head = (page_size - ((uintptr_t)pool & (page_size - 1))) & (page_size - 1);
    assert(is_page_aligned(pool + pool_head));
    // macOS requires mapping memory before mprotecting it, so map memory fixed
    // at our pool's valid address range:
#ifdef __APPLE__
    void *mapped = mmap(pool + pool_head, JIT_POOL_SIZE - pool_head - page_size,
                        PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
    if (mapped == MAP_FAILED) {
        const char *w = "JIT unable to map fixed memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
        return needs_initializing;
    }
    assert(mapped == pool + pool_head);
#endif
    // Done:
    needs_initializing = 0;
    return needs_initializing;
}

// The world's smallest compiler?
_PyJITFunction
_PyJIT_CompileTrace(_PyUOpExecutorObject *executor, _PyUOpInstruction *trace, int size)
{
    if (initialize_jit()) {
        return NULL;
    }
    // First, loop over everything once to find the total compiled size:
    size_t text_size = wrapper_stencil_group.text.body_size;
    size_t data_size = wrapper_stencil_group.data.body_size;
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        text_size += stencil_group->text.body_size;
        data_size += stencil_group->data.body_size;
        assert(stencil_group->text.body_size);
    };
    size_t text_pages = size_to_pages(text_size);
    unsigned char *text = alloc(text_pages);
    if (text == NULL) {
        return NULL;
    }
    size_t data_pages = size_to_pages(data_size);
    unsigned char *data = alloc(data_pages);
    if (data == NULL) {
        return NULL;
    }
    unsigned char *head_text = text;
    unsigned char *head_data = data;
    // First, the wrapper:
    const StencilGroup *stencil_group = &wrapper_stencil_group;
    uint64_t patches[] = GET_PATCHES();
    patches[_JIT_BODY] = (uintptr_t)head_text;
    patches[_JIT_DATA] = (uintptr_t)head_data;
    patches[_JIT_CONTINUE] = (uintptr_t)head_text + stencil_group->text.body_size;
    patches[_JIT_ZERO] = 0;
    emit(stencil_group, patches);
    head_text += stencil_group->text.body_size;
    head_data += stencil_group->data.body_size;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)head_text;
        patches[_JIT_DATA] = (uintptr_t)head_data;
        patches[_JIT_CONTINUE] = (uintptr_t)head_text + stencil_group->text.body_size;
        patches[_JIT_CURRENT_EXECUTOR] = (uintptr_t)executor;
        patches[_JIT_OPARG] = instruction->oparg;
        patches[_JIT_OPERAND] = instruction->operand;
        patches[_JIT_TARGET] = instruction->target;
        patches[_JIT_TOP] = (uintptr_t)text + wrapper_stencil_group.text.body_size;
        patches[_JIT_ZERO] = 0;
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
