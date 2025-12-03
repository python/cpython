.. highlight:: c


.. _first-extension-module:

*********************************
Your first C API extension module
*********************************

This tutorial will take you through creating a simple
Python extension module written in C or C++.

The tutorial assumes basic knowledge about Python: you should be able to
define functions in Python code before starting to write them in C.
See :ref:`tutorial-index` for an introduction to Python itself.

The tutorial should be useful for anyone who can write a basic C library.
While we will mention several concepts that a C beginner would not be expected
to know, like ``static`` functions or linkage declarations, understanding these
is not necessary for success.

As a word warning before we begin: as the code is written, you will need to
compile it with the right tools and settings for your system.
It is generally best to use a third-party tool to handle the details.
This is covered in later chapters, not in this tutorial.

The tutorial assumes that you use a Unix-like system (including macOS and
Linux) or Windows.
If you don't have a preference, currently you're most likely to get the best
experience on Linux.

If you learn better by having the full context at the beginning,
skip to the end to see :ref:`the resulting source <first-extension-result>`,
then return here.
The tutorial will explain every line, although in a different order.

.. note::

   This tutorial uses API that was added in CPython 3.15.
   To create an extension that's compatible with earlier versions of CPython,
   please follow an earlier version of this documentation.

   This tutorial uses some syntax added in C11 and C++20.
   If your extension needs to be compatible with earlier standards,
   please follow tutorials in documentation for Python 3.14 or below.


What we'll do
=============

Let's create an extension module called ``spam`` (the favorite food of Monty
Python fans...) and let's say we want to create a Python interface to the C
standard library function :c:func:`system`.
This function takes a C string as argument, runs the argument as a system
command, and returns a result value as an integer.
In documentation, it might be summarized as::

   #include <stdlib.h>

   int system(const char *command);

Note that like many functions in the C standard library,
this function is already exposed in Python, as :py:func:`os.system`.
We'll make our own "wrapper".

We want this function to be callable from Python as follows:

.. code-block:: pycon

   >>> import spam
   >>> status = spam.system("whoami")
   User Name
   >>> status
   0

.. note::

   The system command ``whoami`` prints out your username.
   It's useful in tutorials like this one because it has the same name on
   both Unix and Windows.


Warming up your build tool: an empty module
===========================================

