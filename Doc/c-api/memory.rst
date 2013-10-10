.. highlightlang:: c


.. _memory:

*****************
Memory Management
*****************

.. sectionauthor:: Vladimir Marangozov <Vladimir.Marangozov@inrialpes.fr>



.. _memoryoverview:

Overview
========

Memory management in Python involves a private heap containing all Python
objects and data structures. The management of this private heap is ensured
internally by the *Python memory manager*.  The Python memory manager has
different components which deal with various dynamic storage management aspects,
like sharing, segmentation, preallocation or caching.

At the lowest level, a raw memory allocator ensures that there is enough room in
the private heap for storing all Python-related data by interacting with the
memory manager of the operating system. On top of the raw memory allocator,
several object-specific allocators operate on the same heap and implement
distinct memory management policies adapted to the peculiarities of every object
type. For example, integer objects are managed differently within the heap than
strings, tuples or dictionaries because integers imply different storage
requirements and speed/space tradeoffs. The Python memory manager thus delegates
some of the work to the object-specific allocators, but ensures that the latter
operate within the bounds of the private heap.

It is important to understand that the management of the Python heap is
performed by the interpreter itself and that the user has no control over it,
even if she regularly manipulates object pointers to memory blocks inside that
heap.  The allocation of heap space for Python objects and other internal
buffers is performed on demand by the Python memory manager through the Python/C
API functions listed in this document.

.. index::
   single: malloc()
   single: calloc()
   single: realloc()
   single: free()

To avoid memory corruption, extension writers should never try to operate on
Python objects with the functions exported by the C library: :c:func:`malloc`,
:c:func:`calloc`, :c:func:`realloc` and :c:func:`free`.  This will result in  mixed
calls between the C allocator and the Python memory manager with fatal
consequences, because they implement different algorithms and operate on
different heaps.  However, one may safely allocate and release memory blocks
with the C library allocator for individual purposes, as shown in the following
example::

   PyObject *res;
   char *buf = (char *) malloc(BUFSIZ); /* for I/O */

   if (buf == NULL)
       return PyErr_NoMemory();
   ...Do some I/O operation involving buf...
   res = PyBytes_FromString(buf);
   free(buf); /* malloc'ed */
   return res;

In this example, the memory request for the I/O buffer is handled by the C
library allocator. The Python memory manager is involved only in the allocation
of the string object returned as a result.

In most situations, however, it is recommended to allocate memory from the
Python heap specifically because the latter is under control of the Python
memory manager. For example, this is required when the interpreter is extended
with new object types written in C. Another reason for using the Python heap is
the desire to *inform* the Python memory manager about the memory needs of the
extension module. Even when the requested memory is used exclusively for
internal, highly-specific purposes, delegating all memory requests to the Python
memory manager causes the interpreter to have a more accurate image of its
memory footprint as a whole. Consequently, under certain circumstances, the
Python memory manager may or may not trigger appropriate actions, like garbage
collection, memory compaction or other preventive procedures. Note that by using
the C library allocator as shown in the previous example, the allocated memory
for the I/O buffer escapes completely the Python memory manager.


Raw Memory Interface
====================

The following function sets are wrappers to the system allocator. These
functions are thread-safe, the :term:`GIL <global interpreter lock>` does not
need to be held.

The default raw memory block allocator uses the following functions:
:c:func:`malloc`, :c:func:`realloc` and :c:func:`free`; call ``malloc(1)`` when
requesting zero bytes.

.. versionadded:: 3.4

.. c:function:: void* PyMem_RawMalloc(size_t n)

   Allocates *n* bytes and returns a pointer of type :c:type:`void\*` to the
   allocated memory, or *NULL* if the request fails. Requesting zero bytes
   returns a distinct non-*NULL* pointer if possible, as if
   ``PyMem_RawMalloc(1)`` had been called instead. The memory will not have
   been initialized in any way.


.. c:function:: void* PyMem_RawRealloc(void *p, size_t n)

   Resizes the memory block pointed to by *p* to *n* bytes. The contents will
   be unchanged to the minimum of the old and the new sizes. If *p* is *NULL*,
   the call is equivalent to ``PyMem_RawMalloc(n)``; else if *n* is equal to
   zero, the memory block is resized but is not freed, and the returned pointer
   is non-*NULL*. Unless *p* is *NULL*, it must have been returned by a
   previous call to :c:func:`PyMem_RawMalloc` or :c:func:`PyMem_RawRealloc`. If
   the request fails, :c:func:`PyMem_RawRealloc` returns *NULL* and *p* remains
   a valid pointer to the previous memory area.


.. c:function:: void PyMem_RawFree(void *p)

   Frees the memory block pointed to by *p*, which must have been returned by a
   previous call to :c:func:`PyMem_RawMalloc` or :c:func:`PyMem_RawRealloc`.
   Otherwise, or if ``PyMem_Free(p)`` has been called before, undefined
   behavior occurs. If *p* is *NULL*, no operation is performed.


.. _memoryinterface:

Memory Interface
================

The following function sets, modeled after the ANSI C standard, but specifying
behavior when requesting zero bytes, are available for allocating and releasing
memory from the Python heap.

