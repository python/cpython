#define Py_BUILD_CORE

#include "Python.h"
#include "pycore_opcode.h"

#include "stencils.h"
#include "stencils.c"

#ifdef MS_WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif


static unsigned char *
alloc(size_t size)
{
    size += sizeof(size_t);
#ifdef MS_WINDOWS
    unsigned char *memory = VirtualAlloc(NULL, size, MEM_COMMIT,
                                         PAGE_EXECUTE_READWRITE);
    if (memory == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
#else
    unsigned char *memory = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        PyErr_NoMemory();
        return NULL;
    }
#endif
    assert(memory);
    *(size_t *)memory = size;
    return memory + sizeof(size_t);
}


void
_PyJustin_Free(unsigned char *memory)
{
    memory -= sizeof(size_t);
    size_t size = *(size_t *)memory;
#ifdef MS_WINDOWS
    VirtualFree(memory, 0, MEM_RELEASE);
#else
    munmap(memory, size);
#endif
}


static unsigned char *
copy_and_patch(unsigned char *memory, const _PyJustin_Stencil *stencil,
               uintptr_t patches[])
{
    memcpy(memory, stencil->body, stencil->size);
    for (size_t i = 0; i < stencil->nholes; i++) {
        const _PyJustin_Hole *hole = &stencil->holes[i];
        uintptr_t *addr = (uintptr_t *)(memory + hole->offset);
        assert(*addr == 0);
        *addr = hole->addend + patches[hole->kind];
    }
    return memory + stencil->size;
}


#define BAD 0xBAD0BAD0BAD0BAD0

// The world's smallest compiler?
// Make sure to call _PyJustin_Free on the memory when you're done with it!
unsigned char *
_PyJustin_CompileTrace(PyCodeObject *code, int trace_size, int *trace)
{
    _Py_CODEUNIT *first_instruction = _PyCode_CODE(code);
    // First, loop over everything once to find the total compiled size:
    size_t size = _PyJustin_Trampoline.size;
    for (int i = 0; i < trace_size; i++) {
        _Py_CODEUNIT instruction = first_instruction[trace[i]];
        _PyJustin_Stencil stencil = _PyJustin_Stencils[instruction.op.code];
        size += stencil.size;
    };
    unsigned char *memory = alloc(size);
    if (memory == NULL) {
        return NULL;
    }
    // Set up our patches:
    uintptr_t patches[] = {
        [_PyJustin_HOLE_base] = BAD,
        [_PyJustin_HOLE_continue] = BAD,
        [_PyJustin_HOLE_next_instr] = BAD,
        [_PyJustin_HOLE_next_trace_0] = BAD,
        [_PyJustin_HOLE_oparg_0] = BAD,
    #define LOAD(SYMBOL) [_PyJustin_LOAD_##SYMBOL] = (uintptr_t)&(SYMBOL)
        LOAD(_Py_Dealloc),
        LOAD(_Py_DecRefTotal_DO_NOT_USE_THIS),
        LOAD(_Py_NegativeRefcount),
        LOAD(PyThreadState_Get),
    #undef LOAD
    };
    unsigned char *current = memory;
    // First the trampoline:
    const _PyJustin_Stencil *stencil = &_PyJustin_Trampoline;
    patches[_PyJustin_HOLE_base] = (uintptr_t)current;
    patches[_PyJustin_HOLE_continue] = (uintptr_t)current + stencil->size;
    patches[_PyJustin_HOLE_next_instr] = BAD;
    patches[_PyJustin_HOLE_next_trace_0] = BAD;
    patches[_PyJustin_HOLE_oparg_0] = BAD;
    current = copy_and_patch(current, stencil, patches);
    int i;
    // Then, all of the stencils:
    for (i = 0; i < trace_size - 1; i++) {
        _Py_CODEUNIT *instruction = first_instruction + trace[i];
        const _PyJustin_Stencil *stencil = &_PyJustin_Stencils[instruction->op.code];
        patches[_PyJustin_HOLE_base] = (uintptr_t)current;
        patches[_PyJustin_HOLE_continue] = (uintptr_t)current + stencil->size;
        patches[_PyJustin_HOLE_next_instr] = (uintptr_t)(first_instruction + trace[i] + 1);
        patches[_PyJustin_HOLE_next_trace_0] = (uintptr_t)(first_instruction + trace[i + 1]);
        patches[_PyJustin_HOLE_oparg_0] = instruction->op.arg;
        current = copy_and_patch(current, stencil, patches);
    };
    // The last one is a little different (since the trace wraps around):
    _Py_CODEUNIT *instruction = first_instruction + trace[i];
    const _PyJustin_Stencil *stencil = &_PyJustin_Stencils[instruction->op.code];
    patches[_PyJustin_HOLE_base] = (uintptr_t)current;
    patches[_PyJustin_HOLE_continue] = (uintptr_t)memory + _PyJustin_Trampoline.size;  // Different!
    patches[_PyJustin_HOLE_next_instr] = (uintptr_t)(first_instruction + trace[i] + 1);
    patches[_PyJustin_HOLE_next_trace_0] = (uintptr_t)(first_instruction + trace[0]);  // Different!
    patches[_PyJustin_HOLE_oparg_0] = instruction->op.arg;
    current = copy_and_patch(current, stencil, patches);
    // Wow, done already?
    assert(memory + size == current);
    return memory;
}