Begin by creating a file named :file:`spammodule.c`.
The name is entirely up to you, though some tools can be picky about
the ``.c`` extension.
(Some people would just use :file:`spam.c` to implement a module
named ``spam``, for example.
If you do this, you'll need to adjust some instructions below.)

Now, while the file is emptly, we'll compile it, so that you can make
and test incremental changes as you follow the rest of the tutorial.

Choose a build tool such as Setuptools or Meson, and follow its instructions
to compile and install the empty :file:`spammodule.c` as if it was a
C extension module.

.. note:: Workaround for missing ``PyInit``

   If your build tool output complains about missing ``PyInit_spam``,
   add the following function to your module for now:

   .. code-block:: c

      // A workaround
      void *PyInit_spam(void) { return NULL; }

   This is a shim for an old-style :ref:`initialization function <extension-export-hook>`,
   which was required in extension modules for CPython 3.14 and below.
   Current CPython will not call it, but some build tools may still assume that
   all extension modules need to define it.

   If you use this workaround, you will get the exception
   ``SystemError: initialization of spam failed without raising an exception``
   instead of an :py:exc:`ImportError` in the next step.

.. note::

   Using a third-party build tool is heavily recommended, as in will take
   care of various details of your platform and Python installation,
   naming the resulting extension, and, later, distributing your work.

   If you don't want to use a tool, you can try to run your compiler directly.
   The following command should work for many flavors of Linux, and generate
   a ``spam.so`` file that you need to put in a directory
   in :py:attr:`sys.path`:

   .. code-block:: shell

      gcc --shared spammodule.c -o spam.so

When your extension is compiled and installed, start Python and try to import
your extension.
This should fail with the following exception:

.. code-block:: pycon

   >>> import spam
   Traceback (most recent call last):
   File "<string>", line 1, in <module>
      import spam
   ImportError: dynamic module does not define module export function (PyModExport_spam or PyInit_spam)


Including the Header
====================


Now, put the first line in the file: include :file:`Python.h` to pull in
all declarations of the Python C API:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: <Python.h>
   :end-at: <Python.h>

This header contains all of the Python C API.

Next, bring in the declaration of the C library function we want to call.
Documentation of the :c:func:`system` function tells us that teh necessary
header is:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: <stdlib.h>
   :end-at: <stdlib.h>

Be sure to put this, and any other standard library includes, *after*
:file:`Python.h`, since Python may define some pre-processor definitions
which affect the standard headers on some systems.

.. tip::

   The ``<stdlib.h>`` include is technically not necessary.
   :file:`Python.h` :ref:`includes several standard header files <capi-system-includes>`
   for its own use and for backwards compatibility,
   and ``stdlib`` is one of them.
   However, it is good practice to explicitly include what you need.

With the includes in place, compile and import the extension again.
You should get the same exception as with the empty file.

.. note::

   Third-party build tools should handle pointing the compiler to
   the CPython headers and libraries, and setting appropriate options.

   If you are running the compiler directly, you will need to do this yourself.
   If your installation of Python comes with a corresponding ``python-config``
   command, you can run something like:

   .. code-block:: shell

      gcc --shared $(python-config --cflags --ldflags) spammodule.c -o spam.so


Module export hook
==================

The exception you should be getting tells you that Python is looking for an
module :ref:`export hook <extension-export-hook>` function.
Let's define one.

First, let's add a prototype:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Export hook prototype
   :end-before: ///

The :c:macro:`PyMODEXPORT_FUNC` macro declares the function's
return type, and adds any special linkage declarations required by the platform
to make the compiled extension export the function.
For C++, it declares the function as ``extern "C"``.

.. tip::
   The prototype is not strictly necessary, but some modern C compilers emit
   warnings when a public, exported function has no prototype declaration.
   It's better to add a prototype than disable the warning, so that in other
   code you're notified in common cases of forgetting a header or misspelling
   a name.

After the prototype, add the function itself.
For now, make it return ``NULL``:

.. code-block:: c

   PyMODEXPORT_FUNC
   PyModExport_spam(void)
   {
      return NULL;
   }

Compile and load the module again.
You should get a different error this time:

.. code-block:: pycon

   >>> import spam
   Traceback (most recent call last):
   File "<string>", line 1, in <module>
      import spam
   SystemError: module export hook for module 'spam' failed without setting an exception

Many functions in the Python C API, including export hooks, are expected to
do two things to signal that they have failed: return ``NULL``, and
set an exception.
Here, Python assumes that you only did half of this.

.. note::

   This is one of a few situation where CPython checks this situation and
   emits a "nice" error message.
   Elsewhere, returning ``NULL`` without setting an exception can
   trigger undefined behavior.


The slot table
==============

Rather than ``NULL``, the export hook should return the information needed to
create a module, as a ``static`` array of :c:type:`PyModuleDef_Slot` entries.
Define this array just before your export hook:

.. code-block:: c

   static PyModuleDef_Slot spam_slots[] = {
      {Py_mod_name, "spam"},
      {Py_mod_doc, PyDoc_STR("A wonderful module with an example function")},
      {0, NULL}
   };

The array contains:

* the module name (as a NUL-terminated UTF-8 encoded C string),
* the docstring (similarly encoded), and
* a zero-filled *sentinel* marking the end of the array.

.. tip::

   The :c:func:`PyDoc_STR` macro can, in a special build mode, omit the
   docstring to save a bit of memory.

Return this array from your export hook, instead of ``NULL``:

.. code-block:: c
   :emphasize-lines: 4

   PyMODEXPORT_FUNC
   PyModExport_spam(void)
   {
      return spam_slots;
   }

Now, recompile and try it out:

.. code-block:: pycon

   >>> import spam
   >>> print(spam)
   <module 'spam' from '/home/encukou/dev/cpython/spam.so'>

You now have a extension module!
Try ``help(spam)`` to see the docstring.

The next step will be adding a function to it.


Adding a function
=================

To expose a C function to Python, you need three things:

* The *implementation*: a C function that does what you need, and
* a *name* to use in Python code.

You can also add a *dosctring*.
OK, *amongst* the things you need to expose a C function are




The :c:type:`PyModuleDef_Slot` array must be passed to the interpreter in the
module's :ref:`export hook <extension-export-hook>` function.
The hook must be named :c:func:`!PyModExport_name`, where *name* is the name
of the module, and it should be the only non-\ ``static`` item defined in the
module file.

Several modern C compilers emit warnings when you define a public function
without a prototype declaration.
Normally, prototypes go in a header file so they can be used from other
C files.
A function without a prototype is allowed in C, but is normally a strong
hint that something is wrong -- for example, the name is misspelled.

In our case, Python will load our function dynamically, so a prototype
isn't needed.
We'll only add one to avoid well-meaning compiler warnings:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Export hook prototype
   :end-before: ///

The :c:macro:`PyMODEXPORT_FUNC` macro declares the function's
``PyModuleDef_Slot *`` return type, and adds any special linkage
declarations required by the platform.
For C++, it declares the function as ``extern "C"``.

Just after the prototype, we'll add the function itself.
Its only job is to return the slot array, which in turn contains
all the information needed to create our module object:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Module export hook
   :end-before: ///


The ``spam`` module
===================


The first line of this file (after any comment describing the purpose of
the module, copyright notices, and the like) will pull in the Python API:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: <Python.h>
   :end-at: <Python.h>

Next, we bring in the declaration of the C library function we want to call.
Documentation of the :c:func:`system` function tells us that we need
``stdlib.h``:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: <stdlib.h>
   :end-at: <stdlib.h>

Be sure to put this, and any other standard library includes, *after*
:file:`Python.h`, since Python may define some pre-processor definitions
which affect the standard headers on some systems.

.. tip::

   The ``<stdlib.h>`` include is technically not necessary.
   :file:`Python.h` :ref:`includes several standard header files <capi-system-includes>`
   for its own use and for backwards compatibility,
   and ``stdlib`` is one of them.
   However, it is good practice to explicitly include what you need.

The next thing we add to our module file is the C function that will be called
when the Python expression ``spam.system(string)`` is evaluated.
We'll see shortly how it ends up being called.
The function should look like this:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Implementation of spam.system
   :end-before: ///

Let's go through this line by line.

This function will not be needed outside the current ``.c`` file, so we will
define it as ``static``.
(This can prevent name conflicts with similarly named functions elsewhere.)
It will return a pointer to a Python object -- the result that Python code
should receive when it calls ``spam.system``:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: static PyObject *
   :end-at: static PyObject *

We'll name the function ``spam_system``.
Combining the module name and the function name like this is standard practice,
and avoids clashes with other uses of ``system``.

The Python function we are defining will take a single argument.
Its C implementation takes two arguments, here named *self* and *arg*,
both pointers to a Python object:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: spam_system(PyObject *self, PyObject *arg)
   :end-at: {

Since this is a module-level function, the *self* argument will point to
a module object.
For a method, *self* would point to the object instance, like a *self*
argument in Python.

The *arg* will be a pointer to a Python object that the Python function
received as its single argument.

In order for our C code to use the information in a Python object, we will need
to convert it to a C value --- in this case, a C string (``char *``).
The :c:func:`PyUnicode_AsUTF8` function does just that: it decodes a Python
string to a C one:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: const char *command = PyUnicode_AsUTF8(arg);
   :end-at: const char *command = PyUnicode_AsUTF8(arg);

If :c:func:`PyUnicode_AsUTF8` was successful, the string value of the argument
has been stored in the local variable *command*.
This is a C string, that is, a pointer to an array of bytes.
You are not supposed to modify the string to which it points,
which is why the variable is declared as ``const char *command``.

.. note::

   As its name implies, :c:func:`PyUnicode_AsUTF8` uses the UTF-8 encoding
   --- the same that :py:meth:`str.decode` uses by default.
   This is not always the correct encoding to use for system commands, but
   it will do for our purposes.

If :c:func:`PyUnicode_AsUTF8` was *not* successful, it returns a ``NULL``
pointer.
When using the Python C API, we always need to handle such error cases.
Let's study this one in a bit more detail.

Errors and exceptions
---------------------

An important convention throughout the Python interpreter is the following:
when a function fails, it should do two things: ensure an exception is set,
and return an error value.

The error value is usually ``-1`` for functions that return :c:type:`!int`,
and ``NULL`` for functions that return a pointer.
However, this convention is not used in *all* cases.
You should always check the documentation for the functions you use.

"Setting an exception"  means that an exception object is stored in the
interpreter's thread state.
(If you are not familiar with threads, think of it like a global variable.)

In our example, :c:func:`PyUnicode_AsUTF8` follows the convention:
on failure, it sets an exception and returns ``NULL``.

Our function needs to detect this, and also return the ``NULL``
error indicator:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: if (command == NULL) {
   :end-at: }

Our function does not neeed to set an exception itself, since
:c:func:`PyUnicode_AsUTF8` has already set it in the error case.

In the non-error case, the ``return`` statement is skipped and execution
continues.

.. note::

   If we were calling a function that does *not* follow the Python convention
   (for example, :c:func:`malloc`), we would need to set an exception using,
   for example, :c:func:`PyErr_SetString`.


Back to the example
-------------------

The next statement is a call to the standard C function :c:func:`system`,
passing it the C string we just got, and storing its result --- a C
integer:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: int status = system(command);
   :end-at: int status = system(command);

Our function must then convert the C integer into a Python integer.
This is done using the function :c:func:`PyLong_FromLong`:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: return PyLong_FromLong(status);
   :end-at: return PyLong_FromLong(status);

In this case, it will return a Python :py:class:`int` object.
(Yes, even integers are objects on the heap in Python!)
We return this object directly, as the result of our function.

Note that when :c:func:`PyLong_FromLong` fails, it sets an exception and
returns ``NULL``, just like :c:func:`PyUnicode_AsUTF8` we used before.
In this case, our function should --- also like before --- return ``NULL``
as well.
In both cases --- on success and on error --- we are returning
:c:func:`PyLong_FromLong`'s result, so we do not need an ``if`` here.

.. note::

   The names used in the C API are peculiar:

   * ``PyUnicode`` is used for Python :py:type:`str`
   * ``PyLong`` is used for Python :py:type:`int`

   This convention dates back to the time when Python strings were implemented
   by what is now :py:type:`bytes`, and integers were restricted to 64 bits
   or so.
   Unicode strings (then named :py:class:`!unicode`) and arbitrarily large
   integers (then named :py:class:`!long`) were added later,
   and in time, were renamed to :py:type:`str` and :py:type:`int` we know
   today.

   However, the C API retains the old naming for backwards compatibility.

   Thus, the :c:func:`PyLong_FromLong` function creates a ``PyLong``
   (Python :py:class:`int`) object from a C :c:type:`!long`.
   (C converts from C :c:type:`!int` to C :c:type:`!long` automatically.)
   And the :c:func:`PyUnicode_AsUTF8` function represents a ``PyUnicode``
   (Python :py:class:`str`) object as a UTF-8-encoded, NUL-terminatedC string.

That is it for the C implementation of the ``spam.function``!

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-at: }
   :end-at: }