The default memory block allocator uses the following functions:
:c:func:`malloc`, :c:func:`realloc` and :c:func:`free`; call ``malloc(1)`` when
requesting zero bytes.

.. warning::

   The :term:`GIL <global interpreter lock>` must be held when using these
   functions.

.. c:function:: void* PyMem_Malloc(size_t n)

   Allocates *n* bytes and returns a pointer of type :c:type:`void\*` to the
   allocated memory, or *NULL* if the request fails. Requesting zero bytes returns
   a distinct non-*NULL* pointer if possible, as if ``PyMem_Malloc(1)`` had
   been called instead. The memory will not have been initialized in any way.


.. c:function:: void* PyMem_Realloc(void *p, size_t n)

   Resizes the memory block pointed to by *p* to *n* bytes. The contents will be
   unchanged to the minimum of the old and the new sizes. If *p* is *NULL*, the
   call is equivalent to ``PyMem_Malloc(n)``; else if *n* is equal to zero,
   the memory block is resized but is not freed, and the returned pointer is
   non-*NULL*.  Unless *p* is *NULL*, it must have been returned by a previous call
   to :c:func:`PyMem_Malloc` or :c:func:`PyMem_Realloc`. If the request fails,
   :c:func:`PyMem_Realloc` returns *NULL* and *p* remains a valid pointer to the
   previous memory area.


.. c:function:: void PyMem_Free(void *p)

   Frees the memory block pointed to by *p*, which must have been returned by a
   previous call to :c:func:`PyMem_Malloc` or :c:func:`PyMem_Realloc`.  Otherwise, or
   if ``PyMem_Free(p)`` has been called before, undefined behavior occurs. If
   *p* is *NULL*, no operation is performed.

The following type-oriented macros are provided for convenience.  Note  that
*TYPE* refers to any C type.


.. c:function:: TYPE* PyMem_New(TYPE, size_t n)

   Same as :c:func:`PyMem_Malloc`, but allocates ``(n * sizeof(TYPE))`` bytes of
   memory.  Returns a pointer cast to :c:type:`TYPE\*`.  The memory will not have
   been initialized in any way.


.. c:function:: TYPE* PyMem_Resize(void *p, TYPE, size_t n)

   Same as :c:func:`PyMem_Realloc`, but the memory block is resized to ``(n *
   sizeof(TYPE))`` bytes.  Returns a pointer cast to :c:type:`TYPE\*`. On return,
   *p* will be a pointer to the new memory area, or *NULL* in the event of
   failure.  This is a C preprocessor macro; p is always reassigned.  Save
   the original value of p to avoid losing memory when handling errors.


.. c:function:: void PyMem_Del(void *p)

   Same as :c:func:`PyMem_Free`.

In addition, the following macro sets are provided for calling the Python memory
allocator directly, without involving the C API functions listed above. However,
note that their use does not preserve binary compatibility across Python
versions and is therefore deprecated in extension modules.

:c:func:`PyMem_MALLOC`, :c:func:`PyMem_REALLOC`, :c:func:`PyMem_FREE`.

:c:func:`PyMem_NEW`, :c:func:`PyMem_RESIZE`, :c:func:`PyMem_DEL`.


Customize Memory Allocators
===========================

.. versionadded:: 3.4

.. c:type:: PyMemAllocator

   Structure used to describe a memory block allocator. The structure has
   four fields:

   +----------------------------------------------------------+---------------------------------------+
   | Field                                                    | Meaning                               |
   +==========================================================+=======================================+
   | ``void *ctx``                                            | user context passed as first argument |
   +----------------------------------------------------------+---------------------------------------+
   | ``void* malloc(void *ctx, size_t size)``                 | allocate a memory block               |
   +----------------------------------------------------------+---------------------------------------+
   | ``void* realloc(void *ctx, void *ptr, size_t new_size)`` | allocate or resize a memory block     |
   +----------------------------------------------------------+---------------------------------------+
   | ``void free(void *ctx, void *ptr)``                      | free a memory block                   |
   +----------------------------------------------------------+---------------------------------------+

.. c:type:: PyMemAllocatorDomain

   Enum used to identify an allocator domain. Domains:

   * :c:data:`PYMEM_DOMAIN_RAW`: functions :c:func:`PyMem_RawMalloc`,
     :c:func:`PyMem_RawRealloc` and :c:func:`PyMem_RawFree`
   * :c:data:`PYMEM_DOMAIN_MEM`: functions :c:func:`PyMem_Malloc`,
     :c:func:`PyMem_Realloc` and :c:func:`PyMem_Free`
   * :c:data:`PYMEM_DOMAIN_OBJ`: functions :c:func:`PyObject_Malloc`,
     :c:func:`PyObject_Realloc` and :c:func:`PyObject_Free`


.. c:function:: void PyMem_GetAllocator(PyMemAllocatorDomain domain, PyMemAllocator *allocator)

   Get the memory block allocator of the specified domain.


