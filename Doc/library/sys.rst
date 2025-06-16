:mod:`!sys` --- System-specific parameters and functions
========================================================

.. module:: sys
   :synopsis: Access system-specific parameters and functions.

--------------

This module provides access to some variables used or maintained by the
interpreter and to functions that interact strongly with the interpreter. It is
always available. Unless explicitly noted otherwise, all variables are read-only.


.. data:: abiflags

   On POSIX systems where Python was built with the standard ``configure``
   script, this contains the ABI flags as specified by :pep:`3149`.

   .. versionadded:: 3.2

   .. versionchanged:: 3.8
      Default flags became an empty string (``m`` flag for pymalloc has been
      removed).

   .. availability:: Unix.


.. function:: addaudithook(hook)

   Append the callable *hook* to the list of active auditing hooks for the
   current (sub)interpreter.

   When an auditing event is raised through the :func:`sys.audit` function, each
   hook will be called in the order it was added with the event name and the
   tuple of arguments. Native hooks added by :c:func:`PySys_AddAuditHook` are
   called first, followed by hooks added in the current (sub)interpreter.  Hooks
   can then log the event, raise an exception to abort the operation,
   or terminate the process entirely.

   Note that audit hooks are primarily for collecting information about internal
   or otherwise unobservable actions, whether by Python or libraries written in
   Python. They are not suitable for implementing a "sandbox". In particular,
   malicious code can trivially disable or bypass hooks added using this
   function. At a minimum, any security-sensitive hooks must be added using the
   C API :c:func:`PySys_AddAuditHook` before initialising the runtime, and any
   modules allowing arbitrary memory modification (such as :mod:`ctypes`) should
   be completely removed or closely monitored.

   .. audit-event:: sys.addaudithook "" sys.addaudithook

      Calling :func:`sys.addaudithook` will itself raise an auditing event
      named ``sys.addaudithook`` with no arguments. If any
      existing hooks raise an exception derived from :class:`RuntimeError`, the
      new hook will not be added and the exception suppressed. As a result,
      callers cannot assume that their hook has been added unless they control
      all existing hooks.

   See the :ref:`audit events table <audit-events>` for all events raised by
   CPython, and :pep:`578` for the original design discussion.

   .. versionadded:: 3.8

   .. versionchanged:: 3.8.1

      Exceptions derived from :class:`Exception` but not :class:`RuntimeError`
      are no longer suppressed.

   .. impl-detail::

      When tracing is enabled (see :func:`settrace`), Python hooks are only
      traced if the callable has a ``__cantrace__`` member that is set to a
      true value. Otherwise, trace functions will skip the hook.


.. data:: argv

   The list of command line arguments passed to a Python script. ``argv[0]`` is the
   script name (it is operating system dependent whether this is a full pathname or
   not).  If the command was executed using the :option:`-c` command line option to
   the interpreter, ``argv[0]`` is set to the string ``'-c'``.  If no script name
   was passed to the Python interpreter, ``argv[0]`` is the empty string.

   To loop over the standard input, or the list of files given on the
   command line, see the :mod:`fileinput` module.

   See also :data:`sys.orig_argv`.

   .. note::
      On Unix, command line arguments are passed by bytes from OS.  Python decodes
      them with filesystem encoding and "surrogateescape" error handler.
      When you need original bytes, you can get it by
      ``[os.fsencode(arg) for arg in sys.argv]``.


.. _auditing:

.. function:: audit(event, *args)

   .. index:: single: auditing

   Raise an auditing event and trigger any active auditing hooks.
   *event* is a string identifying the event, and *args* may contain
   optional arguments with more information about the event.  The
   number and types of arguments for a given event are considered a
   public and stable API and should not be modified between releases.

   For example, one auditing event is named ``os.chdir``. This event has
   one argument called *path* that will contain the requested new
   working directory.

   :func:`sys.audit` will call the existing auditing hooks, passing
   the event name and arguments, and will re-raise the first exception
   from any hook. In general, if an exception is raised, it should not
   be handled and the process should be terminated as quickly as
   possible. This allows hook implementations to decide how to respond
   to particular events: they can merely log the event or abort the
   operation by raising an exception.

   Hooks are added using the :func:`sys.addaudithook` or
   :c:func:`PySys_AddAuditHook` functions.

   The native equivalent of this function is :c:func:`PySys_Audit`. Using the
   native function is preferred when possible.

   See the :ref:`audit events table <audit-events>` for all events raised by
   CPython.

   .. versionadded:: 3.8


.. data:: base_exec_prefix

   Equivalent to :data:`exec_prefix`, but referring to the base Python installation.

   When running under :ref:`sys-path-init-virtual-environments`,
   :data:`exec_prefix` gets overwritten to the virtual environment prefix.
   :data:`base_exec_prefix`, conversely, does not change, and always points to
   the base Python installation.
   Refer to :ref:`sys-path-init-virtual-environments` for more information.

   .. versionadded:: 3.3


.. data:: base_prefix

   Equivalent to :data:`prefix`, but referring to the base Python installation.

   When running under :ref:`virtual environment <venv-def>`,
   :data:`prefix` gets overwritten to the virtual environment prefix.
   :data:`base_prefix`, conversely, does not change, and always points to
   the base Python installation.
   Refer to :ref:`sys-path-init-virtual-environments` for more information.

   .. versionadded:: 3.3


.. data:: byteorder

   An indicator of the native byte order.  This will have the value ``'big'`` on
   big-endian (most-significant byte first) platforms, and ``'little'`` on
   little-endian (least-significant byte first) platforms.


.. data:: builtin_module_names

   A tuple of strings containing the names of all modules that are compiled into this
   Python interpreter.  (This information is not available in any other way ---
   ``modules.keys()`` only lists the imported modules.)

   See also the :data:`sys.stdlib_module_names` list.


.. function:: call_tracing(func, args)

   Call ``func(*args)``, while tracing is enabled.  The tracing state is saved,
   and restored afterwards.  This is intended to be called from a debugger from
   a checkpoint, to recursively debug or profile some other code.

   Tracing is suspended while calling a tracing function set by
   :func:`settrace` or :func:`setprofile` to avoid infinite recursion.
   :func:`!call_tracing` enables explicit recursion of the tracing function.


.. data:: copyright

   A string containing the copyright pertaining to the Python interpreter.


.. function:: _clear_type_cache()

   Clear the internal type cache. The type cache is used to speed up attribute
   and method lookups. Use the function *only* to drop unnecessary references
   during reference leak debugging.

   This function should be used for internal and specialized purposes only.

   .. deprecated:: 3.13
      Use the more general :func:`_clear_internal_caches` function instead.


.. function:: _clear_internal_caches()

   Clear all internal performance-related caches. Use this function *only* to
   release unnecessary references and memory blocks when hunting for leaks.

   .. versionadded:: 3.13


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

   .. audit-event:: sys._current_frames "" sys._current_frames

.. function:: _current_exceptions()

   Return a dictionary mapping each thread's identifier to the topmost exception
   currently active in that thread at the time the function is called.
   If a thread is not currently handling an exception, it is not included in
   the result dictionary.

   This is most useful for statistical profiling.

   This function should be used for internal and specialized purposes only.

   .. audit-event:: sys._current_exceptions "" sys._current_exceptions

   .. versionchanged:: 3.12
      Each value in the dictionary is now a single exception instance, rather
      than a 3-tuple as returned from ``sys.exc_info()``.

