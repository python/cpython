#ifndef Py_INTERNAL_OBJECT_STACK_H
#define Py_INTERNAL_OBJECT_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// _PyObjectStack is a stack of Python objects implemented as a linked list of
// fixed size buffers.

// Chosen so that _PyObjectStackChunk is a power-of-two size.
#define _Py_OBJECT_STACK_CHUNK_SIZE 254

typedef struct _PyObjectStackChunk {
    struct _PyObjectStackChunk *prev;
    Py_ssize_t n;
    PyObject *objs[_Py_OBJECT_STACK_CHUNK_SIZE];
} _PyObjectStackChunk;

typedef struct _PyObjectStack {
    _PyObjectStackChunk *head;
} _PyObjectStack;


extern _PyObjectStackChunk *
_PyObjectStackChunk_New(void);

extern void
_PyObjectStackChunk_Free(_PyObjectStackChunk *);

// Push an item onto the stack. Return -1 on allocation failure, 0 on success.
static inline int
_PyObjectStack_Push(_PyObjectStack *stack, PyObject *obj)
{
    _PyObjectStackChunk *buf = stack->head;
    if (buf == NULL || buf->n == _Py_OBJECT_STACK_CHUNK_SIZE) {
        buf = _PyObjectStackChunk_New();
        if (buf == NULL) {
            return -1;
        }
        buf->prev = stack->head;
        buf->n = 0;
        stack->head = buf;
    }

    assert(buf->n >= 0 && buf->n < _Py_OBJECT_STACK_CHUNK_SIZE);
    buf->objs[buf->n] = obj;
    buf->n++;
    return 0;
}

// Pop the top item from the stack.  Return NULL if the stack is empty.
static inline PyObject *
_PyObjectStack_Pop(_PyObjectStack *stack)
{
    _PyObjectStackChunk *buf = stack->head;
    if (buf == NULL) {
        return NULL;
    }
    assert(buf->n > 0 && buf->n <= _Py_OBJECT_STACK_CHUNK_SIZE);
    buf->n--;
    PyObject *obj = buf->objs[buf->n];
    if (buf->n == 0) {
        stack->head = buf->prev;
        _PyObjectStackChunk_Free(buf);
    }
    return obj;
}

static inline Py_ssize_t
_PyObjectStack_Size(_PyObjectStack *stack)
{
    Py_ssize_t size = 0;
    for (_PyObjectStackChunk *buf = stack->head; buf != NULL; buf = buf->prev) {
        size += buf->n;
    }
    return size;
}

// Merge src into dst, leaving src empty
extern void
_PyObjectStack_Merge(_PyObjectStack *dst, _PyObjectStack *src);

// Remove all items from the stack
extern void
_PyObjectStack_Clear(_PyObjectStack *stack);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBJECT_STACK_H
