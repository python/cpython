.. highlightlang:: c


.. _extending-intro:

******************************
Extending Python with C or C++
******************************

It is quite easy to add new built-in modules to Python, if you know how to
program in C.  Such :dfn:`extension modules` can do two things that can't be
done directly in Python: they can implement new built-in object types, and they
can call C library functions and system calls.

To support extensions, the Python API (Application Programmers Interface)
defines a set of functions, macros and variables that provide access to most
aspects of the Python run-time system.  The Python API is incorporated in a C
source file by including the header ``"Python.h"``.

The compilation of an extension module depends on its intended use as well as on
your system setup; details are given in later chapters.


.. _extending-simpleexample:

A Simple Example
================

Let's create an extension module called ``spam`` (the favorite food of Monty
Python fans...) and let's say we want to create a Python interface to the C
library function :cfunc:`system`. [#]_ This function takes a null-terminated
character string as argument and returns an integer.  We want this function to
be callable from Python as follows::

   >>> import spam
   >>> status = spam.system("ls -l")

Begin by creating a file :file:`spammodule.c`.  (Historically, if a module is
called ``spam``, the C file containing its implementation is called
:file:`spammodule.c`; if the module name is very long, like ``spammify``, the
module name can be just :file:`spammify.c`.)

The first line of our file can be::

   #include <Python.h>

which pulls in the Python API (you can add a comment describing the purpose of
the module and a copyright notice if you like).

.. warning::

   Since Python may define some pre-processor definitions which affect the standard
   headers on some systems, you *must* include :file:`Python.h` before any standard
   headers are included.

All user-visible symbols defined by :file:`Python.h` have a prefix of ``Py`` or
``PY``, except those defined in standard header files. For convenience, and
since they are used extensively by the Python interpreter, ``"Python.h"``
includes a few standard header files: ``<stdio.h>``, ``<string.h>``,
``<errno.h>``, and ``<stdlib.h>``.  If the latter header file does not exist on
your system, it declares the functions :cfunc:`malloc`, :cfunc:`free` and
:cfunc:`realloc` directly.

The next thing we add to our module file is the C function that will be called
when the Python expression ``spam.system(string)`` is evaluated (we'll see
shortly how it ends up being called)::

   static PyObject *
   spam_system(PyObject *self, PyObject *args)
   {
       const char *command;
       int sts;

       if (!PyArg_ParseTuple(args, "s", &command))
           return NULL;
       sts = system(command);
       return Py_BuildValue("i", sts);
   }

There is a straightforward translation from the argument list in Python (for
example, the single expression ``"ls -l"``) to the arguments passed to the C
function.  The C function always has two arguments, conventionally named *self*
and *args*.

The *self* argument is only used when the C function implements a built-in
method, not a function. In the example, *self* will always be a *NULL* pointer,
since we are defining a function, not a method.  (This is done so that the
interpreter doesn't have to understand two different types of C functions.)

The *args* argument will be a pointer to a Python tuple object containing the
arguments.  Each item of the tuple corresponds to an argument in the call's
argument list.  The arguments are Python objects --- in order to do anything
with them in our C function we have to convert them to C values.  The function
:cfunc:`PyArg_ParseTuple` in the Python API checks the argument types and
converts them to C values.  It uses a template string to determine the required
types of the arguments as well as the types of the C variables into which to
store the converted values.  More about this later.

:cfunc:`PyArg_ParseTuple` returns true (nonzero) if all arguments have the right
type and its components have been stored in the variables whose addresses are
passed.  It returns false (zero) if an invalid argument list was passed.  In the
latter case it also raises an appropriate exception so the calling function can
return *NULL* immediately (as we saw in the example).


.. _extending-errors:

Intermezzo: Errors and Exceptions
=================================

An important convention throughout the Python interpreter is the following: when
a function fails, it should set an exception condition and return an error value
(usually a *NULL* pointer).  Exceptions are stored in a static global variable
inside the interpreter; if this variable is *NULL* no exception has occurred.  A
second global variable stores the "associated value" of the exception (the
second argument to :keyword:`raise`).  A third variable contains the stack
traceback in case the error originated in Python code.  These three variables
are the C equivalents of the result in Python of :meth:`sys.exc_info` (see the
section on module :mod:`sys` in the Python Library Reference).  It is important
to know about them to understand how errors are passed around.

The Python API defines a number of functions to set various types of exceptions.

The most common one is :cfunc:`PyErr_SetString`.  Its arguments are an exception
object and a C string.  The exception object is usually a predefined object like
:cdata:`PyExc_ZeroDivisionError`.  The C string indicates the cause of the error
and is converted to a Python string object and stored as the "associated value"
of the exception.

Another useful function is :cfunc:`PyErr_SetFromErrno`, which only takes an
exception argument and constructs the associated value by inspection of the
global variable :cdata:`errno`.  The most general function is
:cfunc:`PyErr_SetObject`, which takes two object arguments, the exception and
its associated value.  You don't need to :cfunc:`Py_INCREF` the objects passed
to any of these functions.

You can test non-destructively whether an exception has been set with
:cfunc:`PyErr_Occurred`.  This returns the current exception object, or *NULL*
if no exception has occurred.  You normally don't need to call
:cfunc:`PyErr_Occurred` to see whether an error occurred in a function call,
since you should be able to tell from the return value.

When a function *f* that calls another function *g* detects that the latter
fails, *f* should itself return an error value (usually *NULL* or ``-1``).  It
should *not* call one of the :cfunc:`PyErr_\*` functions --- one has already
been called by *g*. *f*'s caller is then supposed to also return an error
indication to *its* caller, again *without* calling :cfunc:`PyErr_\*`, and so on
--- the most detailed cause of the error was already reported by the function
that first detected it.  Once the error reaches the Python interpreter's main
loop, this aborts the currently executing Python code and tries to find an
exception handler specified by the Python programmer.

(There are situations where a module can actually give a more detailed error
message by calling another :cfunc:`PyErr_\*` function, and in such cases it is
fine to do so.  As a general rule, however, this is not necessary, and can cause
information about the cause of the error to be lost: most operations can fail
for a variety of reasons.)

To ignore an exception set by a function call that failed, the exception
condition must be cleared explicitly by calling :cfunc:`PyErr_Clear`.  The only
time C code should call :cfunc:`PyErr_Clear` is if it doesn't want to pass the
error on to the interpreter but wants to handle it completely by itself
(possibly by trying something else, or pretending nothing went wrong).

Every failing :cfunc:`malloc` call must be turned into an exception --- the
direct caller of :cfunc:`malloc` (or :cfunc:`realloc`) must call
:cfunc:`PyErr_NoMemory` and return a failure indicator itself.  All the
object-creating functions (for example, :cfunc:`PyLong_FromLong`) already do
this, so this note is only relevant to those who call :cfunc:`malloc` directly.

Also note that, with the important exception of :cfunc:`PyArg_ParseTuple` and
friends, functions that return an integer status usually return a positive value
or zero for success and ``-1`` for failure, like Unix system calls.

Finally, be careful to clean up garbage (by making :cfunc:`Py_XDECREF` or
:cfunc:`Py_DECREF` calls for objects you have already created) when you return
an error indicator!

The choice of which exception to raise is entirely yours.  There are predeclared
C objects corresponding to all built-in Python exceptions, such as
:cdata:`PyExc_ZeroDivisionError`, which you can use directly. Of course, you
should choose exceptions wisely --- don't use :cdata:`PyExc_TypeError` to mean
that a file couldn't be opened (that should probably be :cdata:`PyExc_IOError`).
If something's wrong with the argument list, the :cfunc:`PyArg_ParseTuple`
function usually raises :cdata:`PyExc_TypeError`.  If you have an argument whose
value must be in a particular range or must satisfy other conditions,
:cdata:`PyExc_ValueError` is appropriate.

You can also define a new exception that is unique to your module. For this, you
usually declare a static object variable at the beginning of your file::

   static PyObject *SpamError;

and initialize it in your module's initialization function (:cfunc:`initspam`)
with an exception object (leaving out the error checking for now)::

   PyMODINIT_FUNC
   initspam(void)
   {
       PyObject *m;

       m = Py_InitModule("spam", SpamMethods);
       if (m == NULL)
           return;

       SpamError = PyErr_NewException("spam.error", NULL, NULL);
       Py_INCREF(SpamError);
       PyModule_AddObject(m, "error", SpamError);
   }

Note that the Python name for the exception object is :exc:`spam.error`.  The
:cfunc:`PyErr_NewException` function may create a class with the base class
being :exc:`Exception` (unless another class is passed in instead of *NULL*),
described in :ref:`bltin-exceptions`.

Note also that the :cdata:`SpamError` variable retains a reference to the newly
created exception class; this is intentional!  Since the exception could be
removed from the module by external code, an owned reference to the class is
needed to ensure that it will not be discarded, causing :cdata:`SpamError` to
become a dangling pointer. Should it become a dangling pointer, C code which
raises the exception could cause a core dump or other unintended side effects.

We discuss the use of PyMODINIT_FUNC as a function return type later in this
sample.


.. _backtoexample:

Back to the Example
===================

Going back to our example function, you should now be able to understand this
statement::

   if (!PyArg_ParseTuple(args, "s", &command))
       return NULL;

It returns *NULL* (the error indicator for functions returning object pointers)
if an error is detected in the argument list, relying on the exception set by
:cfunc:`PyArg_ParseTuple`.  Otherwise the string value of the argument has been
copied to the local variable :cdata:`command`.  This is a pointer assignment and
you are not supposed to modify the string to which it points (so in Standard C,
the variable :cdata:`command` should properly be declared as ``const char
*command``).

The next statement is a call to the Unix function :cfunc:`system`, passing it
the string we just got from :cfunc:`PyArg_ParseTuple`::

   sts = system(command);

Our :func:`spam.system` function must return the value of :cdata:`sts` as a
Python object.  This is done using the function :cfunc:`Py_BuildValue`, which is
something like the inverse of :cfunc:`PyArg_ParseTuple`: it takes a format
string and an arbitrary number of C values, and returns a new Python object.
More info on :cfunc:`Py_BuildValue` is given later. ::

   return Py_BuildValue("i", sts);

In this case, it will return an integer object.  (Yes, even integers are objects
on the heap in Python!)

If you have a C function that returns no useful argument (a function returning
:ctype:`void`), the corresponding Python function must return ``None``.   You
need this idiom to do so (which is implemented by the :cmacro:`Py_RETURN_NONE`
macro)::

   Py_INCREF(Py_None);
   return Py_None;

:cdata:`Py_None` is the C name for the special Python object ``None``.  It is a
genuine Python object rather than a *NULL* pointer, which means "error" in most
contexts, as we have seen.


.. _methodtable:

The Module's Method Table and Initialization Function
=====================================================

I promised to show how :cfunc:`spam_system` is called from Python programs.
First, we need to list its name and address in a "method table"::

   static PyMethodDef SpamMethods[] = {
       ...
       {"system",  spam_system, METH_VARARGS,
        "Execute a shell command."},
       ...
       {NULL, NULL, 0, NULL}        /* Sentinel */
   };

Note the third entry (``METH_VARARGS``).  This is a flag telling the interpreter
the calling convention to be used for the C function.  It should normally always
be ``METH_VARARGS`` or ``METH_VARARGS | METH_KEYWORDS``; a value of ``0`` means
that an obsolete variant of :cfunc:`PyArg_ParseTuple` is used.

When using only ``METH_VARARGS``, the function should expect the Python-level
parameters to be passed in as a tuple acceptable for parsing via
:cfunc:`PyArg_ParseTuple`; more information on this function is provided below.

The :const:`METH_KEYWORDS` bit may be set in the third field if keyword
arguments should be passed to the function.  In this case, the C function should
accept a third ``PyObject *`` parameter which will be a dictionary of keywords.
Use :cfunc:`PyArg_ParseTupleAndKeywords` to parse the arguments to such a
function.

The method table must be passed to the interpreter in the module's
initialization function.  The initialization function must be named
:cfunc:`initname`, where *name* is the name of the module, and should be the
only non-\ :keyword:`static` item defined in the module file::

   PyMODINIT_FUNC
   initspam(void)
   {
       (void) Py_InitModule("spam", SpamMethods);
   }

Note that PyMODINIT_FUNC declares the function as ``void`` return type,
declares any special linkage declarations required by the platform, and for  C++
declares the function as ``extern "C"``.

When the Python program imports module :mod:`spam` for the first time,
:cfunc:`initspam` is called. (See below for comments about embedding Python.)
It calls :cfunc:`Py_InitModule`, which creates a "module object" (which is
inserted in the dictionary ``sys.modules`` under the key ``"spam"``), and
inserts built-in function objects into the newly created module based upon the
table (an array of :ctype:`PyMethodDef` structures) that was passed as its
second argument. :cfunc:`Py_InitModule` returns a pointer to the module object
that it creates (which is unused here).  It may abort with a fatal error for
certain errors, or return *NULL* if the module could not be initialized
satisfactorily.

When embedding Python, the :cfunc:`initspam` function is not called
automatically unless there's an entry in the :cdata:`_PyImport_Inittab` table.
The easiest way to handle this is to statically initialize your
statically-linked modules by directly calling :cfunc:`initspam` after the call
to :cfunc:`Py_Initialize`::

   int
   main(int argc, char *argv[])
   {
       /* Pass argv[0] to the Python interpreter */
       Py_SetProgramName(argv[0]);

       /* Initialize the Python interpreter.  Required. */
       Py_Initialize();

       /* Add a static module */
       initspam();

An example may be found in the file :file:`Demo/embed/demo.c` in the Python
source distribution.

.. note::

   Removing entries from ``sys.modules`` or importing compiled modules into
   multiple interpreters within a process (or following a :cfunc:`fork` without an
   intervening :cfunc:`exec`) can create problems for some extension modules.
   Extension module authors should exercise caution when initializing internal data
   structures.

A more substantial example module is included in the Python source distribution
as :file:`Modules/xxmodule.c`.  This file may be used as a  template or simply
read as an example.  The :program:`modulator.py` script included in the source
distribution or Windows install provides  a simple graphical user interface for
declaring the functions and objects which a module should implement, and can
generate a template which can be filled in.  The script lives in the
:file:`Tools/modulator/` directory; see the :file:`README` file there for more
information.


.. _compilation:

Compilation and Linkage
=======================

There are two more things to do before you can use your new extension: compiling
and linking it with the Python system.  If you use dynamic loading, the details
may depend on the style of dynamic loading your system uses; see the chapters
about building extension modules (chapter :ref:`building`) and additional
information that pertains only to building on Windows (chapter
:ref:`building-on-windows`) for more information about this.

If you can't use dynamic loading, or if you want to make your module a permanent
part of the Python interpreter, you will have to change the configuration setup
and rebuild the interpreter.  Luckily, this is very simple on Unix: just place
your file (:file:`spammodule.c` for example) in the :file:`Modules/` directory
of an unpacked source distribution, add a line to the file
:file:`Modules/Setup.local` describing your file::

   spam spammodule.o

and rebuild the interpreter by running :program:`make` in the toplevel
directory.  You can also run :program:`make` in the :file:`Modules/`
subdirectory, but then you must first rebuild :file:`Makefile` there by running
':program:`make` Makefile'.  (This is necessary each time you change the
:file:`Setup` file.)

If your module requires additional libraries to link with, these can be listed
on the line in the configuration file as well, for instance::

   spam spammodule.o -lX11


.. _callingpython:

Calling Python Functions from C
===============================

So far we have concentrated on making C functions callable from Python.  The
reverse is also useful: calling Python functions from C. This is especially the
case for libraries that support so-called "callback" functions.  If a C
interface makes use of callbacks, the equivalent Python often needs to provide a
callback mechanism to the Python programmer; the implementation will require
calling the Python callback functions from a C callback.  Other uses are also
imaginable.

Fortunately, the Python interpreter is easily called recursively, and there is a
standard interface to call a Python function.  (I won't dwell on how to call the
Python parser with a particular string as input --- if you're interested, have a
look at the implementation of the :option:`-c` command line option in
:file:`Modules/main.c` from the Python source code.)

Calling a Python function is easy.  First, the Python program must somehow pass
you the Python function object.  You should provide a function (or some other
interface) to do this.  When this function is called, save a pointer to the
Python function object (be careful to :cfunc:`Py_INCREF` it!) in a global
variable --- or wherever you see fit. For example, the following function might
be part of a module definition::

   static PyObject *my_callback = NULL;

   static PyObject *
   my_set_callback(PyObject *dummy, PyObject *args)
   {
       PyObject *result = NULL;
       PyObject *temp;

       if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
           if (!PyCallable_Check(temp)) {
               PyErr_SetString(PyExc_TypeError, "parameter must be callable");
               return NULL;
           }
           Py_XINCREF(temp);         /* Add a reference to new callback */
           Py_XDECREF(my_callback);  /* Dispose of previous callback */
           my_callback = temp;       /* Remember new callback */
           /* Boilerplate to return "None" */
           Py_INCREF(Py_None);
           result = Py_None;
       }
       return result;
   }

This function must be registered with the interpreter using the
:const:`METH_VARARGS` flag; this is described in section :ref:`methodtable`.  The
:cfunc:`PyArg_ParseTuple` function and its arguments are documented in section
:ref:`parsetuple`.

The macros :cfunc:`Py_XINCREF` and :cfunc:`Py_XDECREF` increment/decrement the
reference count of an object and are safe in the presence of *NULL* pointers
(but note that *temp* will not be  *NULL* in this context).  More info on them
in section :ref:`refcounts`.

.. index:: single: PyEval_CallObject()

Later, when it is time to call the function, you call the C function
:cfunc:`PyEval_CallObject`.  This function has two arguments, both pointers to
arbitrary Python objects: the Python function, and the argument list.  The
argument list must always be a tuple object, whose length is the number of
arguments.  To call the Python function with no arguments, pass in NULL, or 
an empty tuple; to call it with one argument, pass a singleton tuple.
:cfunc:`Py_BuildValue` returns a tuple when its format string consists of zero
or more format codes between parentheses.  For example::

   int arg;
   PyObject *arglist;
   PyObject *result;
   ...
   arg = 123;
   ...
   /* Time to call the callback */
   arglist = Py_BuildValue("(i)", arg);
   result = PyEval_CallObject(my_callback, arglist);
   Py_DECREF(arglist);

:cfunc:`PyEval_CallObject` returns a Python object pointer: this is the return
value of the Python function.  :cfunc:`PyEval_CallObject` is
"reference-count-neutral" with respect to its arguments.  In the example a new
tuple was created to serve as the argument list, which is :cfunc:`Py_DECREF`\
-ed immediately after the call.

The return value of :cfunc:`PyEval_CallObject` is "new": either it is a brand
new object, or it is an existing object whose reference count has been
incremented.  So, unless you want to save it in a global variable, you should
somehow :cfunc:`Py_DECREF` the result, even (especially!) if you are not
interested in its value.

Before you do this, however, it is important to check that the return value
isn't *NULL*.  If it is, the Python function terminated by raising an exception.
If the C code that called :cfunc:`PyEval_CallObject` is called from Python, it
should now return an error indication to its Python caller, so the interpreter
can print a stack trace, or the calling Python code can handle the exception.
If this is not possible or desirable, the exception should be cleared by calling
:cfunc:`PyErr_Clear`.  For example::

   if (result == NULL)
       return NULL; /* Pass error back */
   ...use result...
   Py_DECREF(result); 

Depending on the desired interface to the Python callback function, you may also
have to provide an argument list to :cfunc:`PyEval_CallObject`.  In some cases
the argument list is also provided by the Python program, through the same
interface that specified the callback function.  It can then be saved and used
in the same manner as the function object.  In other cases, you may have to
construct a new tuple to pass as the argument list.  The simplest way to do this
is to call :cfunc:`Py_BuildValue`.  For example, if you want to pass an integral
event code, you might use the following code::

   PyObject *arglist;
   ...
   arglist = Py_BuildValue("(l)", eventcode);
   result = PyEval_CallObject(my_callback, arglist);
   Py_DECREF(arglist);
   if (result == NULL)
       return NULL; /* Pass error back */
   /* Here maybe use the result */
   Py_DECREF(result);

Note the placement of ``Py_DECREF(arglist)`` immediately after the call, before
the error check!  Also note that strictly speaking this code is not complete:
:cfunc:`Py_BuildValue` may run out of memory, and this should be checked.

You may also call a function with keyword arguments by using 
:cfunc:`PyEval_CallObjectWithKeywords`.  As in the above example, we use
:cfunc:`Py_BuildValue` to construct the dictionary. ::

   PyObject *dict;
   ...
   dict = Py_BuildValue("{s:i}", "name", val);
   result = PyEval_CallObjectWithKeywords(my_callback, NULL, dict);
   Py_DECREF(dict);
   if (result == NULL)
       return NULL; /* Pass error back */
   /* Here maybe use the result */
   Py_DECREF(result);

.. _parsetuple:

Extracting Parameters in Extension Functions
============================================

.. index:: single: PyArg_ParseTuple()

The :cfunc:`PyArg_ParseTuple` function is declared as follows::

   int PyArg_ParseTuple(PyObject *arg, char *format, ...);

The *arg* argument must be a tuple object containing an argument list passed
from Python to a C function.  The *format* argument must be a format string,
whose syntax is explained in :ref:`arg-parsing` in the Python/C API Reference
Manual.  The remaining arguments must be addresses of variables whose type is
determined by the format string.

Note that while :cfunc:`PyArg_ParseTuple` checks that the Python arguments have
the required types, it cannot check the validity of the addresses of C variables
passed to the call: if you make mistakes there, your code will probably crash or
at least overwrite random bits in memory.  So be careful!

Note that any Python object references which are provided to the caller are
*borrowed* references; do not decrement their reference count!

Some example calls::

   int ok;
   int i, j;
   long k, l;
   const char *s;
   int size;

   ok = PyArg_ParseTuple(args, ""); /* No arguments */
       /* Python call: f() */

::

   ok = PyArg_ParseTuple(args, "s", &s); /* A string */
       /* Possible Python call: f('whoops!') */

::

   ok = PyArg_ParseTuple(args, "lls", &k, &l, &s); /* Two longs and a string */
       /* Possible Python call: f(1, 2, 'three') */

::

   ok = PyArg_ParseTuple(args, "(ii)s#", &i, &j, &s, &size);
       /* A pair of ints and a string, whose size is also returned */
       /* Possible Python call: f((1, 2), 'three') */

::

   {
       const char *file;
       const char *mode = "r";
       int bufsize = 0;
       ok = PyArg_ParseTuple(args, "s|si", &file, &mode, &bufsize);
       /* A string, and optionally another string and an integer */
       /* Possible Python calls:
          f('spam')
          f('spam', 'w')
          f('spam', 'wb', 100000) */
   }

::

   {
       int left, top, right, bottom, h, v;
       ok = PyArg_ParseTuple(args, "((ii)(ii))(ii)",
                &left, &top, &right, &bottom, &h, &v);
       /* A rectangle and a point */
       /* Possible Python call:
          f(((0, 0), (400, 300)), (10, 10)) */
   }

::

   {
       Py_complex c;
       ok = PyArg_ParseTuple(args, "D:myfunction", &c);
       /* a complex, also providing a function name for errors */
       /* Possible Python call: myfunction(1+2j) */
   }


.. _parsetupleandkeywords:

Keyword Parameters for Extension Functions
==========================================

.. index:: single: PyArg_ParseTupleAndKeywords()

The :cfunc:`PyArg_ParseTupleAndKeywords` function is declared as follows::

   int PyArg_ParseTupleAndKeywords(PyObject *arg, PyObject *kwdict,
                                   char *format, char *kwlist[], ...);

The *arg* and *format* parameters are identical to those of the
:cfunc:`PyArg_ParseTuple` function.  The *kwdict* parameter is the dictionary of
keywords received as the third parameter from the Python runtime.  The *kwlist*
parameter is a *NULL*-terminated list of strings which identify the parameters;
the names are matched with the type information from *format* from left to
right.  On success, :cfunc:`PyArg_ParseTupleAndKeywords` returns true, otherwise
it returns false and raises an appropriate exception.

.. note::

   Nested tuples cannot be parsed when using keyword arguments!  Keyword parameters
   passed in which are not present in the *kwlist* will cause :exc:`TypeError` to
   be raised.

.. index:: single: Philbrick, Geoff

Here is an example module which uses keywords, based on an example by Geoff
Philbrick (philbrick@hks.com):

.. % 

::

   #include "Python.h"

   static PyObject *
   keywdarg_parrot(PyObject *self, PyObject *args, PyObject *keywds)
   {  
       int voltage;
       char *state = "a stiff";
       char *action = "voom";
       char *type = "Norwegian Blue";

       static char *kwlist[] = {"voltage", "state", "action", "type", NULL};

       if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|sss", kwlist, 
                                        &voltage, &state, &action, &type))
           return NULL; 

       printf("-- This parrot wouldn't %s if you put %i Volts through it.\n", 
              action, voltage);
       printf("-- Lovely plumage, the %s -- It's %s!\n", type, state);

       Py_INCREF(Py_None);

       return Py_None;
   }

   static PyMethodDef keywdarg_methods[] = {
       /* The cast of the function is necessary since PyCFunction values
        * only take two PyObject* parameters, and keywdarg_parrot() takes
        * three.
        */
       {"parrot", (PyCFunction)keywdarg_parrot, METH_VARARGS | METH_KEYWORDS,
        "Print a lovely skit to standard output."},
       {NULL, NULL, 0, NULL}   /* sentinel */
   };

