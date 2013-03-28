:mod:`sys` --- System-specific parameters and functions
=======================================================

.. module:: sys
   :synopsis: Access system-specific parameters and functions.


This module provides access to some variables used or maintained by the
interpreter and to functions that interact strongly with the interpreter. It is
always available.


.. data:: abiflags

   On POSIX systems where Python is build with the standard ``configure``
   script, this contains the ABI flags as specified by :pep:`3149`.

   .. versionadded:: 3.2

.. data:: argv

   The list of command line arguments passed to a Python script. ``argv[0]`` is the
   script name (it is operating system dependent whether this is a full pathname or
   not).  If the command was executed using the :option:`-c` command line option to
   the interpreter, ``argv[0]`` is set to the string ``'-c'``.  If no script name
   was passed to the Python interpreter, ``argv[0]`` is the empty string.

   To loop over the standard input, or the list of files given on the
   command line, see the :mod:`fileinput` module.


.. data:: base_exec_prefix

   Set during Python startup, before ``site.py`` is run, to the same value as
   :data:`exec_prefix`. If not running in a
   :ref:`virtual environment <venv-def>`, the values will stay the same; if
   ``site.py`` finds that a virtual environment is in use, the values of
   :data:`prefix` and :data:`exec_prefix` will be changed to point to the
   virtual environment, whereas :data:`base_prefix` and
   :data:`base_exec_prefix` will remain pointing to the base Python
   installation (the one which the virtual environment was created from).

   .. versionadded:: 3.3


.. data:: base_prefix

   Set during Python startup, before ``site.py`` is run, to the same value as
   :data:`prefix`. If not running in a :ref:`virtual environment <venv-def>`, the values
   will stay the same; if ``site.py`` finds that a virtual environment is in
   use, the values of :data:`prefix` and :data:`exec_prefix` will be changed to
   point to the virtual environment, whereas :data:`base_prefix` and
   :data:`base_exec_prefix` will remain pointing to the base Python
   installation (the one which the virtual environment was created from).

   .. versionadded:: 3.3


.. data:: byteorder

   An indicator of the native byte order.  This will have the value ``'big'`` on
   big-endian (most-significant byte first) platforms, and ``'little'`` on
   little-endian (least-significant byte first) platforms.


.. data:: builtin_module_names

   A tuple of strings giving the names of all modules that are compiled into this
   Python interpreter.  (This information is not available in any other way ---
   ``modules.keys()`` only lists the imported modules.)


.. function:: call_tracing(func, args)

   Call ``func(*args)``, while tracing is enabled.  The tracing state is saved,
   and restored afterwards.  This is intended to be called from a debugger from
   a checkpoint, to recursively debug some other code.


.. data:: copyright

   A string containing the copyright pertaining to the Python interpreter.


.. function:: _clear_type_cache()

   Clear the internal type cache. The type cache is used to speed up attribute
   and method lookups. Use the function *only* to drop unnecessary references
   during reference leak debugging.

   This function should be used for internal and specialized purposes only.


.. function:: _current_frames()

   Return a dictionary mapping each thread's identifier to the topmost stack frame
   currently active in that thread at the time the function is called. Note that
   functions in the :mod:`traceback` module can build the call stack given such a
   frame.

   This is most useful for debugging deadlock:  this function does not require the
   deadlocked threads' cooperation, and such threads' call stacks are frozen for as
   long as they remain deadlocked.  The frame returned for a non-deadlocked thread
   may bear no relationship to that thread's current activity by the time calling
   code examines the frame.

   This function should be used for internal and specialized purposes only.


.. function:: _debugmallocstats()

   Print low-level information to stderr about the state of CPython's memory
   allocator.

   If Python is configured --with-pydebug, it also performs some expensive
   internal consistency checks.

   .. versionadded:: 3.3

   .. impl-detail::

      This function is specific to CPython.  The exact output format is not
      defined here, and may change.


.. data:: dllhandle

   Integer specifying the handle of the Python DLL. Availability: Windows.


.. function:: displayhook(value)

   If *value* is not ``None``, this function prints ``repr(value)`` to
   ``sys.stdout``, and saves *value* in ``builtins._``. If ``repr(value)`` is
   not encodable to ``sys.stdout.encoding`` with ``sys.stdout.errors`` error
   handler (which is probably ``'strict'``), encode it to
   ``sys.stdout.encoding`` with ``'backslashreplace'`` error handler.

   ``sys.displayhook`` is called on the result of evaluating an :term:`expression`
   entered in an interactive Python session.  The display of these values can be
   customized by assigning another one-argument function to ``sys.displayhook``.

   Pseudo-code::

       def displayhook(value):
           if value is None:
               return
           # Set '_' to None to avoid recursion
           builtins._ = None
           text = repr(value)
           try:
               sys.stdout.write(text)
           except UnicodeEncodeError:
               bytes = text.encode(sys.stdout.encoding, 'backslashreplace')
               if hasattr(sys.stdout, 'buffer'):
                   sys.stdout.buffer.write(bytes)
               else:
                   text = bytes.decode(sys.stdout.encoding, 'strict')
                   sys.stdout.write(text)
           sys.stdout.write("\n")
           builtins._ = value

   .. versionchanged:: 3.2
      Use ``'backslashreplace'`` error handler on :exc:`UnicodeEncodeError`.


