.. highlightlang:: c


.. _veryhigh:

*************************
The Very High Level Layer
*************************

The functions in this chapter will let you execute Python source code given in a
file or a buffer, but they will not let you interact in a more detailed way with
the interpreter.

Several of these functions accept a start symbol from the grammar as a
parameter.  The available start symbols are :const:`Py_eval_input`,
:const:`Py_file_input`, and :const:`Py_single_input`.  These are described
following the functions which accept them as parameters.

Note also that several of these functions take :ctype:`FILE\*` parameters.  One
particular issue which needs to be handled carefully is that the :ctype:`FILE`
structure for different C libraries can be different and incompatible.  Under
Windows (at least), it is possible for dynamically linked extensions to actually
use different libraries, so care should be taken that :ctype:`FILE\*` parameters
are only passed to these functions if it is certain that they were created by
the same library that the Python runtime is using.


.. cfunction:: int Py_Main(int argc, char **argv)

   The main program for the standard interpreter.  This is made available for
   programs which embed Python.  The *argc* and *argv* parameters should be
   prepared exactly as those which are passed to a C program's :cfunc:`main`
   function.  It is important to note that the argument list may be modified (but
   the contents of the strings pointed to by the argument list are not). The return
   value will be the integer passed to the :func:`sys.exit` function, ``1`` if the
   interpreter exits due to an exception, or ``2`` if the parameter list does not
   represent a valid Python command line.

   Note that if an otherwise unhandled :exc:`SystemError` is raised, this
   function will not return ``1``, but exit the process, as long as
   ``Py_InspectFlag`` is not set.


.. cfunction:: int PyRun_AnyFile(FILE *fp, const char *filename)

   This is a simplified interface to :cfunc:`PyRun_AnyFileExFlags` below, leaving
   *closeit* set to ``0`` and *flags* set to *NULL*.


.. cfunction:: int PyRun_AnyFileFlags(FILE *fp, const char *filename, PyCompilerFlags *flags)

   This is a simplified interface to :cfunc:`PyRun_AnyFileExFlags` below, leaving
   the *closeit* argument set to ``0``.


.. cfunction:: int PyRun_AnyFileEx(FILE *fp, const char *filename, int closeit)

   This is a simplified interface to :cfunc:`PyRun_AnyFileExFlags` below, leaving
   the *flags* argument set to *NULL*.


.. cfunction:: int PyRun_AnyFileExFlags(FILE *fp, const char *filename, int closeit, PyCompilerFlags *flags)

   If *fp* refers to a file associated with an interactive device (console or
   terminal input or Unix pseudo-terminal), return the value of
   :cfunc:`PyRun_InteractiveLoop`, otherwise return the result of
   :cfunc:`PyRun_SimpleFile`.  If *filename* is *NULL*, this function uses
   ``"???"`` as the filename.


.. cfunction:: int PyRun_SimpleString(const char *command)

   This is a simplified interface to :cfunc:`PyRun_SimpleStringFlags` below,
   leaving the *PyCompilerFlags\** argument set to NULL.


.. cfunction:: int PyRun_SimpleStringFlags(const char *command, PyCompilerFlags *flags)

   Executes the Python source code from *command* in the :mod:`__main__` module
   according to the *flags* argument. If :mod:`__main__` does not already exist, it
   is created.  Returns ``0`` on success or ``-1`` if an exception was raised.  If
   there was an error, there is no way to get the exception information. For the
   meaning of *flags*, see below.

   Note that if an otherwise unhandled :exc:`SystemError` is raised, this
   function will not return ``-1``, but exit the process, as long as
   ``Py_InspectFlag`` is not set.


.. cfunction:: int PyRun_SimpleFile(FILE *fp, const char *filename)

   This is a simplified interface to :cfunc:`PyRun_SimpleFileExFlags` below,
   leaving *closeit* set to ``0`` and *flags* set to *NULL*.


.. cfunction:: int PyRun_SimpleFileFlags(FILE *fp, const char *filename, PyCompilerFlags *flags)

   This is a simplified interface to :cfunc:`PyRun_SimpleFileExFlags` below,
   leaving *closeit* set to ``0``.


.. cfunction:: int PyRun_SimpleFileEx(FILE *fp, const char *filename, int closeit)

   This is a simplified interface to :cfunc:`PyRun_SimpleFileExFlags` below,
   leaving *flags* set to *NULL*.


