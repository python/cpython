#include "Python.h"
#include "pyarena.h"

/* An arena list is a linked list that can store either pointers or
   PyObjects.  The type is clear from context.
 */

typedef struct _arena_list {
  struct _arena_list *al_next;
  void *al_pointer;
} PyArenaList;

/* There are two linked lists in an arena, one for malloc pointers and
   one for PyObject.  For each list, there is a pointer to the head
   and to the tail.  The head is used to free the list.  The tail is
   used to add a new element to the list.

   The list always keeps one un-used node at the end of the list.
*/

struct _arena {
  PyArenaList *a_malloc_head;
  PyArenaList *a_malloc_tail;
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
  if (!alist)
    return;

  while (alist) {
    PyArenaList *prev;
    Py_XDECREF((PyObject *)alist->al_pointer);
    alist->al_pointer = NULL;
    prev = alist;
    alist = alist->al_next;
    free(prev);
  }
}

static void
PyArenaList_FreeMalloc(PyArenaList *alist)
{
  if (!alist)
    return;

  while (alist) {
    PyArenaList *prev;
    if (alist->al_pointer) {
      free(alist->al_pointer);
    }
    alist->al_pointer = NULL;
    prev = alist;
    alist = alist->al_next;
    free(prev);
  }
}


PyArena *
PyArena_New()
{
  PyArena* arena = (PyArena *)malloc(sizeof(PyArena));
  if (!arena)
    return NULL;

  arena->a_object_head = PyArenaList_New();
  arena->a_object_tail = arena->a_object_head;
  arena->a_malloc_head = PyArenaList_New();
  arena->a_malloc_tail = arena->a_malloc_head;
  return arena;
}

void
PyArena_Free(PyArena *arena)
{
  assert(arena);
  PyArenaList_FreeObject(arena->a_object_head);
  PyArenaList_FreeMalloc(arena->a_malloc_head);
  free(arena);
}

void *
PyArena_Malloc(PyArena *arena, size_t size) 
{
  /* A better implementation might actually use an arena.  The current
     approach is just a trivial implementation of the API that allows
     it to be tested.
  */
  void *p;
  assert(size != 0);
  p = malloc(size);
  PyArena_AddMallocPointer(arena, p);
  return p;
}

int
PyArena_AddMallocPointer(PyArena *arena, void *pointer) 
{
  assert(pointer);
  PyArenaList *tail = arena->a_malloc_tail;
  assert(tail->al_pointer != pointer);
  tail->al_next = PyArenaList_New();
  tail->al_pointer = pointer;
  arena->a_malloc_tail = tail->al_next;
  return 1;
}

int
PyArena_AddPyObject(PyArena *arena, PyObject *pointer) 
{
  assert(pointer);
  PyArenaList *tail = arena->a_object_tail;
  tail->al_next = PyArenaList_New();
  tail->al_pointer = pointer;
  arena->a_object_tail = tail->al_next;
  return 1;
}