.. data:: dont_write_bytecode

   If this is true, Python won't try to write ``.pyc`` or ``.pyo`` files on the
   import of source modules.  This value is initially set to ``True`` or
   ``False`` depending on the :option:`-B` command line option and the
   :envvar:`PYTHONDONTWRITEBYTECODE` environment variable, but you can set it
   yourself to control bytecode file generation.


.. function:: excepthook(type, value, traceback)

   This function prints out a given traceback and exception to ``sys.stderr``.

   When an exception is raised and uncaught, the interpreter calls
   ``sys.excepthook`` with three arguments, the exception class, exception
   instance, and a traceback object.  In an interactive session this happens just
   before control is returned to the prompt; in a Python program this happens just
   before the program exits.  The handling of such top-level exceptions can be
   customized by assigning another three-argument function to ``sys.excepthook``.


.. data:: __displayhook__
          __excepthook__

   These objects contain the original values of ``displayhook`` and ``excepthook``
   at the start of the program.  They are saved so that ``displayhook`` and
   ``excepthook`` can be restored in case they happen to get replaced with broken
   objects.


.. function:: exc_info()

   This function returns a tuple of three values that give information about the
   exception that is currently being handled.  The information returned is specific
   both to the current thread and to the current stack frame.  If the current stack
   frame is not handling an exception, the information is taken from the calling
   stack frame, or its caller, and so on until a stack frame is found that is
   handling an exception.  Here, "handling an exception" is defined as "executing
   an except clause."  For any stack frame, only information about the exception
   being currently handled is accessible.

   .. index:: object: traceback

   If no exception is being handled anywhere on the stack, a tuple containing
   three ``None`` values is returned.  Otherwise, the values returned are
   ``(type, value, traceback)``.  Their meaning is: *type* gets the type of the
   exception being handled (a subclass of :exc:`BaseException`); *value* gets
   the exception instance (an instance of the exception type); *traceback* gets
   a traceback object (see the Reference Manual) which encapsulates the call
   stack at the point where the exception originally occurred.


.. data:: exec_prefix

   A string giving the site-specific directory prefix where the platform-dependent
   Python files are installed; by default, this is also ``'/usr/local'``.  This can
   be set at build time with the ``--exec-prefix`` argument to the
   :program:`configure` script.  Specifically, all configuration files (e.g. the
   :file:`pyconfig.h` header file) are installed in the directory
   :file:`{exec_prefix}/lib/python{X.Y}/config`, and shared library modules are
   installed in :file:`{exec_prefix}/lib/python{X.Y}/lib-dynload`, where *X.Y*
   is the version number of Python, for example ``3.2``.

   .. note:: If a :ref:`virtual environment <venv-def>` is in effect, this
      value will be changed in ``site.py`` to point to the virtual environment.
      The value for the Python installation will still be available, via
      :data:`base_exec_prefix`.


.. data:: executable

   A string giving the absolute path of the executable binary for the Python
   interpreter, on systems where this makes sense. If Python is unable to retrieve
   the real path to its executable, :data:`sys.executable` will be an empty string
   or ``None``.


.. function:: exit([arg])

   Exit from Python.  This is implemented by raising the :exc:`SystemExit`
   exception, so cleanup actions specified by finally clauses of :keyword:`try`
   statements are honored, and it is possible to intercept the exit attempt at
   an outer level.

   The optional argument *arg* can be an integer giving the exit status
   (defaulting to zero), or another type of object.  If it is an integer, zero
   is considered "successful termination" and any nonzero value is considered
   "abnormal termination" by shells and the like.  Most systems require it to be
   in the range 0-127, and produce undefined results otherwise.  Some systems
   have a convention for assigning specific meanings to specific exit codes, but
   these are generally underdeveloped; Unix programs generally use 2 for command
   line syntax errors and 1 for all other kind of errors.  If another type of
   object is passed, ``None`` is equivalent to passing zero, and any other
   object is printed to :data:`stderr` and results in an exit code of 1.  In
   particular, ``sys.exit("some error message")`` is a quick way to exit a
   program when an error occurs.

   Since :func:`exit` ultimately "only" raises an exception, it will only exit
   the process when called from the main thread, and the exception is not
   intercepted.


.. data:: flags

   The :term:`struct sequence` *flags* exposes the status of command line
   flags. The attributes are read only.

   ============================= =============================
   attribute                     flag
   ============================= =============================
   :const:`debug`                :option:`-d`
   :const:`inspect`              :option:`-i`
   :const:`interactive`          :option:`-i`
   :const:`optimize`             :option:`-O` or :option:`-OO`
   :const:`dont_write_bytecode`  :option:`-B`
   :const:`no_user_site`         :option:`-s`
   :const:`no_site`              :option:`-S`
   :const:`ignore_environment`   :option:`-E`
   :const:`verbose`              :option:`-v`
   :const:`bytes_warning`        :option:`-b`
   :const:`quiet`                :option:`-q`
   :const:`hash_randomization`   :option:`-R`
   ============================= =============================

   .. versionchanged:: 3.2
      Added ``quiet`` attribute for the new :option:`-q` flag.

   .. versionadded:: 3.2.3
      The ``hash_randomization`` attribute.

   .. versionchanged:: 3.3
      Removed obsolete ``division_warning`` attribute.