.. function:: breakpointhook()

   This hook function is called by built-in :func:`breakpoint`.  By default,
   it drops you into the :mod:`pdb` debugger, but it can be set to any other
   function so that you can choose which debugger gets used.

   The signature of this function is dependent on what it calls.  For example,
   the default binding (e.g. ``pdb.set_trace()``) expects no arguments, but
   you might bind it to a function that expects additional arguments
   (positional and/or keyword).  The built-in ``breakpoint()`` function passes
   its ``*args`` and ``**kws`` straight through.  Whatever
   ``breakpointhooks()`` returns is returned from ``breakpoint()``.

   The default implementation first consults the environment variable
   :envvar:`PYTHONBREAKPOINT`.  If that is set to ``"0"`` then this function
   returns immediately; i.e. it is a no-op.  If the environment variable is
   not set, or is set to the empty string, ``pdb.set_trace()`` is called.
   Otherwise this variable should name a function to run, using Python's
   dotted-import nomenclature, e.g. ``package.subpackage.module.function``.
   In this case, ``package.subpackage.module`` would be imported and the
   resulting module must have a callable named ``function()``.  This is run,
   passing in ``*args`` and ``**kws``, and whatever ``function()`` returns,
   ``sys.breakpointhook()`` returns to the built-in :func:`breakpoint`
   function.

   Note that if anything goes wrong while importing the callable named by
   :envvar:`PYTHONBREAKPOINT`, a :exc:`RuntimeWarning` is reported and the
   breakpoint is ignored.

   Also note that if ``sys.breakpointhook()`` is overridden programmatically,
   :envvar:`PYTHONBREAKPOINT` is *not* consulted.

   .. versionadded:: 3.7

.. function:: _debugmallocstats()

   Print low-level information to stderr about the state of CPython's memory
   allocator.

   If Python is :ref:`built in debug mode <debug-build>` (:option:`configure
   --with-pydebug option <--with-pydebug>`), it also performs some expensive
   internal consistency checks.

   .. versionadded:: 3.3

   .. impl-detail::

      This function is specific to CPython.  The exact output format is not
      defined here, and may change.


.. data:: dllhandle

   Integer specifying the handle of the Python DLL.

   .. availability:: Windows.


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

   If this is true, Python won't try to write ``.pyc`` files on the
   import of source modules.  This value is initially set to ``True`` or
   ``False`` depending on the :option:`-B` command line option and the
   :envvar:`PYTHONDONTWRITEBYTECODE` environment variable, but you can set it
   yourself to control bytecode file generation.


.. data:: _emscripten_info

   A :term:`named tuple` holding information about the environment on the
   *wasm32-emscripten* platform. The named tuple is provisional and may change
   in the future.

   .. attribute:: _emscripten_info.emscripten_version

      Emscripten version as tuple of ints (major, minor, micro), e.g. ``(3, 1, 8)``.

   .. attribute:: _emscripten_info.runtime

      Runtime string, e.g. browser user agent, ``'Node.js v14.18.2'``, or ``'UNKNOWN'``.

   .. attribute:: _emscripten_info.pthreads

      ``True`` if Python is compiled with Emscripten pthreads support.

   .. attribute:: _emscripten_info.shared_memory

      ``True`` if Python is compiled with shared memory support.

   .. availability:: Emscripten.

   .. versionadded:: 3.11


.. data:: pycache_prefix

   If this is set (not ``None``), Python will write bytecode-cache ``.pyc``
   files to (and read them from) a parallel directory tree rooted at this
   directory, rather than from ``__pycache__`` directories in the source code
   tree. Any ``__pycache__`` directories in the source code tree will be ignored
   and new ``.pyc`` files written within the pycache prefix. Thus if you use
   :mod:`compileall` as a pre-build step, you must ensure you run it with the
   same pycache prefix (if any) that you will use at runtime.

   A relative path is interpreted relative to the current working directory.

   This value is initially set based on the value of the :option:`-X`
   ``pycache_prefix=PATH`` command-line option or the
   :envvar:`PYTHONPYCACHEPREFIX` environment variable (command-line takes
   precedence). If neither are set, it is ``None``.

   .. versionadded:: 3.8


.. function:: excepthook(type, value, traceback)

   This function prints out a given traceback and exception to ``sys.stderr``.

   When an exception other than :exc:`SystemExit` is raised and uncaught, the interpreter calls
   ``sys.excepthook`` with three arguments, the exception class, exception
   instance, and a traceback object.  In an interactive session this happens just
   before control is returned to the prompt; in a Python program this happens just
   before the program exits.  The handling of such top-level exceptions can be
   customized by assigning another three-argument function to ``sys.excepthook``.

   .. audit-event:: sys.excepthook hook,type,value,traceback sys.excepthook

      Raise an auditing event ``sys.excepthook`` with arguments ``hook``,
      ``type``, ``value``, ``traceback`` when an uncaught exception occurs.
      If no hook has been set, ``hook`` may be ``None``. If any hook raises
      an exception derived from :class:`RuntimeError` the call to the hook will
      be suppressed. Otherwise, the audit hook exception will be reported as
      unraisable and ``sys.excepthook`` will be called.

   .. seealso::

      The :func:`sys.unraisablehook` function handles unraisable exceptions
      and the :func:`threading.excepthook` function handles exception raised
      by :func:`threading.Thread.run`.


.. data:: __breakpointhook__
          __displayhook__
          __excepthook__
          __unraisablehook__

   These objects contain the original values of ``breakpointhook``,
   ``displayhook``, ``excepthook``, and ``unraisablehook`` at the start of the
   program.  They are saved so that ``breakpointhook``, ``displayhook`` and
   ``excepthook``, ``unraisablehook`` can be restored in case they happen to
   get replaced with broken or alternative objects.

   .. versionadded:: 3.7
      __breakpointhook__

   .. versionadded:: 3.8
      __unraisablehook__


.. function:: exception()

   This function, when called while an exception handler is executing (such as
   an ``except`` or ``except*`` clause), returns the exception instance that
   was caught by this handler. When exception handlers are nested within one
   another, only the exception handled by the innermost handler is accessible.

   If no exception handler is executing, this function returns ``None``.

   .. versionadded:: 3.11


.. function:: exc_info()

   This function returns the old-style representation of the handled
   exception. If an exception ``e`` is currently handled (so
   :func:`exception` would return ``e``), :func:`exc_info` returns the
   tuple ``(type(e), e, e.__traceback__)``.
   That is, a tuple containing the type of the exception (a subclass of
   :exc:`BaseException`), the exception itself, and a :ref:`traceback
   object <traceback-objects>` which typically encapsulates the call
   stack at the point where the exception last occurred.

   .. index:: pair: object; traceback

   If no exception is being handled anywhere on the stack, this function
   return a tuple containing three ``None`` values.

   .. versionchanged:: 3.11
      The ``type`` and ``traceback`` fields are now derived from the ``value``
      (the exception instance), so when an exception is modified while it is
      being handled, the changes are reflected in the results of subsequent
      calls to :func:`exc_info`.

.. data:: exec_prefix

   A string giving the site-specific directory prefix where the platform-dependent
   Python files are installed; by default, this is also ``'/usr/local'``.  This can
   be set at build time with the ``--exec-prefix`` argument to the
   :program:`configure` script.  Specifically, all configuration files (e.g. the
   :file:`pyconfig.h` header file) are installed in the directory
   :file:`{exec_prefix}/lib/python{X.Y}/config`, and shared library modules are
   installed in :file:`{exec_prefix}/lib/python{X.Y}/lib-dynload`, where *X.Y*
   is the version number of Python, for example ``3.2``.

   .. note::

      If a :ref:`virtual environment <venv-def>` is in effect, this :data:`exec_prefix`
      will point to the virtual environment. The value for the Python installation
      will still be available, via :data:`base_exec_prefix`.
      Refer to :ref:`sys-path-init-virtual-environments` for more information.

   .. versionchanged:: 3.14

      When running under a :ref:`virtual environment <venv-def>`,
      :data:`prefix` and :data:`exec_prefix` are now set to the virtual
      environment prefix by the :ref:`path initialization <sys-path-init>`,
      instead of :mod:`site`. This means that :data:`prefix` and
      :data:`exec_prefix` always point to the virtual environment, even when
      :mod:`site` is disabled (:option:`-S`).

.. data:: executable

   A string giving the absolute path of the executable binary for the Python
   interpreter, on systems where this makes sense. If Python is unable to retrieve
   the real path to its executable, :data:`sys.executable` will be an empty string
   or ``None``.


