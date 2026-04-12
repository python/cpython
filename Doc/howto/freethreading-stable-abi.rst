.. highlight:: c

.. _freethreading-stable-abi-howto:

******************************************
How to Use Free Threading Stable ABI
******************************************

Starting with the 3.15 release, CPython has support for compiling extensions targeting
a unique kind of Stable ABI with interpreters having the :term:`global interpreter lock` (GIL) disabled.
For 3.14 and 3.13, continue compiling with the version-specific ABI. This document describes how
to adapt C API extensions to support free threading.

Identifying the Free-Threaded Limited API Build in C
====================================================

The CPython C API exposes the :c:macro:`!Py_LIMITED_API` macro: in the free-threaded stable ABI
build it's defined to ``1``, and in the regular build it's not defined.
You can use it to enable code that only runs under the free-threaded build::

    #ifdef Py_TARGET_ABI3T
    /* code that only runs in the free-threaded stable ABI build */
    #endif

If you wish to build youe extension with both ``abi3`` (Stable ABI with GIL) and ``abi3t`` (no-GIL stable ABI) tags,
do one of the following:

- define both :c:macro:`!Py_LIMITED_API` and :c:macro:`!Py_TARGET_ABI3T`, or
- define only :c:macro:`!Py_LIMITED_API` and:

  - on Windows, define :c:macro:`!Py_GIL_DISABLED`;
  - on other systems, use the headers of free-threaded build of Python.

``PyObject`` and ``PyVarObject`` opaqueness
===========================================

Accessing any member of ``PyObject`` directly is now prohibited, like the non-GIL
stable ABI. For instance, prefer ``Py_TYPE()`` and ``Py_SET_TYPE()`` over ``ob_type``,
``Py_REFCNT``, ``Py_IncRef()`` and ``Py_DecRef()`` over ``ob_refcnt``, etc.

Similarly, members of ``PyVarObject`` are not visible. If you need any object of such type
to be passed as a ``PyObject`` parameter to any API function, cast it directly as ``PyObject``.

Module Initialization
=====================

Extension modules need to explicitly indicate that they support running with
the GIL disabled; otherwise importing the extension will raise a warning and
enable the GIL at runtime.

Multi-phase and single-phase initialization is supported to indicate that an extension module
targeting the stable ABI supports running with the GIL disabled, though the former is preferred.

Multi-Phase Initialization
..........................

Extensions that use :ref:`multi-phase initialization <multi-phase-initialization>`
(functions like :c:func:`PyModuleDef_Init`,
:c:func:`PyModExport_* <PyModExport_modulename>` export hook,
:c:func:`PyModule_FromSlotsAndSpec`) should add a
:c:data:`Py_mod_gil` slot in the module definition.
If your extension supports older versions of CPython,
you should guard the slot with a :c:data:`Py_GIL_DISABLED` check.

::

    static struct PyModuleDef_Slot module_slots[] = {
        ...
    #ifdef Py_GIL_DISABLED
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    #endif
        {0, NULL}
    };

Additionally, using ``PyABIInfo_VAR`` and ``Py_mod_abi`` is recommended so that an
extension module loaded for an incompatible interpreter will trigger an exception, rather than
fail with a crash.

.. code-block:: c

   #ifdef PY_VERSION_HEX >= 0x030F0000
   PyABIInfo_VAR(abi_info);
   #endif Py_GIL_DISABLED

   static PyModuleDef_Slot mymodule_slots[] = {
      ...
   #ifdef PY_VERSION_HEX >= 0x030F0000
      {Py_mod_abi, &abi_info},
   #endif
      {0, NULL}
   };

Single-Phase Initialization
...........................

Although members of ``PyModuleDef`` is still available for no-GIL Stable ABI and can be used
for :ref:`single-phase initialization <single-phase-initialization>`
(that is, :c:func:`PyModule_Create`), they are not exposed when targeting the regular Stable ABI.
Prefer multi-phased initializtion when possible.

General API Guidelines
======================

Most of the C API is thread-safe, but there are some exceptions.

* **Struct Fields**: Accessing fields in Python C API objects or structs
  directly is not thread-safe if the field may be concurrently modified.
* **Borrowed References**: C API functions that return
  :term:`borrowed references <borrowed reference>` may not be thread-safe if
  the containing object is modified concurrently.  See the section on
  :ref:`borrowed references <borrowed-references>` for more information.


Container Thread Safety
.......................

Containers like :c:struct:`PyListObject`,
:c:struct:`PyDictObject`, and :c:struct:`PySetObject` perform internal locking
in the free-threaded build.  For example, the :c:func:`PyList_Append` will
lock the list before appending an item.

Borrowed References
===================

.. _borrowed-references:

Some C API functions return :term:`borrowed references <borrowed reference>`.
These APIs are not thread-safe if the containing object is modified
concurrently.  For example, it's not safe to use :c:func:`PyList_GetItem`
if the list may be modified concurrently.

The following table lists some borrowed reference APIs and their replacements
that return :term:`strong references <strong reference>`.