::

   void
   initkeywdarg(void)
   {
     /* Create the module and add the functions */
     Py_InitModule("keywdarg", keywdarg_methods);
   }


.. _buildvalue:

Building Arbitrary Values
=========================

This function is the counterpart to :cfunc:`PyArg_ParseTuple`.  It is declared
as follows::

   PyObject *Py_BuildValue(char *format, ...);

It recognizes a set of format units similar to the ones recognized by
:cfunc:`PyArg_ParseTuple`, but the arguments (which are input to the function,
not output) must not be pointers, just values.  It returns a new Python object,
suitable for returning from a C function called from Python.

One difference with :cfunc:`PyArg_ParseTuple`: while the latter requires its
first argument to be a tuple (since Python argument lists are always represented
as tuples internally), :cfunc:`Py_BuildValue` does not always build a tuple.  It
builds a tuple only if its format string contains two or more format units. If
the format string is empty, it returns ``None``; if it contains exactly one
format unit, it returns whatever object is described by that format unit.  To
force it to return a tuple of size 0 or one, parenthesize the format string.

Examples (to the left the call, to the right the resulting Python value)::

   Py_BuildValue("")                        None
   Py_BuildValue("i", 123)                  123
   Py_BuildValue("iii", 123, 456, 789)      (123, 456, 789)
   Py_BuildValue("s", "hello")              'hello'
   Py_BuildValue("y", "hello")              b'hello'
   Py_BuildValue("ss", "hello", "world")    ('hello', 'world')
   Py_BuildValue("s#", "hello", 4)          'hell'
   Py_BuildValue("y#", "hello", 4)          b'hell'
   Py_BuildValue("()")                      ()
   Py_BuildValue("(i)", 123)                (123,)
   Py_BuildValue("(ii)", 123, 456)          (123, 456)
   Py_BuildValue("(i,i)", 123, 456)         (123, 456)
   Py_BuildValue("[i,i]", 123, 456)         [123, 456]
   Py_BuildValue("{s:i,s:i}",
                 "abc", 123, "def", 456)    {'abc': 123, 'def': 456}
   Py_BuildValue("((ii)(ii)) (ii)",
                 1, 2, 3, 4, 5, 6)          (((1, 2), (3, 4)), (5, 6))


