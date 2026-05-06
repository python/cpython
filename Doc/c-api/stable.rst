.. highlight:: c

.. _stable:

***********************
C API and ABI Stability
***********************

Unless documented otherwise, Python's C API is covered by the Backwards
Compatibility Policy, :pep:`387`.
Most changes to it are source-compatible (typically by only adding new API).
Changing existing API or removing API is only done after a deprecation period
or to fix serious issues.

CPython's Application Binary Interface (ABI) is forward- and
backwards-compatible across a minor release (if these are compiled the same
way; see :ref:`stable-abi-platform` below).
So, code compiled for Python 3.10.0 will work on 3.10.8 and vice versa,
but will need to be compiled separately for 3.9.x and 3.11.x.

There are two tiers of C API with different stability expectations:

- :ref:`Unstable API <unstable-c-api>`, may change in minor versions without
  a deprecation period. It is marked by the ``PyUnstable`` prefix in names.
- :ref:`Limited API <limited-c-api>`, is compatible across several minor releases.
  When :c:macro:`Py_LIMITED_API` is defined, only this subset is exposed
  from ``Python.h``.

These are discussed in more detail below.

Names prefixed by an underscore, such as ``_Py_InternalState``,
are private API that can change without notice even in patch releases.
If you need to use this API, consider reaching out to
`CPython developers <https://discuss.python.org/c/core-dev/c-api/30>`_
to discuss adding public API for your use case.

.. _unstable-c-api:

Unstable C API
==============

.. index:: single: PyUnstable

Any API named with the ``PyUnstable`` prefix exposes CPython implementation
details, and may change in every minor release (e.g. from 3.9 to 3.10) without
any deprecation warnings.
However, it will not change in a bugfix release (e.g. from 3.10.0 to 3.10.1).

It is generally intended for specialized, low-level tools like debuggers.

Projects that use this API are expected to follow
CPython development and spend extra effort adjusting to changes.

.. _stable-abi:
.. _stable-application-binary-interface:

Stable Application Binary Interfaces
====================================

Python's :dfn:`Stable ABI` allows extensions to be compatible with multiple
versions of Python, without recompilation.

.. note::

   For simplicity, this document talks about *extensions*, but Stable ABI
   works the same way for all uses of the API – for example, embedding Python.

There are two Stable ABIs:

- ``abi3``, introduced in Python 3.2, is compatible with
  **non**-:term:`free-threaded <free-threaded build>` builds of CPython.

- ``abi3t``, introduced in Python 3.15, is compatible with
  :term:`free-threaded <free-threaded build>` builds of CPython.
  It has stricter API limitations than ``abi3``.

   .. versionadded:: next

      ``abi3t`` was added in :pep:`803`

It is possible for an extension to be compiled for *both* ``abi3`` and
``abi3t`` at the same time; the result will be compatible with
both free-threaded and non-free-threaded builds of Python.
Currently, this has no downsides compared to compiling for ``abi3t`` only.

Each Stable ABI is versioned using the first two numbers of the Python version.
For example, Stable ABI 3.14 corresponds to Python 3.14.
An extension compiled for Stable ABI 3.x is ABI-compatible with Python 3.x
and above.

Extensions that target a stable ABI must only use a limited subset of
the C API. This subset is known as the :dfn:`Limited API`; its contents
are :ref:`listed below <limited-api-list>`.

On Windows, extensions that use a Stable ABI should be linked against
``python3.dll`` rather than a version-specific library such as
``python39.dll``.
This library only exposes the relevant symbols.

On some platforms, Python will look for and load shared library files named
with the ``abi3`` or ``abi3t`` tag (for example, ``mymodule.abi3.so``).
:term:`Free-threaded <free-threaded build>` interpreters only recognize the
``abi3t`` tag, while non-free-threaded ones will prefer ``abi3`` but fall back
to ``abi3t``.
Thus, extensions compatible with both ABIs should use the ``abi3t`` tag.