.. cfunction:: int PyRun_SimpleFileExFlags(FILE *fp, const char *filename, int closeit, PyCompilerFlags *flags)

   Similar to :cfunc:`PyRun_SimpleStringFlags`, but the Python source code is read
   from *fp* instead of an in-memory string. *filename* should be the name of the
   file.  If *closeit* is true, the file is closed before PyRun_SimpleFileExFlags
   returns.


.. cfunction:: int PyRun_InteractiveOne(FILE *fp, const char *filename)

   This is a simplified interface to :cfunc:`PyRun_InteractiveOneFlags` below,
   leaving *flags* set to *NULL*.


.. cfunction:: int PyRun_InteractiveOneFlags(FILE *fp, const char *filename, PyCompilerFlags *flags)

   Read and execute a single statement from a file associated with an interactive
   device according to the *flags* argument.  If *filename* is *NULL*, ``"???"`` is
   used instead.  The user will be prompted using ``sys.ps1`` and ``sys.ps2``.
   Returns ``0`` when the input was executed successfully, ``-1`` if there was an
   exception, or an error code from the :file:`errcode.h` include file distributed
   as part of Python if there was a parse error.  (Note that :file:`errcode.h` is
   not included by :file:`Python.h`, so must be included specifically if needed.)


.. cfunction:: int PyRun_InteractiveLoop(FILE *fp, const char *filename)

   This is a simplified interface to :cfunc:`PyRun_InteractiveLoopFlags` below,
   leaving *flags* set to *NULL*.


.. cfunction:: int PyRun_InteractiveLoopFlags(FILE *fp,  const char *filename, PyCompilerFlags *flags)

   Read and execute statements from a file associated with an interactive device
   until EOF is reached.  If *filename* is *NULL*, ``"???"`` is used instead.  The
   user will be prompted using ``sys.ps1`` and ``sys.ps2``.  Returns ``0`` at EOF.


.. cfunction:: struct _node* PyParser_SimpleParseString(const char *str, int start)

   This is a simplified interface to
   :cfunc:`PyParser_SimpleParseStringFlagsFilename` below, leaving  *filename* set
   to *NULL* and *flags* set to ``0``.


.. cfunction:: struct _node* PyParser_SimpleParseStringFlags( const char *str, int start, int flags)

   This is a simplified interface to
   :cfunc:`PyParser_SimpleParseStringFlagsFilename` below, leaving  *filename* set
   to *NULL*.


.. cfunction:: struct _node* PyParser_SimpleParseStringFlagsFilename( const char *str, const char *filename, int start, int flags)

   Parse Python source code from *str* using the start token *start* according to
   the *flags* argument.  The result can be used to create a code object which can
   be evaluated efficiently. This is useful if a code fragment must be evaluated
   many times.


.. cfunction:: struct _node* PyParser_SimpleParseFile(FILE *fp, const char *filename, int start)

   This is a simplified interface to :cfunc:`PyParser_SimpleParseFileFlags` below,
   leaving *flags* set to ``0``


.. cfunction:: struct _node* PyParser_SimpleParseFileFlags(FILE *fp, const char *filename, int start, int flags)

   Similar to :cfunc:`PyParser_SimpleParseStringFlagsFilename`, but the Python
   source code is read from *fp* instead of an in-memory string.


.. cfunction:: PyObject* PyRun_String(const char *str, int start, PyObject *globals, PyObject *locals)

   This is a simplified interface to :cfunc:`PyRun_StringFlags` below, leaving
   *flags* set to *NULL*.


.. cfunction:: PyObject* PyRun_StringFlags(const char *str, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags)

   Execute Python source code from *str* in the context specified by the
   dictionaries *globals* and *locals* with the compiler flags specified by
   *flags*.  The parameter *start* specifies the start token that should be used to
   parse the source code.

   Returns the result of executing the code as a Python object, or *NULL* if an
   exception was raised.


.. cfunction:: PyObject* PyRun_File(FILE *fp, const char *filename, int start, PyObject *globals, PyObject *locals)

   This is a simplified interface to :cfunc:`PyRun_FileExFlags` below, leaving
   *closeit* set to ``0`` and *flags* set to *NULL*.


.. cfunction:: PyObject* PyRun_FileEx(FILE *fp, const char *filename, int start, PyObject *globals, PyObject *locals, int closeit)

   This is a simplified interface to :cfunc:`PyRun_FileExFlags` below, leaving
   *flags* set to *NULL*.


