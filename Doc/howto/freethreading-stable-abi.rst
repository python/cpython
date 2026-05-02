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


Critical Sections
=================

.. _critical-sections:

Replacements:

+-------------------------------------------+---------------------------------------+
| Macro functions                           | C API functions                       |
+===========================================+=======================================+
|:c:macro:`Py_BEGIN_CRITICAL_SECTION`       |``PyCriticalSection_Begin``            |
|:c:macro:`Py_END_CRITICAL_SECTION`         |``PyCriticalSection_End``              |
+-------------------------------------------+---------------------------------------+
|:c:macro:`Py_BEGIN_CRITICAL_SECTION2`      |``PyCriticalSection2_Begin``           |
|:c:macro:`Py_END_CRITICAL_SECTION2`        |``PyCriticalSection2_End``             |
+-------------------------------------------+---------------------------------------+
|:c:macro:`Py_BEGIN_CRITICAL_SECTION_MUTEX` |``PyCriticalSection_BeginMutex``       |
|:c:macro:`Py_END_CRITICAL_SECTION`         |``PyCriticalSection_End``              |
+-------------------------------------------+---------------------------------------+
|:c:macro:`Py_BEGIN_CRITICAL_SECTION2_MUTEX`|``PyCriticalSection2_BeginMutex``      |
|:c:macro:`Py_END_CRITICAL_SECTION2`        |``PyCriticalSection2_End``             |
+-------------------------------------------+---------------------------------------+

Platform-specific considerations
................................

On some platforms, Python will look for and load shared library files named
with the ``abi3`` or ``abi3t`` tag (for example, ``mymodule.abi3t.so``).
:term:`Free-threaded <free-threaded build>` interpreters prefer ``abi3t``,
but can fall back to ``abi3``.
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
``uv`` supports targeting PEP 803 as of 0.11.3: ``https://github.com/astral-sh/uv/releases/tag/0.11.3``.

`Other build tools will support this ABI as well <https://packaging.python.org/en/latest/guides/tool-recommendations/#build-backends-for-extension-modules>`_.

.. seealso::

   `Porting Extension Modules to Support Free-Threading
   <https://py-free-threading.github.io/porting/>`_:
   A community-maintained porting guide for extension authors.