.. data:: float_info

   A :term:`struct sequence` holding information about the float type. It
   contains low level information about the precision and internal
   representation.  The values correspond to the various floating-point
   constants defined in the standard header file :file:`float.h` for the 'C'
   programming language; see section 5.2.4.2.2 of the 1999 ISO/IEC C standard
   [C99]_, 'Characteristics of floating types', for details.

   .. tabularcolumns:: |l|l|L|

   +---------------------+----------------+--------------------------------------------------+
   | attribute           | float.h macro  | explanation                                      |
   +=====================+================+==================================================+
   | :const:`epsilon`    | DBL_EPSILON    | difference between 1 and the least value greater |
   |                     |                | than 1 that is representable as a float          |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`dig`        | DBL_DIG        | maximum number of decimal digits that can be     |
   |                     |                | faithfully represented in a float;  see below    |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`mant_dig`   | DBL_MANT_DIG   | float precision: the number of base-``radix``    |
   |                     |                | digits in the significand of a float             |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`max`        | DBL_MAX        | maximum representable finite float               |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`max_exp`    | DBL_MAX_EXP    | maximum integer e such that ``radix**(e-1)`` is  |
   |                     |                | a representable finite float                     |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`max_10_exp` | DBL_MAX_10_EXP | maximum integer e such that ``10**e`` is in the  |
   |                     |                | range of representable finite floats             |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`min`        | DBL_MIN        | minimum positive normalized float                |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`min_exp`    | DBL_MIN_EXP    | minimum integer e such that ``radix**(e-1)`` is  |
   |                     |                | a normalized float                               |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`min_10_exp` | DBL_MIN_10_EXP | minimum integer e such that ``10**e`` is a       |
   |                     |                | normalized float                                 |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`radix`      | FLT_RADIX      | radix of exponent representation                 |
   +---------------------+----------------+--------------------------------------------------+
   | :const:`rounds`     | FLT_ROUNDS     | integer constant representing the rounding mode  |
   |                     |                | used for arithmetic operations.  This reflects   |
   |                     |                | the value of the system FLT_ROUNDS macro at      |
   |                     |                | interpreter startup time.  See section 5.2.4.2.2 |
   |                     |                | of the C99 standard for an explanation of the    |
   |                     |                | possible values and their meanings.              |
   +---------------------+----------------+--------------------------------------------------+

   The attribute :attr:`sys.float_info.dig` needs further explanation.  If
   ``s`` is any string representing a decimal number with at most
   :attr:`sys.float_info.dig` significant digits, then converting ``s`` to a
   float and back again will recover a string representing the same decimal
   value::

      >>> import sys
      >>> sys.float_info.dig
      15
      >>> s = '3.14159265358979'    # decimal string with 15 significant digits
      >>> format(float(s), '.15g')  # convert to float and back -> same value
      '3.14159265358979'

   But for strings with more than :attr:`sys.float_info.dig` significant digits,
   this isn't always true::

      >>> s = '9876543211234567'    # 16 significant digits is too many!
      >>> format(float(s), '.16g')  # conversion changes value
      '9876543211234568'

.. data:: float_repr_style

   A string indicating how the :func:`repr` function behaves for
   floats.  If the string has value ``'short'`` then for a finite
   float ``x``, ``repr(x)`` aims to produce a short string with the
   property that ``float(repr(x)) == x``.  This is the usual behaviour
   in Python 3.1 and later.  Otherwise, ``float_repr_style`` has value
   ``'legacy'`` and ``repr(x)`` behaves in the same way as it did in
   versions of Python prior to 3.1.

   .. versionadded:: 3.1


.. function:: getcheckinterval()

   Return the interpreter's "check interval"; see :func:`setcheckinterval`.

   .. deprecated:: 3.2
      Use :func:`getswitchinterval` instead.


.. function:: getdefaultencoding()

   Return the name of the current default string encoding used by the Unicode
   implementation.


.. function:: getdlopenflags()

   Return the current value of the flags that are used for :c:func:`dlopen` calls.
   The flag constants are defined in the :mod:`ctypes` and :mod:`DLFCN` modules.
   Availability: Unix.


.. function:: getfilesystemencoding()

   Return the name of the encoding used to convert Unicode filenames into
   system file names. The result value depends on the operating system:

   * On Mac OS X, the encoding is ``'utf-8'``.

   * On Unix, the encoding is the user's preference according to the result of
     nl_langinfo(CODESET), or ``'utf-8'`` if ``nl_langinfo(CODESET)`` failed.

   * On Windows NT+, file names are Unicode natively, so no conversion is
     performed. :func:`getfilesystemencoding` still returns ``'mbcs'``, as
     this is the encoding that applications should use when they explicitly
     want to convert Unicode strings to byte strings that are equivalent when
     used as file names.

   * On Windows 9x, the encoding is ``'mbcs'``.

   .. versionchanged:: 3.2
      On Unix, use ``'utf-8'`` instead of ``None`` if ``nl_langinfo(CODESET)``
      failed. :func:`getfilesystemencoding` result cannot be ``None``.


.. function:: getrefcount(object)

   Return the reference count of the *object*.  The count returned is generally one
   higher than you might expect, because it includes the (temporary) reference as
   an argument to :func:`getrefcount`.


.. function:: getrecursionlimit()

   Return the current value of the recursion limit, the maximum depth of the Python
   interpreter stack.  This limit prevents infinite recursion from causing an
   overflow of the C stack and crashing Python.  It can be set by
   :func:`setrecursionlimit`.