.. function:: exit([arg])

   Raise a :exc:`SystemExit` exception, signaling an intention to exit the interpreter.

   The optional argument *arg* can be an integer giving the exit status
   (defaulting to zero), or another type of object.  If it is an integer, zero
   is considered "successful termination" and any nonzero value is considered
   "abnormal termination" by shells and the like.  Most systems require it to be
   in the range 0--127, and produce undefined results otherwise.  Some systems
   have a convention for assigning specific meanings to specific exit codes, but
   these are generally underdeveloped; Unix programs generally use 2 for command
   line syntax errors and 1 for all other kind of errors.  If another type of
   object is passed, ``None`` is equivalent to passing zero, and any other
   object is printed to :data:`stderr` and results in an exit code of 1.  In
   particular, ``sys.exit("some error message")`` is a quick way to exit a
   program when an error occurs.

   Since :func:`exit` ultimately "only" raises an exception, it will only exit
   the process when called from the main thread, and the exception is not
   intercepted. Cleanup actions specified by finally clauses of :keyword:`try` statements
   are honored, and it is possible to intercept the exit attempt at an outer level.

   .. versionchanged:: 3.6
      If an error occurs in the cleanup after the Python interpreter
      has caught :exc:`SystemExit` (such as an error flushing buffered data
      in the standard streams), the exit status is changed to 120.


.. data:: flags

   The :term:`named tuple` *flags* exposes the status of command line
   flags.  Flags should only be accessed only by name and not by index.  The
   attributes are read only.

   .. list-table::

      * - .. attribute:: flags.debug
        - :option:`-d`

      * - .. attribute:: flags.inspect
        - :option:`-i`

      * - .. attribute:: flags.interactive
        - :option:`-i`

      * - .. attribute:: flags.isolated
        - :option:`-I`

      * - .. attribute:: flags.optimize
        - :option:`-O` or :option:`-OO`

      * - .. attribute:: flags.dont_write_bytecode
        - :option:`-B`

      * - .. attribute:: flags.no_user_site
        - :option:`-s`

      * - .. attribute:: flags.no_site
        - :option:`-S`

      * - .. attribute:: flags.ignore_environment
        - :option:`-E`

      * - .. attribute:: flags.verbose
        - :option:`-v`

      * - .. attribute:: flags.bytes_warning
        - :option:`-b`

      * - .. attribute:: flags.quiet
        - :option:`-q`

      * - .. attribute:: flags.hash_randomization
        - :option:`-R`

      * - .. attribute:: flags.dev_mode
        - :option:`-X dev <-X>` (:ref:`Python Development Mode <devmode>`)

      * - .. attribute:: flags.utf8_mode
        - :option:`-X utf8 <-X>`

      * - .. attribute:: flags.safe_path
        - :option:`-P`

      * - .. attribute:: flags.int_max_str_digits
        - :option:`-X int_max_str_digits <-X>`
          (:ref:`integer string conversion length limitation <int_max_str_digits>`)

      * - .. attribute:: flags.warn_default_encoding
        - :option:`-X warn_default_encoding <-X>`

      * - .. attribute:: flags.gil
        - :option:`-X gil <-X>` and :envvar:`PYTHON_GIL`

      * - .. attribute:: flags.thread_inherit_context
        - :option:`-X thread_inherit_context <-X>` and
          :envvar:`PYTHON_THREAD_INHERIT_CONTEXT`

      * - .. attribute:: flags.context_aware_warnings
        - :option:`-X context_aware_warnings <-X>` and
          :envvar:`PYTHON_CONTEXT_AWARE_WARNINGS`


   .. versionchanged:: 3.2
      Added ``quiet`` attribute for the new :option:`-q` flag.

   .. versionadded:: 3.2.3
      The ``hash_randomization`` attribute.

   .. versionchanged:: 3.3
      Removed obsolete ``division_warning`` attribute.

   .. versionchanged:: 3.4
      Added ``isolated`` attribute for :option:`-I` ``isolated`` flag.

   .. versionchanged:: 3.7
      Added the ``dev_mode`` attribute for the new :ref:`Python Development
      Mode <devmode>` and the ``utf8_mode`` attribute for the new  :option:`-X`
      ``utf8`` flag.

   .. versionchanged:: 3.10
      Added ``warn_default_encoding`` attribute for :option:`-X` ``warn_default_encoding`` flag.

   .. versionchanged:: 3.11
      Added the ``safe_path`` attribute for :option:`-P` option.

   .. versionchanged:: 3.11
      Added the ``int_max_str_digits`` attribute.

   .. versionchanged:: 3.13
      Added the ``gil`` attribute.

   .. versionchanged:: 3.14
      Added the ``thread_inherit_context`` attribute.

   .. versionchanged:: 3.14
      Added the ``context_aware_warnings`` attribute.


.. data:: float_info

   A :term:`named tuple` holding information about the float type. It
   contains low level information about the precision and internal
   representation.  The values correspond to the various floating-point
   constants defined in the standard header file :file:`float.h` for the 'C'
   programming language; see section 5.2.4.2.2 of the 1999 ISO/IEC C standard
   [C99]_, 'Characteristics of floating types', for details.

   .. list-table:: Attributes of the :data:`!float_info` :term:`named tuple`
      :header-rows: 1

      * - attribute
        - float.h macro
        - explanation

      * - .. attribute:: float_info.epsilon
        - :c:macro:`!DBL_EPSILON`
        - difference between 1.0 and the least value greater than 1.0 that is
          representable as a float.

          See also :func:`math.ulp`.

      * - .. attribute:: float_info.dig
        - :c:macro:`!DBL_DIG`
        - The maximum number of decimal digits that can be faithfully
          represented in a float; see below.

      * - .. attribute:: float_info.mant_dig
        - :c:macro:`!DBL_MANT_DIG`
        - Float precision: the number of base-``radix`` digits in the
          significand of a float.

      * - .. attribute:: float_info.max
        - :c:macro:`!DBL_MAX`
        - The maximum representable positive finite float.

      * - .. attribute:: float_info.max_exp
        - :c:macro:`!DBL_MAX_EXP`
        - The maximum integer *e* such that ``radix**(e-1)`` is a representable
          finite float.

      * - .. attribute:: float_info.max_10_exp
        - :c:macro:`!DBL_MAX_10_EXP`
        - The maximum integer *e* such that ``10**e`` is in the range of
          representable finite floats.

      * - .. attribute:: float_info.min
        - :c:macro:`!DBL_MIN`
        - The minimum representable positive *normalized* float.

          Use :func:`math.ulp(0.0) <math.ulp>` to get the smallest positive
          *denormalized* representable float.

      * - .. attribute:: float_info.min_exp
        - :c:macro:`!DBL_MIN_EXP`
        - The minimum integer *e* such that ``radix**(e-1)`` is a normalized
          float.

      * - .. attribute:: float_info.min_10_exp
        - :c:macro:`!DBL_MIN_10_EXP`
        - The minimum integer *e* such that ``10**e`` is a normalized float.

      * - .. attribute:: float_info.radix
        - :c:macro:`!FLT_RADIX`
        - The radix of exponent representation.

      * - .. attribute:: float_info.rounds
        - :c:macro:`!FLT_ROUNDS`
        - An integer representing the rounding mode for floating-point arithmetic.
          This reflects the value of the system :c:macro:`!FLT_ROUNDS` macro
          at interpreter startup time:

          * ``-1``: indeterminable
          * ``0``: toward zero
          * ``1``: to nearest
          * ``2``: toward positive infinity
          * ``3``: toward negative infinity

          All other values for :c:macro:`!FLT_ROUNDS` characterize
          implementation-defined rounding behavior.

   The attribute :attr:`sys.float_info.dig` needs further explanation.  If
   ``s`` is any string representing a decimal number with at most
   :attr:`!sys.float_info.dig` significant digits, then converting ``s`` to a
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


.. function:: getallocatedblocks()

   Return the number of memory blocks currently allocated by the interpreter,
   regardless of their size.  This function is mainly useful for tracking
   and debugging memory leaks.  Because of the interpreter's internal
   caches, the result can vary from call to call; you may have to call
   :func:`_clear_internal_caches` and :func:`gc.collect` to get more
   predictable results.

   If a Python build or implementation cannot reasonably compute this
   information, :func:`getallocatedblocks` is allowed to return 0 instead.

   .. versionadded:: 3.4


.. function:: getunicodeinternedsize()

   Return the number of unicode objects that have been interned.

   .. versionadded:: 3.12


