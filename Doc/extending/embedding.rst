.. highlight:: c


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
the very least, you have to call the function :c:func:`Py_Initialize`.  There are
optional calls to pass command line arguments to Python.  Then later you can
call the interpreter from any part of the application.

There are several different ways to call the interpreter: you can pass a string
containing Python statements to :c:func:`PyRun_SimpleString`, or you can pass a
stdio file pointer and a file name (for identification in error messages only)
to :c:func:`PyRun_SimpleFile`.  You can also call the lower-level operations
described in the previous chapters to construct and use Python objects.


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

   #define PY_SSIZE_T_CLEAN
   #include <Python.h>

   int
   main(int argc, char *argv[])
   {
       PyStatus status;
       PyConfig config;
       PyConfig_InitPythonConfig(&config);

       /* optional but recommended */
       status = PyConfig_SetBytesString(&config, &config.program_name, argv[0]);
       if (PyStatus_Exception(status)) {
           goto exception;
       }

       status = Py_InitializeFromConfig(&config);
       if (PyStatus_Exception(status)) {
           goto exception;
       }
       PyConfig_Clear(&config);

       PyRun_SimpleString("from time import time,ctime\n"
                          "print('Today is', ctime(time()))\n");
       if (Py_FinalizeEx() < 0) {
           exit(120);
       }
       return 0;

     exception:
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
   }

.. note::

   ``#define PY_SSIZE_T_CLEAN`` was used to indicate that ``Py_ssize_t`` should be
   used in some APIs instead of ``int``.
   It is not necessary since Python 3.13, but we keep it here for backward compatibility.
   See :ref:`arg-parsing-string-and-buffers` for a description of this macro.

Setting :c:member:`PyConfig.program_name` should be called before
:c:func:`Py_InitializeFromConfig` to inform the interpreter about paths to Python run-time
libraries.  Next, the Python interpreter is initialized with
:c:func:`Py_Initialize`, followed by the execution of a hard-coded Python script
that prints the date and time.  Afterwards, the :c:func:`Py_FinalizeEx` call shuts
the interpreter down, followed by the end of the program.  In a real program,
you may want to get the Python script from another source, perhaps a text-editor
routine, a file, or a database.  Getting the Python code from a file can better
be done by using the :c:func:`PyRun_SimpleFile` function, which saves you the
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
array.  If you :ref:`compile and link <compiling>` this program (let's call
the finished executable :program:`call`), and use it to execute a Python
script, such as:

.. code-block:: python

   def multiply(a,b):
       print("Will compute", a, "times", b)
       c = 0
       for i in range(0, a):
           c = c + b
       return c

then the result should be:

.. code-block:: shell-session

   $ call multiply multiply 3 2
   Will compute 3 times 2
   Result of call: 6

Although the program is quite large for its functionality, most of the code is
for data conversion between Python and C, and for error reporting.  The
interesting part with respect to embedding Python starts with ::

   Py_Initialize();
   pName = PyUnicode_DecodeFSDefault(argv[1]);
   /* Error checking of pName left out */
   pModule = PyImport_Import(pName);

After initializing the interpreter, the script is loaded using
:c:func:`PyImport_Import`.  This routine needs a Python string as its argument,
which is constructed using the :c:func:`PyUnicode_FromString` data conversion
routine. ::

   pFunc = PyObject_GetAttrString(pModule, argv[2]);
   /* pFunc is a new reference */

   if (pFunc && PyCallable_Check(pFunc)) {
       ...
   }
   Py_XDECREF(pFunc);

Once the script is loaded, the name we're looking for is retrieved using
:c:func:`PyObject_GetAttrString`.  If the name exists, and the object returned is
callable, you can safely assume that it is a function.  The program then
proceeds by constructing a tuple of arguments as normal.  The call to the Python
function is then made with::

   pValue = PyObject_CallObject(pFunc, pArgs);

Upon return of the function, ``pValue`` is either ``NULL`` or it contains a
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
       return PyLong_FromLong(numargs);
   }

   static PyMethodDef EmbMethods[] = {
       {"numargs", emb_numargs, METH_VARARGS,
        "Return the number of arguments received by the process."},
       {NULL, NULL, 0, NULL}
   };

   static PyModuleDef EmbModule = {
       PyModuleDef_HEAD_INIT, "emb", NULL, -1, EmbMethods,
       NULL, NULL, NULL, NULL
   };

   static PyObject*
   PyInit_emb(void)
   {
       return PyModule_Create(&EmbModule);
   }

Insert the above code just above the :c:func:`main` function. Also, insert the
following two statements before the call to :c:func:`Py_Initialize`::

   numargs = argc;
   PyImport_AppendInittab("emb", &PyInit_emb);

These two lines initialize the ``numargs`` variable, and make the
:func:`!emb.numargs` function accessible to the embedded Python interpreter.
With these extensions, the Python script can do things like

.. code-block:: python

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


.. _compiling:

Compiling and Linking under Unix-like systems
=============================================

It is not necessarily trivial to find the right flags to pass to your
compiler (and linker) in order to embed the Python interpreter into your
application, particularly because Python needs to load library modules
implemented as C dynamic extensions (:file:`.so` files) linked against
it.

To find out the required compiler and linker flags, you can execute the
:file:`python{X.Y}-config` script which is generated as part of the
installation process (a :file:`python3-config` script may also be
available).  This script has several options, of which the following will
be directly useful to you:

* ``pythonX.Y-config --cflags`` will give you the recommended flags when
  compiling:

  .. code-block:: shell-session

     $ /opt/bin/python3.11-config --cflags
     -I/opt/include/python3.11 -I/opt/include/python3.11 -Wsign-compare  -DNDEBUG -g -fwrapv -O3 -Wall

* ``pythonX.Y-config --ldflags --embed`` will give you the recommended flags
  when linking:

  .. code-block:: shell-session

     $ /opt/bin/python3.11-config --ldflags --embed
     -L/opt/lib/python3.11/config-3.11-x86_64-linux-gnu -L/opt/lib -lpython3.11 -lpthread -ldl  -lutil -lm

.. note::
   To avoid confusion between several Python installations (and especially
   between the system Python and your own compiled Python), it is recommended
   that you use the absolute path to :file:`python{X.Y}-config`, as in the above
   example.

If this procedure doesn't work for you (it is not guaranteed to work for
all Unix-like platforms; however, we welcome :ref:`bug reports <reporting-bugs>`)
you will have to read your system's documentation about dynamic linking and/or
examine Python's :file:`Makefile` (use :func:`sysconfig.get_makefile_filename`
to find its location) and compilation
options.  In this case, the :mod:`sysconfig` module is a useful tool to
programmatically extract the configuration values that you will want to
combine together.  For example:

.. code-block:: pycon

   >>> import sysconfig
   >>> sysconfig.get_config_var('LIBS')
   '-lpthread -ldl  -lutil'
   >>> sysconfig.get_config_var('LINKFORSHARED')
   '-Xlinker -export-dynamic'


.. XXX similar documentation for Windows missing