+-----------------------------------+-----------------------------------+
| Borrowed reference API            | Strong reference API              |
+===================================+===================================+
| :c:func:`PyList_GetItem`          | :c:func:`PyList_GetItemRef`       |
+-----------------------------------+-----------------------------------+
| :c:func:`PyList_GET_ITEM`         | :c:func:`PyList_GetItemRef`       |
+-----------------------------------+-----------------------------------+
| :c:func:`PyDict_GetItem`          | :c:func:`PyDict_GetItemRef`       |
+-----------------------------------+-----------------------------------+
| :c:func:`PyDict_GetItemWithError` | :c:func:`PyDict_GetItemRef`       |
+-----------------------------------+-----------------------------------+
| :c:func:`PyDict_GetItemString`    | :c:func:`PyDict_GetItemStringRef` |
+-----------------------------------+-----------------------------------+
| :c:func:`PyDict_SetDefault`       | :c:func:`PyDict_SetDefaultRef`    |
+-----------------------------------+-----------------------------------+
| :c:func:`PyDict_Next`             | none (see :ref:`PyDict_Next`)     |
+-----------------------------------+-----------------------------------+
| :c:func:`!PyWeakref_GetObject`    | :c:func:`PyWeakref_GetRef`        |
+-----------------------------------+-----------------------------------+
| :c:func:`!PyWeakref_GET_OBJECT`   | :c:func:`PyWeakref_GetRef`        |
+-----------------------------------+-----------------------------------+
| :c:func:`PyImport_AddModule`      | :c:func:`PyImport_AddModuleRef`   |
+-----------------------------------+-----------------------------------+
| :c:func:`PyCell_GET`              | :c:func:`PyCell_Get`              |
+-----------------------------------+-----------------------------------+

Not all APIs that return borrowed references are problematic.  For
example, :c:func:`PyTuple_GetItem` is safe because tuples are immutable.
Similarly, not all uses of the above APIs are problematic.  For example,
:c:func:`PyDict_GetItem` is often used for parsing keyword argument
dictionaries in function calls; those keyword argument dictionaries are
effectively private (not accessible by other threads), so using borrowed
references in that context is safe.

Some of these functions were added in Python 3.13.  You can use the
`pythoncapi-compat <https://github.com/python/pythoncapi-compat>`_ package
to provide implementations of these functions for older Python versions.


Thread State and GIL APIs
=========================

Python provides a set of functions and macros to manage thread state and the
GIL, such as:

* :c:func:`PyGILState_Ensure` and :c:func:`PyGILState_Release`
* :c:func:`PyEval_SaveThread` and :c:func:`PyEval_RestoreThread`
* :c:macro:`Py_BEGIN_ALLOW_THREADS` and :c:macro:`Py_END_ALLOW_THREADS`

These functions should still be used in the free-threaded build to manage
thread state even when the :term:`GIL` is disabled.  For example, if you
create a thread outside of Python, you must call :c:func:`PyGILState_Ensure`
before calling into the Python API to ensure that the thread has a valid
Python thread state.

You should continue to call :c:func:`PyEval_SaveThread` or
:c:macro:`Py_BEGIN_ALLOW_THREADS` around blocking operations, such as I/O or
lock acquisitions, to allow other threads to run the
:term:`cyclic garbage collector <garbage collection>`.


Protecting Internal Extension State
===================================

Your extension may have internal state that was previously protected by the
GIL.  You may need to add locking to protect this state.  The approach will
depend on your extension, but some common patterns include:

* **Caches**: global caches are a common source of shared state.  Consider
  using a lock to protect the cache or disabling it in the free-threaded build
  if the cache is not critical for performance.
* **Global State**: global state may need to be protected by a lock or moved
  to thread local storage. C11 and C++11 provide the ``thread_local`` or
  ``_Thread_local`` for
  `thread-local storage <https://en.cppreference.com/w/c/language/storage_duration>`_.


Critical Sections
=================

.. _critical-sections:

In the free-threaded build, CPython provides a mechanism called "critical
sections" to protect data that would otherwise be protected by the GIL.
While extension authors may not interact with the internal critical section
implementation directly, understanding their behavior is crucial when using
certain C API functions or managing shared state in the free-threaded build.

What Are Critical Sections?
...........................

Conceptually, critical sections act as a deadlock avoidance layer built on
top of simple mutexes. Each thread maintains a stack of active critical
sections. When a thread needs to acquire a lock associated with a critical
section (e.g., implicitly when calling a thread-safe C API function like
:c:func:`PyDict_SetItem`, or explicitly using macros), it attempts to acquire
the underlying mutex.

Using Critical Sections
.......................

The primary APIs for using critical sections are:

* :c:macro:`Py_BEGIN_CRITICAL_SECTION` and :c:macro:`Py_END_CRITICAL_SECTION` -
  For locking a single object

* :c:macro:`Py_BEGIN_CRITICAL_SECTION2` and :c:macro:`Py_END_CRITICAL_SECTION2`
  - For locking two objects simultaneously