But, we are not done yet: we still need to define how the implementation
is called from Python code.


The module's method table
=========================

To make our function available to Python programs,
we need to list it in a "method table" -- an array of :c:type:`PyMethodDef`
structures with a zero-filled *sentinel*
marking the end of the array.

.. note:: Why not "PyFunctionDef"?

   The C API uses a common mechanism to define both functions of modules
   and methods of classes.
   You might say that, in the C API, module-level functions act like methods
   of a module object.

Since we are defining a single function, the array will have a single entry
before the sentinel:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Module method table
   :end-before: ///

The :c:member:`!PyMethodDef.ml_name` and :c:member:`!PyMethodDef.ml_meth`
fields contain the Python name of the function, and the address of
the implementation.

The :c:member:`!PyMethodDef.ml_flags` field tells the interpreter
the calling convention to be used for the C function.
In particular, :c:data:`METH_O` specifies that this function takes a single
unnamed (positional-only) argument.
This is among the simplest, but least powerful, calling conventions.
Perfect for your first function!

Finally, :c:member:`!PyMethodDef.ml_doc` gives the docstring.
If this is left out (or ``NULL``), the function will not have a docstring.
The :c:func:`PyDoc_STR` macro is used to save a bit of memory when building
without docstrings.


ABI information
===============

