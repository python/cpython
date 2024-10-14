.. highlight:: c

.. _freethreading-extensions-howto:

******************************************
C API Extension Support for Free Threading
******************************************

Starting with the 3.13 release, CPython has experimental support for running
with the :term:`global interpreter lock` (GIL) disabled in a configuration
called :term:`free threading`.  This document describes how to adapt C API
extensions to support free threading.


Identifying the Free-Threaded Build in C
========================================

The CPython C API exposes the ``Py_GIL_DISABLED`` macro: in the free-threaded
build it's defined to ``1``, and in the regular build it's not defined.
You can use it to enable code that only runs under the free-threaded build::

    #ifdef Py_GIL_DISABLED
    /* code that only runs in the free-threaded build */
    #endif

Module Initialization
=====================

Extension modules need to explicitly indicate that they support running with
the GIL disabled; otherwise importing the extension will raise a warning and
enable the GIL at runtime.

There are two ways to indicate that an extension module supports running with
the GIL disabled depending on whether the extension uses multi-phase or
single-phase initialization.

Multi-Phase Initialization
..........................

Extensions that use multi-phase initialization (i.e.,
:c:func:`PyModuleDef_Init`) should add a :c:data:`Py_mod_gil` slot in the
module definition.  If your extension supports older versions of CPython,
you should guard the slot with a :c:data:`PY_VERSION_HEX` check.

::

    static struct PyModuleDef_Slot module_slots[] = {
        ...
    #if PY_VERSION_HEX >= 0x030D0000
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    #endif
        {0, NULL}
    };

    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        .m_slots = module_slots,
        ...
    };


Single-Phase Initialization
...........................

Extensions that use single-phase initialization (i.e.,
:c:func:`PyModule_Create`) should call :c:func:`PyUnstable_Module_SetGIL` to
indicate that they support running with the GIL disabled.  The function is
only defined in the free-threaded build, so you should guard the call with
``#ifdef Py_GIL_DISABLED`` to avoid compilation errors in the regular build.

::

    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        ...
    };

    PyMODINIT_FUNC
    PyInit_mymodule(void)
    {
        PyObject *m = PyModule_Create(&moduledef);
        if (m == NULL) {
            return NULL;
        }
    #ifdef Py_GIL_DISABLED
        PyUnstable_Module_SetGIL(m, Py_MOD_GIL_NOT_USED);
    #endif
        return m;
    }


General API Guidelines
======================

Most of the C API is thread-safe, but there are some exceptions.

* **Struct Fields**: Accessing fields in Python C API objects or structs
  directly is not thread-safe if the field may be concurrently modified.
* **Macros**: Accessor macros like :c:macro:`PyList_GET_ITEM` and
  :c:macro:`PyList_SET_ITEM` do not perform any error checking or locking.
  These macros are not thread-safe if the container object may be modified
  concurrently.
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

.. _PyDict_Next:

``PyDict_Next``
'''''''''''''''

A notable exception is :c:func:`PyDict_Next`, which does not lock the
dictionary.  You should use :c:macro:`Py_BEGIN_CRITICAL_SECTION` to protect
the dictionary while iterating over it if the dictionary may be concurrently
modified::

    Py_BEGIN_CRITICAL_SECTION(dict);
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        ...
    }
    Py_END_CRITICAL_SECTION();


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
| :c:func:`PyWeakref_GetObject`     | :c:func:`PyWeakref_GetRef`        |
+-----------------------------------+-----------------------------------+
| :c:func:`PyWeakref_GET_OBJECT`    | :c:func:`PyWeakref_GetRef`        |
+-----------------------------------+-----------------------------------+
| :c:func:`PyImport_AddModule`      | :c:func:`PyImport_AddModuleRef`   |
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


.. _free-threaded-memory-allocation:

Memory Allocation APIs
======================

Python's memory management C API provides functions in three different
:ref:`allocation domains <allocator-domains>`: "raw", "mem", and "object".
For thread-safety, the free-threaded build requires that only Python objects
are allocated using the object domain, and that all Python object are
allocated using that domain.  This differs from the prior Python versions,
where this was only a best practice and not a hard requirement.

.. note::

   Search for uses of :c:func:`PyObject_Malloc` in your
   extension and check that the allocated memory is used for Python objects.
   Use :c:func:`PyMem_Malloc` to allocate buffers instead of
   :c:func:`PyObject_Malloc`.


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


Building Extensions for the Free-Threaded Build
===============================================

C API extensions need to be built specifically for the free-threaded build.
The wheels, shared libraries, and binaries are indicated by a ``t`` suffix.

* `pypa/manylinux <https://github.com/pypa/manylinux>`_ supports the
  free-threaded build, with the ``t`` suffix, such as ``python3.13t``.
* `pypa/cibuildwheel <https://github.com/pypa/cibuildwheel>`_ supports the
  free-threaded build if you set
  `CIBW_FREE_THREADED_SUPPORT <https://cibuildwheel.pypa.io/en/stable/options/#free-threaded-support>`_.

Limited C API and Stable ABI
............................

The free-threaded build does not currently support the
:ref:`Limited C API <limited-c-api>` or the stable ABI.  If you use
`setuptools <https://setuptools.pypa.io/en/latest/setuptools.html>`_ to build
your extension and currently set ``py_limited_api=True`` you can use
``py_limited_api=not sysconfig.get_config_var("Py_GIL_DISABLED")`` to opt out
of the limited API when building with the free-threaded build.

.. note::
    You will need to build separate wheels specifically for the free-threaded
    build.  If you currently use the stable ABI, you can continue to build a
    single wheel for multiple non-free-threaded Python versions.


Windows
.......

Due to a limitation of the official Windows installer, you will need to
manually define ``Py_GIL_DISABLED=1`` when building extensions from source.

.. seealso::

   `Porting Extension Modules to Support Free-Threading
   <https://py-free-threading.github.io/porting/>`_:
   A community-maintained porting guide for extension authors.
