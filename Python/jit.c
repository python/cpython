#include "Python.h"
#include "pycore_abstract.h"
#include "pycore_dict.h"
#include "pycore_floatobject.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode.h"
#include "pycore_pyerrors.h"
#include "pycore_sliceobject.h"

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
_PyJIT_Free(unsigned char *memory)
{
    memory -= sizeof(size_t);
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
        // assert(*addr == 0);
        *addr += hole->addend + patches[hole->kind];
    }
    return memory + stencil->nbytes;
}


// The world's smallest compiler?
// Make sure to call _PyJIT_Free on the memory when you're done with it!
unsigned char *
_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace)
{
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
        PyErr_NoMemory();
        return NULL;
    }
    unsigned char *head = memory;
    uintptr_t patches[] = GET_PATCHES();
    // First, the trampoline:
    const Stencil *stencil = &trampoline_stencil;
    patches[HOLE_base] = (uintptr_t)head;
    patches[HOLE_continue] = (uintptr_t)head + stencil->nbytes;
    head = copy_and_patch(head, stencil, patches);
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        patches[HOLE_base] = (uintptr_t)head;
        patches[HOLE_continue] = (i != size - 1) 
                               ? (uintptr_t)head + stencil->nbytes
                               : (uintptr_t)memory + trampoline_stencil.nbytes;
        patches[HOLE_next_instr] = (uintptr_t)trace[i];
        patches[HOLE_next_trace] = (uintptr_t)trace[(i + 1) % size];
        patches[HOLE_oparg] = instruction->op.arg;
        head = copy_and_patch(head, stencil, patches);
    };
    // Wow, done already?
    assert(memory + nbytes == head);
    return memory;
}
