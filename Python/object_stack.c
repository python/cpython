// Stack of Python objects

#include "Python.h"
#include "pycore_freelist.h"
#include "pycore_pystate.h"
#include "pycore_object_stack.h"

extern _PyObjectStackChunk *_PyObjectStackChunk_New(void);
extern void _PyObjectStackChunk_Free(_PyObjectStackChunk *);

static struct _Py_object_stack_state *
get_state(void)
{
    _PyFreeListState *state = _PyFreeListState_GET();
    return &state->object_stacks;
}

_PyObjectStackChunk *
_PyObjectStackChunk_New(void)
{
    _PyObjectStackChunk *buf;
    struct _Py_object_stack_state *state = get_state();
    if (state->numfree > 0) {
        buf = state->free_list;
        state->free_list = buf->prev;
        state->numfree--;
    }
    else {
        // NOTE: we use PyMem_RawMalloc() here because this is used by the GC
        // during mimalloc heap traversal. In that context, it is not safe to
        // allocate mimalloc memory, such as via PyMem_Malloc().
        buf = PyMem_RawMalloc(sizeof(_PyObjectStackChunk));
        if (buf == NULL) {
            return NULL;
        }
    }
    buf->prev = NULL;
    buf->n = 0;
    return buf;
}

void
_PyObjectStackChunk_Free(_PyObjectStackChunk *buf)
{
    assert(buf->n == 0);
    struct _Py_object_stack_state *state = get_state();
    if (state->numfree >= 0 &&
        state->numfree < _PyObjectStackChunk_MAXFREELIST)
    {
        buf->prev = state->free_list;
        state->free_list = buf;
        state->numfree++;
    }
    else {
        PyMem_RawFree(buf);
    }
}

void
_PyObjectStack_Clear(_PyObjectStack *queue)
{
    while (queue->head != NULL) {
        _PyObjectStackChunk *buf = queue->head;
        buf->n = 0;
        queue->head = buf->prev;
        _PyObjectStackChunk_Free(buf);
    }
}

void
_PyObjectStackChunk_ClearFreeList(_PyFreeListState *free_lists, int is_finalization)
{
    if (!is_finalization) {
        // Ignore requests to clear the free list during GC. We use object
        // stacks during GC, so emptying the free-list is counterproductive.
        return;
    }

    struct _Py_object_stack_state *state = &free_lists->object_stacks;
    while (state->numfree > 0) {
        _PyObjectStackChunk *buf = state->free_list;
        state->free_list = buf->prev;
        state->numfree--;
        PyMem_RawFree(buf);
    }
    state->numfree = -1;
}