.. _refcounts:

Reference Counts
================

In languages like C or C++, the programmer is responsible for dynamic allocation
and deallocation of memory on the heap.  In C, this is done using the functions
:cfunc:`malloc` and :cfunc:`free`.  In C++, the operators :keyword:`new` and
:keyword:`delete` are used with essentially the same meaning and we'll restrict
the following discussion to the C case.

Every block of memory allocated with :cfunc:`malloc` should eventually be
returned to the pool of available memory by exactly one call to :cfunc:`free`.
It is important to call :cfunc:`free` at the right time.  If a block's address
is forgotten but :cfunc:`free` is not called for it, the memory it occupies
cannot be reused until the program terminates.  This is called a :dfn:`memory
leak`.  On the other hand, if a program calls :cfunc:`free` for a block and then
continues to use the block, it creates a conflict with re-use of the block
through another :cfunc:`malloc` call.  This is called :dfn:`using freed memory`.
It has the same bad consequences as referencing uninitialized data --- core
dumps, wrong results, mysterious crashes.

Common causes of memory leaks are unusual paths through the code.  For instance,
a function may allocate a block of memory, do some calculation, and then free
the block again.  Now a change in the requirements for the function may add a
test to the calculation that detects an error condition and can return
prematurely from the function.  It's easy to forget to free the allocated memory
block when taking this premature exit, especially when it is added later to the
code.  Such leaks, once introduced, often go undetected for a long time: the
error exit is taken only in a small fraction of all calls, and most modern
machines have plenty of virtual memory, so the leak only becomes apparent in a
long-running process that uses the leaking function frequently.  Therefore, it's
important to prevent leaks from happening by having a coding convention or
strategy that minimizes this kind of errors.