These macros must be used in matching pairs and must appear in the same C
scope, since they establish a new local scope.  These macros are no-ops in
non-free-threaded builds, so they can be safely added to code that needs to
support both build types.

A common use of a critical section would be to lock an object while accessing
an internal attribute of it.  For example, if an extension type has an internal
count field, you could use a critical section while reading or writing that
field::

    // read the count, returns new reference to internal count value
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(obj);
    result = Py_NewRef(obj->count);
    Py_END_CRITICAL_SECTION();
    return result;

    // write the count, consumes reference from new_count
    Py_BEGIN_CRITICAL_SECTION(obj);
    obj->count = new_count;
    Py_END_CRITICAL_SECTION();


How Critical Sections Work
..........................

Unlike traditional locks, critical sections do not guarantee exclusive access
throughout their entire duration. If a thread would block while holding a
critical section (e.g., by acquiring another lock or performing I/O), the
critical section is temporarily suspended—all locks are released—and then
resumed when the blocking operation completes.

This behavior is similar to what happens with the GIL when a thread makes a
blocking call. The key differences are:

* Critical sections operate on a per-object basis rather than globally

* Critical sections follow a stack discipline within each thread (the "begin" and
  "end" macros enforce this since they must be paired and within the same scope)

* Critical sections automatically release and reacquire locks around potential
  blocking operations

Deadlock Avoidance
..................

Critical sections help avoid deadlocks in two ways:

1. If a thread tries to acquire a lock that's already held by another thread,
   it first suspends all of its active critical sections, temporarily releasing
   their locks

2. When the blocking operation completes, only the top-most critical section is
   reacquired first

This means you cannot rely on nested critical sections to lock multiple objects
at once, as the inner critical section may suspend the outer ones. Instead, use
:c:macro:`Py_BEGIN_CRITICAL_SECTION2` to lock two objects simultaneously.

Note that the locks described above are only :c:type:`PyMutex` based locks.
The critical section implementation does not know about or affect other locking
mechanisms that might be in use, like POSIX mutexes.  Also note that while
blocking on any :c:type:`PyMutex` causes the critical sections to be
suspended, only the mutexes that are part of the critical sections are
released.  If :c:type:`PyMutex` is used without a critical section, it will
not be released and therefore does not get the same deadlock avoidance.

Important Considerations
........................

* Critical sections may temporarily release their locks, allowing other threads
  to modify the protected data. Be careful about making assumptions about the
  state of the data after operations that might block.

* Because locks can be temporarily released (suspended), entering a critical
  section does not guarantee exclusive access to the protected resource
  throughout the section's duration. If code within a critical section calls
  another function that blocks (e.g., acquires another lock, performs blocking
  I/O), all locks held by the thread via critical sections will be released.
  This is similar to how the GIL can be released during blocking calls.

* Only the lock(s) associated with the most recently entered (top-most)
  critical section are guaranteed to be held at any given time. Locks for
  outer, nested critical sections might have been suspended.

* You can lock at most two objects simultaneously with these APIs. If you need
  to lock more objects, you'll need to restructure your code.

* While critical sections will not deadlock if you attempt to lock the same
  object twice, they are less efficient than purpose-built reentrant locks for
  this use case.

* When using :c:macro:`Py_BEGIN_CRITICAL_SECTION2`, the order of the objects
  doesn't affect correctness (the implementation handles deadlock avoidance),
  but it's good practice to always lock objects in a consistent order.

* Remember that the critical section macros are primarily for protecting access
  to *Python objects* that might be involved in internal CPython operations
  susceptible to the deadlock scenarios described above. For protecting purely
  internal extension state, standard mutexes or other synchronization
  primitives might be more appropriate.

Platform-specific considerations
................................

On some platforms, Python will look for and load shared library files named
with the ``abi3`` or ``abi3t`` tag (for example, ``mymodule.abi3.so``).
:term:`Free-threaded <free-threaded build>` interpreters only recognize the
``abi3t`` tag, while non-free-threaded ones will prefer ``abi3`` but fall back
to ``abi3t``.
Thus, extensions compatible with both ABIs should use the ``abi3t`` tag.

Python does not necessarily check that extensions it loads
have compatible ABI.
Extension authors are encouraged to add a check using the :c:macro:`Py_mod_abi`
slot or the :c:func:`PyABIInfo_Check` function.

Limited C API Build Tools
.........................

If you use
`setuptools <https://setuptools.pypa.io/en/latest/setuptools.html>`_ to build
your extension, a future version of ``setuptools`` will allow ``py_limited_api=True``
to be set to allow targeting limited API when building with the free-threaded build.

`Other build tools will support this ABI as well <https://packaging.python.org/en/latest/guides/tool-recommendations/#build-backends-for-extension-modules>`_.

.. seealso::

   `Porting Extension Modules to Support Free-Threading
   <https://py-free-threading.github.io/porting/>`_:
   A community-maintained porting guide for extension authors.
