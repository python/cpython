#include "Python.h"
#include "pycore_abstract.h"
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

static unsigned char *
alloc(size_t size)
{
    assert((size & 7) == 0);
    if (JIT_POOL_SIZE - page_size < pool_head + size) {
        PyErr_WarnEx(PyExc_RuntimeWarning, "JIT out of memory", 0);
        return NULL;
    }
    unsigned char *memory = pool + pool_head;
    pool_head += size;
    assert(((uintptr_t)(pool + pool_head) & 7) == 0);
    assert(((uintptr_t)memory & 7) == 0);
    return memory;
}

static int
mark_writeable(unsigned char *memory, size_t nbytes)
{
    unsigned char *page = (unsigned char *)((uintptr_t)memory & ~(page_size - 1));
    size_t page_nbytes = memory + nbytes - page;
#ifdef MS_WINDOWS
    DWORD old;
    if (!VirtualProtect(page, page_nbytes, PAGE_READWRITE, &old)) {
        int code = GetLastError();
#else
    if (mprotect(page, page_nbytes, PROT_READ | PROT_WRITE)) {
        int code = errno;
#endif
        const char *w = "JIT unable to map writable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, code);
        return -1;
    }
    return 0;
}

static int
mark_executable(unsigned char *memory, size_t nbytes)
{
    unsigned char *page = (unsigned char *)((uintptr_t)memory & ~(page_size - 1));
    size_t page_nbytes = memory + nbytes - page;
#ifdef MS_WINDOWS
    DWORD old;
    if (!FlushInstructionCache(GetCurrentProcess(), memory, nbytes) ||
        !VirtualProtect(page, page_nbytes, PAGE_EXECUTE_READ, &old))
    {
        int code = GetLastError();
#else
    __builtin___clear_cache((char *)memory, (char *)memory + nbytes);
    if (mprotect(page, page_nbytes, PROT_EXEC | PROT_READ)) {
        int code = errno;
#endif
        const char *w = "JIT unable to map executable memory (%d)";
        PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, code);
        return -1;
    }
    return 0;
}

static void
patch_one(unsigned char *location, HoleKind kind, uint64_t patch)
{
    uint32_t *addr = (uint32_t *)location;
    switch (kind) {
        case R_386_32: {
            *addr = (uint32_t)patch;
            return;
        }
        case R_386_PC32:
        case R_X86_64_GOTPC32:
        case R_X86_64_GOTPCRELX:
        case R_X86_64_PC32:
        case R_X86_64_PLT32:
        case R_X86_64_REX_GOTPCRELX: {
            patch -= (uintptr_t)location;
            *addr = (uint32_t)patch;
            return;
        }
        case R_AARCH64_ABS64:
        case R_X86_64_64: {
            *(uint64_t *)addr = patch;
            return;
        }
        case R_AARCH64_ADR_GOT_PAGE: {
            patch = ((patch >> 12) << 12) - (((uintptr_t)location >> 12) << 12);
            assert((*addr & 0x9F000000) == 0x90000000);
            assert((patch & 0xFFF) == 0);
            uint32_t lo = (patch << 17) & 0x60000000;
            uint32_t hi = (patch >> 9) & 0x00FFFFE0;
            *addr = (*addr & 0x9F00001F) | hi | lo;
            return;
        }
        case R_AARCH64_CALL26:
        case R_AARCH64_JUMP26: {
            patch -= (uintptr_t)location;
            assert(((*addr & 0xFC000000) == 0x14000000) ||
                   ((*addr & 0xFC000000) == 0x94000000));
            assert((patch & 0x3) == 0);
            *addr = (*addr & 0xFC000000) | ((uint32_t)(patch >> 2) & 0x03FFFFFF);
            return;
        }
        case R_AARCH64_LD64_GOT_LO12_NC: {
            patch &= (1 << 12) - 1;
            assert(((*addr & 0x3B000000) == 0x39000000) ||
                   ((*addr & 0x11C00000) == 0x11000000));
            int shift = 0;
            if ((*addr & 0x3B000000) == 0x39000000) {
                shift = ((*addr >> 30) & 0x3);
                if (shift == 0 && (*addr & 0x04800000) == 0x04800000) {
                    shift = 4;
                }
            }
            assert(((patch & ((1 << shift) - 1)) == 0));
            *addr = (*addr & 0xFFC003FF) | ((uint32_t)((patch >> shift) << 10) & 0x003FFC00);
            return;
        }
        case R_AARCH64_MOVW_UABS_G0_NC: {
            assert(((*addr >> 21) & 0x3) == 0);
            *addr = (*addr & 0xFFE0001F) | (((patch >>  0) & 0xFFFF) << 5);
            return;
        }
        case R_AARCH64_MOVW_UABS_G1_NC: {
            assert(((*addr >> 21) & 0x3) == 1);
            *addr = (*addr & 0xFFE0001F) | (((patch >> 16) & 0xFFFF) << 5);
            return;
        }
        case R_AARCH64_MOVW_UABS_G2_NC: {
            assert(((*addr >> 21) & 0x3) == 2);
            *addr = (*addr & 0xFFE0001F) | (((patch >> 32) & 0xFFFF) << 5);
            return;
        }
        case R_AARCH64_MOVW_UABS_G3: {
            assert(((*addr >> 21) & 0x3) == 3);
            *addr = (*addr & 0xFFE0001F) | (((patch >> 48) & 0xFFFF) << 5);
            return;
        }
        case R_X86_64_GOTOFF64: {
            patch -= (uintptr_t)location;
            *(uint64_t *)addr = patch;
            return;
        }
    }
    Py_UNREACHABLE();
}

static void
copy_and_patch(unsigned char *exec, unsigned char *read, const Stencil *stencil, uint64_t patches[])
{
    memcpy(exec, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        uint64_t patch = patches[hole->value] + hole->addend;
        patch_one(exec + hole->offset, hole->kind, patch);
    }
    memcpy(read, stencil->bytes_data, stencil->nbytes_data);
    for (size_t i = 0; i < stencil->nholes_data; i++) {
        const Hole *hole = &stencil->holes_data[i];
        uint64_t patch = patches[hole->value] + hole->addend;
        patch_one(read + hole->offset, hole->kind, patch);
    }
}

static int needs_initializing = 1;
unsigned char *deoptimize_stub;
unsigned char *error_stub;

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
    assert(((uintptr_t)(pool + pool_head) & (page_size - 1)) == 0);
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
        const Stencil *stencil = &deoptimize_stencil;
        deoptimize_stub = alloc(stencil->nbytes);
        if (deoptimize_stub == NULL ||
            mark_writeable(deoptimize_stub, stencil->nbytes))
        {
            return needs_initializing;
        }
        unsigned char *data = alloc(stencil->nbytes_data);
        if (data == NULL ||
            mark_writeable(data, stencil->nbytes_data))
        {
            return needs_initializing;
        }
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)deoptimize_stub;
        patches[_JIT_DATA] = (uintptr_t)data;
        patches[_JIT_ZERO] = 0;
        copy_and_patch(deoptimize_stub, data, stencil, patches);
        if (mark_executable(deoptimize_stub, stencil->nbytes)) {
            return needs_initializing;
        }
        if (mark_executable(data, stencil->nbytes_data)) {
            return needs_initializing;
        }
    }
    // Write our error stub:
    {
        const Stencil *stencil = &error_stencil;
        error_stub = alloc(stencil->nbytes +  stencil->nbytes_data);
        if (error_stub == NULL || mark_writeable(error_stub, stencil->nbytes)) {
            return needs_initializing;
        }
        unsigned char *data = alloc(stencil->nbytes_data);
        if (data == NULL || mark_writeable(data, stencil->nbytes_data)) {
            return needs_initializing;
        }
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)error_stub;
        patches[_JIT_DATA] = (uintptr_t)data;
        patches[_JIT_ZERO] = 0;
        copy_and_patch(error_stub, data, stencil, patches);
        if (mark_executable(error_stub, stencil->nbytes)) {
            return needs_initializing;
        }
        if (mark_executable(data, stencil->nbytes_data)) {
            return needs_initializing;
        }
    }
    // Done:
    needs_initializing = 0;
    return needs_initializing;
}

