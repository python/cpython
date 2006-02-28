#include "Python.h"
#include "pyarena.h"

/* A simple arena block structure */
/* TODO(jhylton): Measurement to justify block size. */

#define DEFAULT_BLOCK_SIZE 8192
typedef struct _block {
	size_t ab_size;
	size_t ab_offset;
	struct _block *ab_next;
	void *ab_mem;
} block;

/* The arena manages two kinds of memory, blocks of raw memory
   and a list of PyObject* pointers.  PyObjects are decrefed
   when the arena is freed.
*/
   
struct _arena {
	block *a_head;
	block *a_cur;
        PyObject *a_objects;
};

static block *
block_new(size_t size)
{
	/* Allocate header and block as one unit. 
	   ab_mem points just past header. */
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
		/* If we need to allocate more memory than will fit in
		   the default block, allocate a one-off block that is
		   exactly the right size. */
		/* TODO(jhylton): Think about space waste at end of block */
		block *new = block_new(
				size < DEFAULT_BLOCK_SIZE ?
				DEFAULT_BLOCK_SIZE : size);
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
        if (!arena->a_head) {
                free((void *)arena);
                return NULL;
        }
        arena->a_objects = PyList_New(16);
        if (!arena->a_objects) {
                block_free(arena->a_head);
                free((void *)arena);
                return NULL;
        }
	return arena;
}

void
PyArena_Free(PyArena *arena)
{
	assert(arena);
	block_free(arena->a_head);
        assert(arena->a_objects->ob_refcnt == 1);
        Py_DECREF(arena->a_objects);
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
PyArena_AddPyObject(PyArena *arena, PyObject *obj) 
{
        return PyList_Append(arena->a_objects, obj) >= 0;
}