.. function:: getandroidapilevel()

   Return the build-time API level of Android as an integer. This represents the
   minimum version of Android this build of Python can run on. For runtime
   version information, see :func:`platform.android_ver`.

   .. availability:: Android.

   .. versionadded:: 3.7


.. function:: getdefaultencoding()

   Return ``'utf-8'``. This is the name of the default string encoding, used
   in methods like :meth:`str.encode`.


.. function:: getdlopenflags()

   Return the current value of the flags that are used for
   :c:func:`dlopen` calls.  Symbolic names for the flag values can be
   found in the :mod:`os` module (:samp:`RTLD_{xxx}` constants, e.g.
   :const:`os.RTLD_LAZY`).

   .. availability:: Unix.


.. function:: getfilesystemencoding()

   Get the :term:`filesystem encoding <filesystem encoding and error handler>`:
   the encoding used with the :term:`filesystem error handler <filesystem
   encoding and error handler>` to convert between Unicode filenames and bytes
   filenames. The filesystem error handler is returned from
   :func:`getfilesystemencodeerrors`.

   For best compatibility, str should be used for filenames in all cases,
   although representing filenames as bytes is also supported. Functions
   accepting or returning filenames should support either str or bytes and
   internally convert to the system's preferred representation.

   :func:`os.fsencode` and :func:`os.fsdecode` should be used to ensure that
   the correct encoding and errors mode are used.

   The :term:`filesystem encoding and error handler` are configured at Python
   startup by the :c:func:`PyConfig_Read` function: see
   :c:member:`~PyConfig.filesystem_encoding` and
   :c:member:`~PyConfig.filesystem_errors` members of :c:type:`PyConfig`.

   .. versionchanged:: 3.2
      :func:`getfilesystemencoding` result cannot be ``None`` anymore.

   .. versionchanged:: 3.6
      Windows is no longer guaranteed to return ``'mbcs'``. See :pep:`529`
      and :func:`_enablelegacywindowsfsencoding` for more information.

   .. versionchanged:: 3.7
      Return ``'utf-8'`` if the :ref:`Python UTF-8 Mode <utf8-mode>` is
      enabled.


.. function:: getfilesystemencodeerrors()

   Get the :term:`filesystem error handler <filesystem encoding and error
   handler>`: the error handler used with the :term:`filesystem encoding
   <filesystem encoding and error handler>` to convert between Unicode
   filenames and bytes filenames. The filesystem encoding is returned from
   :func:`getfilesystemencoding`.

   :func:`os.fsencode` and :func:`os.fsdecode` should be used to ensure that
   the correct encoding and errors mode are used.

   The :term:`filesystem encoding and error handler` are configured at Python
   startup by the :c:func:`PyConfig_Read` function: see
   :c:member:`~PyConfig.filesystem_encoding` and
   :c:member:`~PyConfig.filesystem_errors` members of :c:type:`PyConfig`.

   .. versionadded:: 3.6

.. function:: get_int_max_str_digits()

   Returns the current value for the :ref:`integer string conversion length
   limitation <int_max_str_digits>`. See also :func:`set_int_max_str_digits`.

   .. versionadded:: 3.11

.. function:: getrefcount(object)

   Return the reference count of the *object*.  The count returned is generally one
   higher than you might expect, because it includes the (temporary) reference as
   an argument to :func:`getrefcount`.

   Note that the returned value may not actually reflect how many
   references to the object are actually held.  For example, some
   objects are :term:`immortal` and have a very high refcount that does not
   reflect the actual number of references.  Consequently, do not rely
   on the returned value to be accurate, other than a value of 0 or 1.

   .. impl-detail::

      :term:`Immortal <immortal>` objects with a large reference count can be
      identified via :func:`_is_immortal`.

   .. versionchanged:: 3.12
      Immortal objects have very large refcounts that do not match
      the actual number of references to the object.

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

   See `recursive sizeof recipe <https://code.activestate.com/recipes/577504-compute-memory-footprint-of-an-object-and-its-cont/>`_
   for an example of using :func:`getsizeof` recursively to find the size of
   containers and all their contents.

.. function:: getswitchinterval()

   Return the interpreter's "thread switch interval" in seconds; see
   :func:`setswitchinterval`.

   .. versionadded:: 3.2


.. function:: _getframe([depth])

   Return a frame object from the call stack.  If optional integer *depth* is
   given, return the frame object that many calls below the top of the stack.  If
   that is deeper than the call stack, :exc:`ValueError` is raised.  The default
   for *depth* is zero, returning the frame at the top of the call stack.

   .. audit-event:: sys._getframe frame sys._getframe

   .. impl-detail::

      This function should be used for internal and specialized purposes only.
      It is not guaranteed to exist in all implementations of Python.


.. function:: _getframemodulename([depth])

   Return the name of a module from the call stack.  If optional integer *depth*
   is given, return the module that many calls below the top of the stack.  If
   that is deeper than the call stack, or if the module is unidentifiable,
   ``None`` is returned.  The default for *depth* is zero, returning the
   module at the top of the call stack.

   .. audit-event:: sys._getframemodulename depth sys._getframemodulename

   .. impl-detail::

      This function should be used for internal and specialized purposes only.
      It is not guaranteed to exist in all implementations of Python.


.. function:: getobjects(limit[, type])

   This function only exists if CPython was built using the
   specialized configure option :option:`--with-trace-refs`.
   It is intended only for debugging garbage-collection issues.

   Return a list of up to *limit* dynamically allocated Python objects.
   If *type* is given, only objects of that exact type (not subtypes)
   are included.

   Objects from the list are not safe to use.
   Specifically, the result will include objects from all interpreters that
   share their object allocator state (that is, ones created with
   :c:member:`PyInterpreterConfig.use_main_obmalloc` set to 1
   or using :c:func:`Py_NewInterpreter`, and the
   :ref:`main interpreter <sub-interpreter-support>`).
   Mixing objects from different interpreters may lead to crashes
   or other unexpected behavior.

   .. impl-detail::

      This function should be used for specialized purposes only.
      It is not guaranteed to exist in all implementations of Python.

   .. versionchanged:: 3.14

      The result may include objects from other interpreters.


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
   *service_pack_major*, *suite_mask*, *product_type* and
   *platform_version*. *service_pack* contains a string,
   *platform_version* a 3-tuple and all other values are
   integers. The components can also be accessed by name, so
   ``sys.getwindowsversion()[0]`` is equivalent to
   ``sys.getwindowsversion().major``. For compatibility with prior
   versions, only the first 5 elements are retrievable by indexing.

   *platform* will be ``2`` (VER_PLATFORM_WIN32_NT).

   *product_type* may be one of the following values:

   +---------------------------------------+---------------------------------+
   | Constant                              | Meaning                         |
   +=======================================+=================================+
   | ``1`` (VER_NT_WORKSTATION)            | The system is a workstation.    |
   +---------------------------------------+---------------------------------+
   | ``2`` (VER_NT_DOMAIN_CONTROLLER)      | The system is a domain          |
   |                                       | controller.                     |
   +---------------------------------------+---------------------------------+
   | ``3`` (VER_NT_SERVER)                 | The system is a server, but not |
   |                                       | a domain controller.            |
   +---------------------------------------+---------------------------------+

   This function wraps the Win32 :c:func:`!GetVersionEx` function; see the
   Microsoft documentation on :c:func:`!OSVERSIONINFOEX` for more information
   about these fields.

   *platform_version* returns the major version, minor version and
   build number of the current operating system, rather than the version that
   is being emulated for the process. It is intended for use in logging rather
   than for feature detection.

   .. note::
      *platform_version* derives the version from kernel32.dll which can be of a different
      version than the OS version. Please use :mod:`platform` module for achieving accurate
      OS version.

   .. availability:: Windows.

   .. versionchanged:: 3.2
      Changed to a named tuple and added *service_pack_minor*,
      *service_pack_major*, *suite_mask*, and *product_type*.

   .. versionchanged:: 3.6
      Added *platform_version*