Before defining the module itself, we'll add one more piece of boilerplate:
a variable with ABI compatibility information.
This is a variable that CPython checks to avoid loading incompatible modules
--- it allows raising an exception rather than crashing (or worse)
if we, for example, load the extension on an incompatible Python version.

The ABI information defined using a macro that collects the relevant
build-time settings:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// ABI information
   :end-before: ///

This macro expands to a variable definition like
``static PyABIInfo abi_info = { ... data ... };``


The slot table
==============

Now, let's fit all the pieces of our module together.

The method table we just defined, ``spam_methods``,
must be referenced in the module definition table:
an array of :c:type:`PyModuleDef_Slot` structures.
Like with the method table, a zero-filled *sentinel* marks the end.

Besides the method table, this "slot table" will contain the module's
top-level information: the name, a docstring, and the ABI compatibility
information we've just defined:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Module slot table
   :end-before: ///


Module export hook
==================

The :c:type:`PyModuleDef_Slot` array must be passed to the interpreter in the
module's :ref:`export hook <extension-export-hook>` function.
The hook must be named :c:func:`!PyModExport_name`, where *name* is the name
of the module, and it should be the only non-\ ``static`` item defined in the
module file.

Several modern C compilers emit warnings when you define a public function
without a prototype declaration.
Normally, prototypes go in a header file so they can be used from other
C files.
A function without a prototype is allowed in C, but is normally a strong
hint that something is wrong -- for example, the name is misspelled.

