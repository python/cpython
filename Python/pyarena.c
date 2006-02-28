#include "Python.h"
#include "pyarena.h"

/* An arena list is a linked list that can store PyObjects. */

typedef struct _arena_list {
    struct _arena_list *al_next;
    void *al_pointer;
} PyArenaList;

/* A simple arena block structure */
/* TODO(jhylton): Measurement to justify block size. */

#define DEFAULT_BLOCK_SIZE 8192
typedef struct _block {
    size_t ab_size;
    size_t ab_offset;
    struct _block *ab_next;
    void *ab_mem;
} block;

struct _arena {
    block *a_head;
    block *a_cur;
    PyArenaList *a_object_head;
    PyArenaList *a_object_tail;
};

static PyArenaList*
PyArenaList_New(void) 
{
  PyArenaList *alist = (PyArenaList *)malloc(sizeof(PyArenaList));
  if (!alist)
    return NULL;

  alist->al_next = NULL;
  alist->al_pointer = NULL;
  return alist;
}

static void
PyArenaList_FreeObject(PyArenaList *alist) 
{
    while (alist) {
        PyArenaList *prev;
        Py_XDECREF((PyObject *)alist->al_pointer);
        alist->al_pointer = NULL;
        prev = alist;
        alist = alist->al_next;
        free(prev);
    }
}

static block *
block_new(size_t size)
{
    /* Allocate header and block as one unit.  ab_mem points just past header. */
    block *b = (block *)malloc(sizeof(block) + size);
    if (!b)
        return NULL;
    b->ab_size = size;
    b->ab_mem = (void *)(b + 1);
    b->ab_next = NULL;
    b->ab_offset = 0;
    return b;
}

static void
block_free(block *b) {
    while (b) {
        block *next = b->ab_next;
        free(b);
        b = next;
    }
}

static void *
block_alloc(block *b, size_t size)
{
    void *p;
    assert(b);
    if (b->ab_offset + size > b->ab_size) {
        /* If we need to allocate more memory than will fit in the default
           block, allocate a one-off block that is exactly the right size. */
        /* TODO(jhylton): Think more about space waste at end of block */
        block *new = block_new(
            size < DEFAULT_BLOCK_SIZE ? DEFAULT_BLOCK_SIZE : size);
        if (!new)
            return NULL;
        assert(!b->ab_next);
        b->ab_next = new;
        b = new;
    }

    assert(b->ab_offset + size <= b->ab_size);
    p = (void *)(((char *)b->ab_mem) + b->ab_offset);
    b->ab_offset += size;
    return p;
}

PyArena *
PyArena_New()
{
  PyArena* arena = (PyArena *)malloc(sizeof(PyArena));
  if (!arena)
    return NULL;

  arena->a_head = block_new(DEFAULT_BLOCK_SIZE);
  arena->a_cur = arena->a_head;
  arena->a_object_head = PyArenaList_New();
  arena->a_object_tail = arena->a_object_head;
  return arena;
}

void
PyArena_Free(PyArena *arena)
{
    assert(arena);
    block_free(arena->a_head);
    PyArenaList_FreeObject(arena->a_object_head);
    free(arena);
}

void *
PyArena_Malloc(PyArena *arena, size_t size) 
{
    void *p = block_alloc(arena->a_cur, size);
    if (!p)
        return NULL;
    /* Reset cur if we allocated a new block. */
    if (arena->a_cur->ab_next) {
        arena->a_cur = arena->a_cur->ab_next;
    }
    return p;
}

int
PyArena_AddPyObject(PyArena *arena, PyObject *pointer) 
{
  PyArenaList *tail = arena->a_object_tail;
  assert(pointer);
  tail->al_next = PyArenaList_New();
  tail->al_pointer = pointer;
  arena->a_object_tail = tail->al_next;
  return 1;
}