.. function:: get_asyncgen_hooks()

   Returns an *asyncgen_hooks* object, which is similar to a
   :class:`~collections.namedtuple` of the form ``(firstiter, finalizer)``,
   where *firstiter* and *finalizer* are expected to be either ``None`` or
   functions which take an :term:`asynchronous generator iterator` as an
   argument, and are used to schedule finalization of an asynchronous
   generator by an event loop.

   .. versionadded:: 3.6
      See :pep:`525` for more details.

   .. note::
      This function has been added on a provisional basis (see :pep:`411`
      for details.)


.. function:: get_coroutine_origin_tracking_depth()

   Get the current coroutine origin tracking depth, as set by
   :func:`set_coroutine_origin_tracking_depth`.

   .. versionadded:: 3.7

   .. note::
      This function has been added on a provisional basis (see :pep:`411`
      for details.)  Use it only for debugging purposes.


.. data:: hash_info

   A :term:`named tuple` giving parameters of the numeric hash
   implementation.  For more details about hashing of numeric types, see
   :ref:`numeric-hash`.

   .. attribute:: hash_info.width

      The width in bits used for hash values

   .. attribute:: hash_info.modulus

      The prime modulus P used for numeric hash scheme

   .. attribute:: hash_info.inf

      The hash value returned for a positive infinity

   .. attribute:: hash_info.nan

      (This attribute is no longer used)

   .. attribute:: hash_info.imag

      The multiplier used for the imaginary part of a complex number

   .. attribute:: hash_info.algorithm

      The name of the algorithm for hashing of str, bytes, and memoryview

   .. attribute:: hash_info.hash_bits

      The internal output size of the hash algorithm

   .. attribute:: hash_info.seed_bits

      The size of the seed key of the hash algorithm

   .. versionadded:: 3.2

   .. versionchanged:: 3.4
      Added *algorithm*, *hash_bits* and *seed_bits*


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
   :term:`named tuple`  :data:`sys.version_info` may be used for a more
   human-friendly encoding of the same information.

   More details of ``hexversion`` can be found at :ref:`apiabiversion`.


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
   language versions, however.)  See :pep:`421` for more information.

   .. versionadded:: 3.3

   .. note::

      The addition of new required attributes must go through the normal PEP
      process. See :pep:`421` for more information.

.. data:: int_info

   A :term:`named tuple` that holds information about Python's internal
   representation of integers.  The attributes are read only.

   .. attribute:: int_info.bits_per_digit

      The number of bits held in each digit.
      Python integers are stored internally in base ``2**int_info.bits_per_digit``.

   .. attribute:: int_info.sizeof_digit

      The size in bytes of the C type used to represent a digit.

   .. attribute:: int_info.default_max_str_digits

      The default value for :func:`sys.get_int_max_str_digits`
      when it is not otherwise explicitly configured.

   .. attribute:: int_info.str_digits_check_threshold

      The minimum non-zero value for :func:`sys.set_int_max_str_digits`,
      :envvar:`PYTHONINTMAXSTRDIGITS`, or :option:`-X int_max_str_digits <-X>`.

   .. versionadded:: 3.1

   .. versionchanged:: 3.11

      Added :attr:`~int_info.default_max_str_digits` and
      :attr:`~int_info.str_digits_check_threshold`.


.. data:: __interactivehook__

   When this attribute exists, its value is automatically called (with no
   arguments) when the interpreter is launched in :ref:`interactive mode
   <tut-interactive>`.  This is done after the :envvar:`PYTHONSTARTUP` file is
   read, so that you can set this hook there.  The :mod:`site` module
   :ref:`sets this <rlcompleter-config>`.

   .. audit-event:: cpython.run_interactivehook hook sys.__interactivehook__

      Raises an :ref:`auditing event <auditing>`
      ``cpython.run_interactivehook`` with the hook object as the argument when
      the hook is called on startup.

   .. versionadded:: 3.4


.. function:: intern(string)

   Enter *string* in the table of "interned" strings and return the interned string
   -- which is *string* itself or a copy. Interning strings is useful to gain a
   little performance on dictionary lookup -- if the keys in a dictionary are
   interned, and the lookup key is interned, the key comparisons (after hashing)
   can be done by a pointer compare instead of a string compare.  Normally, the
   names used in Python programs are automatically interned, and the dictionaries
   used to hold module, class or instance attributes have interned keys.

   Interned strings are not :term:`immortal`; you must keep a reference to the
   return value of :func:`intern` around to benefit from it.


.. function:: _is_gil_enabled()

   Return :const:`True` if the :term:`GIL` is enabled and :const:`False` if
   it is disabled.

   .. versionadded:: 3.13

   .. impl-detail::

      It is not guaranteed to exist in all implementations of Python.

.. function:: is_finalizing()

   Return :const:`True` if the main Python interpreter is
   :term:`shutting down <interpreter shutdown>`. Return :const:`False` otherwise.

   See also the :exc:`PythonFinalizationError` exception.

   .. versionadded:: 3.5

.. data:: _jit

   Utilities for observing just-in-time compilation.

   .. impl-detail::

      JIT compilation is an *experimental implementation detail* of CPython.
      ``sys._jit`` is not guaranteed to exist or behave the same way in all
      Python implementations, versions, or build configurations.

   .. versionadded:: 3.14

   .. function:: _jit.is_available()

      Return ``True`` if the current Python executable supports JIT compilation,
      and ``False`` otherwise.  This can be controlled by building CPython with
      the ``--experimental-jit`` option on Windows, and the
      :option:`--enable-experimental-jit` option on all other platforms.

   .. function:: _jit.is_enabled()

      Return ``True`` if JIT compilation is enabled for the current Python
      process (implies :func:`sys._jit.is_available`), and ``False`` otherwise.
      If JIT compilation is available, this can be controlled by setting the
      :envvar:`PYTHON_JIT` environment variable to ``0`` (disabled) or ``1``
      (enabled) at interpreter startup.

   .. function:: _jit.is_active()

      Return ``True`` if the topmost Python frame is currently executing JIT
      code (implies :func:`sys._jit.is_enabled`), and ``False`` otherwise.

      .. note::

         This function is intended for testing and debugging the JIT itself.
         It should be avoided for any other purpose.

      .. note::

         Due to the nature of tracing JIT compilers, repeated calls to this
         function may give surprising results. For example, branching on its
         return value will likely lead to unexpected behavior (if doing so
         causes JIT code to be entered or exited):

         .. code-block:: pycon

            >>> for warmup in range(BIG_NUMBER):
            ...     # This line is "hot", and is eventually JIT-compiled:
            ...     if sys._jit.is_active():
            ...         # This line is "cold", and is run in the interpreter:
            ...         assert sys._jit.is_active()
            ...
            Traceback (most recent call last):
              File "<stdin>", line 5, in <module>
                assert sys._jit.is_active()
                       ~~~~~~~~~~~~~~~~~~^^
            AssertionError

.. data:: last_exc

   This variable is not always defined; it is set to the exception instance
   when an exception is not handled and the interpreter prints an error message
   and a stack traceback.  Its intended use is to allow an interactive user to
   import a debugger module and engage in post-mortem debugging without having
   to re-execute the command that caused the error.  (Typical use is
   ``import pdb; pdb.pm()`` to enter the post-mortem debugger; see :mod:`pdb`
   module for more information.)

   .. versionadded:: 3.12

.. function:: _is_immortal(op)

   Return :const:`True` if the given object is :term:`immortal`, :const:`False`
   otherwise.

   .. note::

      Objects that are immortal (and thus return ``True`` upon being passed
      to this function) are not guaranteed to be immortal in future versions,
      and vice versa for mortal objects.

   .. versionadded:: 3.14

   .. impl-detail::

      This function should be used for specialized purposes only.
      It is not guaranteed to exist in all implementations of Python.

.. function:: _is_interned(string)

   Return :const:`True` if the given string is "interned", :const:`False`
   otherwise.

   .. versionadded:: 3.13

   .. impl-detail::

      It is not guaranteed to exist in all implementations of Python.


