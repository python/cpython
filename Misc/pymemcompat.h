/* The idea of this file is that you bundle it with your extension,
   #include it, program to Python 2.3's memory API and have your
   extension build with any version of Python from 1.5.2 through to
   2.3 (and hopefully beyond). */

#ifndef Py_PYMEMCOMPAT_H
#define Py_PYMEMCOMPAT_H

#include "Python.h"

/* There are three "families" of memory API: the "raw memory", "object
   memory" and "object" families.  (This is ignoring the matter of the
   cycle collector, about which more is said below).

   Raw Memory:

       PyMem_Malloc, PyMem_Realloc, PyMem_Free

   Object Memory:

       PyObject_Malloc, PyObject_Realloc, PyObject_Free

   Object:

       PyObject_New, PyObject_NewVar, PyObject_Del

   The raw memory and object memory allocators both mimic the
   malloc/realloc/free interface from ANSI C, but the object memory
   allocator can (and, since 2.3, does by default) use a different
   allocation strategy biased towards lots of lots of "small"
   allocations.

   The object family is used for allocating Python objects, and the
   initializers take care of some basic initialization (setting the
   refcount to 1 and filling out the ob_type field) as well as having
   a somewhat different interface.

   Do not mix the families!  E.g. do not allocate memory with
   PyMem_Malloc and free it with PyObject_Free.  You may get away with
   it quite a lot of the time, but there *are* scenarios where this
   will break.  You Have Been Warned. 

   Also, in many versions of Python there are an insane amount of
   memory interfaces to choose from.  Use the ones described above. */

#if PY_VERSION_HEX < 0x01060000
/* raw memory interface already present */

/* there is no object memory interface in 1.5.2 */
#define PyObject_Malloc		PyMem_Malloc
#define PyObject_Realloc	PyMem_Realloc
#define PyObject_Free		PyMem_Free

/* the object interface is there, but the names have changed */
#define PyObject_New		PyObject_NEW
#define PyObject_NewVar		PyObject_NEW_VAR
#define PyObject_Del		PyMem_Free
#endif

/* If your object is a container you probably want to support the
   cycle collector, which was new in Python 2.0.

   Unfortunately, the interface to the collector that was present in
   Python 2.0 and 2.1 proved to be tricky to use, and so changed in
   2.2 -- in a way that can't easily be papered over with macros.

   This file contains macros that let you program to the 2.2 GC API.
   Your module will compile against any Python since version 1.5.2,
   but the type will only participate in the GC in versions 2.2 and
   up.  Some work is still necessary on your part to only fill out the
   tp_traverse and tp_clear fields when they exist and set tp_flags
   appropriately.

   It is possible to support both the 2.0 and 2.2 GC APIs, but it's
   not pretty and this comment block is too narrow to contain a
   desciption of what's required... */

#if PY_VERSION_HEX < 0x020200B1
#define PyObject_GC_New         PyObject_New
#define PyObject_GC_NewVar      PyObject_NewVar
#define PyObject_GC_Del         PyObject_Del
#define PyObject_GC_Track(op)
#define PyObject_GC_UnTrack(op)
#endif

#endif /* !Py_PYMEMCOMPAT_H */