Since Python makes heavy use of :cfunc:`malloc` and :cfunc:`free`, it needs a
strategy to avoid memory leaks as well as the use of freed memory.  The chosen
method is called :dfn:`reference counting`.  The principle is simple: every
object contains a counter, which is incremented when a reference to the object
is stored somewhere, and which is decremented when a reference to it is deleted.
When the counter reaches zero, the last reference to the object has been deleted
and the object is freed.

An alternative strategy is called :dfn:`automatic garbage collection`.
(Sometimes, reference counting is also referred to as a garbage collection
strategy, hence my use of "automatic" to distinguish the two.)  The big
advantage of automatic garbage collection is that the user doesn't need to call
:cfunc:`free` explicitly.  (Another claimed advantage is an improvement in speed
or memory usage --- this is no hard fact however.)  The disadvantage is that for
C, there is no truly portable automatic garbage collector, while reference
counting can be implemented portably (as long as the functions :cfunc:`malloc`
and :cfunc:`free` are available --- which the C Standard guarantees). Maybe some
day a sufficiently portable automatic garbage collector will be available for C.
Until then, we'll have to live with reference counts.

While Python uses the traditional reference counting implementation, it also
offers a cycle detector that works to detect reference cycles.  This allows
applications to not worry about creating direct or indirect circular references;
these are the weakness of garbage collection implemented using only reference
counting.  Reference cycles consist of objects which contain (possibly indirect)
references to themselves, so that each object in the cycle has a reference count
which is non-zero.  Typical reference counting implementations are not able to
reclaim the memory belonging to any objects in a reference cycle, or referenced
from the objects in the cycle, even though there are no further references to
the cycle itself.