.. function:: getsizeof(object[, default])

   Return the size of an object in bytes. The object can be any type of
   object. All built-in objects will return correct results, but this
   does not have to hold true for third-party extensions as it is implementation
   specific.

   Only the memory consumption directly attributed to the object is
   accounted for, not the memory consumption of objects it refers to.

   If given, *default* will be returned if the object does not provide means to
   retrieve the size.  Otherwise a :exc:`TypeError` will be raised.

   :func:`getsizeof` calls the object's ``__sizeof__`` method and adds an
   additional garbage collector overhead if the object is managed by the garbage
   collector.

   See `recursive sizeof recipe <http://code.activestate.com/recipes/577504>`_
   for an example of using :func:`getsizeof` recursively to find the size of
   containers and all their contents.

.. function:: getswitchinterval()

   Return the interpreter's "thread switch interval"; see
   :func:`setswitchinterval`.

   .. versionadded:: 3.2


.. function:: _getframe([depth])

   Return a frame object from the call stack.  If optional integer *depth* is
   given, return the frame object that many calls below the top of the stack.  If
   that is deeper than the call stack, :exc:`ValueError` is raised.  The default
   for *depth* is zero, returning the frame at the top of the call stack.

   .. impl-detail::

      This function should be used for internal and specialized purposes only.
      It is not guaranteed to exist in all implementations of Python.


.. function:: getprofile()

   .. index::
      single: profile function
      single: profiler

   Get the profiler function as set by :func:`setprofile`.


.. function:: gettrace()

   .. index::
      single: trace function
      single: debugger

   Get the trace function as set by :func:`settrace`.

   .. impl-detail::

      The :func:`gettrace` function is intended only for implementing debuggers,
      profilers, coverage tools and the like.  Its behavior is part of the
      implementation platform, rather than part of the language definition, and
      thus may not be available in all Python implementations.


.. function:: getwindowsversion()

   Return a named tuple describing the Windows version
   currently running.  The named elements are *major*, *minor*,
   *build*, *platform*, *service_pack*, *service_pack_minor*,
   *service_pack_major*, *suite_mask*, and *product_type*.
   *service_pack* contains a string while all other values are
   integers. The components can also be accessed by name, so
   ``sys.getwindowsversion()[0]`` is equivalent to
   ``sys.getwindowsversion().major``. For compatibility with prior
   versions, only the first 5 elements are retrievable by indexing.

   *platform* may be one of the following values:

   +-----------------------------------------+-------------------------+
   | Constant                                | Platform                |
   +=========================================+=========================+
   | :const:`0 (VER_PLATFORM_WIN32s)`        | Win32s on Windows 3.1   |
   +-----------------------------------------+-------------------------+
   | :const:`1 (VER_PLATFORM_WIN32_WINDOWS)` | Windows 95/98/ME        |
   +-----------------------------------------+-------------------------+
   | :const:`2 (VER_PLATFORM_WIN32_NT)`      | Windows NT/2000/XP/x64  |
   +-----------------------------------------+-------------------------+
   | :const:`3 (VER_PLATFORM_WIN32_CE)`      | Windows CE              |
   +-----------------------------------------+-------------------------+

   *product_type* may be one of the following values:

   +---------------------------------------+---------------------------------+
   | Constant                              | Meaning                         |
   +=======================================+=================================+
   | :const:`1 (VER_NT_WORKSTATION)`       | The system is a workstation.    |
   +---------------------------------------+---------------------------------+
   | :const:`2 (VER_NT_DOMAIN_CONTROLLER)` | The system is a domain          |
   |                                       | controller.                     |
   +---------------------------------------+---------------------------------+
   | :const:`3 (VER_NT_SERVER)`            | The system is a server, but not |
   |                                       | a domain controller.            |
   +---------------------------------------+---------------------------------+


   This function wraps the Win32 :c:func:`GetVersionEx` function; see the
   Microsoft documentation on :c:func:`OSVERSIONINFOEX` for more information
   about these fields.

   Availability: Windows.

   .. versionchanged:: 3.2
      Changed to a named tuple and added *service_pack_minor*,
      *service_pack_major*, *suite_mask*, and *product_type*.


.. data:: hash_info

   A :term:`struct sequence` giving parameters of the numeric hash
   implementation.  For more details about hashing of numeric types, see
   :ref:`numeric-hash`.

   +---------------------+--------------------------------------------------+
   | attribute           | explanation                                      |
   +=====================+==================================================+
   | :const:`width`      | width in bits used for hash values               |
   +---------------------+--------------------------------------------------+
   | :const:`modulus`    | prime modulus P used for numeric hash scheme     |
   +---------------------+--------------------------------------------------+
   | :const:`inf`        | hash value returned for a positive infinity      |
   +---------------------+--------------------------------------------------+
   | :const:`nan`        | hash value returned for a nan                    |
   +---------------------+--------------------------------------------------+
   | :const:`imag`       | multiplier used for the imaginary part of a      |
   |                     | complex number                                   |
   +---------------------+--------------------------------------------------+

   .. versionadded:: 3.2


.. data:: hexversion

   The version number encoded as a single integer.  This is guaranteed to increase
   with each version, including proper support for non-production releases.  For
   example, to test that the Python interpreter is at least version 1.5.2, use::

      if sys.hexversion >= 0x010502F0:
          # use some advanced feature
          ...
      else:
          # use an alternative implementation or warn the user
          ...

   This is called ``hexversion`` since it only really looks meaningful when viewed
   as the result of passing it to the built-in :func:`hex` function.  The
   :term:`struct sequence`  :data:`sys.version_info` may be used for a more
   human-friendly encoding of the same information.

   More details of ``hexversion`` can be found at :ref:`apiabiversion`