.. cfunction:: PyObject* PyRun_FileFlags(FILE *fp, const char *filename, int start, PyObject *globals, PyObject *locals, PyCompilerFlags *flags)

   This is a simplified interface to :cfunc:`PyRun_FileExFlags` below, leaving
   *closeit* set to ``0``.


.. cfunction:: PyObject* PyRun_FileExFlags(FILE *fp, const char *filename, int start, PyObject *globals, PyObject *locals, int closeit, PyCompilerFlags *flags)

   Similar to :cfunc:`PyRun_StringFlags`, but the Python source code is read from
   *fp* instead of an in-memory string. *filename* should be the name of the file.
   If *closeit* is true, the file is closed before :cfunc:`PyRun_FileExFlags`
   returns.


.. cfunction:: PyObject* Py_CompileString(const char *str, const char *filename, int start)

   This is a simplified interface to :cfunc:`Py_CompileStringFlags` below, leaving
   *flags* set to *NULL*.


.. cfunction:: PyObject* Py_CompileStringFlags(const char *str, const char *filename, int start, PyCompilerFlags *flags)

   Parse and compile the Python source code in *str*, returning the resulting code
   object.  The start token is given by *start*; this can be used to constrain the
   code which can be compiled and should be :const:`Py_eval_input`,
   :const:`Py_file_input`, or :const:`Py_single_input`.  The filename specified by
   *filename* is used to construct the code object and may appear in tracebacks or
   :exc:`SyntaxError` exception messages.  This returns *NULL* if the code cannot
   be parsed or compiled.


.. cfunction:: PyObject* PyEval_EvalCode(PyCodeObject *co, PyObject *globals, PyObject *locals)

   This is a simplified interface to :cfunc:`PyEval_EvalCodeEx`, with just
   the code object, and the dictionaries of global and local variables.
   The other arguments are set to *NULL*.


.. cfunction:: PyObject* PyEval_EvalCodeEx(PyCodeObject *co, PyObject *globals, PyObject *locals, PyObject **args, int argcount, PyObject **kws, int kwcount, PyObject **defs, int defcount, PyObject *closure)

   Evaluate a precompiled code object, given a particular environment for its
   evaluation.  This environment consists of dictionaries of global and local
   variables, arrays of arguments, keywords and defaults, and a closure tuple of
   cells.


.. cfunction:: PyObject* PyEval_EvalFrame(PyFrameObject *f)

   Evaluate an execution frame.  This is a simplified interface to
   PyEval_EvalFrameEx, for backward compatibility.


.. cfunction:: PyObject* PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)

   This is the main, unvarnished function of Python interpretation.  It is
   literally 2000 lines long.  The code object associated with the execution
   frame *f* is executed, interpreting bytecode and executing calls as needed.
   The additional *throwflag* parameter can mostly be ignored - if true, then
   it causes an exception to immediately be thrown; this is used for the
   :meth:`throw` methods of generator objects.


.. cfunction:: int PyEval_MergeCompilerFlags(PyCompilerFlags *cf)

   This function changes the flags of the current evaluation frame, and returns
   true on success, false on failure.


.. cvar:: int Py_eval_input

   .. index:: single: Py_CompileString()

   The start symbol from the Python grammar for isolated expressions; for use with
   :cfunc:`Py_CompileString`.


.. cvar:: int Py_file_input

   .. index:: single: Py_CompileString()

   The start symbol from the Python grammar for sequences of statements as read
   from a file or other source; for use with :cfunc:`Py_CompileString`.  This is
   the symbol to use when compiling arbitrarily long Python source code.


.. cvar:: int Py_single_input

   .. index:: single: Py_CompileString()

   The start symbol from the Python grammar for a single statement; for use with
   :cfunc:`Py_CompileString`. This is the symbol used for the interactive
   interpreter loop.


.. ctype:: struct PyCompilerFlags

   This is the structure used to hold compiler flags.  In cases where code is only
   being compiled, it is passed as ``int flags``, and in cases where code is being
   executed, it is passed as ``PyCompilerFlags *flags``.  In this case, ``from
   __future__ import`` can modify *flags*.

   Whenever ``PyCompilerFlags *flags`` is *NULL*, :attr:`cf_flags` is treated as
   equal to ``0``, and any modification due to ``from __future__ import`` is
   discarded.  ::

      struct PyCompilerFlags {
          int cf_flags;
      }


.. cvar:: int CO_FUTURE_DIVISION

   This bit can be set in *flags* to cause division operator ``/`` to be
   interpreted as "true division" according to :pep:`238`.