The cycle detector is able to detect garbage cycles and can reclaim them so long
as there are no finalizers implemented in Python (:meth:`__del__` methods).
When there are such finalizers, the detector exposes the cycles through the
:mod:`gc` module (specifically, the
``garbage`` variable in that module).  The :mod:`gc` module also exposes a way
to run the detector (the :func:`collect` function), as well as configuration
interfaces and the ability to disable the detector at runtime.  The cycle
detector is considered an optional component; though it is included by default,
it can be disabled at build time using the :option:`--without-cycle-gc` option
to the :program:`configure` script on Unix platforms (including Mac OS X) or by
removing the definition of ``WITH_CYCLE_GC`` in the :file:`pyconfig.h` header on
other platforms.  If the cycle detector is disabled in this way, the :mod:`gc`
module will not be available.


.. _refcountsinpython:

Reference Counting in Python
----------------------------

There are two macros, ``Py_INCREF(x)`` and ``Py_DECREF(x)``, which handle the
incrementing and decrementing of the reference count. :cfunc:`Py_DECREF` also
frees the object when the count reaches zero. For flexibility, it doesn't call
:cfunc:`free` directly --- rather, it makes a call through a function pointer in
the object's :dfn:`type object`.  For this purpose (and others), every object
also contains a pointer to its type object.

