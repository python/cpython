// Stack of Python objects

#include "Python.h"
#include "pycore_freelist.h"
#include "pycore_pystate.h"
#include "pycore_object_stack.h"

extern _PyObjectStackChunk *_PyObjectStackChunk_New(void);
extern void _PyObjectStackChunk_Free(_PyObjectStackChunk *);

static struct _Py_object_stack_freelist *
get_object_stack_freelist(void)
{
    struct _Py_object_freelists *freelists = _Py_object_freelists_GET();
    return &freelists->object_stacks;
}

_PyObjectStackChunk *
_PyObjectStackChunk_New(void)
{
    _PyObjectStackChunk *buf;
    struct _Py_object_stack_freelist *obj_stack_freelist = get_object_stack_freelist();
    if (obj_stack_freelist->numfree > 0) {
        buf = obj_stack_freelist->items;
        obj_stack_freelist->items = buf->prev;
        obj_stack_freelist->numfree--;
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
    struct _Py_object_stack_freelist *obj_stack_freelist = get_object_stack_freelist();
    if (obj_stack_freelist->numfree >= 0 &&
        obj_stack_freelist->numfree < _PyObjectStackChunk_MAXFREELIST)
    {
        buf->prev = obj_stack_freelist->items;
        obj_stack_freelist->items = buf;
        obj_stack_freelist->numfree++;
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

void
_PyObjectStackChunk_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization)
{
    if (!is_finalization) {
        // Ignore requests to clear the free list during GC. We use object
        // stacks during GC, so emptying the free-list is counterproductive.
        return;
    }

    struct _Py_object_stack_freelist *freelist = &freelists->object_stacks;
    while (freelist->numfree > 0) {
        _PyObjectStackChunk *buf = freelist->items;
        freelist->items = buf->prev;
        freelist->numfree--;
        PyMem_RawFree(buf);
    }
    freelist->numfree = -1;
}
