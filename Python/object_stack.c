// Stack of Python objects

#include "Python.h"
#include "pycore_freelist.h"
#include "pycore_pystate.h"
#include "pycore_object_stack.h"

extern _PyObjectStackChunk *_PyObjectStackChunk_New(void);
extern void _PyObjectStackChunk_Free(_PyObjectStackChunk *);

_PyObjectStackChunk *
_PyObjectStackChunk_New(void)
{
    _PyObjectStackChunk *buf = _Py_FREELIST_POP_MEM(object_stack_chunks);
    if (buf == NULL) {
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
    _Py_FREELIST_FREE(object_stack_chunks, buf, PyMem_RawFree);
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
_PyObjectStack_Merge(_PyObjectStack *dst, _PyObjectStack *src)
{
    if (src->head == NULL) {
        return;
    }

    if (dst->head != NULL) {
        // First, append dst to the bottom of src
        _PyObjectStackChunk *last = src->head;
        while (last->prev != NULL) {
            last = last->prev;
        }
        last->prev = dst->head;
    }

    // Now that src has all the chunks, set dst to src
    dst->head = src->head;
    src->head = NULL;
}