Python does not necessarily check that extensions it loads
have compatible ABI.
Extension authors are encouraged to add a check using the :c:macro:`Py_mod_abi`
slot or the :c:func:`PyABIInfo_Check` function, but the user
(or their packaging tool) is ultimately responsible for ensuring that,
for example, extensions built for Stable ABI 3.10 are not installed for lower
versions of Python.

All functions in Stable ABI are present as functions in Python's shared
library, not solely as macros.
This makes them usable are usable from languages that don't use the C
preprocessor, including Python's :py:mod:`ctypes`.


.. _abi3-compiling:

Compiling for Stable ABI
------------------------

.. note::

   Build tools (such as, for example, meson-python, scikit-build-core,
   or Setuptools) often have a mechanism for setting macros and synchronizing
   them with extension filenames and other metadata.
   Prefer using such a mechanism, if it exists, over defining the
   macros manually.

   The rest of this section is mainly relevant for tool authors, and for
   people who compile extensions manually.

   .. seealso:: `list of recommended tools`_ in the Python Packaging User Guide

      .. _list of recommended tools: https://packaging.python.org/en/latest/guides/tool-recommendations/#build-backends-for-extension-modules

To compile for a Stable ABI, define one or both of the following macros
to the lowest Python version your extension should support, in
:c:macro:`Py_PACK_VERSION` format.
Typically, you should choose a specific value rather than the version of
the Python headers you are compiling against.

The macros must be defined before including ``Python.h``.
Since :c:macro:`Py_PACK_VERSION` is not available at this point, you
will need to use the numeric value directly.
For reference, the values for a few recent Python versions are:

.. version-hex-cheatsheet::

When one of the macros is defined, ``Python.h`` will only expose API that is
compatible with the given Stable ABI -- that is, the
:ref:`Limited API <limited-api-list>` plus some definitions that need to be
visible to the compiler but should not be used directly.
When both are defined, ``Python.h`` will only expose API compatible with
both Stable ABIs.

.. c:macro:: Py_LIMITED_API

   Target ``abi3``, that is,
   non-:term:`free-threaded builds <free-threaded build>` of CPython.
   See :ref:`above <abi3-compiling>` for common information.

.. c:macro:: Py_TARGET_ABI3T

   Target ``abi3t``, that is,
   :term:`free-threaded builds <free-threaded build>` of CPython.
   See :ref:`above <abi3-compiling>` for common information.

   .. versionadded:: next

Both macros specify a target ABI; the different naming style is due to
backwards compatibility.

.. admonition:: Historical note

   You can also define ``Py_LIMITED_API`` as ``3``. This works the same as
   ``0x03020000`` (Python 3.2, the version that introduced Stable ABI).

When both are defined, ``Python.h`` may, or may not, redefine
:c:macro:`!Py_LIMITED_API` to match :c:macro:`!Py_TARGET_ABI3T`.

On a :term:`free-threaded build` -- that is, when
:c:macro:`Py_GIL_DISABLED` is defined -- :c:macro:`!Py_TARGET_ABI3T`
defaults to the value of :c:macro:`!Py_LIMITED_API`.
This means that there are two ways to build for both ``abi3`` and ``abi3t``:

- define both :c:macro:`!Py_LIMITED_API` and :c:macro:`!Py_TARGET_ABI3T`, or
- define only :c:macro:`!Py_LIMITED_API` and:

  - on Windows, define :c:macro:`!Py_GIL_DISABLED`;
  - on other systems, use the headers of free-threaded build of Python.


.. _limited-api-scope-and-performance:

Stable ABI Scope and Performance
--------------------------------

The goal for Stable ABI is to allow everything that is possible with the
full C API, but possibly with a performance penalty.
Generally, compatibility with Stable ABI will require some changes to an
extension's source code.

For example, while :c:func:`PyList_GetItem` is available, its "unsafe" macro
variant :c:func:`PyList_GET_ITEM` is not.
The macro can be faster because it can rely on version-specific implementation
details of the list object.