.. data:: implementation

   An object containing information about the implementation of the
   currently running Python interpreter.  The following attributes are
   required to exist in all Python implementations.

   *name* is the implementation's identifier, e.g. ``'cpython'``.  The actual
   string is defined by the Python implementation, but it is guaranteed to be
   lower case.

   *version* is a named tuple, in the same format as
   :data:`sys.version_info`.  It represents the version of the Python
   *implementation*.  This has a distinct meaning from the specific
   version of the Python *language* to which the currently running
   interpreter conforms, which ``sys.version_info`` represents.  For
   example, for PyPy 1.8 ``sys.implementation.version`` might be
   ``sys.version_info(1, 8, 0, 'final', 0)``, whereas ``sys.version_info``
   would be ``sys.version_info(2, 7, 2, 'final', 0)``.  For CPython they
   are the same value, since it is the reference implementation.

   *hexversion* is the implementation version in hexadecimal format, like
   :data:`sys.hexversion`.

   *cache_tag* is the tag used by the import machinery in the filenames of
   cached modules.  By convention, it would be a composite of the
   implementation's name and version, like ``'cpython-33'``.  However, a
   Python implementation may use some other value if appropriate.  If
   ``cache_tag`` is set to ``None``, it indicates that module caching should
   be disabled.

   :data:`sys.implementation` may contain additional attributes specific to
   the Python implementation.  These non-standard attributes must start with
   an underscore, and are not described here.  Regardless of its contents,
   :data:`sys.implementation` will not change during a run of the interpreter,
   nor between implementation versions.  (It may change between Python
   language versions, however.)  See `PEP 421` for more information.

   .. versionadded:: 3.3


.. data:: int_info

   A :term:`struct sequence` that holds information about Python's internal
   representation of integers.  The attributes are read only.

   .. tabularcolumns:: |l|L|

   +-------------------------+----------------------------------------------+
   | Attribute               | Explanation                                  |
   +=========================+==============================================+
   | :const:`bits_per_digit` | number of bits held in each digit.  Python   |
   |                         | integers are stored internally in base       |
   |                         | ``2**int_info.bits_per_digit``               |
   +-------------------------+----------------------------------------------+
   | :const:`sizeof_digit`   | size in bytes of the C type used to          |
   |                         | represent a digit                            |
   +-------------------------+----------------------------------------------+

   .. versionadded:: 3.1


.. function:: intern(string)

   Enter *string* in the table of "interned" strings and return the interned string
   -- which is *string* itself or a copy. Interning strings is useful to gain a
   little performance on dictionary lookup -- if the keys in a dictionary are
   interned, and the lookup key is interned, the key comparisons (after hashing)
   can be done by a pointer compare instead of a string compare.  Normally, the
   names used in Python programs are automatically interned, and the dictionaries
   used to hold module, class or instance attributes have interned keys.

   Interned strings are not immortal; you must keep a reference to the return
   value of :func:`intern` around to benefit from it.


.. data:: last_type
          last_value
          last_traceback

   These three variables are not always defined; they are set when an exception is
   not handled and the interpreter prints an error message and a stack traceback.
   Their intended use is to allow an interactive user to import a debugger module
   and engage in post-mortem debugging without having to re-execute the command
   that caused the error.  (Typical use is ``import pdb; pdb.pm()`` to enter the
   post-mortem debugger; see :mod:`pdb` module for
   more information.)

   The meaning of the variables is the same as that of the return values from
   :func:`exc_info` above.


.. data:: maxsize

   An integer giving the maximum value a variable of type :c:type:`Py_ssize_t` can
   take.  It's usually ``2**31 - 1`` on a 32-bit platform and ``2**63 - 1`` on a
   64-bit platform.


.. data:: maxunicode

   An integer giving the value of the largest Unicode code point,
   i.e. ``1114111`` (``0x10FFFF`` in hexadecimal).

   .. versionchanged:: 3.3
      Before :pep:`393`, ``sys.maxunicode`` used to be either ``0xFFFF``
      or ``0x10FFFF``, depending on the configuration option that specified
      whether Unicode characters were stored as UCS-2 or UCS-4.


.. data:: meta_path

    A list of :term:`finder` objects that have their :meth:`find_module`
    methods called to see if one of the objects can find the module to be
    imported. The :meth:`find_module` method is called at least with the
    absolute name of the module being imported. If the module to be imported is
    contained in package then the parent package's :attr:`__path__` attribute
    is passed in as a second argument. The method returns ``None`` if
    the module cannot be found, else returns a :term:`loader`.

    :data:`sys.meta_path` is searched before any implicit default finders or
    :data:`sys.path`.

    See :pep:`302` for the original specification.


.. data:: modules

   This is a dictionary that maps module names to modules which have already been
   loaded.  This can be manipulated to force reloading of modules and other tricks.