The big question now remains: when to use ``Py_INCREF(x)`` and ``Py_DECREF(x)``?
Let's first introduce some terms.  Nobody "owns" an object; however, you can
:dfn:`own a reference` to an object.  An object's reference count is now defined
as the number of owned references to it.  The owner of a reference is
responsible for calling :cfunc:`Py_DECREF` when the reference is no longer
needed.  Ownership of a reference can be transferred.  There are three ways to
dispose of an owned reference: pass it on, store it, or call :cfunc:`Py_DECREF`.
Forgetting to dispose of an owned reference creates a memory leak.

It is also possible to :dfn:`borrow` [#]_ a reference to an object.  The
borrower of a reference should not call :cfunc:`Py_DECREF`.  The borrower must
not hold on to the object longer than the owner from which it was borrowed.
Using a borrowed reference after the owner has disposed of it risks using freed
memory and should be avoided completely. [#]_

The advantage of borrowing over owning a reference is that you don't need to
take care of disposing of the reference on all possible paths through the code
--- in other words, with a borrowed reference you don't run the risk of leaking
when a premature exit is taken.  The disadvantage of borrowing over leaking is
that there are some subtle situations where in seemingly correct code a borrowed
reference can be used after the owner from which it was borrowed has in fact
disposed of it.

A borrowed reference can be changed into an owned reference by calling
:cfunc:`Py_INCREF`.  This does not affect the status of the owner from which the
reference was borrowed --- it creates a new owned reference, and gives full
owner responsibilities (the new owner must dispose of the reference properly, as
well as the previous owner).


.. _ownershiprules:

Ownership Rules
---------------

Whenever an object reference is passed into or out of a function, it is part of
the function's interface specification whether ownership is transferred with the
reference or not.

Most functions that return a reference to an object pass on ownership with the
reference.  In particular, all functions whose function it is to create a new
object, such as :cfunc:`PyLong_FromLong` and :cfunc:`Py_BuildValue`, pass
ownership to the receiver.  Even if the object is not actually new, you still
receive ownership of a new reference to that object.  For instance,
:cfunc:`PyLong_FromLong` maintains a cache of popular values and can return a
reference to a cached item.

Many functions that extract objects from other objects also transfer ownership
with the reference, for instance :cfunc:`PyObject_GetAttrString`.  The picture
is less clear, here, however, since a few common routines are exceptions:
:cfunc:`PyTuple_GetItem`, :cfunc:`PyList_GetItem`, :cfunc:`PyDict_GetItem`, and
:cfunc:`PyDict_GetItemString` all return references that you borrow from the
tuple, list or dictionary.

The function :cfunc:`PyImport_AddModule` also returns a borrowed reference, even
though it may actually create the object it returns: this is possible because an
owned reference to the object is stored in ``sys.modules``.

When you pass an object reference into another function, in general, the
function borrows the reference from you --- if it needs to store it, it will use
:cfunc:`Py_INCREF` to become an independent owner.  There are exactly two
important exceptions to this rule: :cfunc:`PyTuple_SetItem` and
:cfunc:`PyList_SetItem`.  These functions take over ownership of the item passed
to them --- even if they fail!  (Note that :cfunc:`PyDict_SetItem` and friends
don't take over ownership --- they are "normal.")

When a C function is called from Python, it borrows references to its arguments
from the caller.  The caller owns a reference to the object, so the borrowed
reference's lifetime is guaranteed until the function returns.  Only when such a
borrowed reference must be stored or passed on, it must be turned into an owned
reference by calling :cfunc:`Py_INCREF`.

The object reference returned from a C function that is called from Python must
be an owned reference --- ownership is transferred from the function to its
caller.


.. _thinice:

Thin Ice
--------

There are a few situations where seemingly harmless use of a borrowed reference
can lead to problems.  These all have to do with implicit invocations of the
interpreter, which can cause the owner of a reference to dispose of it.

The first and most important case to know about is using :cfunc:`Py_DECREF` on
an unrelated object while borrowing a reference to a list item.  For instance::

   void
   bug(PyObject *list)
   {
       PyObject *item = PyList_GetItem(list, 0);

       PyList_SetItem(list, 1, PyLong_FromLong(0L));
       PyObject_Print(item, stdout, 0); /* BUG! */
   }

This function first borrows a reference to ``list[0]``, then replaces
``list[1]`` with the value ``0``, and finally prints the borrowed reference.
Looks harmless, right?  But it's not!

Let's follow the control flow into :cfunc:`PyList_SetItem`.  The list owns
references to all its items, so when item 1 is replaced, it has to dispose of
the original item 1.  Now let's suppose the original item 1 was an instance of a
user-defined class, and let's further suppose that the class defined a
:meth:`__del__` method.  If this class instance has a reference count of 1,
disposing of it will call its :meth:`__del__` method.

Since it is written in Python, the :meth:`__del__` method can execute arbitrary
Python code.  Could it perhaps do something to invalidate the reference to
``item`` in :cfunc:`bug`?  You bet!  Assuming that the list passed into
:cfunc:`bug` is accessible to the :meth:`__del__` method, it could execute a
statement to the effect of ``del list[0]``, and assuming this was the last
reference to that object, it would free the memory associated with it, thereby
invalidating ``item``.

The solution, once you know the source of the problem, is easy: temporarily
increment the reference count.  The correct version of the function reads::

   void
   no_bug(PyObject *list)
   {
       PyObject *item = PyList_GetItem(list, 0);

       Py_INCREF(item);
       PyList_SetItem(list, 1, PyLong_FromLong(0L));
       PyObject_Print(item, stdout, 0);
       Py_DECREF(item);
   }

This is a true story.  An older version of Python contained variants of this bug
and someone spent a considerable amount of time in a C debugger to figure out
why his :meth:`__del__` methods would fail...

The second case of problems with a borrowed reference is a variant involving
threads.  Normally, multiple threads in the Python interpreter can't get in each
other's way, because there is a global lock protecting Python's entire object
space.  However, it is possible to temporarily release this lock using the macro
:cmacro:`Py_BEGIN_ALLOW_THREADS`, and to re-acquire it using
:cmacro:`Py_END_ALLOW_THREADS`.  This is common around blocking I/O calls, to
let other threads use the processor while waiting for the I/O to complete.
Obviously, the following function has the same problem as the previous one::

   void
   bug(PyObject *list)
   {
       PyObject *item = PyList_GetItem(list, 0);
       Py_BEGIN_ALLOW_THREADS
       ...some blocking I/O call...
       Py_END_ALLOW_THREADS
       PyObject_Print(item, stdout, 0); /* BUG! */
   }


.. _nullpointers:

NULL Pointers
-------------

In general, functions that take object references as arguments do not expect you
to pass them *NULL* pointers, and will dump core (or cause later core dumps) if
you do so.  Functions that return object references generally return *NULL* only
to indicate that an exception occurred.  The reason for not testing for *NULL*
arguments is that functions often pass the objects they receive on to other
function --- if each function were to test for *NULL*, there would be a lot of
redundant tests and the code would run more slowly.

It is better to test for *NULL* only at the "source:" when a pointer that may be
*NULL* is received, for example, from :cfunc:`malloc` or from a function that
may raise an exception.

The macros :cfunc:`Py_INCREF` and :cfunc:`Py_DECREF` do not check for *NULL*
pointers --- however, their variants :cfunc:`Py_XINCREF` and :cfunc:`Py_XDECREF`
do.

The macros for checking for a particular object type (``Pytype_Check()``) don't
check for *NULL* pointers --- again, there is much code that calls several of
these in a row to test an object against various different expected types, and
this would generate redundant tests.  There are no variants with *NULL*
checking.

The C function calling mechanism guarantees that the argument list passed to C
functions (``args`` in the examples) is never *NULL* --- in fact it guarantees
that it is always a tuple. [#]_

It is a severe error to ever let a *NULL* pointer "escape" to the Python user.

.. % Frank Stajano:
.. % A pedagogically buggy example, along the lines of the previous listing,
.. % would be helpful here -- showing in more concrete terms what sort of
.. % actions could cause the problem. I can't very well imagine it from the
.. % description.


.. _cplusplus:

Writing Extensions in C++
=========================

It is possible to write extension modules in C++.  Some restrictions apply.  If
the main program (the Python interpreter) is compiled and linked by the C
compiler, global or static objects with constructors cannot be used.  This is
not a problem if the main program is linked by the C++ compiler.  Functions that
will be called by the Python interpreter (in particular, module initialization
functions) have to be declared using ``extern "C"``. It is unnecessary to
enclose the Python header files in ``extern "C" {...}`` --- they use this form
already if the symbol ``__cplusplus`` is defined (all recent C++ compilers
define this symbol).


.. _using-cobjects:

Providing a C API for an Extension Module
=========================================

.. sectionauthor:: Konrad Hinsen <hinsen@cnrs-orleans.fr>


Many extension modules just provide new functions and types to be used from
Python, but sometimes the code in an extension module can be useful for other
extension modules. For example, an extension module could implement a type
"collection" which works like lists without order. Just like the standard Python
list type has a C API which permits extension modules to create and manipulate
lists, this new collection type should have a set of C functions for direct
manipulation from other extension modules.

At first sight this seems easy: just write the functions (without declaring them
:keyword:`static`, of course), provide an appropriate header file, and document
the C API. And in fact this would work if all extension modules were always
linked statically with the Python interpreter. When modules are used as shared
libraries, however, the symbols defined in one module may not be visible to
another module. The details of visibility depend on the operating system; some
systems use one global namespace for the Python interpreter and all extension
modules (Windows, for example), whereas others require an explicit list of
imported symbols at module link time (AIX is one example), or offer a choice of
different strategies (most Unices). And even if symbols are globally visible,
the module whose functions one wishes to call might not have been loaded yet!

Portability therefore requires not to make any assumptions about symbol
visibility. This means that all symbols in extension modules should be declared
:keyword:`static`, except for the module's initialization function, in order to
avoid name clashes with other extension modules (as discussed in section
:ref:`methodtable`). And it means that symbols that *should* be accessible from
other extension modules must be exported in a different way.

Python provides a special mechanism to pass C-level information (pointers) from
one extension module to another one: CObjects. A CObject is a Python data type
which stores a pointer (:ctype:`void \*`).  CObjects can only be created and
accessed via their C API, but they can be passed around like any other Python
object. In particular,  they can be assigned to a name in an extension module's
namespace. Other extension modules can then import this module, retrieve the
value of this name, and then retrieve the pointer from the CObject.

There are many ways in which CObjects can be used to export the C API of an
extension module. Each name could get its own CObject, or all C API pointers
could be stored in an array whose address is published in a CObject. And the
various tasks of storing and retrieving the pointers can be distributed in
different ways between the module providing the code and the client modules.

The following example demonstrates an approach that puts most of the burden on
the writer of the exporting module, which is appropriate for commonly used
library modules. It stores all C API pointers (just one in the example!) in an
array of :ctype:`void` pointers which becomes the value of a CObject. The header
file corresponding to the module provides a macro that takes care of importing
the module and retrieving its C API pointers; client modules only have to call
this macro before accessing the C API.

The exporting module is a modification of the :mod:`spam` module from section
:ref:`extending-simpleexample`. The function :func:`spam.system` does not call
the C library function :cfunc:`system` directly, but a function
:cfunc:`PySpam_System`, which would of course do something more complicated in
reality (such as adding "spam" to every command). This function
:cfunc:`PySpam_System` is also exported to other extension modules.

The function :cfunc:`PySpam_System` is a plain C function, declared
:keyword:`static` like everything else::

   static int
   PySpam_System(const char *command)
   {
       return system(command);
   }

The function :cfunc:`spam_system` is modified in a trivial way::

   static PyObject *
   spam_system(PyObject *self, PyObject *args)
   {
       const char *command;
       int sts;

       if (!PyArg_ParseTuple(args, "s", &command))
           return NULL;
       sts = PySpam_System(command);
       return Py_BuildValue("i", sts);
   }

In the beginning of the module, right after the line ::

   #include "Python.h"

two more lines must be added::

   #define SPAM_MODULE
   #include "spammodule.h"

The ``#define`` is used to tell the header file that it is being included in the
exporting module, not a client module. Finally, the module's initialization
function must take care of initializing the C API pointer array::

   PyMODINIT_FUNC
   initspam(void)
   {
       PyObject *m;
       static void *PySpam_API[PySpam_API_pointers];
       PyObject *c_api_object;

       m = Py_InitModule("spam", SpamMethods);
       if (m == NULL)
           return;

       /* Initialize the C API pointer array */
       PySpam_API[PySpam_System_NUM] = (void *)PySpam_System;

       /* Create a CObject containing the API pointer array's address */
       c_api_object = PyCObject_FromVoidPtr((void *)PySpam_API, NULL);

       if (c_api_object != NULL)
           PyModule_AddObject(m, "_C_API", c_api_object);
   }

Note that ``PySpam_API`` is declared :keyword:`static`; otherwise the pointer
array would disappear when :func:`initspam` terminates!

The bulk of the work is in the header file :file:`spammodule.h`, which looks
like this::

   #ifndef Py_SPAMMODULE_H
   #define Py_SPAMMODULE_H
   #ifdef __cplusplus
   extern "C" {
   #endif

   /* Header file for spammodule */

   /* C API functions */
   #define PySpam_System_NUM 0
   #define PySpam_System_RETURN int
   #define PySpam_System_PROTO (const char *command)

   /* Total number of C API pointers */
   #define PySpam_API_pointers 1


   #ifdef SPAM_MODULE
   /* This section is used when compiling spammodule.c */

   static PySpam_System_RETURN PySpam_System PySpam_System_PROTO;

   #else
   /* This section is used in modules that use spammodule's API */

   static void **PySpam_API;

   #define PySpam_System \
    (*(PySpam_System_RETURN (*)PySpam_System_PROTO) PySpam_API[PySpam_System_NUM])

   /* Return -1 and set exception on error, 0 on success. */
   static int
   import_spam(void)
   {
       PyObject *module = PyImport_ImportModule("spam");

       if (module != NULL) {
           PyObject *c_api_object = PyObject_GetAttrString(module, "_C_API");
           if (c_api_object == NULL)
               return -1;
           if (PyCObject_Check(c_api_object))
               PySpam_API = (void **)PyCObject_AsVoidPtr(c_api_object);
           Py_DECREF(c_api_object);
       }
       return 0;
   }

   #endif

   #ifdef __cplusplus
   }
   #endif

   #endif /* !defined(Py_SPAMMODULE_H) */

All that a client module must do in order to have access to the function
:cfunc:`PySpam_System` is to call the function (or rather macro)
:cfunc:`import_spam` in its initialization function::

   PyMODINIT_FUNC
   initclient(void)
   {
       PyObject *m;

       m = Py_InitModule("client", ClientMethods);
       if (m == NULL)
           return;
       if (import_spam() < 0)
           return;
       /* additional initialization can happen here */
   }

The main disadvantage of this approach is that the file :file:`spammodule.h` is
rather complicated. However, the basic structure is the same for each function
that is exported, so it has to be learned only once.

Finally it should be mentioned that CObjects offer additional functionality,
which is especially useful for memory allocation and deallocation of the pointer
stored in a CObject. The details are described in the Python/C API Reference
Manual in the section :ref:`cobjects` and in the implementation of CObjects (files
:file:`Include/cobject.h` and :file:`Objects/cobject.c` in the Python source
code distribution).

.. rubric:: Footnotes

.. [#] An interface for this function already exists in the standard module :mod:`os`
   --- it was chosen as a simple and straightforward example.

.. [#] The metaphor of "borrowing" a reference is not completely correct: the owner
   still has a copy of the reference.

.. [#] Checking that the reference count is at least 1 **does not work** --- the
   reference count itself could be in freed memory and may thus be reused for
   another object!

.. [#] These guarantees don't hold when you use the "old" style calling convention ---
   this is still found in much existing code.

