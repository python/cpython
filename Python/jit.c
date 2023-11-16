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
unsigned char *deoptimize_stub;
unsigned char *error_stub;

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
    DWORD old;
    if (!FlushInstructionCache(GetCurrentProcess(), memory, size) ||
        !VirtualProtect(memory, size, PAGE_EXECUTE_READ, &old))
    {
        int code = GetLastError();
#else
    __builtin___clear_cache((char *)memory, (char *)memory + size);
    if (mprotect(memory, size, PROT_EXEC | PROT_READ)) {
        int code = errno;
#endif
        const char *w = "JIT unable to map executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, code);
        return -1;
    }
    return 0;
}

static size_t
bytes_to_pages(size_t nbytes)
{
    return (nbytes + page_size - 1) / page_size;
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
    memcpy(base, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        patch(base, &stencil->holes[i], patches);
    }
}

static void
emit(const StencilGroup *stencil_group, uint64_t patches[])
{
    unsigned char *data = (unsigned char *)(uintptr_t)patches[_JIT_DATA];
    copy_and_patch(data, &stencil_group->data, patches);
    unsigned char *body = (unsigned char *)(uintptr_t)patches[_JIT_BODY];
    copy_and_patch(body, &stencil_group->body, patches);
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
    // Write our deopt stub:
    {
        const StencilGroup *stencil_group = &deoptimize_stencil_group;
        size_t pages_body = bytes_to_pages(stencil_group->body.nbytes);
        deoptimize_stub = alloc(pages_body);
        if (deoptimize_stub == NULL) {
            return needs_initializing;
        }
        size_t pages_data = bytes_to_pages(stencil_group->data.nbytes);
        unsigned char *data = alloc(pages_data);
        if (data == NULL) {
            return needs_initializing;
        }
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)deoptimize_stub;
        patches[_JIT_DATA] = (uintptr_t)data;
        patches[_JIT_ZERO] = 0;
        emit(stencil_group, patches);
        if (mark_executable(deoptimize_stub, pages_body)) {
            return needs_initializing;
        }
    }
    // Write our error stub:
    {
        const StencilGroup *stencil_group = &error_stencil_group;
        size_t pages_body = bytes_to_pages(stencil_group->body.nbytes);
        error_stub = alloc(pages_body);
        if (error_stub == NULL) {
            return needs_initializing;
        }
        size_t pages_data = bytes_to_pages(stencil_group->data.nbytes);
        unsigned char *data = alloc(pages_data);
        if (data == NULL) {
            return needs_initializing;
        }
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)error_stub;
        patches[_JIT_DATA] = (uintptr_t)data;
        patches[_JIT_ZERO] = 0;
        emit(stencil_group, patches);
        if (mark_executable(error_stub, pages_body)) {
            return needs_initializing;
        }
    }
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
    size_t nbytes_body = trampoline_stencil_group.body.nbytes;
    size_t nbytes_data = trampoline_stencil_group.data.nbytes;
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        nbytes_body += stencil_group->body.nbytes;
        nbytes_data += stencil_group->data.nbytes;
        assert(stencil_group->body.nbytes);
    };
    size_t pages_body = bytes_to_pages(nbytes_body);
    unsigned char *body = alloc(pages_body);
    if (body == NULL) {
        return NULL;
    }
    size_t pages_data = bytes_to_pages(nbytes_data);
    unsigned char *data = alloc(pages_data);
    if (data == NULL) {
        return NULL;
    }
    unsigned char *head_body = body;
    unsigned char *head_data = data;
    // First, the trampoline:
    const StencilGroup *stencil_group = &trampoline_stencil_group;
    uint64_t patches[] = GET_PATCHES();
    patches[_JIT_BODY] = (uintptr_t)head_body;
    patches[_JIT_DATA] = (uintptr_t)head_data;
    patches[_JIT_CONTINUE] = (uintptr_t)head_body + stencil_group->body.nbytes;
    patches[_JIT_ZERO] = 0;
    emit(stencil_group, patches);
    head_body += stencil_group->body.nbytes;
    head_data += stencil_group->data.nbytes;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        const StencilGroup *stencil_group = &stencil_groups[instruction->opcode];
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)head_body;
        patches[_JIT_DATA] = (uintptr_t)head_data;
        patches[_JIT_CONTINUE] = (uintptr_t)head_body + stencil_group->body.nbytes;
        patches[_JIT_CURRENT_EXECUTOR] = (uintptr_t)executor;
        patches[_JIT_DEOPTIMIZE] = (uintptr_t)deoptimize_stub;
        patches[_JIT_ERROR] = (uintptr_t)error_stub;
        patches[_JIT_OPARG] = instruction->oparg;
        patches[_JIT_OPERAND] = instruction->operand;
        patches[_JIT_TARGET] = instruction->target;
        patches[_JIT_TOP] = (uintptr_t)body + trampoline_stencil_group.body.nbytes;
        patches[_JIT_ZERO] = 0;
        emit(stencil_group, patches);
        head_body += stencil_group->body.nbytes;
        head_data += stencil_group->data.nbytes;
    };
    if (mark_executable(body, pages_body)) {
        return NULL;
    }
    // Wow, done already?
    assert(head_body == body + nbytes_body);
    assert(head_data == data + nbytes_data);
    return (_PyJITFunction)body;
}