.. data:: path

   .. index:: triple: module; search; path

   A list of strings that specifies the search path for modules. Initialized from
   the environment variable :envvar:`PYTHONPATH`, plus an installation-dependent
   default.

   As initialized upon program startup, the first item of this list, ``path[0]``,
   is the directory containing the script that was used to invoke the Python
   interpreter.  If the script directory is not available (e.g.  if the interpreter
   is invoked interactively or if the script is read from standard input),
   ``path[0]`` is the empty string, which directs Python to search modules in the
   current directory first.  Notice that the script directory is inserted *before*
   the entries inserted as a result of :envvar:`PYTHONPATH`.

   A program is free to modify this list for its own purposes.  Only strings
   and bytes should be added to :data:`sys.path`; all other data types are
   ignored during import.


   .. seealso::
      Module :mod:`site` This describes how to use .pth files to extend
      :data:`sys.path`.


.. data:: path_hooks

    A list of callables that take a path argument to try to create a
    :term:`finder` for the path. If a finder can be created, it is to be
    returned by the callable, else raise :exc:`ImportError`.

    Originally specified in :pep:`302`.


.. data:: path_importer_cache

    A dictionary acting as a cache for :term:`finder` objects. The keys are
    paths that have been passed to :data:`sys.path_hooks` and the values are
    the finders that are found. If a path is a valid file system path but no
    finder is found on :data:`sys.path_hooks` then ``None`` is
    stored.

    Originally specified in :pep:`302`.

    .. versionchanged:: 3.3
       ``None`` is stored instead of :class:`imp.NullImporter` when no finder
       is found.


.. data:: platform

   This string contains a platform identifier that can be used to append
   platform-specific components to :data:`sys.path`, for instance.

   For Unix systems, except on Linux, this is the lowercased OS name as
   returned by ``uname -s`` with the first part of the version as returned by
   ``uname -r`` appended, e.g. ``'sunos5'`` or ``'freebsd8'``, *at the time
   when Python was built*.  Unless you want to test for a specific system
   version, it is therefore recommended to use the following idiom::

      if sys.platform.startswith('freebsd'):
          # FreeBSD-specific code here...
      elif sys.platform.startswith('linux'):
          # Linux-specific code here...

   For other systems, the values are:

   ================ ===========================
   System           ``platform`` value
   ================ ===========================
   Linux            ``'linux'``
   Windows          ``'win32'``
   Windows/Cygwin   ``'cygwin'``
   Mac OS X         ``'darwin'``
   OS/2             ``'os2'``
   OS/2 EMX         ``'os2emx'``
   ================ ===========================

   .. versionchanged:: 3.3
      On Linux, :attr:`sys.platform` doesn't contain the major version anymore.
      It is always ``'linux'``, instead of ``'linux2'`` or ``'linux3'``.  Since
      older Python versions include the version number, it is recommended to
      always use the ``startswith`` idiom presented above.

   .. seealso::

      :attr:`os.name` has a coarser granularity.  :func:`os.uname` gives
      system-dependent version information.

      The :mod:`platform` module provides detailed checks for the
      system's identity.


.. data:: prefix

   A string giving the site-specific directory prefix where the platform
   independent Python files are installed; by default, this is the string
   ``'/usr/local'``.  This can be set at build time with the ``--prefix``
   argument to the :program:`configure` script.  The main collection of Python
   library modules is installed in the directory :file:`{prefix}/lib/python{X.Y}`
   while the platform independent header files (all except :file:`pyconfig.h`) are
   stored in :file:`{prefix}/include/python{X.Y}`, where *X.Y* is the version
   number of Python, for example ``3.2``.

   .. note:: If a :ref:`virtual environment <venv-def>` is in effect, this
      value will be changed in ``site.py`` to point to the virtual
      environment. The value for the Python installation will still be
      available, via :data:`base_prefix`.


.. data:: ps1
          ps2

   .. index::
      single: interpreter prompts
      single: prompts, interpreter

   Strings specifying the primary and secondary prompt of the interpreter.  These
   are only defined if the interpreter is in interactive mode.  Their initial
   values in this case are ``'>>> '`` and ``'... '``.  If a non-string object is
   assigned to either variable, its :func:`str` is re-evaluated each time the
   interpreter prepares to read a new interactive command; this can be used to
   implement a dynamic prompt.


.. function:: setcheckinterval(interval)

   Set the interpreter's "check interval".  This integer value determines how often
   the interpreter checks for periodic things such as thread switches and signal
   handlers.  The default is ``100``, meaning the check is performed every 100
   Python virtual instructions. Setting it to a larger value may increase
   performance for programs using threads.  Setting it to a value ``<=`` 0 checks
   every virtual instruction, maximizing responsiveness as well as overhead.

   .. deprecated:: 3.2
      This function doesn't have an effect anymore, as the internal logic for
      thread switching and asynchronous tasks has been rewritten.  Use
      :func:`setswitchinterval` instead.


.. function:: setdlopenflags(n)

   Set the flags used by the interpreter for :c:func:`dlopen` calls, such as when
   the interpreter loads extension modules.  Among other things, this will enable a
   lazy resolving of symbols when importing a module, if called as
   ``sys.setdlopenflags(0)``.  To share symbols across extension modules, call as
   ``sys.setdlopenflags(os.RTLD_GLOBAL)``.  Symbolic names for the flag modules
   can be found in the :mod:`os` module (``RTLD_xxx`` constants, e.g.
   :data:`os.RTLD_LAZY`).

   Availability: Unix.