.. data:: last_type
          last_value
          last_traceback

   These three variables are deprecated; use :data:`sys.last_exc` instead.
   They hold the legacy representation of ``sys.last_exc``, as returned
   from :func:`exc_info` above.

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

    A list of :term:`meta path finder` objects that have their
    :meth:`~importlib.abc.MetaPathFinder.find_spec` methods called to see if one
    of the objects can find the module to be imported. By default, it holds entries
    that implement Python's default import semantics. The
    :meth:`~importlib.abc.MetaPathFinder.find_spec` method is called with at
    least the absolute name of the module being imported. If the module to be
    imported is contained in a package, then the parent package's
    :attr:`~module.__path__`
    attribute is passed in as a second argument. The method returns a
    :term:`module spec`, or ``None`` if the module cannot be found.

    .. seealso::

        :class:`importlib.abc.MetaPathFinder`
          The abstract base class defining the interface of finder objects on
          :data:`meta_path`.
        :class:`importlib.machinery.ModuleSpec`
          The concrete class which
          :meth:`~importlib.abc.MetaPathFinder.find_spec` should return
          instances of.

    .. versionchanged:: 3.4

        :term:`Module specs <module spec>` were introduced in Python 3.4, by
        :pep:`451`.

    .. versionchanged:: 3.12

        Removed the fallback that looked for a :meth:`!find_module` method
        if a :data:`meta_path` entry didn't have a
        :meth:`~importlib.abc.MetaPathFinder.find_spec` method.

.. data:: modules

   This is a dictionary that maps module names to modules which have already been
   loaded.  This can be manipulated to force reloading of modules and other tricks.
   However, replacing the dictionary will not necessarily work as expected and
   deleting essential items from the dictionary may cause Python to fail.  If
   you want to iterate over this global dictionary always use
   ``sys.modules.copy()`` or ``tuple(sys.modules)`` to avoid exceptions as its
   size may change during iteration as a side effect of code or activity in
   other threads.


.. data:: orig_argv

   The list of the original command line arguments passed to the Python
   executable.

   The elements of :data:`sys.orig_argv` are the arguments to the Python interpreter,
   while the elements of :data:`sys.argv` are the arguments to the user's program.
   Arguments consumed by the interpreter itself will be present in :data:`sys.orig_argv`
   and missing from :data:`sys.argv`.

   .. versionadded:: 3.10


.. data:: path

   .. index:: triple: module; search; path

   A list of strings that specifies the search path for modules. Initialized from
   the environment variable :envvar:`PYTHONPATH`, plus an installation-dependent
   default.

   By default, as initialized upon program startup, a potentially unsafe path
   is prepended to :data:`sys.path` (*before* the entries inserted as a result
   of :envvar:`PYTHONPATH`):

   * ``python -m module`` command line: prepend the current working
     directory.
   * ``python script.py`` command line: prepend the script's directory.
     If it's a symbolic link, resolve symbolic links.
   * ``python -c code`` and ``python`` (REPL) command lines: prepend an empty
     string, which means the current working directory.

   To not prepend this potentially unsafe path, use the :option:`-P` command
   line option or the :envvar:`PYTHONSAFEPATH` environment variable.

   A program is free to modify this list for its own purposes.  Only strings
   should be added to :data:`sys.path`; all other data types are
   ignored during import.


   .. seealso::
      * Module :mod:`site` This describes how to use .pth files to
        extend :data:`sys.path`.

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


.. data:: platform

   A string containing a platform identifier. Known values are:

   ================ ===========================
   System           ``platform`` value
   ================ ===========================
   AIX              ``'aix'``
   Android          ``'android'``
   Emscripten       ``'emscripten'``
   FreeBSD          ``'freebsd'``
   iOS              ``'ios'``
   Linux            ``'linux'``
   macOS            ``'darwin'``
   Windows          ``'win32'``
   Windows/Cygwin   ``'cygwin'``
   WASI             ``'wasi'``
   ================ ===========================

   On Unix systems not listed in the table, the value is the lowercased OS name
   as returned by ``uname -s``, with the first part of the version as returned by
   ``uname -r`` appended, e.g. ``'sunos5'``, *at the time when Python was built*.
   Unless you want to test for a specific system version, it is therefore
   recommended to use the following idiom::

      if sys.platform.startswith('sunos'):
          # SunOS-specific code here...

   .. versionchanged:: 3.3
      On Linux, :data:`sys.platform` doesn't contain the major version anymore.
      It is always ``'linux'``, instead of ``'linux2'`` or ``'linux3'``.

   .. versionchanged:: 3.8
      On AIX, :data:`sys.platform` doesn't contain the major version anymore.
      It is always ``'aix'``, instead of ``'aix5'`` or ``'aix7'``.

   .. versionchanged:: 3.13
      On Android, :data:`sys.platform` now returns ``'android'`` rather than
      ``'linux'``.

   .. versionchanged:: 3.14
      On FreeBSD, :data:`sys.platform` doesn't contain the major version anymore.
      It is always ``'freebsd'``, instead of ``'freebsd13'`` or ``'freebsd14'``.

   .. seealso::

      :data:`os.name` has a coarser granularity.  :func:`os.uname` gives
      system-dependent version information.

      The :mod:`platform` module provides detailed checks for the
      system's identity.


.. data:: platlibdir

   Name of the platform-specific library directory. It is used to build the
   path of standard library and the paths of installed extension modules.

   It is equal to ``"lib"`` on most platforms. On Fedora and SuSE, it is equal
   to ``"lib64"`` on 64-bit platforms which gives the following ``sys.path``
   paths (where ``X.Y`` is the Python ``major.minor`` version):

   * ``/usr/lib64/pythonX.Y/``:
     Standard library (like ``os.py`` of the :mod:`os` module)
   * ``/usr/lib64/pythonX.Y/lib-dynload/``:
     C extension modules of the standard library (like the :mod:`errno` module,
     the exact filename is platform specific)
   * ``/usr/lib/pythonX.Y/site-packages/`` (always use ``lib``, not
     :data:`sys.platlibdir`): Third-party modules
   * ``/usr/lib64/pythonX.Y/site-packages/``:
     C extension modules of third-party packages

   .. versionadded:: 3.9


.. data:: prefix

   A string giving the site-specific directory prefix where the platform
   independent Python files are installed; on Unix, the default is
   :file:`/usr/local`. This can be set at build time with the :option:`--prefix`
   argument to the :program:`configure` script.  See
   :ref:`installation_paths` for derived paths.

   .. note::

      If a :ref:`virtual environment <venv-def>` is in effect, this :data:`prefix`
      will point to the virtual environment. The value for the Python installation
      will still be available, via :data:`base_prefix`.
      Refer to :ref:`sys-path-init-virtual-environments` for more information.

   .. versionchanged:: 3.14

      When running under a :ref:`virtual environment <venv-def>`,
      :data:`prefix` and :data:`exec_prefix` are now set to the virtual
      environment prefix by the :ref:`path initialization <sys-path-init>`,
      instead of :mod:`site`. This means that :data:`prefix` and
      :data:`exec_prefix` always point to the virtual environment, even when
      :mod:`site` is disabled (:option:`-S`).


.. data:: ps1
          ps2

   .. index::
      single: interpreter prompts
      single: prompts, interpreter
      single: >>>; interpreter prompt
      single: ...; interpreter prompt

   Strings specifying the primary and secondary prompt of the interpreter.  These
   are only defined if the interpreter is in interactive mode.  Their initial
   values in this case are ``'>>> '`` and ``'... '``.  If a non-string object is
   assigned to either variable, its :func:`str` is re-evaluated each time the
   interpreter prepares to read a new interactive command; this can be used to
   implement a dynamic prompt.


.. function:: setdlopenflags(n)

   Set the flags used by the interpreter for :c:func:`dlopen` calls, such as when
   the interpreter loads extension modules.  Among other things, this will enable a
   lazy resolving of symbols when importing a module, if called as
   ``sys.setdlopenflags(0)``.  To share symbols across extension modules, call as
   ``sys.setdlopenflags(os.RTLD_GLOBAL)``.  Symbolic names for the flag values
   can be found in the :mod:`os` module (:samp:`RTLD_{xxx}` constants, e.g.
   :const:`os.RTLD_LAZY`).

   .. availability:: Unix.

