.. highlightlang:: c


.. _utilities:

*********
Utilities
*********

The functions in this chapter perform various utility tasks, ranging from
helping C code be more portable across platforms, using Python modules from C,
and parsing function arguments and constructing Python values from C values.


.. _os:

Operating System Utilities
==========================


.. cfunction:: int Py_FdIsInteractive(FILE *fp, const char *filename)

   Return true (nonzero) if the standard I/O file *fp* with name *filename* is
   deemed interactive.  This is the case for files for which ``isatty(fileno(fp))``
   is true.  If the global flag :cdata:`Py_InteractiveFlag` is true, this function
   also returns true if the *filename* pointer is *NULL* or if the name is equal to
   one of the strings ``'<stdin>'`` or ``'???'``.


.. cfunction:: long PyOS_GetLastModificationTime(char *filename)

   Return the time of last modification of the file *filename*. The result is
   encoded in the same way as the timestamp returned by the standard C library
   function :cfunc:`time`.


.. cfunction:: void PyOS_AfterFork()

   Function to update some internal state after a process fork; this should be
   called in the new process if the Python interpreter will continue to be used.
   If a new executable is loaded into the new process, this function does not need
   to be called.


.. cfunction:: int PyOS_CheckStack()

   Return true when the interpreter runs out of stack space.  This is a reliable
   check, but is only available when :const:`USE_STACKCHECK` is defined (currently
   on Windows using the Microsoft Visual C++ compiler).  :const:`USE_STACKCHECK`
   will be defined automatically; you should never change the definition in your
   own code.


.. cfunction:: PyOS_sighandler_t PyOS_getsig(int i)

   Return the current signal handler for signal *i*.  This is a thin wrapper around
   either :cfunc:`sigaction` or :cfunc:`signal`.  Do not call those functions
   directly! :ctype:`PyOS_sighandler_t` is a typedef alias for :ctype:`void
   (\*)(int)`.


.. cfunction:: PyOS_sighandler_t PyOS_setsig(int i, PyOS_sighandler_t h)

   Set the signal handler for signal *i* to be *h*; return the old signal handler.
   This is a thin wrapper around either :cfunc:`sigaction` or :cfunc:`signal`.  Do
   not call those functions directly!  :ctype:`PyOS_sighandler_t` is a typedef
   alias for :ctype:`void (\*)(int)`.

.. _systemfunctions:

System Functions
================

These are utility functions that make functionality from the :mod:`sys` module
accessible to C code.  They all work with the current interpreter thread's
:mod:`sys` module's dict, which is contained in the internal thread state structure.

.. cfunction:: PyObject *PySys_GetObject(char *name)

   Return the object *name* from the :mod:`sys` module or *NULL* if it does
   not exist, without setting an exception.

.. cfunction:: FILE *PySys_GetFile(char *name, FILE *def)

   Return the :ctype:`FILE*` associated with the object *name* in the
   :mod:`sys` module, or *def* if *name* is not in the module or is not associated
   with a :ctype:`FILE*`.

.. cfunction:: int PySys_SetObject(char *name, PyObject *v)

   Set *name* in the :mod:`sys` module to *v* unless *v* is *NULL*, in which
   case *name* is deleted from the sys module. Returns ``0`` on success, ``-1``
   on error.

.. cfunction:: void PySys_ResetWarnOptions(void)

   Reset :data:`sys.warnoptions` to an empty list.

.. cfunction:: void PySys_AddWarnOption(char *s)

   Append *s* to :data:`sys.warnoptions`.

.. cfunction:: void PySys_SetPath(char *path)

   Set :data:`sys.path` to a list object of paths found in *path* which should
   be a list of paths separated with the platform's search path delimiter
   (``:`` on Unix, ``;`` on Windows).

.. cfunction:: void PySys_WriteStdout(const char *format, ...)

   Write the output string described by *format* to :data:`sys.stdout`.  No
   exceptions are raised, even if truncation occurs (see below).

   *format* should limit the total size of the formatted output string to
   1000 bytes or less -- after 1000 bytes, the output string is truncated.
   In particular, this means that no unrestricted "%s" formats should occur;
   these should be limited using "%.<N>s" where <N> is a decimal number
   calculated so that <N> plus the maximum size of other formatted text does not
   exceed 1000 bytes.  Also watch out for "%f", which can print hundreds of
   digits for very large numbers.

   If a problem occurs, or :data:`sys.stdout` is unset, the formatted message
   is written to the real (C level) *stdout*.

.. cfunction:: void PySys_WriteStderr(const char *format, ...)

   As above, but write to :data:`sys.stderr` or *stderr* instead.


.. _processcontrol:

Process Control
===============


.. cfunction:: void Py_FatalError(const char *message)

   .. index:: single: abort()

   Print a fatal error message and kill the process.  No cleanup is performed.
   This function should only be invoked when a condition is detected that would
   make it dangerous to continue using the Python interpreter; e.g., when the
   object administration appears to be corrupted.  On Unix, the standard C library
   function :cfunc:`abort` is called which will attempt to produce a :file:`core`
   file.


.. cfunction:: void Py_Exit(int status)

   .. index::
      single: Py_Finalize()
      single: exit()

   Exit the current process.  This calls :cfunc:`Py_Finalize` and then calls the
   standard C library function ``exit(status)``.