.. function:: setprofile(profilefunc)

   .. index::
      single: profile function
      single: profiler

   Set the system's profile function, which allows you to implement a Python source
   code profiler in Python.  See chapter :ref:`profile` for more information on the
   Python profiler.  The system's profile function is called similarly to the
   system's trace function (see :func:`settrace`), but it isn't called for each
   executed line of code (only on call and return, but the return event is reported
   even when an exception has been set).  The function is thread-specific, but
   there is no way for the profiler to know about context switches between threads,
   so it does not make sense to use this in the presence of multiple threads. Also,
   its return value is not used, so it can simply return ``None``.


.. function:: setrecursionlimit(limit)

   Set the maximum depth of the Python interpreter stack to *limit*.  This limit
   prevents infinite recursion from causing an overflow of the C stack and crashing
   Python.

   The highest possible limit is platform-dependent.  A user may need to set the
   limit higher when they have a program that requires deep recursion and a platform
   that supports a higher limit.  This should be done with care, because a too-high
   limit can lead to a crash.


.. function:: setswitchinterval(interval)

   Set the interpreter's thread switch interval (in seconds).  This floating-point
   value determines the ideal duration of the "timeslices" allocated to
   concurrently running Python threads.  Please note that the actual value
   can be higher, especially if long-running internal functions or methods
   are used.  Also, which thread becomes scheduled at the end of the interval
   is the operating system's decision.  The interpreter doesn't have its
   own scheduler.

   .. versionadded:: 3.2


.. function:: settrace(tracefunc)

   .. index::
      single: trace function
      single: debugger

   Set the system's trace function, which allows you to implement a Python
   source code debugger in Python.  The function is thread-specific; for a
   debugger to support multiple threads, it must be registered using
   :func:`settrace` for each thread being debugged.

   Trace functions should have three arguments: *frame*, *event*, and
   *arg*. *frame* is the current stack frame.  *event* is a string: ``'call'``,
   ``'line'``, ``'return'``, ``'exception'``, ``'c_call'``, ``'c_return'``, or
   ``'c_exception'``. *arg* depends on the event type.

   The trace function is invoked (with *event* set to ``'call'``) whenever a new
   local scope is entered; it should return a reference to a local trace
   function to be used that scope, or ``None`` if the scope shouldn't be traced.

   The local trace function should return a reference to itself (or to another
   function for further tracing in that scope), or ``None`` to turn off tracing
   in that scope.

   The events have the following meaning:

   ``'call'``
      A function is called (or some other code block entered).  The
      global trace function is called; *arg* is ``None``; the return value
      specifies the local trace function.

   ``'line'``
      The interpreter is about to execute a new line of code or re-execute the
      condition of a loop.  The local trace function is called; *arg* is
      ``None``; the return value specifies the new local trace function.  See
      :file:`Objects/lnotab_notes.txt` for a detailed explanation of how this
      works.

   ``'return'``
      A function (or other code block) is about to return.  The local trace
      function is called; *arg* is the value that will be returned, or ``None``
      if the event is caused by an exception being raised.  The trace function's
      return value is ignored.

   ``'exception'``
      An exception has occurred.  The local trace function is called; *arg* is a
      tuple ``(exception, value, traceback)``; the return value specifies the
      new local trace function.

   ``'c_call'``
      A C function is about to be called.  This may be an extension function or
      a built-in.  *arg* is the C function object.

   ``'c_return'``
      A C function has returned. *arg* is the C function object.

   ``'c_exception'``
      A C function has raised an exception.  *arg* is the C function object.

   Note that as an exception is propagated down the chain of callers, an
   ``'exception'`` event is generated at each level.

   For more information on code and frame objects, refer to :ref:`types`.

   .. impl-detail::

      The :func:`settrace` function is intended only for implementing debuggers,
      profilers, coverage tools and the like.  Its behavior is part of the
      implementation platform, rather than part of the language definition, and
      thus may not be available in all Python implementations.


.. function:: settscdump(on_flag)

   Activate dumping of VM measurements using the Pentium timestamp counter, if
   *on_flag* is true. Deactivate these dumps if *on_flag* is off. The function is
   available only if Python was compiled with ``--with-tsc``. To understand
   the output of this dump, read :file:`Python/ceval.c` in the Python sources.

   .. impl-detail::
      This function is intimately bound to CPython implementation details and
      thus not likely to be implemented elsewhere.


.. data:: stdin
          stdout
          stderr

   :term:`File objects <file object>` used by the interpreter for standard
   input, output and errors:

   * ``stdin`` is used for all interactive input (including calls to
     :func:`input`);
   * ``stdout`` is used for the output of :func:`print` and :term:`expression`
     statements and for the prompts of :func:`input`;
   * The interpreter's own prompts and its error messages go to ``stderr``.

   By default, these streams are regular text streams as returned by the
   :func:`open` function.  Their parameters are chosen as follows:

   * The character encoding is platform-dependent.  Under Windows, if the stream
     is interactive (that is, if its :meth:`isatty` method returns True), the
     console codepage is used, otherwise the ANSI code page.  Under other
     platforms, the locale encoding is used (see :meth:`locale.getpreferredencoding`).

     Under all platforms though, you can override this value by setting the
     :envvar:`PYTHONIOENCODING` environment variable.

   * When interactive, standard streams are line-buffered.  Otherwise, they
     are block-buffered like regular text files.  You can override this
     value with the :option:`-u` command-line option.

   To write or read binary data from/to the standard streams, use the
   underlying binary :data:`~io.TextIOBase.buffer`.  For example, to write
   bytes to :data:`stdout`, use ``sys.stdout.buffer.write(b'abc')``.  Using
   :meth:`io.TextIOBase.detach`, streams can be made binary by default.  This
   function sets :data:`stdin` and :data:`stdout` to binary::

      def make_streams_binary():
          sys.stdin = sys.stdin.detach()
          sys.stdout = sys.stdout.detach()

   Note that the streams may be replaced with objects (like :class:`io.StringIO`)
   that do not support the :attr:`~io.BufferedIOBase.buffer` attribute or the
   :meth:`~io.BufferedIOBase.detach` method and can raise :exc:`AttributeError`
   or :exc:`io.UnsupportedOperation`.


