.. highlight:: c

.. _building:

*****************************
Building C and C++ Extensions
*****************************

A C extension for CPython is a shared library (e.g. a ``.so`` file on Linux,
``.pyd`` on Windows), which exports an *initialization function*.

To be importable, the shared library must be available on :envvar:`PYTHONPATH`,
and must be named after the module name, with an appropriate extension.
When using setuptools, the correct filename is generated automatically.

The initialization function has the signature:

.. c:function:: PyObject* PyInit_modulename(void)

It returns either a fully initialized module, or a :c:type:`PyModuleDef`
instance. See :ref:`initializing-modules` for details.

.. highlight:: python

For modules with ASCII-only names, the function must be named
``PyInit_<modulename>``, with ``<modulename>`` replaced by the name of the
module. When using :ref:`multi-phase-initialization`, non-ASCII module names
are allowed. In this case, the initialization function name is
``PyInitU_<modulename>``, with ``<modulename>`` encoded using Python's
*punycode* encoding with hyphens replaced by underscores. In Python::

    def initfunc_name(name):
        try:
            suffix = b'_' + name.encode('ascii')
        except UnicodeEncodeError:
            suffix = b'U_' + name.encode('punycode').replace(b'-', b'_')
        return b'PyInit' + suffix

It is possible to export multiple modules from a single shared library by
defining multiple initialization functions. However, importing them requires
using symbolic links or a custom importer, because by default only the
function corresponding to the filename is found.
See the *"Multiple modules in one library"* section in :pep:`489` for details.


.. highlight:: c

.. _install-index:
.. _setuptools-index:

Building C and C++ Extensions with setuptools
=============================================

Python 3.12 and newer no longer come with distutils. Please refer to the
``setuptools`` documentation at
https://setuptools.readthedocs.io/en/latest/setuptools.html
to learn more about how build and distribute C/C++ extensions with setuptools.