// The world's smallest compiler?
_PyJITFunction
_PyJIT_CompileTrace(_PyUOpInstruction *trace, int size)
{
    if (initialize_jit()) {
        return NULL;
    }
    size_t *offsets = PyMem_Malloc(size * sizeof(size_t));
    if (offsets == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    size_t nbytes_data = trampoline_stencil.nbytes_data;
    for (int i = 0; i < size; i++) {
        offsets[i] = nbytes;
        _PyUOpInstruction *instruction = &trace[i];
        const Stencil *stencil = &stencils[instruction->opcode];
        if (OPCODE_HAS_ARG(instruction->opcode)) {
            nbytes += oparg_stencil.nbytes;
            nbytes_data += oparg_stencil.nbytes_data;
        }
        if (OPCODE_HAS_OPERAND(instruction->opcode)) {
            nbytes += operand_stencil.nbytes;
            nbytes_data += operand_stencil.nbytes_data;
        }
        nbytes += stencil->nbytes;
        nbytes_data += stencil->nbytes_data;
        assert(stencil->nbytes);
    };
    unsigned char *memory = alloc(nbytes);
    if (memory == NULL || mark_writeable(memory, nbytes)) {
        PyMem_Free(offsets);
        return NULL;
    }
    unsigned char *data = alloc(nbytes_data);
    if (data == NULL || mark_writeable(data, nbytes_data)) {
        PyMem_Free(offsets);
        return NULL;
    }
    unsigned char *head = memory;
    unsigned char *head_data = data;
    // First, the trampoline:
    const Stencil *stencil = &trampoline_stencil;
    uint64_t patches[] = GET_PATCHES();
    patches[_JIT_BODY] = (uintptr_t)head;
    patches[_JIT_DATA] = (uintptr_t)head_data;
    patches[_JIT_CONTINUE] = (uintptr_t)head + stencil->nbytes;
    patches[_JIT_ZERO] = 0;
    copy_and_patch(head, head_data, stencil, patches);
    head += stencil->nbytes;
    head_data += stencil->nbytes_data;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        // You may be wondering why we pass the next oparg and operand through
        // the call (instead of just looking them up in the next instruction if
        // needed). This is because these values are being encoded in the
        // *addresses* of externs. Unfortunately, clang is incredibly clever:
        // the ELF ABI actually has some limits on the valid address range of an
        // extern (it must be in range(1, 2**31 - 2**24)). So, if we load them
        // in the same compilation unit as they are being used, clang *will*
        // optimize the function as if the oparg can never be zero and the
        // operand always fits in 32 bits, for example. That's obviously bad.
        if (OPCODE_HAS_ARG(instruction->opcode)) {
            const Stencil *stencil = &oparg_stencil;
            uint64_t patches[] = GET_PATCHES();
            patches[_JIT_BODY] = (uintptr_t)head;
            patches[_JIT_DATA] = (uintptr_t)head_data;
            patches[_JIT_CONTINUE] = (uintptr_t)head + stencil->nbytes;
            patches[_JIT_OPARG] = instruction->oparg;
            patches[_JIT_ZERO] = 0;
            copy_and_patch(head, head_data, stencil, patches);
            head += stencil->nbytes;
            head_data += stencil->nbytes_data;
        }
        if (OPCODE_HAS_OPERAND(instruction->opcode)) {
            const Stencil *stencil = &operand_stencil;
            uint64_t patches[] = GET_PATCHES();
            patches[_JIT_BODY] = (uintptr_t)head;
            patches[_JIT_DATA] = (uintptr_t)head_data;
            patches[_JIT_CONTINUE] = (uintptr_t)head + stencil->nbytes;
            patches[_JIT_OPERAND] = instruction->operand;
            patches[_JIT_ZERO] = 0;
            copy_and_patch(head, head_data, stencil, patches);
            head += stencil->nbytes;
            head_data += stencil->nbytes_data;
        }
        const Stencil *stencil = &stencils[instruction->opcode];
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BODY] = (uintptr_t)head;
        patches[_JIT_DATA] = (uintptr_t)head_data;
        patches[_JIT_CONTINUE] = (uintptr_t)head + stencil->nbytes;
        patches[_JIT_DEOPTIMIZE] = (uintptr_t)deoptimize_stub;
        patches[_JIT_ERROR] = (uintptr_t)error_stub;
        patches[_JIT_JUMP] = (uintptr_t)memory + offsets[instruction->oparg % size];
        patches[_JIT_ZERO] = 0;
        copy_and_patch(head, head_data, stencil, patches);
        head += stencil->nbytes;
        head_data += stencil->nbytes_data;
    };
    PyMem_Free(offsets);
    if (mark_executable(memory, nbytes)) {
        return NULL;
    }
    if (mark_executable(data, nbytes_data)) {
        return needs_initializing;
    }
    // Wow, done already?
    assert(memory + nbytes == head);
    assert(data + nbytes_data == head_data);
    return (_PyJITFunction)memory;
}