.. function:: set_int_max_str_digits(maxdigits)

   Set the :ref:`integer string conversion length limitation
   <int_max_str_digits>` used by this interpreter. See also
   :func:`get_int_max_str_digits`.

   .. versionadded:: 3.11

.. function:: setprofile(profilefunc)

   .. index::
      single: profile function
      single: profiler

   Set the system's profile function, which allows you to implement a Python source
   code profiler in Python.  See chapter :ref:`profile` for more information on the
   Python profiler.  The system's profile function is called similarly to the
   system's trace function (see :func:`settrace`), but it is called with different events,
   for example it isn't called for each executed line of code (only on call and return,
   but the return event is reported even when an exception has been set). The function is
   thread-specific, but there is no way for the profiler to know about context switches between
   threads, so it does not make sense to use this in the presence of multiple threads. Also,
   its return value is not used, so it can simply return ``None``.  Error in the profile
   function will cause itself unset.

   .. note::
      The same tracing mechanism is used for :func:`!setprofile` as :func:`settrace`.
      To trace calls with :func:`!setprofile` inside a tracing function
      (e.g. in a debugger breakpoint), see :func:`call_tracing`.

   Profile functions should have three arguments: *frame*, *event*, and
   *arg*. *frame* is the current stack frame.  *event* is a string: ``'call'``,
   ``'return'``, ``'c_call'``, ``'c_return'``, or ``'c_exception'``. *arg* depends
   on the event type.

   The events have the following meaning:

   ``'call'``
      A function is called (or some other code block entered).  The
      profile function is called; *arg* is ``None``.

   ``'return'``
      A function (or other code block) is about to return.  The profile
      function is called; *arg* is the value that will be returned, or ``None``
      if the event is caused by an exception being raised.

   ``'c_call'``
      A C function is about to be called.  This may be an extension function or
      a built-in.  *arg* is the C function object.

   ``'c_return'``
      A C function has returned. *arg* is the C function object.

   ``'c_exception'``
      A C function has raised an exception.  *arg* is the C function object.

   .. audit-event:: sys.setprofile "" sys.setprofile


.. function:: setrecursionlimit(limit)

   Set the maximum depth of the Python interpreter stack to *limit*.  This limit
   prevents infinite recursion from causing an overflow of the C stack and crashing
   Python.

   The highest possible limit is platform-dependent.  A user may need to set the
   limit higher when they have a program that requires deep recursion and a platform
   that supports a higher limit.  This should be done with care, because a too-high
   limit can lead to a crash.

   If the new limit is too low at the current recursion depth, a
   :exc:`RecursionError` exception is raised.

   .. versionchanged:: 3.5.1
      A :exc:`RecursionError` exception is now raised if the new limit is too
      low at the current recursion depth.


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
   debugger to support multiple threads, it must register a trace function using
   :func:`settrace` for each thread being debugged or use :func:`threading.settrace`.

   Trace functions should have three arguments: *frame*, *event*, and
   *arg*. *frame* is the current stack frame.  *event* is a string: ``'call'``,
   ``'line'``, ``'return'``, ``'exception'`` or ``'opcode'``.  *arg* depends on
   the event type.

   The trace function is invoked (with *event* set to ``'call'``) whenever a new
   local scope is entered; it should return a reference to a local trace
   function to be used for the new scope, or ``None`` if the scope shouldn't be
   traced.

   The local trace function should return a reference to itself, or to another
   function which would then be used as the local trace function for the scope.

   If there is any error occurred in the trace function, it will be unset, just
   like ``settrace(None)`` is called.

   .. note::
      Tracing is disabled while calling the trace function (e.g. a function set by
      :func:`!settrace`). For recursive tracing see :func:`call_tracing`.

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
      Per-line events may be disabled for a frame by setting
      :attr:`~frame.f_trace_lines` to :const:`False` on that
      :ref:`frame <frame-objects>`.

   ``'return'``
      A function (or other code block) is about to return.  The local trace
      function is called; *arg* is the value that will be returned, or ``None``
      if the event is caused by an exception being raised.  The trace function's
      return value is ignored.

   ``'exception'``
      An exception has occurred.  The local trace function is called; *arg* is a
      tuple ``(exception, value, traceback)``; the return value specifies the
      new local trace function.

   ``'opcode'``
      The interpreter is about to execute a new opcode (see :mod:`dis` for
      opcode details).  The local trace function is called; *arg* is
      ``None``; the return value specifies the new local trace function.
      Per-opcode events are not emitted by default: they must be explicitly
      requested by setting :attr:`~frame.f_trace_opcodes` to :const:`True` on the
      :ref:`frame <frame-objects>`.

   Note that as an exception is propagated down the chain of callers, an
   ``'exception'`` event is generated at each level.

   For more fine-grained usage, it's possible to set a trace function by
   assigning ``frame.f_trace = tracefunc`` explicitly, rather than relying on
   it being set indirectly via the return value from an already installed
   trace function. This is also required for activating the trace function on
   the current frame, which :func:`settrace` doesn't do. Note that in order
   for this to work, a global tracing function must have been installed
   with :func:`settrace` in order to enable the runtime tracing machinery,
   but it doesn't need to be the same tracing function (e.g. it could be a
   low overhead tracing function that simply returns ``None`` to disable
   itself immediately on each frame).

   For more information on code and frame objects, refer to :ref:`types`.

   .. audit-event:: sys.settrace "" sys.settrace

   .. impl-detail::

      The :func:`settrace` function is intended only for implementing debuggers,
      profilers, coverage tools and the like.  Its behavior is part of the
      implementation platform, rather than part of the language definition, and
      thus may not be available in all Python implementations.

   .. versionchanged:: 3.7

      ``'opcode'`` event type added; :attr:`~frame.f_trace_lines` and
      :attr:`~frame.f_trace_opcodes` attributes added to frames

.. function:: set_asyncgen_hooks([firstiter] [, finalizer])

   Accepts two optional keyword arguments which are callables that accept an
   :term:`asynchronous generator iterator` as an argument. The *firstiter*
   callable will be called when an asynchronous generator is iterated for the
   first time. The *finalizer* will be called when an asynchronous generator
   is about to be garbage collected.

   .. audit-event:: sys.set_asyncgen_hooks_firstiter "" sys.set_asyncgen_hooks

   .. audit-event:: sys.set_asyncgen_hooks_finalizer "" sys.set_asyncgen_hooks

   Two auditing events are raised because the underlying API consists of two
   calls, each of which must raise its own event.

   .. versionadded:: 3.6
      See :pep:`525` for more details, and for a reference example of a
      *finalizer* method see the implementation of
      ``asyncio.Loop.shutdown_asyncgens`` in
      :source:`Lib/asyncio/base_events.py`

   .. note::
      This function has been added on a provisional basis (see :pep:`411`
      for details.)

.. function:: set_coroutine_origin_tracking_depth(depth)

   Allows enabling or disabling coroutine origin tracking. When
   enabled, the ``cr_origin`` attribute on coroutine objects will
   contain a tuple of (filename, line number, function name) tuples
   describing the traceback where the coroutine object was created,
   with the most recent call first. When disabled, ``cr_origin`` will
   be ``None``.

   To enable, pass a *depth* value greater than zero; this sets the
   number of frames whose information will be captured. To disable,
   set *depth* to zero.

   This setting is thread-specific.

   .. versionadded:: 3.7

   .. note::
      This function has been added on a provisional basis (see :pep:`411`
      for details.)  Use it only for debugging purposes.

.. function:: activate_stack_trampoline(backend, /)

   Activate the stack profiler trampoline *backend*.
   The only supported backend is ``"perf"``.

   Stack trampolines cannot be activated if the JIT is active.

   .. availability:: Linux.

   .. versionadded:: 3.12

   .. seealso::

      * :ref:`perf_profiling`
      * https://perf.wiki.kernel.org

.. function:: deactivate_stack_trampoline()

   Deactivate the current stack profiler trampoline backend.

   If no stack profiler is activated, this function has no effect.

   .. availability:: Linux.

   .. versionadded:: 3.12

.. function:: is_stack_trampoline_active()

   Return ``True`` if a stack profiler trampoline is active.

   .. availability:: Linux.

   .. versionadded:: 3.12