.. data:: __stdin__
          __stdout__
          __stderr__

   These objects contain the original values of ``stdin``, ``stderr`` and
   ``stdout`` at the start of the program.  They are used during finalization,
   and could be useful to print to the actual standard stream no matter if the
   ``sys.std*`` object has been redirected.

   It can also be used to restore the actual files to known working file objects
   in case they have been overwritten with a broken object.  However, the
   preferred way to do this is to explicitly save the previous stream before
   replacing it, and restore the saved object.

   .. note::
       Under some conditions ``stdin``, ``stdout`` and ``stderr`` as well as the
       original values ``__stdin__``, ``__stdout__`` and ``__stderr__`` can be
       None. It is usually the case for Windows GUI apps that aren't connected
       to a console and Python apps started with :program:`pythonw`.


.. data:: thread_info

   A :term:`struct sequence` holding information about the thread
   implementation.

   .. tabularcolumns:: |l|p{0.7\linewidth}|

   +------------------+---------------------------------------------------------+
   | Attribute        | Explanation                                             |
   +==================+=========================================================+
   | :const:`name`    | Name of the thread implementation:                      |
   |                  |                                                         |
   |                  |  * ``'nt'``: Windows threads                            |
   |                  |  * ``'os2'``: OS/2 threads                              |
   |                  |  * ``'pthread'``: POSIX threads                         |
   |                  |  * ``'solaris'``: Solaris threads                       |
   +------------------+---------------------------------------------------------+
   | :const:`lock`    | Name of the lock implementation:                        |
   |                  |                                                         |
   |                  |  * ``'semaphore'``: a lock uses a semaphore             |
   |                  |  * ``'mutex+cond'``: a lock uses a mutex                |
   |                  |    and a condition variable                             |
   |                  |  * ``None`` if this information is unknown              |
   +------------------+---------------------------------------------------------+
   | :const:`version` | Name and version of the thread library. It is a string, |
   |                  | or ``None`` if these informations are unknown.          |
   +------------------+---------------------------------------------------------+

   .. versionadded:: 3.3


.. data:: tracebacklimit

   When this variable is set to an integer value, it determines the maximum number
   of levels of traceback information printed when an unhandled exception occurs.
   The default is ``1000``.  When set to ``0`` or less, all traceback information
   is suppressed and only the exception type and value are printed.


.. data:: version

   A string containing the version number of the Python interpreter plus additional
   information on the build number and compiler used.  This string is displayed
   when the interactive interpreter is started.  Do not extract version information
   out of it, rather, use :data:`version_info` and the functions provided by the
   :mod:`platform` module.


.. data:: api_version

   The C API version for this interpreter.  Programmers may find this useful when
   debugging version conflicts between Python and extension modules.


.. data:: version_info

   A tuple containing the five components of the version number: *major*, *minor*,
   *micro*, *releaselevel*, and *serial*.  All values except *releaselevel* are
   integers; the release level is ``'alpha'``, ``'beta'``, ``'candidate'``, or
   ``'final'``.  The ``version_info`` value corresponding to the Python version 2.0
   is ``(2, 0, 0, 'final', 0)``.  The components can also be accessed by name,
   so ``sys.version_info[0]`` is equivalent to ``sys.version_info.major``
   and so on.

   .. versionchanged:: 3.1
      Added named component attributes.

.. data:: warnoptions

   This is an implementation detail of the warnings framework; do not modify this
   value.  Refer to the :mod:`warnings` module for more information on the warnings
   framework.


.. data:: winver

   The version number used to form registry keys on Windows platforms. This is
   stored as string resource 1000 in the Python DLL.  The value is normally the
   first three characters of :const:`version`.  It is provided in the :mod:`sys`
   module for informational purposes; modifying this value has no effect on the
   registry keys used by Python. Availability: Windows.


.. data:: _xoptions

   A dictionary of the various implementation-specific flags passed through
   the :option:`-X` command-line option.  Option names are either mapped to
   their values, if given explicitly, or to :const:`True`.  Example::

      $ ./python -Xa=b -Xc
      Python 3.2a3+ (py3k, Oct 16 2010, 20:14:50)
      [GCC 4.4.3] on linux2
      Type "help", "copyright", "credits" or "license" for more information.
      >>> import sys
      >>> sys._xoptions
      {'a': 'b', 'c': True}

   .. impl-detail::

      This is a CPython-specific way of accessing options passed through
      :option:`-X`.  Other implementations may export them through other
      means, or not at all.

   .. versionadded:: 3.2


.. rubric:: Citations

.. [C99] ISO/IEC 9899:1999.  "Programming languages -- C."  A public draft of this standard is available at http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf .

