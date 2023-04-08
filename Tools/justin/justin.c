#include "Python.h"
#include "pycore_abstract.h"
#include "pycore_floatobject.h"
#include "pycore_long.h"
#include "pycore_opcode.h"
#include "pycore_sliceobject.h"
#include "pycore_justin.h"

// #include "justin.h"
#include "generated.h"

#ifdef MS_WINDOWS
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif


static void *
alloc(size_t nbytes)
{
    nbytes += sizeof(size_t);
#ifdef MS_WINDOWS
    void *memory = VirtualAlloc(NULL, nbytes, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (memory == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
#else
    void *memory = mmap(NULL, nbytes, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        PyErr_NoMemory();
        return NULL;
    }
#endif
    assert(memory);
    *(size_t *)memory = nbytes;
    return memory + sizeof(size_t);
}


void
_PyJustin_Free(void *memory)
{
    memory -= sizeof(size_t);
#ifdef MS_WINDOWS
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    size_t nbytes = *(size_t *)memory;
    munmap(memory, nbytes);
#endif
}


static void *
copy_and_patch(void *memory, const Stencil *stencil, uintptr_t patches[])
{
    memcpy(memory, stencil->bytes, stencil->nbytes);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const Hole *hole = &stencil->holes[i];
        uintptr_t *addr = (uintptr_t *)(memory + hole->offset);
        assert(*addr == 0);
        *addr = hole->addend + patches[hole->kind];
    }
    return memory + stencil->nbytes;
}


// The world's smallest compiler?
// Make sure to call _PyJustin_Free on the memory when you're done with it!
void *
_PyJustin_CompileTrace(PyCodeObject *code, int size, int *trace)
{
    _Py_CODEUNIT *first_instruction = _PyCode_CODE(code);
    // First, loop over everything once to find the total compiled size:
    size_t nbytes = trampoline_stencil.nbytes;
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = first_instruction + trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        if (stencil->nbytes == 0) {
            return NULL;
        }
        nbytes += stencil->nbytes;
    };
    void *memory = alloc(nbytes);
    if (memory == NULL) {
        return NULL;
    }
    void *head = memory;
    uintptr_t patches[] = GET_PATCHES();
    // First, the trampoline:
    const Stencil *stencil = &trampoline_stencil;
    patches[HOLE_base] = (uintptr_t)head;
    patches[HOLE_continue] = (uintptr_t)head + stencil->nbytes;
    head = copy_and_patch(head, stencil, patches);
    // Then, all of the stencils:
    for (int i = 0; i < size; i++) {
        _Py_CODEUNIT *instruction = first_instruction + trace[i];
        const Stencil *stencil = &stencils[instruction->op.code];
        patches[HOLE_base] = (uintptr_t)head;
        patches[HOLE_continue] = (i != size - 1) 
                               ? (uintptr_t)head + stencil->nbytes
                               : (uintptr_t)memory + trampoline_stencil.nbytes;
        patches[HOLE_next_instr] = (uintptr_t)(first_instruction + trace[i]);
        patches[HOLE_next_trace] = (uintptr_t)(first_instruction + trace[(i + 1) % size]);
        patches[HOLE_oparg] = instruction->op.arg;
        head = copy_and_patch(head, stencil, patches);
    };
    // Wow, done already?
    assert(memory + nbytes == head);
    return memory;
}