.. c:function:: void PyMem_SetAllocator(PyMemAllocatorDomain domain, PyMemAllocator *allocator)

   Set the memory block allocator of the specified domain.

   The new allocator must return a distinct non-NULL pointer when requesting
   zero bytes.

   For the :c:data:`PYMEM_DOMAIN_RAW` domain, the allocator must be
   thread-safe: the :term:`GIL <global interpreter lock>` is not held when the
   allocator is called.

   If the new allocator is not a hook (does not call the previous allocator),
   the :c:func:`PyMem_SetupDebugHooks` function must be called to reinstall the
   debug hooks on top on the new allocator.


.. c:function:: void PyMem_SetupDebugHooks(void)

   Setup hooks to detect bugs in the following Python memory allocator
   functions:

   - :c:func:`PyMem_RawMalloc`, :c:func:`PyMem_RawRealloc`,
     :c:func:`PyMem_RawFree`
   - :c:func:`PyMem_Malloc`, :c:func:`PyMem_Realloc`, :c:func:`PyMem_Free`
   - :c:func:`PyObject_Malloc`, :c:func:`PyObject_Realloc`,
     :c:func:`PyObject_Free`

   Newly allocated memory is filled with the byte ``0xCB``, freed memory is
   filled with the byte ``0xDB``. Additionnal checks:

   - detect API violations, ex: :c:func:`PyObject_Free` called on a buffer
     allocated by :c:func:`PyMem_Malloc`
   - detect write before the start of the buffer (buffer underflow)
   - detect write after the end of the buffer (buffer overflow)

   The function does nothing if Python is not compiled is debug mode.


Customize PyObject Arena Allocator
==================================

Python has a *pymalloc* allocator for allocations smaller than 512 bytes. This
allocator is optimized for small objects with a short lifetime. It uses memory
mappings called "arenas" with a fixed size of 256 KB. It falls back to
:c:func:`PyMem_RawMalloc` and :c:func:`PyMem_RawRealloc` for allocations larger
than 512 bytes.  *pymalloc* is the default allocator used by
:c:func:`PyObject_Malloc`.

The default arena allocator uses the following functions:

* :c:func:`VirtualAlloc` and :c:func:`VirtualFree` on Windows,
* :c:func:`mmap` and :c:func:`munmap` if available,
* :c:func:`malloc` and :c:func:`free` otherwise.

.. versionadded:: 3.4

.. c:type:: PyObjectArenaAllocator

   Structure used to describe an arena allocator. The structure has
   three fields:

   +--------------------------------------------------+---------------------------------------+
   | Field                                            | Meaning                               |
   +==================================================+=======================================+
   | ``void *ctx``                                    | user context passed as first argument |
   +--------------------------------------------------+---------------------------------------+
   | ``void* alloc(void *ctx, size_t size)``          | allocate an arena of size bytes       |
   +--------------------------------------------------+---------------------------------------+
   | ``void free(void *ctx, size_t size, void *ptr)`` | free an arena                         |
   +--------------------------------------------------+---------------------------------------+

.. c:function:: PyObject_GetArenaAllocator(PyObjectArenaAllocator *allocator)

   Get the arena allocator.

.. c:function:: PyObject_SetArenaAllocator(PyObjectArenaAllocator *allocator)

   Set the arena allocator.


.. _memoryexamples:

Examples
========

Here is the example from section :ref:`memoryoverview`, rewritten so that the
I/O buffer is allocated from the Python heap by using the first function set::

   PyObject *res;
   char *buf = (char *) PyMem_Malloc(BUFSIZ); /* for I/O */

   if (buf == NULL)
       return PyErr_NoMemory();
   /* ...Do some I/O operation involving buf... */
   res = PyBytes_FromString(buf);
   PyMem_Free(buf); /* allocated with PyMem_Malloc */
   return res;

The same code using the type-oriented function set::

   PyObject *res;
   char *buf = PyMem_New(char, BUFSIZ); /* for I/O */

   if (buf == NULL)
       return PyErr_NoMemory();
   /* ...Do some I/O operation involving buf... */
   res = PyBytes_FromString(buf);
   PyMem_Del(buf); /* allocated with PyMem_New */
   return res;

Note that in the two examples above, the buffer is always manipulated via
functions belonging to the same set. Indeed, it is required to use the same
memory API family for a given memory block, so that the risk of mixing different
allocators is reduced to a minimum. The following code sequence contains two
errors, one of which is labeled as *fatal* because it mixes two different
allocators operating on different heaps. ::

   char *buf1 = PyMem_New(char, BUFSIZ);
   char *buf2 = (char *) malloc(BUFSIZ);
   char *buf3 = (char *) PyMem_Malloc(BUFSIZ);
   ...
   PyMem_Del(buf3);  /* Wrong -- should be PyMem_Free() */
   free(buf2);       /* Right -- allocated via malloc() */
   free(buf1);       /* Fatal -- should be PyMem_Del()  */

In addition to the functions aimed at handling raw memory blocks from the Python
heap, objects in Python are allocated and released with :c:func:`PyObject_New`,
:c:func:`PyObject_NewVar` and :c:func:`PyObject_Del`.

These will be explained in the next chapter on defining and implementing new
object types in C.

