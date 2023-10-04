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

static unsigned char pool[JIT_POOL_SIZE];
static size_t pool_head;
static size_t page_size;

static unsigned char *
alloc(size_t size)
{
    assert((size & 7) == 0);
    if (JIT_POOL_SIZE - page_size < pool_head + size) {
        return NULL;
    }
    unsigned char *memory = pool + pool_head;
    pool_head += size;
    assert(((uintptr_t)(pool + pool_head) & 7) == 0);
    assert(((uintptr_t)memory & 7) == 0);
    return memory;
}

static int initialized = 0;

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
        case R_X86_64_PC32:
        case R_X86_64_PLT32: {
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
            // assert((patch & ((1ULL << 33) - 1)) == patch);  // XXX: This should be signed.
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
            // assert((patch & ((1ULL << 29) - 1)) == patch);  // XXX: This should be signed.
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
            *addr = patch;
            return;
        }
        case R_X86_64_GOTPC32:
        case R_X86_64_GOTPCRELX:
        case R_X86_64_REX_GOTPCRELX:
        default: {
            Py_UNREACHABLE();
        }
    }
}

static void
copy_and_patch(unsigned char *memory, const Stencil *stencil, uint64_t patches[])
{
    memcpy(memory, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        uint64_t patch = patches[hole->value] + hole->addend;
        patch_one(memory + hole->offset, hole->kind, patch);
    }
}

// The world's smallest compiler?
_PyJITFunction
_PyJIT_CompileTrace(_PyUOpInstruction *trace, int size)
{
    if (initialized < 0) {
        return NULL;
    }
    if (initialized == 0) {
        initialized = -1;
#ifdef MS_WINDOWS
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
#else
        page_size = sysconf(_SC_PAGESIZE);
#endif
        assert(page_size);
        assert((page_size & (page_size - 1)) == 0);
        pool_head = (page_size - ((uintptr_t)pool & (page_size - 1))) & (page_size - 1);
        assert(((uintptr_t)(pool + pool_head) & (page_size - 1)) == 0);
#ifdef __APPLE__
        void *mapped = mmap(pool + pool_head, JIT_POOL_SIZE - pool_head - page_size,
                            PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE, -1, 0);
        if (mapped == MAP_FAILED) {
            const char *w = "JIT unable to map fixed memory (%d)";
            PyErr_WarnFormat(PyExc_RuntimeWarning, 0, w, errno);
            return NULL;
        }
        assert(mapped == pool + pool_head);
#endif
        initialized = 1;
    }
    assert(initialized > 0);
    size_t *offsets = PyMem_Malloc(size * sizeof(size_t));
    if (offsets == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    for (int i = 0; i < size; i++) {
        offsets[i] = nbytes;
        _PyUOpInstruction *instruction = &trace[i];
        const Stencil *stencil = &stencils[instruction->opcode];
        nbytes += stencil->nbytes;
        assert(stencil->nbytes);
    };
    unsigned char *memory = alloc(nbytes);
    if (memory == NULL) {
        PyErr_WarnEx(PyExc_RuntimeWarning, "JIT out of memory", 0);
        PyMem_Free(offsets);
        return NULL;
    }
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
        PyMem_Free(offsets);
        return NULL;
    }
    unsigned char *head = memory;
    // First, the trampoline:
    _PyUOpInstruction *instruction_continue = &trace[0];
    const Stencil *stencil = &trampoline_stencil;
    uint64_t patches[] = GET_PATCHES();
    patches[_JIT_BASE] = (uintptr_t)head;
    patches[_JIT_CONTINUE] = (uintptr_t)head + offsets[0];
    patches[_JIT_CONTINUE_OPARG] = instruction_continue->oparg;
    patches[_JIT_CONTINUE_OPERAND] = instruction_continue->operand;
    patches[_JIT_ZERO] = 0;
    copy_and_patch(head, stencil, patches);
    head += stencil->nbytes;
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _PyUOpInstruction *instruction = &trace[i];
        _PyUOpInstruction *instruction_continue = &trace[(i + 1) % size];
        _PyUOpInstruction *instruction_jump = &trace[instruction->oparg % size];
        const Stencil *stencil = &stencils[instruction->opcode];
        uint64_t patches[] = GET_PATCHES();
        patches[_JIT_BASE] = (uintptr_t)memory + offsets[i];
        patches[_JIT_CONTINUE] = (uintptr_t)memory + offsets[(i + 1) % size];
        patches[_JIT_CONTINUE_OPARG] = instruction_continue->oparg;
        patches[_JIT_CONTINUE_OPERAND] = instruction_continue->operand;
        patches[_JIT_JUMP] = (uintptr_t)memory + offsets[instruction->oparg % size];
        patches[_JIT_JUMP_OPARG] = instruction_jump->oparg;
        patches[_JIT_JUMP_OPERAND] = instruction_jump->operand;
        patches[_JIT_ZERO] = 0;
        copy_and_patch(head, stencil, patches);
        head += stencil->nbytes;
    };
    PyMem_Free(offsets);
#ifdef MS_WINDOWS
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
        return NULL;
    }
    // Wow, done already?
    assert(memory + nbytes == head);
    return (_PyJITFunction)memory;
}