.. function:: remote_exec(pid, script)

   Executes *script*, a file containing Python code in the remote
   process with the given *pid*.

   This function returns immediately, and the code will be executed by the
   target process's main thread at the next available opportunity, similarly
   to how signals are handled. There is no interface to determine when the
   code has been executed. The caller is responsible for making sure that
   the file still exists whenever the remote process tries to read it and that
   it hasn't been overwritten.

   The remote process must be running a CPython interpreter of the same major
   and minor version as the local process. If either the local or remote
   interpreter is pre-release (alpha, beta, or release candidate) then the
   local and remote interpreters must be the same exact version.

   .. audit-event:: remote_debugger_script script_path

      When the script is executed in the remote process, an
      :ref:`auditing event <auditing>`
      ``sys.remote_debugger_script`` is raised
      with the path in the remote process.

   .. availability:: Unix, Windows.
   .. versionadded:: 3.14


.. function:: _enablelegacywindowsfsencoding()

   Changes the :term:`filesystem encoding and error handler` to 'mbcs' and
   'replace' respectively, for consistency with versions of Python prior to
   3.6.

   This is equivalent to defining the :envvar:`PYTHONLEGACYWINDOWSFSENCODING`
   environment variable before launching Python.

   See also :func:`sys.getfilesystemencoding` and
   :func:`sys.getfilesystemencodeerrors`.

   .. availability:: Windows.

   .. note::
      Changing the filesystem encoding after Python startup is risky because
      the old fsencoding or paths encoded by the old fsencoding may be cached
      somewhere. Use :envvar:`PYTHONLEGACYWINDOWSFSENCODING` instead.

   .. versionadded:: 3.6
      See :pep:`529` for more details.

   .. deprecated-removed:: 3.13 3.16
      Use :envvar:`PYTHONLEGACYWINDOWSFSENCODING` instead.

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

   These streams are regular :term:`text files <text file>` like those
   returned by the :func:`open` function.  Their parameters are chosen as
   follows:

   * The encoding and error handling are is initialized from
     :c:member:`PyConfig.stdio_encoding` and :c:member:`PyConfig.stdio_errors`.

     On Windows, UTF-8 is used for the console device.  Non-character
     devices such as disk files and pipes use the system locale
     encoding (i.e. the ANSI codepage).  Non-console character
     devices such as NUL (i.e. where ``isatty()`` returns ``True``) use the
     value of the console input and output codepages at startup,
     respectively for stdin and stdout/stderr. This defaults to the
     system :term:`locale encoding` if the process is not initially attached
     to a console.

     The special behaviour of the console can be overridden
     by setting the environment variable PYTHONLEGACYWINDOWSSTDIO
     before starting Python. In that case, the console codepages are
     used as for any other character device.

     Under all platforms, you can override the character encoding by
     setting the :envvar:`PYTHONIOENCODING` environment variable before
     starting Python or by using the new :option:`-X` ``utf8`` command
     line option and :envvar:`PYTHONUTF8` environment variable.  However,
     for the Windows console, this only applies when
     :envvar:`PYTHONLEGACYWINDOWSSTDIO` is also set.

   * When interactive, the ``stdout`` stream is line-buffered. Otherwise,
     it is block-buffered like regular text files.  The ``stderr`` stream
     is line-buffered in both cases.  You can make both streams unbuffered
     by passing the :option:`-u` command-line option or setting the
     :envvar:`PYTHONUNBUFFERED` environment variable.

   .. versionchanged:: 3.9
      Non-interactive ``stderr`` is now line-buffered instead of fully
      buffered.

   .. note::

      To write or read binary data from/to the standard streams, use the
      underlying binary :data:`~io.TextIOBase.buffer` object.  For example, to
      write bytes to :data:`stdout`, use ``sys.stdout.buffer.write(b'abc')``.

      However, if you are writing a library (and do not control in which
      context its code will be executed), be aware that the standard streams
      may be replaced with file-like objects like :class:`io.StringIO` which
      do not support the :attr:`!buffer` attribute.


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
       ``None``. It is usually the case for Windows GUI apps that aren't connected
       to a console and Python apps started with :program:`pythonw`.


.. data:: stdlib_module_names

   A frozenset of strings containing the names of standard library modules.

   It is the same on all platforms. Modules which are not available on
   some platforms and modules disabled at Python build are also listed.
   All module kinds are listed: pure Python, built-in, frozen and extension
   modules. Test modules are excluded.

   For packages, only the main package is listed: sub-packages and sub-modules
   are not listed. For example, the ``email`` package is listed, but the
   ``email.mime`` sub-package and the ``email.message`` sub-module are not
   listed.

   See also the :data:`sys.builtin_module_names` list.

   .. versionadded:: 3.10


.. data:: thread_info

   A :term:`named tuple` holding information about the thread
   implementation.

   .. attribute:: thread_info.name

      The name of the thread implementation:

      * ``"nt"``: Windows threads
      * ``"pthread"``: POSIX threads
      * ``"pthread-stubs"``: stub POSIX threads
        (on WebAssembly platforms without threading support)
      * ``"solaris"``: Solaris threads

   .. attribute:: thread_info.lock

      The name of the lock implementation:

      * ``"semaphore"``: a lock uses a semaphore
      * ``"mutex+cond"``: a lock uses a mutex and a condition variable
      * ``None`` if this information is unknown

   .. attribute:: thread_info.version

      The name and version of the thread library.
      It is a string, or ``None`` if this information is unknown.

   .. versionadded:: 3.3


.. data:: tracebacklimit

   When this variable is set to an integer value, it determines the maximum number
   of levels of traceback information printed when an unhandled exception occurs.
   The default is ``1000``.  When set to ``0`` or less, all traceback information
   is suppressed and only the exception type and value are printed.


.. function:: unraisablehook(unraisable, /)

   Handle an unraisable exception.

   Called when an exception has occurred but there is no way for Python to
   handle it. For example, when a destructor raises an exception or during
   garbage collection (:func:`gc.collect`).

   The *unraisable* argument has the following attributes:

   * :attr:`!exc_type`: Exception type.
   * :attr:`!exc_value`: Exception value, can be ``None``.
   * :attr:`!exc_traceback`: Exception traceback, can be ``None``.
   * :attr:`!err_msg`: Error message, can be ``None``.
   * :attr:`!object`: Object causing the exception, can be ``None``.

   The default hook formats :attr:`!err_msg` and :attr:`!object` as:
   ``f'{err_msg}: {object!r}'``; use "Exception ignored in" error message
   if :attr:`!err_msg` is ``None``.

   :func:`sys.unraisablehook` can be overridden to control how unraisable
   exceptions are handled.

   .. seealso::

      :func:`excepthook` which handles uncaught exceptions.

   .. warning::

      Storing :attr:`!exc_value` using a custom hook can create a reference cycle.
      It should be cleared explicitly to break the reference cycle when the
      exception is no longer needed.

      Storing :attr:`!object` using a custom hook can resurrect it if it is set to an
      object which is being finalized. Avoid storing :attr:`!object` after the custom
      hook completes to avoid resurrecting objects.

   .. audit-event:: sys.unraisablehook hook,unraisable sys.unraisablehook

      Raise an auditing event ``sys.unraisablehook`` with arguments
      *hook*, *unraisable* when an exception that cannot be handled occurs.
      The *unraisable* object is the same as what will be passed to the hook.
      If no hook has been set, *hook* may be ``None``.

   .. versionadded:: 3.8

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
   major and minor versions of the running Python interpreter.  It is provided in the :mod:`sys`
   module for informational purposes; modifying this value has no effect on the
   registry keys used by Python.

   .. availability:: Windows.


.. data:: monitoring
   :noindex:

   Namespace containing functions and constants for register callbacks
   and controlling monitoring events.
   See  :mod:`sys.monitoring` for details.

.. data:: _xoptions

   A dictionary of the various implementation-specific flags passed through
   the :option:`-X` command-line option.  Option names are either mapped to
   their values, if given explicitly, or to :const:`True`.  Example:

   .. code-block:: shell-session

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

.. [C99] ISO/IEC 9899:1999.  "Programming languages -- C."  A public draft of this standard is available at https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf\ .
