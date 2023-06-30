#include "Python.h"
#include "pycore_abstract.h"
#include "pycore_ceval.h"
#include "pycore_jit.h"
#include "pycore_opcode.h"

#include "ceval_macros.h"
#include "jit_stencils.h"

#ifdef MS_WINDOWS
    #include <windows.h>
    #define MAP_FAILED NULL
    #define MMAP(SIZE) \
        VirtualAlloc(NULL, (SIZE), MEM_COMMIT, PAGE_EXECUTE_READWRITE)
    #define MUNMAP(MEMORY, SIZE) \
        VirtualFree((MEMORY), 0, MEM_RELEASE)
#else
    #include <sys/mman.h>
    #define MMAP(SIZE)                                         \
        mmap(NULL, (SIZE), PROT_READ | PROT_WRITE | PROT_EXEC, \
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define MUNMAP(MEMORY, SIZE) \
        munmap((MEMORY), (SIZE))
#endif


static unsigned char *
alloc(size_t nbytes)
{
    nbytes += sizeof(size_t);
    unsigned char *memory = MMAP(nbytes);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    assert(memory);
    *(size_t *)memory = nbytes;
    return memory + sizeof(size_t);
}


void
_PyJIT_Free(_PyJITFunction trace)
{
    unsigned char *memory = (unsigned char *)trace - sizeof(size_t);
    size_t nbytes = *(size_t *)memory;
    MUNMAP(memory, nbytes);
}


static unsigned char *
copy_and_patch(unsigned char *memory, const Stencil *stencil, uintptr_t patches[])
{
    memcpy(memory, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        uintptr_t *addr = (uintptr_t *)(memory + hole->offset);
        // XXX: This can't handle 32-bit relocations...
        // XXX: Use += to allow multiple relocations for one offset.
        // XXX: Get rid of pc, and replace it with HOLE_base + addend.
        *addr = patches[hole->kind] + hole->addend + hole->pc * (uintptr_t)addr;
    }
    for (size_t i = 0; i < stencil->nloads; i++) {
        const SymbolLoad *load = &stencil->loads[i];
        uintptr_t *addr = (uintptr_t *)(memory + load->offset);
        uintptr_t value = (uintptr_t)dlsym(RTLD_DEFAULT, load->symbol);
        if (value == 0 && dlerror()) {
            return NULL;
        }
        *addr = value + load->addend + load->pc * (uintptr_t)addr;
    }
    return memory + stencil->nbytes;
}

// The world's smallest compiler?
// Make sure to call _PyJIT_Free on the memory when you're done with it!
_PyJITFunction
_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace)
{
    // // XXX: Testing
    // printf("XXX: Searching for symbols...\n");
    // for (size_t i = 0; i < Py_ARRAY_LENGTH(stencils); i++) {
    //     const Stencil *stencil = &stencils[i];
    //     for (size_t j = 0; j < stencil->nloads; j++) {
    //         const SymbolLoad *load = &stencil->loads[j];
    //         uintptr_t value = (uintptr_t)dlsym(RTLD_DEFAULT, load->symbol);
    //         if (value == 0) {
    //             printf("XXX: - %s (%s)\n", dlerror(), load->symbol);
    //         }
    //     }
    // }
    // printf("XXX: Done searching for symbols!\n");
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        if (stencil->nbytes == 0) {
            return NULL;
        }
        nbytes += stencil->nbytes;
    };
    unsigned char *memory = alloc(nbytes);
    if (memory == NULL) {
        return NULL;
    }
    unsigned char *head = memory;
    uintptr_t patches[] = GET_PATCHES();
    // First, the trampoline:
    const Stencil *stencil = &trampoline_stencil;
    patches[HOLE_base] = (uintptr_t)head;
    patches[HOLE_continue] = (uintptr_t)head + stencil->nbytes;
    head = copy_and_patch(head, stencil, patches);
    if (head == NULL) {
        _PyJIT_Free((_PyJITFunction)memory);
        return NULL;
    }
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        patches[HOLE_base] = (uintptr_t)head;
        patches[HOLE_continue] = (i != size - 1)
                               ? (uintptr_t)head + stencil->nbytes
                               : (uintptr_t)memory + trampoline_stencil.nbytes;
        patches[HOLE_next_instr] = (uintptr_t)instruction;
        patches[HOLE_oparg_plus_one] = instruction->op.arg + 1;
        head = copy_and_patch(head, stencil, patches);
        if (head == NULL) {
            _PyJIT_Free((_PyJITFunction)memory);
            return NULL;
        }
    };
    // Wow, done already?
    assert(memory + nbytes == head);
    return (_PyJITFunction)memory;
}