.. cfunction:: int Py_AtExit(void (*func) ())

   .. index::
      single: Py_Finalize()
      single: cleanup functions

   Register a cleanup function to be called by :cfunc:`Py_Finalize`.  The cleanup
   function will be called with no arguments and should return no value.  At most
   32 cleanup functions can be registered.  When the registration is successful,
   :cfunc:`Py_AtExit` returns ``0``; on failure, it returns ``-1``.  The cleanup
   function registered last is called first. Each cleanup function will be called
   at most once.  Since Python's internal finalization will have completed before
   the cleanup function, no Python APIs should be called by *func*.


.. _importing:

Importing Modules
=================


.. cfunction:: PyObject* PyImport_ImportModule(const char *name)

   .. index::
      single: package variable; __all__
      single: __all__ (package variable)
      single: modules (in module sys)

   This is a simplified interface to :cfunc:`PyImport_ImportModuleEx` below,
   leaving the *globals* and *locals* arguments set to *NULL* and *level* set
   to 0.  When the *name*
   argument contains a dot (when it specifies a submodule of a package), the
   *fromlist* argument is set to the list ``['*']`` so that the return value is the
   named module rather than the top-level package containing it as would otherwise
   be the case.  (Unfortunately, this has an additional side effect when *name* in
   fact specifies a subpackage instead of a submodule: the submodules specified in
   the package's ``__all__`` variable are  loaded.)  Return a new reference to the
   imported module, or *NULL* with an exception set on failure.  Before Python 2.4,
   the module may still be created in the failure case --- examine ``sys.modules``
   to find out.  Starting with Python 2.4, a failing import of a module no longer
   leaves the module in ``sys.modules``.

   .. versionchanged:: 2.6
      always use absolute imports

   .. index:: single: modules (in module sys)


.. cfunction:: PyObject* PyImport_ImportModuleNoBlock(const char *name)

   .. index::
      single: `cfunc:PyImport_ImportModule`

   This version of `cfunc:PyImport_ImportModule` does not block. It's intended
   to be used in C function which import other modules to execute a function.
   The import may block if another thread holds the import lock. The function
  `cfunc:PyImport_ImportModuleNoBlock` doesn't block. It first tries to fetch
   the module from sys.modules and falls back to `cfunc:PyImport_ImportModule`
   unless the the lock is hold. In the latter case the function raises an
   ImportError.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyImport_ImportModuleEx(char *name, PyObject *globals, PyObject *locals, PyObject *fromlist)

   .. index:: builtin: __import__

   Import a module.  This is best described by referring to the built-in Python
   function :func:`__import__`, as the standard :func:`__import__` function calls
   this function directly.

   The return value is a new reference to the imported module or top-level package,
   or *NULL* with an exception set on failure (before Python 2.4, the module may
   still be created in this case).  Like for :func:`__import__`, the return value
   when a submodule of a package was requested is normally the top-level package,
   unless a non-empty *fromlist* was given.

   Failing imports remove incomplete module objects, like with
   :cfunc:`PyImport_ImportModule`.

   .. versionchanged:: 2.6
      The function is an alias for `cfunc:PyImport_ImportModuleLevel` with
      -1 as level, meaning relative import.


.. cfunction:: PyObject* PyImport_ImportModuleLevel(char *name, PyObject *globals, PyObject *locals, PyObject *fromlist, int level)

   Import a module.  This is best described by referring to the built-in Python
   function :func:`__import__`, as the standard :func:`__import__` function calls
   this function directly.

   The return value is a new reference to the imported module or top-level package,
   or *NULL* with an exception set on failure.  Like for :func:`__import__`,
   the return value when a submodule of a package was requested is normally the
   top-level package, unless a non-empty *fromlist* was given.

   ..versionadded:: 2.5


.. cfunction:: PyObject* PyImport_Import(PyObject *name)

   This is a higher-level interface that calls the current "import hook function".
   It invokes the :func:`__import__` function from the ``__builtins__`` of the
   current globals.  This means that the import is done using whatever import hooks
   are installed in the current environment.

   .. versionchanged:: 2.6
      always use absolute imports


.. cfunction:: PyObject* PyImport_ReloadModule(PyObject *m)

   Reload a module.  Return a new reference to the reloaded module, or *NULL* with
   an exception set on failure (the module still exists in this case).


.. cfunction:: PyObject* PyImport_AddModule(const char *name)

   Return the module object corresponding to a module name.  The *name* argument
   may be of the form ``package.module``. First check the modules dictionary if
   there's one there, and if not, create a new one and insert it in the modules
   dictionary. Return *NULL* with an exception set on failure.

   .. note::

      This function does not load or import the module; if the module wasn't already
      loaded, you will get an empty module object. Use :cfunc:`PyImport_ImportModule`
      or one of its variants to import a module.  Package structures implied by a
      dotted name for *name* are not created if not already present.


.. cfunction:: PyObject* PyImport_ExecCodeModule(char *name, PyObject *co)

   .. index:: builtin: compile

   Given a module name (possibly of the form ``package.module``) and a code object
   read from a Python bytecode file or obtained from the built-in function
   :func:`compile`, load the module.  Return a new reference to the module object,
   or *NULL* with an exception set if an error occurred.  Before Python 2.4, the
   module could still be created in error cases.  Starting with Python 2.4, *name*
   is removed from ``sys.modules`` in error cases, and even if *name* was already
   in ``sys.modules`` on entry to :cfunc:`PyImport_ExecCodeModule`.  Leaving
   incompletely initialized modules in ``sys.modules`` is dangerous, as imports of
   such modules have no way to know that the module object is an unknown (and
   probably damaged with respect to the module author's intents) state.

   This function will reload the module if it was already imported.  See
   :cfunc:`PyImport_ReloadModule` for the intended way to reload a module.

   If *name* points to a dotted name of the form ``package.module``, any package
   structures not already created will still not be created.


.. cfunction:: long PyImport_GetMagicNumber()

   Return the magic number for Python bytecode files (a.k.a. :file:`.pyc` and
   :file:`.pyo` files).  The magic number should be present in the first four bytes
   of the bytecode file, in little-endian byte order.


.. cfunction:: PyObject* PyImport_GetModuleDict()

   Return the dictionary used for the module administration (a.k.a.
   ``sys.modules``).  Note that this is a per-interpreter variable.


.. cfunction:: void _PyImport_Init()

   Initialize the import mechanism.  For internal use only.


.. cfunction:: void PyImport_Cleanup()

   Empty the module table.  For internal use only.


.. cfunction:: void _PyImport_Fini()

   Finalize the import mechanism.  For internal use only.


.. cfunction:: PyObject* _PyImport_FindExtension(char *, char *)

   For internal use only.


.. cfunction:: PyObject* _PyImport_FixupExtension(char *, char *)

   For internal use only.


.. cfunction:: int PyImport_ImportFrozenModule(char *name)

   Load a frozen module named *name*.  Return ``1`` for success, ``0`` if the
   module is not found, and ``-1`` with an exception set if the initialization
   failed.  To access the imported module on a successful load, use
   :cfunc:`PyImport_ImportModule`.  (Note the misnomer --- this function would
   reload the module if it was already imported.)


.. ctype:: struct _frozen

   .. index:: single: freeze utility

   This is the structure type definition for frozen module descriptors, as
   generated by the :program:`freeze` utility (see :file:`Tools/freeze/` in the
   Python source distribution).  Its definition, found in :file:`Include/import.h`,
   is::

      struct _frozen {
          char *name;
          unsigned char *code;
          int size;
      };


.. cvar:: struct _frozen* PyImport_FrozenModules

   This pointer is initialized to point to an array of :ctype:`struct _frozen`
   records, terminated by one whose members are all *NULL* or zero.  When a frozen
   module is imported, it is searched in this table.  Third-party code could play
   tricks with this to provide a dynamically created collection of frozen modules.


.. cfunction:: int PyImport_AppendInittab(char *name, void (*initfunc)(void))

   Add a single module to the existing table of built-in modules.  This is a
   convenience wrapper around :cfunc:`PyImport_ExtendInittab`, returning ``-1`` if
   the table could not be extended.  The new module can be imported by the name
   *name*, and uses the function *initfunc* as the initialization function called
   on the first attempted import.  This should be called before
   :cfunc:`Py_Initialize`.


.. ctype:: struct _inittab

   Structure describing a single entry in the list of built-in modules.  Each of
   these structures gives the name and initialization function for a module built
   into the interpreter.  Programs which embed Python may use an array of these
   structures in conjunction with :cfunc:`PyImport_ExtendInittab` to provide
   additional built-in modules.  The structure is defined in
   :file:`Include/import.h` as::

      struct _inittab {
          char *name;
          void (*initfunc)(void);
      };


.. cfunction:: int PyImport_ExtendInittab(struct _inittab *newtab)

   Add a collection of modules to the table of built-in modules.  The *newtab*
   array must end with a sentinel entry which contains *NULL* for the :attr:`name`
   field; failure to provide the sentinel value can result in a memory fault.
   Returns ``0`` on success or ``-1`` if insufficient memory could be allocated to
   extend the internal table.  In the event of failure, no modules are added to the
   internal table.  This should be called before :cfunc:`Py_Initialize`.


.. _marshalling-utils:

Data marshalling support
========================

These routines allow C code to work with serialized objects using the same data
format as the :mod:`marshal` module.  There are functions to write data into the
serialization format, and additional functions that can be used to read the data
back.  Files used to store marshalled data must be opened in binary mode.

Numeric values are stored with the least significant byte first.

The module supports two versions of the data format: version 0 is the historical
version, version 1 (new in Python 2.4) shares interned strings in the file, and
upon unmarshalling. *Py_MARSHAL_VERSION* indicates the current file format
(currently 1).


.. cfunction:: void PyMarshal_WriteLongToFile(long value, FILE *file, int version)

   Marshal a :ctype:`long` integer, *value*, to *file*.  This will only write the
   least-significant 32 bits of *value*; regardless of the size of the native
   :ctype:`long` type.  *version* indicates the file format.


.. cfunction:: void PyMarshal_WriteObjectToFile(PyObject *value, FILE *file, int version)

   Marshal a Python object, *value*, to *file*.
   *version* indicates the file format.


.. cfunction:: PyObject* PyMarshal_WriteObjectToString(PyObject *value, int version)

   Return a string object containing the marshalled representation of *value*.
   *version* indicates the file format.


The following functions allow marshalled values to be read back in.

XXX What about error detection?  It appears that reading past the end of the
file will always result in a negative numeric value (where that's relevant), but
it's not clear that negative values won't be handled properly when there's no
error.  What's the right way to tell? Should only non-negative values be written
using these routines?


.. cfunction:: long PyMarshal_ReadLongFromFile(FILE *file)

   Return a C :ctype:`long` from the data stream in a :ctype:`FILE\*` opened for
   reading.  Only a 32-bit value can be read in using this function, regardless of
   the native size of :ctype:`long`.


.. cfunction:: int PyMarshal_ReadShortFromFile(FILE *file)

   Return a C :ctype:`short` from the data stream in a :ctype:`FILE\*` opened for
   reading.  Only a 16-bit value can be read in using this function, regardless of
   the native size of :ctype:`short`.


.. cfunction:: PyObject* PyMarshal_ReadObjectFromFile(FILE *file)

   Return a Python object from the data stream in a :ctype:`FILE\*` opened for
   reading.  On error, sets the appropriate exception (:exc:`EOFError` or
   :exc:`TypeError`) and returns *NULL*.


.. cfunction:: PyObject* PyMarshal_ReadLastObjectFromFile(FILE *file)

   Return a Python object from the data stream in a :ctype:`FILE\*` opened for
   reading.  Unlike :cfunc:`PyMarshal_ReadObjectFromFile`, this function assumes
   that no further objects will be read from the file, allowing it to aggressively
   load file data into memory so that the de-serialization can operate from data in
   memory rather than reading a byte at a time from the file.  Only use these
   variant if you are certain that you won't be reading anything else from the
   file.  On error, sets the appropriate exception (:exc:`EOFError` or
   :exc:`TypeError`) and returns *NULL*.


.. cfunction:: PyObject* PyMarshal_ReadObjectFromString(char *string, Py_ssize_t len)

   Return a Python object from the data stream in a character buffer containing
   *len* bytes pointed to by *string*.  On error, sets the appropriate exception
   (:exc:`EOFError` or :exc:`TypeError`) and returns *NULL*.


.. _arg-parsing:

Parsing arguments and building values
=====================================

These functions are useful when creating your own extensions functions and
methods.  Additional information and examples are available in
:ref:`extending-index`.

The first three of these functions described, :cfunc:`PyArg_ParseTuple`,
:cfunc:`PyArg_ParseTupleAndKeywords`, and :cfunc:`PyArg_Parse`, all use *format
strings* which are used to tell the function about the expected arguments.  The
format strings use the same syntax for each of these functions.

A format string consists of zero or more "format units."  A format unit
describes one Python object; it is usually a single character or a parenthesized
sequence of format units.  With a few exceptions, a format unit that is not a
parenthesized sequence normally corresponds to a single address argument to
these functions.  In the following description, the quoted form is the format
unit; the entry in (round) parentheses is the Python object type that matches
the format unit; and the entry in [square] brackets is the type of the C
variable(s) whose address should be passed.

``s`` (string or Unicode object) [const char \*]
   Convert a Python string or Unicode object to a C pointer to a character string.
   You must not provide storage for the string itself; a pointer to an existing
   string is stored into the character pointer variable whose address you pass.
   The C string is NUL-terminated.  The Python string must not contain embedded NUL
   bytes; if it does, a :exc:`TypeError` exception is raised. Unicode objects are
   converted to C strings using the default encoding.  If this conversion fails, a
   :exc:`UnicodeError` is raised.

``s#`` (string, Unicode or any read buffer compatible object) [const char \*, int]
   This variant on ``s`` stores into two C variables, the first one a pointer to a
   character string, the second one its length.  In this case the Python string may
   contain embedded null bytes.  Unicode objects pass back a pointer to the default
   encoded string version of the object if such a conversion is possible.  All
   other read-buffer compatible objects pass back a reference to the raw internal
   data representation.

``y`` (bytes object) [const char \*]
   This variant on ``s`` convert a Python bytes object to a C pointer to a
   character string. The bytes object must not contain embedded NUL bytes; if it
   does, a :exc:`TypeError` exception is raised.

``y#`` (bytes object) [const char \*, int]
   This variant on ``s#`` stores into two C variables, the first one a pointer to a
   character string, the second one its length.  This only accepts bytes objects.

``z`` (string or ``None``) [const char \*]
   Like ``s``, but the Python object may also be ``None``, in which case the C
   pointer is set to *NULL*.

``z#`` (string or ``None`` or any read buffer compatible object) [const char \*, int]
   This is to ``s#`` as ``z`` is to ``s``.

``u`` (Unicode object) [Py_UNICODE \*]
   Convert a Python Unicode object to a C pointer to a NUL-terminated buffer of
   16-bit Unicode (UTF-16) data.  As with ``s``, there is no need to provide
   storage for the Unicode data buffer; a pointer to the existing Unicode data is
   stored into the :ctype:`Py_UNICODE` pointer variable whose address you pass.

``u#`` (Unicode object) [Py_UNICODE \*, int]
   This variant on ``u`` stores into two C variables, the first one a pointer to a
   Unicode data buffer, the second one its length. Non-Unicode objects are handled
   by interpreting their read-buffer pointer as pointer to a :ctype:`Py_UNICODE`
   array.

``Z`` (Unicode or ``None``) [Py_UNICODE \*]
   Like ``s``, but the Python object may also be ``None``, in which case the C
   pointer is set to *NULL*.

``Z#`` (Unicode or ``None``) [Py_UNICODE \*, int]
   This is to ``u#`` as ``Z`` is to ``u``.

``es`` (string, Unicode object or character buffer compatible object) [const char \*encoding, char \*\*buffer]
   This variant on ``s`` is used for encoding Unicode and objects convertible to
   Unicode into a character buffer. It only works for encoded data without embedded
   NUL bytes.

   This format requires two arguments.  The first is only used as input, and
   must be a :ctype:`const char\*` which points to the name of an encoding as a
   NUL-terminated string, or *NULL*, in which case the default encoding is used.
   An exception is raised if the named encoding is not known to Python.  The
   second argument must be a :ctype:`char\*\*`; the value of the pointer it
   references will be set to a buffer with the contents of the argument text.
   The text will be encoded in the encoding specified by the first argument.

   :cfunc:`PyArg_ParseTuple` will allocate a buffer of the needed size, copy the
   encoded data into this buffer and adjust *\*buffer* to reference the newly
   allocated storage.  The caller is responsible for calling :cfunc:`PyMem_Free` to
   free the allocated buffer after use.

``et`` (string, Unicode object or character buffer compatible object) [const char \*encoding, char \*\*buffer]
   Same as ``es`` except that 8-bit string objects are passed through without
   recoding them.  Instead, the implementation assumes that the string object uses
   the encoding passed in as parameter.

``es#`` (string, Unicode object or character buffer compatible object) [const char \*encoding, char \*\*buffer, int \*buffer_length]
   This variant on ``s#`` is used for encoding Unicode and objects convertible to
   Unicode into a character buffer.  Unlike the ``es`` format, this variant allows
   input data which contains NUL characters.

   It requires three arguments.  The first is only used as input, and must be a
   :ctype:`const char\*` which points to the name of an encoding as a
   NUL-terminated string, or *NULL*, in which case the default encoding is used.
   An exception is raised if the named encoding is not known to Python.  The
   second argument must be a :ctype:`char\*\*`; the value of the pointer it
   references will be set to a buffer with the contents of the argument text.
   The text will be encoded in the encoding specified by the first argument.
   The third argument must be a pointer to an integer; the referenced integer
   will be set to the number of bytes in the output buffer.

   There are two modes of operation:

   If *\*buffer* points a *NULL* pointer, the function will allocate a buffer of
   the needed size, copy the encoded data into this buffer and set *\*buffer* to
   reference the newly allocated storage.  The caller is responsible for calling
   :cfunc:`PyMem_Free` to free the allocated buffer after usage.

   If *\*buffer* points to a non-*NULL* pointer (an already allocated buffer),
   :cfunc:`PyArg_ParseTuple` will use this location as the buffer and interpret the
   initial value of *\*buffer_length* as the buffer size.  It will then copy the
   encoded data into the buffer and NUL-terminate it.  If the buffer is not large
   enough, a :exc:`ValueError` will be set.

   In both cases, *\*buffer_length* is set to the length of the encoded data
   without the trailing NUL byte.

``et#`` (string, Unicode object or character buffer compatible object) [const char \*encoding, char \*\*buffer]
   Same as ``es#`` except that string objects are passed through without recoding
   them. Instead, the implementation assumes that the string object uses the
   encoding passed in as parameter.

``b`` (integer) [char]
   Convert a Python integer to a tiny int, stored in a C :ctype:`char`.

``B`` (integer) [unsigned char]
   Convert a Python integer to a tiny int without overflow checking, stored in a C
   :ctype:`unsigned char`.

``h`` (integer) [short int]
   Convert a Python integer to a C :ctype:`short int`.

``H`` (integer) [unsigned short int]
   Convert a Python integer to a C :ctype:`unsigned short int`, without overflow
   checking.

``i`` (integer) [int]
   Convert a Python integer to a plain C :ctype:`int`.

``I`` (integer) [unsigned int]
   Convert a Python integer to a C :ctype:`unsigned int`, without overflow
   checking.

``l`` (integer) [long int]
   Convert a Python integer to a C :ctype:`long int`.

``k`` (integer) [unsigned long]
   Convert a Python integer to a C :ctype:`unsigned long` without
   overflow checking.

``L`` (integer) [PY_LONG_LONG]
   Convert a Python integer to a C :ctype:`long long`.  This format is only
   available on platforms that support :ctype:`long long` (or :ctype:`_int64` on
   Windows).

``K`` (integer) [unsigned PY_LONG_LONG]
   Convert a Python integer to a C :ctype:`unsigned long long`
   without overflow checking.  This format is only available on platforms that
   support :ctype:`unsigned long long` (or :ctype:`unsigned _int64` on Windows).

``n`` (integer) [Py_ssize_t]
   Convert a Python integer to a C :ctype:`Py_ssize_t`.

``c`` (string of length 1) [char]
   Convert a Python character, represented as a string of length 1, to a C
   :ctype:`char`.

``f`` (float) [float]
   Convert a Python floating point number to a C :ctype:`float`.

``d`` (float) [double]
   Convert a Python floating point number to a C :ctype:`double`.

``D`` (complex) [Py_complex]
   Convert a Python complex number to a C :ctype:`Py_complex` structure.

``O`` (object) [PyObject \*]
   Store a Python object (without any conversion) in a C object pointer.  The C
   program thus receives the actual object that was passed.  The object's reference
   count is not increased.  The pointer stored is not *NULL*.

``O!`` (object) [*typeobject*, PyObject \*]
   Store a Python object in a C object pointer.  This is similar to ``O``, but
   takes two C arguments: the first is the address of a Python type object, the
   second is the address of the C variable (of type :ctype:`PyObject\*`) into which
   the object pointer is stored.  If the Python object does not have the required
   type, :exc:`TypeError` is raised.

``O&`` (object) [*converter*, *anything*]
   Convert a Python object to a C variable through a *converter* function.  This
   takes two arguments: the first is a function, the second is the address of a C
   variable (of arbitrary type), converted to :ctype:`void \*`.  The *converter*
   function in turn is called as follows::

      status = converter(object, address);

   where *object* is the Python object to be converted and *address* is the
   :ctype:`void\*` argument that was passed to the :cfunc:`PyArg_Parse\*` function.
   The returned *status* should be ``1`` for a successful conversion and ``0`` if
   the conversion has failed.  When the conversion fails, the *converter* function
   should raise an exception.

``S`` (string) [PyStringObject \*]
   Like ``O`` but requires that the Python object is a string object.  Raises
   :exc:`TypeError` if the object is not a string object.  The C variable may also
   be declared as :ctype:`PyObject\*`.

``U`` (Unicode string) [PyUnicodeObject \*]
   Like ``O`` but requires that the Python object is a Unicode object.  Raises
   :exc:`TypeError` if the object is not a Unicode object.  The C variable may also
   be declared as :ctype:`PyObject\*`.

``t#`` (read-only character buffer) [char \*, int]
   Like ``s#``, but accepts any object which implements the read-only buffer
   interface.  The :ctype:`char\*` variable is set to point to the first byte of
   the buffer, and the :ctype:`int` is set to the length of the buffer.  Only
   single-segment buffer objects are accepted; :exc:`TypeError` is raised for all
   others.

``w`` (read-write character buffer) [char \*]
   Similar to ``s``, but accepts any object which implements the read-write buffer
   interface.  The caller must determine the length of the buffer by other means,
   or use ``w#`` instead.  Only single-segment buffer objects are accepted;
   :exc:`TypeError` is raised for all others.

``w#`` (read-write character buffer) [char \*, int]
   Like ``s#``, but accepts any object which implements the read-write buffer
   interface.  The :ctype:`char \*` variable is set to point to the first byte of
   the buffer, and the :ctype:`int` is set to the length of the buffer.  Only
   single-segment buffer objects are accepted; :exc:`TypeError` is raised for all
   others.

``(items)`` (tuple) [*matching-items*]
   The object must be a Python sequence whose length is the number of format units
   in *items*.  The C arguments must correspond to the individual format units in
   *items*.  Format units for sequences may be nested.

It is possible to pass "long" integers (integers whose value exceeds the
platform's :const:`LONG_MAX`) however no proper range checking is done --- the
most significant bits are silently truncated when the receiving field is too
small to receive the value (actually, the semantics are inherited from downcasts
in C --- your mileage may vary).

A few other characters have a meaning in a format string.  These may not occur
inside nested parentheses.  They are:

``|``
   Indicates that the remaining arguments in the Python argument list are optional.
   The C variables corresponding to optional arguments should be initialized to
   their default value --- when an optional argument is not specified,
   :cfunc:`PyArg_ParseTuple` does not touch the contents of the corresponding C
   variable(s).

``:``
   The list of format units ends here; the string after the colon is used as the
   function name in error messages (the "associated value" of the exception that
   :cfunc:`PyArg_ParseTuple` raises).

``;``
   The list of format units ends here; the string after the semicolon is used as
   the error message *instead* of the default error message.  Clearly, ``:`` and
   ``;`` mutually exclude each other.

Note that any Python object references which are provided to the caller are
*borrowed* references; do not decrement their reference count!

Additional arguments passed to these functions must be addresses of variables
whose type is determined by the format string; these are used to store values
from the input tuple.  There are a few cases, as described in the list of format
units above, where these parameters are used as input values; they should match
what is specified for the corresponding format unit in that case.

For the conversion to succeed, the *arg* object must match the format and the
format must be exhausted.  On success, the :cfunc:`PyArg_Parse\*` functions
return true, otherwise they return false and raise an appropriate exception.


.. cfunction:: int PyArg_ParseTuple(PyObject *args, const char *format, ...)

   Parse the parameters of a function that takes only positional parameters into
   local variables.  Returns true on success; on failure, it returns false and
   raises the appropriate exception.


.. cfunction:: int PyArg_VaParse(PyObject *args, const char *format, va_list vargs)

   Identical to :cfunc:`PyArg_ParseTuple`, except that it accepts a va_list rather
   than a variable number of arguments.


.. cfunction:: int PyArg_ParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char *keywords[], ...)

   Parse the parameters of a function that takes both positional and keyword
   parameters into local variables.  Returns true on success; on failure, it
   returns false and raises the appropriate exception.


.. cfunction:: int PyArg_VaParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char *keywords[], va_list vargs)

   Identical to :cfunc:`PyArg_ParseTupleAndKeywords`, except that it accepts a
   va_list rather than a variable number of arguments.


.. XXX deprecated, will be removed
.. cfunction:: int PyArg_Parse(PyObject *args, const char *format, ...)

   Function used to deconstruct the argument lists of "old-style" functions ---
   these are functions which use the :const:`METH_OLDARGS` parameter parsing
   method.  This is not recommended for use in parameter parsing in new code, and
   most code in the standard interpreter has been modified to no longer use this
   for that purpose.  It does remain a convenient way to decompose other tuples,
   however, and may continue to be used for that purpose.


.. cfunction:: int PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)

   A simpler form of parameter retrieval which does not use a format string to
   specify the types of the arguments.  Functions which use this method to retrieve
   their parameters should be declared as :const:`METH_VARARGS` in function or
   method tables.  The tuple containing the actual parameters should be passed as
   *args*; it must actually be a tuple.  The length of the tuple must be at least
   *min* and no more than *max*; *min* and *max* may be equal.  Additional
   arguments must be passed to the function, each of which should be a pointer to a
   :ctype:`PyObject\*` variable; these will be filled in with the values from
   *args*; they will contain borrowed references.  The variables which correspond
   to optional parameters not given by *args* will not be filled in; these should
   be initialized by the caller. This function returns true on success and false if
   *args* is not a tuple or contains the wrong number of elements; an exception
   will be set if there was a failure.

   This is an example of the use of this function, taken from the sources for the
   :mod:`_weakref` helper module for weak references::

      static PyObject *
      weakref_ref(PyObject *self, PyObject *args)
      {
          PyObject *object;
          PyObject *callback = NULL;
          PyObject *result = NULL;

          if (PyArg_UnpackTuple(args, "ref", 1, 2, &object, &callback)) {
              result = PyWeakref_NewRef(object, callback);
          }
          return result;
      }

   The call to :cfunc:`PyArg_UnpackTuple` in this example is entirely equivalent to
   this call to :cfunc:`PyArg_ParseTuple`::

      PyArg_ParseTuple(args, "O|O:ref", &object, &callback)


.. cfunction:: PyObject* Py_BuildValue(const char *format, ...)

   Create a new value based on a format string similar to those accepted by the
   :cfunc:`PyArg_Parse\*` family of functions and a sequence of values.  Returns
   the value or *NULL* in the case of an error; an exception will be raised if
   *NULL* is returned.

   :cfunc:`Py_BuildValue` does not always build a tuple.  It builds a tuple only if
   its format string contains two or more format units.  If the format string is
   empty, it returns ``None``; if it contains exactly one format unit, it returns
   whatever object is described by that format unit.  To force it to return a tuple
   of size 0 or one, parenthesize the format string.

   When memory buffers are passed as parameters to supply data to build objects, as
   for the ``s`` and ``s#`` formats, the required data is copied.  Buffers provided
   by the caller are never referenced by the objects created by
   :cfunc:`Py_BuildValue`.  In other words, if your code invokes :cfunc:`malloc`
   and passes the allocated memory to :cfunc:`Py_BuildValue`, your code is
   responsible for calling :cfunc:`free` for that memory once
   :cfunc:`Py_BuildValue` returns.

   In the following description, the quoted form is the format unit; the entry in
   (round) parentheses is the Python object type that the format unit will return;
   and the entry in [square] brackets is the type of the C value(s) to be passed.

   The characters space, tab, colon and comma are ignored in format strings (but
   not within format units such as ``s#``).  This can be used to make long format
   strings a tad more readable.

   ``s`` (string) [char \*]
      Convert a null-terminated C string to a Python object.  If the C string pointer
      is *NULL*, ``None`` is used.

   ``s#`` (string) [char \*, int]
      Convert a C string and its length to a Python object.  If the C string pointer
      is *NULL*, the length is ignored and ``None`` is returned.

   ``z`` (string or ``None``) [char \*]
      Same as ``s``.

   ``z#`` (string or ``None``) [char \*, int]
      Same as ``s#``.

   ``u`` (Unicode string) [Py_UNICODE \*]
      Convert a null-terminated buffer of Unicode (UCS-2 or UCS-4) data to a Python
      Unicode object.  If the Unicode buffer pointer is *NULL*, ``None`` is returned.

   ``u#`` (Unicode string) [Py_UNICODE \*, int]
      Convert a Unicode (UCS-2 or UCS-4) data buffer and its length to a Python
      Unicode object.   If the Unicode buffer pointer is *NULL*, the length is ignored
      and ``None`` is returned.

   ``U`` (string) [char \*]
      Convert a null-terminated C string to a Python unicode object. If the C string
      pointer is *NULL*, ``None`` is used.

   ``U#`` (string) [char \*, int]
      Convert a C string and its length to a Python unicode object. If the C string
      pointer is *NULL*, the length is ignored and ``None`` is returned.

   ``i`` (integer) [int]
      Convert a plain C :ctype:`int` to a Python integer object.

   ``b`` (integer) [char]
      Convert a plain C :ctype:`char` to a Python integer object.

   ``h`` (integer) [short int]
      Convert a plain C :ctype:`short int` to a Python integer object.

   ``l`` (integer) [long int]
      Convert a C :ctype:`long int` to a Python integer object.

   ``B`` (integer) [unsigned char]
      Convert a C :ctype:`unsigned char` to a Python integer object.

   ``H`` (integer) [unsigned short int]
      Convert a C :ctype:`unsigned short int` to a Python integer object.

   ``I`` (integer/long) [unsigned int]
      Convert a C :ctype:`unsigned int` to a Python long integer object.

   ``k`` (integer/long) [unsigned long]
      Convert a C :ctype:`unsigned long` to a Python long integer object.

   ``L`` (long) [PY_LONG_LONG]
      Convert a C :ctype:`long long` to a Python integer object. Only available
      on platforms that support :ctype:`long long`.

   ``K`` (long) [unsigned PY_LONG_LONG]
      Convert a C :ctype:`unsigned long long` to a Python integer object. Only
      available on platforms that support :ctype:`unsigned long long`.

   ``n`` (int) [Py_ssize_t]
      Convert a C :ctype:`Py_ssize_t` to a Python integer.

   ``c`` (string of length 1) [char]
      Convert a C :ctype:`int` representing a character to a Python string of length
      1.

   ``d`` (float) [double]
      Convert a C :ctype:`double` to a Python floating point number.

   ``f`` (float) [float]
      Same as ``d``.

   ``D`` (complex) [Py_complex \*]
      Convert a C :ctype:`Py_complex` structure to a Python complex number.

   ``O`` (object) [PyObject \*]
      Pass a Python object untouched (except for its reference count, which is
      incremented by one).  If the object passed in is a *NULL* pointer, it is assumed
      that this was caused because the call producing the argument found an error and
      set an exception. Therefore, :cfunc:`Py_BuildValue` will return *NULL* but won't
      raise an exception.  If no exception has been raised yet, :exc:`SystemError` is
      set.

   ``S`` (object) [PyObject \*]
      Same as ``O``.

   ``N`` (object) [PyObject \*]
      Same as ``O``, except it doesn't increment the reference count on the object.
      Useful when the object is created by a call to an object constructor in the
      argument list.

   ``O&`` (object) [*converter*, *anything*]
      Convert *anything* to a Python object through a *converter* function.  The
      function is called with *anything* (which should be compatible with :ctype:`void
      \*`) as its argument and should return a "new" Python object, or *NULL* if an
      error occurred.

   ``(items)`` (tuple) [*matching-items*]
      Convert a sequence of C values to a Python tuple with the same number of items.

   ``[items]`` (list) [*matching-items*]
      Convert a sequence of C values to a Python list with the same number of items.

   ``{items}`` (dictionary) [*matching-items*]
      Convert a sequence of C values to a Python dictionary.  Each pair of consecutive
      C values adds one item to the dictionary, serving as key and value,
      respectively.

   If there is an error in the format string, the :exc:`SystemError` exception is
   set and *NULL* returned.


.. _string-conversion:

String conversion and formatting
================================

Functions for number conversion and formatted string output.


.. cfunction:: int PyOS_snprintf(char *str, size_t size,  const char *format, ...)

   Output not more than *size* bytes to *str* according to the format string
   *format* and the extra arguments. See the Unix man page :manpage:`snprintf(2)`.


.. cfunction:: int PyOS_vsnprintf(char *str, size_t size, const char *format, va_list va)

   Output not more than *size* bytes to *str* according to the format string
   *format* and the variable argument list *va*. Unix man page
   :manpage:`vsnprintf(2)`.

:cfunc:`PyOS_snprintf` and :cfunc:`PyOS_vsnprintf` wrap the Standard C library
functions :cfunc:`snprintf` and :cfunc:`vsnprintf`. Their purpose is to
guarantee consistent behavior in corner cases, which the Standard C functions do
not.

The wrappers ensure that *str*[*size*-1] is always ``'\0'`` upon return. They
never write more than *size* bytes (including the trailing ``'\0'``) into str.
Both functions require that ``str != NULL``, ``size > 0`` and ``format !=
NULL``.

If the platform doesn't have :cfunc:`vsnprintf` and the buffer size needed to
avoid truncation exceeds *size* by more than 512 bytes, Python aborts with a
*Py_FatalError*.

The return value (*rv*) for these functions should be interpreted as follows:

* When ``0 <= rv < size``, the output conversion was successful and *rv*
  characters were written to *str* (excluding the trailing ``'\0'`` byte at
  *str*[*rv*]).

* When ``rv >= size``, the output conversion was truncated and a buffer with
  ``rv + 1`` bytes would have been needed to succeed. *str*[*size*-1] is ``'\0'``
  in this case.

* When ``rv < 0``, "something bad happened." *str*[*size*-1] is ``'\0'`` in
  this case too, but the rest of *str* is undefined. The exact cause of the error
  depends on the underlying platform.

The following functions provide locale-independent string to number conversions.


.. cfunction:: double PyOS_ascii_strtod(const char *nptr, char **endptr)

   Convert a string to a :ctype:`double`. This function behaves like the Standard C
   function :cfunc:`strtod` does in the C locale. It does this without changing the
   current locale, since that would not be thread-safe.

   :cfunc:`PyOS_ascii_strtod` should typically be used for reading configuration
   files or other non-user input that should be locale independent.

   See the Unix man page :manpage:`strtod(2)` for details.


.. cfunction:: char * PyOS_ascii_formatd(char *buffer, size_t buf_len, const char *format, double d)

   Convert a :ctype:`double` to a string using the ``'.'`` as the decimal
   separator. *format* is a :cfunc:`printf`\ -style format string specifying the
   number format. Allowed conversion characters are ``'e'``, ``'E'``, ``'f'``,
   ``'F'``, ``'g'`` and ``'G'``.

   The return value is a pointer to *buffer* with the converted string or NULL if
   the conversion failed.


.. cfunction:: double PyOS_ascii_atof(const char *nptr)

   Convert a string to a :ctype:`double` in a locale-independent way.

   See the Unix man page :manpage:`atof(2)` for details.

   
.. cfunction:: char * PyOS_stricmp(char *s1, char *s2)

   Case insensitive comparsion of strings. The functions works almost
   identical to :cfunc:`strcmp` except that it ignores the case.

   .. versionadded:: 2.6


.. cfunction:: char * PyOS_strnicmp(char *s1, char *s2, Py_ssize_t  size)

   Case insensitive comparsion of strings. The functions works almost
   identical to :cfunc:`strncmp` except that it ignores the case.

   .. versionadded:: 2.6


.. _reflection:

Reflection
==========

.. cfunction:: PyObject* PyEval_GetBuiltins()

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.


.. cfunction:: PyObject* PyEval_GetLocals()

   Return a dictionary of the local variables in the current execution frame,
   or *NULL* if no frame is currently executing.
   

.. cfunction:: PyObject* PyEval_GetGlobals()

   Return a dictionary of the global variables in the current execution frame,
   or *NULL* if no frame is currently executing.


.. cfunction:: PyFrameObject* PyEval_GetFrame()

   Return the current thread state's frame, which is *NULL* if no frame is
   currently executing.


.. cfunction:: int PyEval_GetRestricted()

   If there is a current frame and it is executing in restricted mode, return true,
   otherwise false.


.. cfunction:: const char* PyEval_GetFuncName(PyObject *func)

   Return the name of *func* if it is a function, class or instance object, else the
   name of *func*\s type.


.. cfunction:: const char* PyEval_GetFuncDesc(PyObject *func)

   Return a description string, depending on the type of *func*.
   Return values include "()" for functions and methods, " constructor",
   " instance", and " object".  Concatenated with the result of
   :cfunc:`PyEval_GetFuncName`, the result will be a description of
   *func*.