For another example, when *not* compiling for Stable ABI, some C API
functions are inlined or replaced by macros.
Compiling for Stable ABI disables this inlining, allowing stability as
Python's data structures are improved, but possibly reducing performance.

By leaving out the :c:macro:`!Py_LIMITED_API` or :c:macro:`!Py_TARGET_ABI3T`
definition, it is possible to compile Stable-ABI-compatible source
for a version-specific ABI.
A potentially faster version-specific extension can then be distributed
alongside a version compiled for Stable ABI -- a slower but more compatible
fallback.


.. _limited-api-caveats:

Stable ABI Caveats
------------------

Note that compiling for Stable ABI is *not* a complete guarantee that code will
be compatible with the expected Python versions.
Stable ABI prevents *ABI* issues, like linker errors due to missing
symbols or data corruption due to changes in structure layouts or function
signatures.
However, other changes in Python can change the *behavior* of extensions.

One issue that the :c:macro:`Py_TARGET_ABI3T` and :c:macro:`Py_LIMITED_API`
macros do not guard against is calling a function with arguments that are
invalid in a lower Python version.
For example, consider a function that starts accepting ``NULL`` for an
argument. In Python 3.9, ``NULL`` now selects a default behavior, but in
Python 3.8, the argument will be used directly, causing a ``NULL`` dereference
and crash. A similar argument works for fields of structs.

For these reasons, we recommend testing an extension with *all* minor Python
versions it supports.

We also recommend reviewing documentation of all used API to check
if it is explicitly part of the Limited API. Even with ``Py_LIMITED_API``
defined, a few private declarations are exposed for technical reasons (or
even unintentionally, as bugs).

Also note that while compiling with ``Py_LIMITED_API`` 3.8 means that the
extension should *load* on Python 3.12, and *compile* with Python 3.12,
the same source will not necessarily compile with ``Py_LIMITED_API``
set to 3.12.
In general, parts of the Limited API may be deprecated and removed,
provided that Stable ABI stays stable.


.. _stable-abi-platform:

Platform Considerations
=======================

ABI stability depends not only on Python, but also on the compiler used,
lower-level libraries and compiler options. For the purposes of
the :ref:`Stable ABIs <stable-abi>`, these details define a “platform”. They
usually depend on the OS type and processor architecture

It is the responsibility of each particular distributor of Python
to ensure that all Python versions on a particular platform are built
in a way that does not break the Stable ABIs, or the version-specific ABIs.
This is the case with Windows and macOS releases from ``python.org`` and many
third-party distributors.


ABI Checking
============

.. versionadded:: 3.15

Python includes a rudimentary check for ABI compatibility.

This check is not comprehensive.
It only guards against common cases of incompatible modules being
installed for the wrong interpreter.
It also does not take :ref:`platform incompatibilities <stable-abi-platform>`
into account.
It can only be done after an extension is successfully loaded.

Despite these limitations, it is recommended that extension modules use this
mechanism, so that detectable incompatibilities raise exceptions rather than
crash.

Most modules can use this check via the :c:data:`Py_mod_abi`
slot and the :c:macro:`PyABIInfo_VAR` macro, for example like this:

.. code-block:: c

   PyABIInfo_VAR(abi_info);

   static PyModuleDef_Slot mymodule_slots[] = {
      {Py_mod_abi, &abi_info},
      ...
   };


The full API is described below for advanced use cases.

.. c:function:: int PyABIInfo_Check(PyABIInfo *info, const char *module_name)

   Verify that the given *info* is compatible with the currently running
   interpreter.

   Return 0 on success. On failure, raise an exception and return -1.

   If the ABI is incompatible, the raised exception will be :py:exc:`ImportError`.

   The *module_name* argument can be ``NULL``, or point to a NUL-terminated
   UTF-8-encoded string used for error messages.

   Note that if *info* describes the ABI that the current code uses (as defined
   by :c:macro:`PyABIInfo_VAR`, for example), using any other Python C API
   may lead to crashes.
   In particular, it is not safe to examine the raised exception.

   .. versionadded:: 3.15