In our case, Python will load our function dynamically, so a prototype
isn't needed.
We'll only add one to avoid well-meaning compiler warnings:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Export hook prototype
   :end-before: ///

The :c:macro:`PyMODEXPORT_FUNC` macro declares the function's
``PyModuleDef_Slot *`` return type, and adds any special linkage
declarations required by the platform.
For C++, it declares the function as ``extern "C"``.

Just after the prototype, we'll add the function itself.
Its only job is to return the slot array, which in turn contains
all the information needed to create our module object:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Module export hook
   :end-before: ///


Legacy initialization function
==============================

Lastly, we will add a function that serves no purpose but to detect
installation mistakes, and to avoid errors with older versions of some common
build tools.
You may want to try building the extension without this, and only add it
if you encounter errors involving ``PyInit``:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
   :start-after: /// Legacy initialization function

This is an old-style :ref:`initialization function <extension-export-hook>`
that was required in extension modules for CPython 3.14 and below.
Current CPython will not call it, but some build tools may still assume that
all extension modules need to define it.

When called, it raises a :py:exc:`SystemError` exception using
:c:func:`PyErr_SetString`, and returns the error indicator, ``NULL``.
Thus, if this extension does end up loaded on Python 3.14, the user will
get a proper error message.


.. _first-extension-result:

End of file
===========

That's it!
You have written a simple Python C API extension module.
The result is repeated below.

Now, you'll need to compile it.
We'll see how to do this in :ref:`building`.

Here is the entire source file, for your convenience:

.. _extending-spammodule-source:

.. literalinclude:: ../includes/capi-extension/spammodule-01.c
