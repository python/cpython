.. highlightlang:: c


.. _embedding:

***************************************
Embedding Python in Another Application
***************************************

The previous chapters discussed how to extend Python, that is, how to extend the
functionality of Python by attaching a library of C functions to it.  It is also
possible to do it the other way around: enrich your C/C++ application by
embedding Python in it.  Embedding provides your application with the ability to
implement some of the functionality of your application in Python rather than C
or C++. This can be used for many purposes; one example would be to allow users
to tailor the application to their needs by writing some scripts in Python.  You
can also use it yourself if some of the functionality can be written in Python
more easily.

Embedding Python is similar to extending it, but not quite.  The difference is
that when you extend Python, the main program of the application is still the
Python interpreter, while if you embed Python, the main program may have nothing
to do with Python --- instead, some parts of the application occasionally call
the Python interpreter to run some Python code.

So if you are embedding Python, you are providing your own main program.  One of
the things this main program has to do is initialize the Python interpreter.  At
the very least, you have to call the function :cfunc:`Py_Initialize` (on Mac OS,
call :cfunc:`PyMac_Initialize` instead).  There are optional calls to pass
command line arguments to Python.  Then later you can call the interpreter from
any part of the application.

There are several different ways to call the interpreter: you can pass a string
containing Python statements to :cfunc:`PyRun_SimpleString`, or you can pass a
stdio file pointer and a file name (for identification in error messages only)
to :cfunc:`PyRun_SimpleFile`.  You can also call the lower-level operations
described in the previous chapters to construct and use Python objects.

A simple demo of embedding Python can be found in the directory
:file:`Demo/embed/` of the source distribution.


.. seealso::

   :ref:`c-api-index`
      The details of Python's C interface are given in this manual. A great deal of
      necessary information can be found here.


.. _high-level-embedding:

Very High Level Embedding
=========================

The simplest form of embedding Python is the use of the very high level
interface. This interface is intended to execute a Python script without needing
to interact with the application directly. This can for example be used to
perform some operation on a file. ::

   #include <Python.h>

   int
   main(int argc, char *argv[])
   {
     Py_Initialize();
     PyRun_SimpleString("from time import time,ctime\n"
                        "print('Today is', ctime(time()))\n");
     Py_Finalize();
     return 0;
   }

The above code first initializes the Python interpreter with
:cfunc:`Py_Initialize`, followed by the execution of a hard-coded Python script
that print the date and time.  Afterwards, the :cfunc:`Py_Finalize` call shuts
the interpreter down, followed by the end of the program.  In a real program,
you may want to get the Python script from another source, perhaps a text-editor
routine, a file, or a database.  Getting the Python code from a file can better
be done by using the :cfunc:`PyRun_SimpleFile` function, which saves you the
trouble of allocating memory space and loading the file contents.


.. _lower-level-embedding:

Beyond Very High Level Embedding: An overview
=============================================

The high level interface gives you the ability to execute arbitrary pieces of
Python code from your application, but exchanging data values is quite
cumbersome to say the least. If you want that, you should use lower level calls.
At the cost of having to write more C code, you can achieve almost anything.

It should be noted that extending Python and embedding Python is quite the same
activity, despite the different intent. Most topics discussed in the previous
chapters are still valid. To show this, consider what the extension code from
Python to C really does:

#. Convert data values from Python to C,

#. Perform a function call to a C routine using the converted values, and

#. Convert the data values from the call from C to Python.

When embedding Python, the interface code does:

#. Convert data values from C to Python,

#. Perform a function call to a Python interface routine using the converted
   values, and

#. Convert the data values from the call from Python to C.

As you can see, the data conversion steps are simply swapped to accommodate the
different direction of the cross-language transfer. The only difference is the
routine that you call between both data conversions. When extending, you call a
C routine, when embedding, you call a Python routine.

This chapter will not discuss how to convert data from Python to C and vice
versa.  Also, proper use of references and dealing with errors is assumed to be
understood.  Since these aspects do not differ from extending the interpreter,
you can refer to earlier chapters for the required information.


.. _pure-embedding:

Pure Embedding
==============

The first program aims to execute a function in a Python script. Like in the
section about the very high level interface, the Python interpreter does not
directly interact with the application (but that will change in the next
section).

The code to run a function defined in a Python script is:

.. literalinclude:: ../includes/run-func.c


This code loads a Python script using ``argv[1]``, and calls the function named
in ``argv[2]``.  Its integer arguments are the other values of the ``argv``
array.  If you compile and link this program (let's call the finished executable
:program:`call`), and use it to execute a Python script, such as::

   def multiply(a,b):
       print("Will compute", a, "times", b)
       c = 0
       for i in range(0, a):
           c = c + b
       return c