.. c:macro:: PyABIInfo_VAR(NAME)

   Define a static :c:struct:`PyABIInfo` variable with the given *NAME* that
   describes the ABI that the current code will use.
   This macro expands to:

   .. code-block:: c

      static PyABIInfo NAME = {
          1, 0,
          PyABIInfo_DEFAULT_FLAGS,
          PY_VERSION_HEX,
          PyABIInfo_DEFAULT_ABI_VERSION
      }

   .. versionadded:: 3.15

.. c:type:: PyABIInfo

   .. c:member:: uint8_t abiinfo_major_version

      The major version of :c:struct:`PyABIInfo`. Can be set to:

      * ``0`` to skip all checking, or
      * ``1`` to specify this version of :c:struct:`!PyABIInfo`.

   .. c:member:: uint8_t abiinfo_minor_version

      The minor version of :c:struct:`PyABIInfo`.
      Must be set to ``0``; larger values are reserved for backwards-compatible
      future versions of :c:struct:`!PyABIInfo`.

   .. c:member:: uint16_t flags

      .. c:namespace:: NULL

      This field is usually set to the following macro:

      .. c:macro:: PyABIInfo_DEFAULT_FLAGS

         Default flags, based on current values of macros such as
         :c:macro:`Py_LIMITED_API` and :c:macro:`Py_GIL_DISABLED`.

      Alternately, the field can be set to the following flags, combined
      by bitwise OR.
      Unused bits must be set to zero.

      ABI variant -- one of:

         .. c:macro:: PyABIInfo_STABLE

            Specifies that Stable ABI is used.

         .. c:macro:: PyABIInfo_INTERNAL

            Specifies ABI specific to a particular build of CPython.
            Internal use only.

      Free-threading compatibility -- one of:

         .. c:macro:: PyABIInfo_FREETHREADED

            Specifies ABI compatible with :term:`free-threaded builds
            <free-threaded build>` of CPython.
            (That is, ones compiled with :option:`--disable-gil`; with ``t``
            in :py:data:`sys.abiflags`)

         .. c:macro:: PyABIInfo_GIL

            Specifies ABI compatible with non-free-threaded builds of CPython
            (ones compiled *without* :option:`--disable-gil`).

         .. c:macro:: PyABIInfo_FREETHREADING_AGNOSTIC

            Specifies ABI compatible with both free-threaded and
            non-free-threaded builds of CPython, that is, both
            ``abi3`` and ``abi3t``.

   .. c:member:: uint32_t build_version

      The version of the Python headers used to build the code, in the format
      used by :c:macro:`PY_VERSION_HEX`.

      This can be set to ``0`` to skip any checks related to this field.
      This option is meant mainly for projects that do not use the CPython
      headers directly, and do not emulate a specific version of them.

   .. c:member:: uint32_t abi_version

      The ABI version.

      For Stable ABI, this field should be the value of
      :c:macro:`Py_LIMITED_API` or :c:macro:`Py_TARGET_ABI3T`.
      If both are defined, use the smaller value.
      (If :c:macro:`Py_LIMITED_API` is ``3``; use
      :c:expr:`Py_PACK_VERSION(3, 2)` instead of ``3``.)

      Otherwise, it should be set to :c:macro:`PY_VERSION_HEX`.

      It can also be set to ``0`` to skip any checks related to this field.

      .. c:namespace:: NULL

      .. c:macro:: PyABIInfo_DEFAULT_ABI_VERSION

         The value that should be used for this field, based on current
         values of macros such as :c:macro:`Py_LIMITED_API`,
         :c:macro:`PY_VERSION_HEX` and :c:macro:`Py_GIL_DISABLED`.

   .. versionadded:: 3.15


.. _limited-c-api:
.. _limited-api-list:

Contents of Limited API
=======================

This is the definitive list of :ref:`Limited API <limited-c-api>` for
Python |version|:

.. limited-api-list::