then the result should be::

   $ call multiply multiply 3 2
   Will compute 3 times 2
   Result of call: 6

Although the program is quite large for its functionality, most of the code is
for data conversion between Python and C, and for error reporting.  The
interesting part with respect to embedding Python starts with ::

   Py_Initialize();
   pName = PyString_FromString(argv[1]);
   /* Error checking of pName left out */
   pModule = PyImport_Import(pName);

After initializing the interpreter, the script is loaded using
:cfunc:`PyImport_Import`.  This routine needs a Python string as its argument,
which is constructed using the :cfunc:`PyString_FromString` data conversion
routine. ::

   pFunc = PyObject_GetAttrString(pModule, argv[2]);
   /* pFunc is a new reference */

   if (pFunc && PyCallable_Check(pFunc)) {
       ...
   }
   Py_XDECREF(pFunc);

Once the script is loaded, the name we're looking for is retrieved using
:cfunc:`PyObject_GetAttrString`.  If the name exists, and the object returned is
callable, you can safely assume that it is a function.  The program then
proceeds by constructing a tuple of arguments as normal.  The call to the Python
function is then made with::

   pValue = PyObject_CallObject(pFunc, pArgs);

Upon return of the function, ``pValue`` is either *NULL* or it contains a
reference to the return value of the function.  Be sure to release the reference
after examining the value.


.. _extending-with-embedding:

Extending Embedded Python
=========================

Until now, the embedded Python interpreter had no access to functionality from
the application itself.  The Python API allows this by extending the embedded
interpreter.  That is, the embedded interpreter gets extended with routines
provided by the application. While it sounds complex, it is not so bad.  Simply
forget for a while that the application starts the Python interpreter.  Instead,
consider the application to be a set of subroutines, and write some glue code
that gives Python access to those routines, just like you would write a normal
Python extension.  For example::

   static int numargs=0;

   /* Return the number of arguments of the application command line */
   static PyObject*
   emb_numargs(PyObject *self, PyObject *args)
   {
       if(!PyArg_ParseTuple(args, ":numargs"))
           return NULL;
       return Py_BuildValue("i", numargs);
   }

   static PyMethodDef EmbMethods[] = {
       {"numargs", emb_numargs, METH_VARARGS,
        "Return the number of arguments received by the process."},
       {NULL, NULL, 0, NULL}
   };

Insert the above code just above the :cfunc:`main` function. Also, insert the
following two statements directly after :cfunc:`Py_Initialize`::

   numargs = argc;
   Py_InitModule("emb", EmbMethods);

These two lines initialize the ``numargs`` variable, and make the
:func:`emb.numargs` function accessible to the embedded Python interpreter.
With these extensions, the Python script can do things like ::

   import emb
   print("Number of arguments", emb.numargs())

In a real application, the methods will expose an API of the application to
Python.

.. TODO: threads, code examples do not really behave well if errors happen
   (what to watch out for)


.. _embeddingincplusplus:

Embedding Python in C++
=======================

It is also possible to embed Python in a C++ program; precisely how this is done
will depend on the details of the C++ system used; in general you will need to
write the main program in C++, and use the C++ compiler to compile and link your
program.  There is no need to recompile Python itself using C++.


.. _link-reqs:

Linking Requirements
====================

While the :program:`configure` script shipped with the Python sources will
correctly build Python to export the symbols needed by dynamically linked
extensions, this is not automatically inherited by applications which embed the
Python library statically, at least on Unix.  This is an issue when the
application is linked to the static runtime library (:file:`libpython.a`) and
needs to load dynamic extensions (implemented as :file:`.so` files).

The problem is that some entry points are defined by the Python runtime solely
for extension modules to use.  If the embedding application does not use any of
these entry points, some linkers will not include those entries in the symbol
table of the finished executable.  Some additional options are needed to inform
the linker not to remove these symbols.

Determining the right options to use for any given platform can be quite
difficult, but fortunately the Python configuration already has those values.
To retrieve them from an installed Python interpreter, start an interactive
interpreter and have a short session like this::

   >>> import distutils.sysconfig
   >>> distutils.sysconfig.get_config_var('LINKFORSHARED')
   '-Xlinker -export-dynamic'

.. index:: module: distutils.sysconfig

The contents of the string presented will be the options that should be used.
If the string is empty, there's no need to add any additional options.  The
:const:`LINKFORSHARED` definition corresponds to the variable of the same name
in Python's top-level :file:`Makefile`.

