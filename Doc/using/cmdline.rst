.. highlightlang:: sh

.. ATTENTION: You probably should update Misc/python.man, too, if you modify
   this file.

.. _using-on-general:

Command line and environment
============================

The CPython interpreter scans the command line and the environment for various
settings.

.. impl-detail::

   Other implementations' command line schemes may differ.  See
   :ref:`implementations` for further resources.


.. _using-on-cmdline:

Command line
------------

When invoking Python, you may specify any of these options::

    python [-bBdEhiIOqsSuvVWx?] [-c command | -m module-name | script | - ] [args]

The most common use case is, of course, a simple invocation of a script::

    python myscript.py


.. _using-on-interface-options:

Interface options
~~~~~~~~~~~~~~~~~

The interpreter interface resembles that of the UNIX shell, but provides some
additional methods of invocation:

* When called with standard input connected to a tty device, it prompts for
  commands and executes them until an EOF (an end-of-file character, you can
  produce that with :kbd:`Ctrl-D` on UNIX or :kbd:`Ctrl-Z, Enter` on Windows) is read.
* When called with a file name argument or with a file as standard input, it
  reads and executes a script from that file.
* When called with a directory name argument, it reads and executes an
  appropriately named script from that directory.
* When called with ``-c command``, it executes the Python statement(s) given as
  *command*.  Here *command* may contain multiple statements separated by
  newlines. Leading whitespace is significant in Python statements!
* When called with ``-m module-name``, the given module is located on the
  Python module path and executed as a script.

In non-interactive mode, the entire input is parsed before it is executed.

An interface option terminates the list of options consumed by the interpreter,
all consecutive arguments will end up in :data:`sys.argv` -- note that the first
element, subscript zero (``sys.argv[0]``), is a string reflecting the program's
source.

.. cmdoption:: -c <command>

   Execute the Python code in *command*.  *command* can be one or more
   statements separated by newlines, with significant leading whitespace as in
   normal module code.

   If this option is given, the first element of :data:`sys.argv` will be
   ``"-c"`` and the current directory will be added to the start of
   :data:`sys.path` (allowing modules in that directory to be imported as top
   level modules).


.. cmdoption:: -m <module-name>

   Search :data:`sys.path` for the named module and execute its contents as
   the :mod:`__main__` module.

   Since the argument is a *module* name, you must not give a file extension
   (``.py``).  The module name should be a valid absolute Python module name, but
   the implementation may not always enforce this (e.g. it may allow you to
   use a name that includes a hyphen).

   Package names (including namespace packages) are also permitted. When a
   package name is supplied instead
   of a normal module, the interpreter will execute ``<pkg>.__main__`` as
   the main module. This behaviour is deliberately similar to the handling
   of directories and zipfiles that are passed to the interpreter as the
   script argument.

   .. note::

      This option cannot be used with built-in modules and extension modules
      written in C, since they do not have Python module files. However, it
      can still be used for precompiled modules, even if the original source
      file is not available.

   If this option is given, the first element of :data:`sys.argv` will be the
   full path to the module file (while the module file is being located, the
   first element will be set to ``"-m"``). As with the :option:`-c` option,
   the current directory will be added to the start of :data:`sys.path`.

   Many standard library modules contain code that is invoked on their execution
   as a script.  An example is the :mod:`timeit` module::

       python -mtimeit -s 'setup here' 'benchmarked code here'
       python -mtimeit -h # for details

   .. seealso::
      :func:`runpy.run_module`
         Equivalent functionality directly available to Python code

      :pep:`338` -- Executing modules as scripts


   .. versionchanged:: 3.1
      Supply the package name to run a ``__main__`` submodule.

   .. versionchanged:: 3.4
      namespace packages are also supported


.. describe:: -

   Read commands from standard input (:data:`sys.stdin`).  If standard input is
   a terminal, :option:`-i` is implied.

   If this option is given, the first element of :data:`sys.argv` will be
   ``"-"`` and the current directory will be added to the start of
   :data:`sys.path`.


.. describe:: <script>

   Execute the Python code contained in *script*, which must be a filesystem
   path (absolute or relative) referring to either a Python file, a directory
   containing a ``__main__.py`` file, or a zipfile containing a
   ``__main__.py`` file.

   If this option is given, the first element of :data:`sys.argv` will be the
   script name as given on the command line.

   If the script name refers directly to a Python file, the directory
   containing that file is added to the start of :data:`sys.path`, and the
   file is executed as the :mod:`__main__` module.

   If the script name refers to a directory or zipfile, the script name is
   added to the start of :data:`sys.path` and the ``__main__.py`` file in
   that location is executed as the :mod:`__main__` module.

   .. seealso::
      :func:`runpy.run_path`
         Equivalent functionality directly available to Python code


If no interface option is given, :option:`-i` is implied, ``sys.argv[0]`` is
an empty string (``""``) and the current directory will be added to the
start of :data:`sys.path`.  Also, tab-completion and history editing is
automatically enabled, if available on your platform (see
:ref:`rlcompleter-config`).

.. seealso::  :ref:`tut-invoking`

.. versionchanged:: 3.4
   Automatic enabling of tab-completion and history editing.


Generic options
~~~~~~~~~~~~~~~

.. cmdoption:: -?
               -h
               --help

   Print a short description of all command line options.


.. cmdoption:: -V
               --version

   Print the Python version number and exit.  Example output could be:

   .. code-block:: none

       Python 3.7.0b2+

   When given twice, print more information about the build, like:

   .. code-block:: none

       Python 3.7.0b2+ (3.7:0c076caaa8, Sep 22 2018, 12:04:24)
       [GCC 6.2.0 20161005]

   .. versionadded:: 3.6
      The ``-VV`` option.

.. _using-on-misc-options:

Miscellaneous options
~~~~~~~~~~~~~~~~~~~~~

.. cmdoption:: -b

   Issue a warning when comparing :class:`bytes` or :class:`bytearray` with
   :class:`str` or :class:`bytes` with :class:`int`.  Issue an error when the
   option is given twice (:option:`!-bb`).

   .. versionchanged:: 3.5
      Affects comparisons of :class:`bytes` with :class:`int`.

.. cmdoption:: -B

   If given, Python won't try to write ``.pyc`` files on the
   import of source modules.  See also :envvar:`PYTHONDONTWRITEBYTECODE`.


.. cmdoption:: --check-hash-based-pycs default|always|never

   Control the validation behavior of hash-based ``.pyc`` files. See
   :ref:`pyc-invalidation`. When set to ``default``, checked and unchecked
   hash-based bytecode cache files are validated according to their default
   semantics. When set to ``always``, all hash-based ``.pyc`` files, whether
   checked or unchecked, are validated against their corresponding source
   file. When set to ``never``, hash-based ``.pyc`` files are not validated
   against their corresponding source files.

   The semantics of timestamp-based ``.pyc`` files are unaffected by this
   option.


.. cmdoption:: -d

   Turn on parser debugging output (for expert only, depending on compilation
   options).  See also :envvar:`PYTHONDEBUG`.


.. cmdoption:: -E

   Ignore all :envvar:`PYTHON*` environment variables, e.g.
   :envvar:`PYTHONPATH` and :envvar:`PYTHONHOME`, that might be set.


.. cmdoption:: -i

   When a script is passed as first argument or the :option:`-c` option is used,
   enter interactive mode after executing the script or the command, even when
   :data:`sys.stdin` does not appear to be a terminal.  The
   :envvar:`PYTHONSTARTUP` file is not read.

   This can be useful to inspect global variables or a stack trace when a script
   raises an exception.  See also :envvar:`PYTHONINSPECT`.


.. cmdoption:: -I

   Run Python in isolated mode. This also implies -E and -s.
   In isolated mode :data:`sys.path` contains neither the script's directory nor
   the user's site-packages directory. All :envvar:`PYTHON*` environment
   variables are ignored, too. Further restrictions may be imposed to prevent
   the user from injecting malicious code.

   .. versionadded:: 3.4


.. cmdoption:: -O

   Remove assert statements and any code conditional on the value of
   :const:`__debug__`.  Augment the filename for compiled
   (:term:`bytecode`) files by adding ``.opt-1`` before the ``.pyc``
   extension (see :pep:`488`).  See also :envvar:`PYTHONOPTIMIZE`.

   .. versionchanged:: 3.5
      Modify ``.pyc`` filenames according to :pep:`488`.


.. cmdoption:: -OO

   Do :option:`-O` and also discard docstrings.  Augment the filename
   for compiled (:term:`bytecode`) files by adding ``.opt-2`` before the
   ``.pyc`` extension (see :pep:`488`).

   .. versionchanged:: 3.5
      Modify ``.pyc`` filenames according to :pep:`488`.


.. cmdoption:: -q

   Don't display the copyright and version messages even in interactive mode.

   .. versionadded:: 3.2


.. cmdoption:: -R

   Turn on hash randomization. This option only has an effect if the
   :envvar:`PYTHONHASHSEED` environment variable is set to ``0``, since hash
   randomization is enabled by default.

   On previous versions of Python, this option turns on hash randomization,
   so that the :meth:`__hash__` values of str, bytes and datetime
   are "salted" with an unpredictable random value.  Although they remain
   constant within an individual Python process, they are not predictable
   between repeated invocations of Python.

   Hash randomization is intended to provide protection against a
   denial-of-service caused by carefully-chosen inputs that exploit the worst
   case performance of a dict construction, O(n^2) complexity.  See
   http://www.ocert.org/advisories/ocert-2011-003.html for details.

   :envvar:`PYTHONHASHSEED` allows you to set a fixed value for the hash
   seed secret.

   .. versionchanged:: 3.7
      The option is no longer ignored.

   .. versionadded:: 3.2.3


.. cmdoption:: -s

   Don't add the :data:`user site-packages directory <site.USER_SITE>` to
   :data:`sys.path`.

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. cmdoption:: -S

   Disable the import of the module :mod:`site` and the site-dependent
   manipulations of :data:`sys.path` that it entails.  Also disable these
   manipulations if :mod:`site` is explicitly imported later (call
   :func:`site.main` if you want them to be triggered).


.. cmdoption:: -u

   Force the stdout and stderr streams to be unbuffered.  This option has no
   effect on the stdin stream.

   See also :envvar:`PYTHONUNBUFFERED`.

   .. versionchanged:: 3.7
      The text layer of the stdout and stderr streams now is unbuffered.


.. cmdoption:: -v

   Print a message each time a module is initialized, showing the place
   (filename or built-in module) from which it is loaded.  When given twice
   (:option:`!-vv`), print a message for each file that is checked for when
   searching for a module.  Also provides information on module cleanup at exit.
   See also :envvar:`PYTHONVERBOSE`.


.. _using-on-warnings:
.. cmdoption:: -W arg

   Warning control.  Python's warning machinery by default prints warning
   messages to :data:`sys.stderr`.  A typical warning message has the following
   form:

   .. code-block:: none

       file:line: category: message

   By default, each warning is printed once for each source line where it
   occurs.  This option controls how often warnings are printed.

   Multiple :option:`-W` options may be given; when a warning matches more than
   one option, the action for the last matching option is performed.  Invalid
   :option:`-W` options are ignored (though, a warning message is printed about
   invalid options when the first warning is issued).

   Warnings can also be controlled using the :envvar:`PYTHONWARNINGS`
   environment variable and from within a Python program using the
   :mod:`warnings` module.

   The simplest settings apply a particular action unconditionally to all
   warnings emitted by a process (even those that are otherwise ignored by
   default)::

       -Wdefault  # Warn once per call location
       -Werror    # Convert to exceptions
       -Walways   # Warn every time
       -Wmodule   # Warn once per calling module
       -Wonce     # Warn once per Python process
       -Wignore   # Never warn

   The action names can be abbreviated as desired (e.g. ``-Wi``, ``-Wd``,
   ``-Wa``, ``-We``) and the interpreter will resolve them to the appropriate
   action name.

   See :ref:`warning-filter` and :ref:`describing-warning-filters` for more
   details.


.. cmdoption:: -x

   Skip the first line of the source, allowing use of non-Unix forms of
   ``#!cmd``.  This is intended for a DOS specific hack only.


.. cmdoption:: -X

   Reserved for various implementation-specific options.  CPython currently
   defines the following possible values:

   * ``-X faulthandler`` to enable :mod:`faulthandler`;
   * ``-X showrefcount`` to output the total reference count and number of used
     memory blocks when the program finishes or after each statement in the
     interactive interpreter. This only works on debug builds.
   * ``-X tracemalloc`` to start tracing Python memory allocations using the
     :mod:`tracemalloc` module. By default, only the most recent frame is
     stored in a traceback of a trace. Use ``-X tracemalloc=NFRAME`` to start
     tracing with a traceback limit of *NFRAME* frames. See the
     :func:`tracemalloc.start` for more information.
   * ``-X showalloccount`` to output the total count of allocated objects for
     each type when the program finishes. This only works when Python was built with
     ``COUNT_ALLOCS`` defined.
   * ``-X importtime`` to show how long each import takes. It shows module
     name, cumulative time (including nested imports) and self time (excluding
     nested imports).  Note that its output may be broken in multi-threaded
     application.  Typical usage is ``python3 -X importtime -c 'import
     asyncio'``.  See also :envvar:`PYTHONPROFILEIMPORTTIME`.
   * ``-X dev``: enable CPython's "development mode", introducing additional
     runtime checks which are too expensive to be enabled by default. It should
     not be more verbose than the default if the code is correct: new warnings
     are only emitted when an issue is detected. Effect of the developer mode:

     * Add ``default`` warning filter, as :option:`-W` ``default``.
     * Install debug hooks on memory allocators: see the
       :c:func:`PyMem_SetupDebugHooks` C function.
     * Enable the :mod:`faulthandler` module to dump the Python traceback
       on a crash.
     * Enable :ref:`asyncio debug mode <asyncio-debug-mode>`.
     * Set the :attr:`~sys.flags.dev_mode` attribute of :attr:`sys.flags` to
       ``True``

   * ``-X utf8`` enables UTF-8 mode for operating system interfaces, overriding
     the default locale-aware mode. ``-X utf8=0`` explicitly disables UTF-8
     mode (even when it would otherwise activate automatically).
     See :envvar:`PYTHONUTF8` for more details.

   It also allows passing arbitrary values and retrieving them through the
   :data:`sys._xoptions` dictionary.

   .. versionchanged:: 3.2
      The :option:`-X` option was added.

   .. versionadded:: 3.3
      The ``-X faulthandler`` option.

   .. versionadded:: 3.4
      The ``-X showrefcount`` and ``-X tracemalloc`` options.

   .. versionadded:: 3.6
      The ``-X showalloccount`` option.

   .. versionadded:: 3.7
      The ``-X importtime``, ``-X dev`` and ``-X utf8`` options.


Options you shouldn't use
~~~~~~~~~~~~~~~~~~~~~~~~~

.. cmdoption:: -J

   Reserved for use by Jython_.

.. _Jython: http://www.jython.org/


.. _using-on-envvars:

Environment variables
---------------------

These environment variables influence Python's behavior, they are processed
before the command-line switches other than -E or -I.  It is customary that
command-line switches override environmental variables where there is a
conflict.

.. envvar:: PYTHONHOME

   Change the location of the standard Python libraries.  By default, the
   libraries are searched in :file:`{prefix}/lib/python{version}` and
   :file:`{exec_prefix}/lib/python{version}`, where :file:`{prefix}` and
   :file:`{exec_prefix}` are installation-dependent directories, both defaulting
   to :file:`/usr/local`.

   When :envvar:`PYTHONHOME` is set to a single directory, its value replaces
   both :file:`{prefix}` and :file:`{exec_prefix}`.  To specify different values
   for these, set :envvar:`PYTHONHOME` to :file:`{prefix}:{exec_prefix}`.


.. envvar:: PYTHONPATH

   Augment the default search path for module files.  The format is the same as
   the shell's :envvar:`PATH`: one or more directory pathnames separated by
   :data:`os.pathsep` (e.g. colons on Unix or semicolons on Windows).
   Non-existent directories are silently ignored.

   In addition to normal directories, individual :envvar:`PYTHONPATH` entries
   may refer to zipfiles containing pure Python modules (in either source or
   compiled form). Extension modules cannot be imported from zipfiles.

   The default search path is installation dependent, but generally begins with
   :file:`{prefix}/lib/python{version}` (see :envvar:`PYTHONHOME` above).  It
   is *always* appended to :envvar:`PYTHONPATH`.

   An additional directory will be inserted in the search path in front of
   :envvar:`PYTHONPATH` as described above under
   :ref:`using-on-interface-options`. The search path can be manipulated from
   within a Python program as the variable :data:`sys.path`.


.. envvar:: PYTHONSTARTUP

   If this is the name of a readable file, the Python commands in that file are
   executed before the first prompt is displayed in interactive mode.  The file
   is executed in the same namespace where interactive commands are executed so
   that objects defined or imported in it can be used without qualification in
   the interactive session.  You can also change the prompts :data:`sys.ps1` and
   :data:`sys.ps2` and the hook :data:`sys.__interactivehook__` in this file.


.. envvar:: PYTHONOPTIMIZE

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-O` option.  If set to an integer, it is equivalent to specifying
   :option:`-O` multiple times.


.. envvar:: PYTHONBREAKPOINT

   If this is set, it names a callable using dotted-path notation.  The module
   containing the callable will be imported and then the callable will be run
   by the default implementation of :func:`sys.breakpointhook` which itself is
   called by built-in :func:`breakpoint`.  If not set, or set to the empty
   string, it is equivalent to the value "pdb.set_trace".  Setting this to the
   string "0" causes the default implementation of :func:`sys.breakpointhook`
   to do nothing but return immediately.

   .. versionadded:: 3.7

.. envvar:: PYTHONDEBUG

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-d` option.  If set to an integer, it is equivalent to specifying
   :option:`-d` multiple times.


.. envvar:: PYTHONINSPECT

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-i` option.

   This variable can also be modified by Python code using :data:`os.environ`
   to force inspect mode on program termination.


.. envvar:: PYTHONUNBUFFERED

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-u` option.


.. envvar:: PYTHONVERBOSE

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-v` option.  If set to an integer, it is equivalent to specifying
   :option:`-v` multiple times.


.. envvar:: PYTHONCASEOK

   If this is set, Python ignores case in :keyword:`import` statements.  This
   only works on Windows and OS X.


.. envvar:: PYTHONDONTWRITEBYTECODE

   If this is set to a non-empty string, Python won't try to write ``.pyc``
   files on the import of source modules.  This is equivalent to
   specifying the :option:`-B` option.


.. envvar:: PYTHONHASHSEED

   If this variable is not set or set to ``random``, a random value is used
   to seed the hashes of str, bytes and datetime objects.

   If :envvar:`PYTHONHASHSEED` is set to an integer value, it is used as a fixed
   seed for generating the hash() of the types covered by the hash
   randomization.

   Its purpose is to allow repeatable hashing, such as for selftests for the
   interpreter itself, or to allow a cluster of python processes to share hash
   values.

   The integer must be a decimal number in the range [0,4294967295].  Specifying
   the value 0 will disable hash randomization.

   .. versionadded:: 3.2.3


.. envvar:: PYTHONIOENCODING

   If this is set before running the interpreter, it overrides the encoding used
   for stdin/stdout/stderr, in the syntax ``encodingname:errorhandler``.  Both
   the ``encodingname`` and the ``:errorhandler`` parts are optional and have
   the same meaning as in :func:`str.encode`.

   For stderr, the ``:errorhandler`` part is ignored; the handler will always be
   ``'backslashreplace'``.

   .. versionchanged:: 3.4
      The ``encodingname`` part is now optional.

   .. versionchanged:: 3.6
      On Windows, the encoding specified by this variable is ignored for interactive
      console buffers unless :envvar:`PYTHONLEGACYWINDOWSSTDIO` is also specified.
      Files and pipes redirected through the standard streams are not affected.

.. envvar:: PYTHONNOUSERSITE

   If this is set, Python won't add the :data:`user site-packages directory
   <site.USER_SITE>` to :data:`sys.path`.

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. envvar:: PYTHONUSERBASE

   Defines the :data:`user base directory <site.USER_BASE>`, which is used to
   compute the path of the :data:`user site-packages directory <site.USER_SITE>`
   and :ref:`Distutils installation paths <inst-alt-install-user>` for
   ``python setup.py install --user``.

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. envvar:: PYTHONEXECUTABLE

   If this environment variable is set, ``sys.argv[0]`` will be set to its
   value instead of the value got through the C runtime.  Only works on
   Mac OS X.

.. envvar:: PYTHONWARNINGS

   This is equivalent to the :option:`-W` option. If set to a comma
   separated string, it is equivalent to specifying :option:`-W` multiple
   times, with filters later in the list taking precedence over those earlier
   in the list.

   The simplest settings apply a particular action unconditionally to all
   warnings emitted by a process (even those that are otherwise ignored by
   default)::

       PYTHONWARNINGS=default  # Warn once per call location
       PYTHONWARNINGS=error    # Convert to exceptions
       PYTHONWARNINGS=always   # Warn every time
       PYTHONWARNINGS=module   # Warn once per calling module
       PYTHONWARNINGS=once     # Warn once per Python process
       PYTHONWARNINGS=ignore   # Never warn

   See :ref:`warning-filter` and :ref:`describing-warning-filters` for more
   details.


.. envvar:: PYTHONFAULTHANDLER

   If this environment variable is set to a non-empty string,
   :func:`faulthandler.enable` is called at startup: install a handler for
   :const:`SIGSEGV`, :const:`SIGFPE`, :const:`SIGABRT`, :const:`SIGBUS` and
   :const:`SIGILL` signals to dump the Python traceback.  This is equivalent to
   :option:`-X` ``faulthandler`` option.

   .. versionadded:: 3.3


.. envvar:: PYTHONTRACEMALLOC

   If this environment variable is set to a non-empty string, start tracing
   Python memory allocations using the :mod:`tracemalloc` module. The value of
   the variable is the maximum number of frames stored in a traceback of a
   trace. For example, ``PYTHONTRACEMALLOC=1`` stores only the most recent
   frame. See the :func:`tracemalloc.start` for more information.

   .. versionadded:: 3.4


.. envvar:: PYTHONPROFILEIMPORTTIME

   If this environment variable is set to a non-empty string, Python will
   show how long each import takes.  This is exactly equivalent to setting
   ``-X importtime`` on the command line.

   .. versionadded:: 3.7


.. envvar:: PYTHONASYNCIODEBUG

   If this environment variable is set to a non-empty string, enable the
   :ref:`debug mode <asyncio-debug-mode>` of the :mod:`asyncio` module.

   .. versionadded:: 3.4


.. envvar:: PYTHONMALLOC

   Set the Python memory allocators and/or install debug hooks.

   Set the family of memory allocators used by Python:

   * ``default``: use the :ref:`default memory allocators
     <default-memory-allocators>`.
   * ``malloc``: use the :c:func:`malloc` function of the C library
     for all domains (:c:data:`PYMEM_DOMAIN_RAW`, :c:data:`PYMEM_DOMAIN_MEM`,
     :c:data:`PYMEM_DOMAIN_OBJ`).
   * ``pymalloc``: use the :ref:`pymalloc allocator <pymalloc>` for
     :c:data:`PYMEM_DOMAIN_MEM` and :c:data:`PYMEM_DOMAIN_OBJ` domains and use
     the :c:func:`malloc` function for the :c:data:`PYMEM_DOMAIN_RAW` domain.

   Install debug hooks:

   * ``debug``: install debug hooks on top of the :ref:`default memory
     allocators <default-memory-allocators>`.
   * ``malloc_debug``: same as ``malloc`` but also install debug hooks
   * ``pymalloc_debug``: same as ``pymalloc`` but also install debug hooks

   See the :ref:`default memory allocators <default-memory-allocators>` and the
   :c:func:`PyMem_SetupDebugHooks` function (install debug hooks on Python
   memory allocators).

   .. versionchanged:: 3.7
      Added the ``"default"`` allocator.

   .. versionadded:: 3.6


.. envvar:: PYTHONMALLOCSTATS

   If set to a non-empty string, Python will print statistics of the
   :ref:`pymalloc memory allocator <pymalloc>` every time a new pymalloc object
   arena is created, and on shutdown.

   This variable is ignored if the :envvar:`PYTHONMALLOC` environment variable
   is used to force the :c:func:`malloc` allocator of the C library, or if
   Python is configured without ``pymalloc`` support.

   .. versionchanged:: 3.6
      This variable can now also be used on Python compiled in release mode.
      It now has no effect if set to an empty string.


.. envvar:: PYTHONLEGACYWINDOWSFSENCODING

   If set to a non-empty string, the default filesystem encoding and errors mode
   will revert to their pre-3.6 values of 'mbcs' and 'replace', respectively.
   Otherwise, the new defaults 'utf-8' and 'surrogatepass' are used.

   This may also be enabled at runtime with
   :func:`sys._enablelegacywindowsfsencoding()`.

   .. availability:: Windows.

   .. versionadded:: 3.6
      See :pep:`529` for more details.

.. envvar:: PYTHONLEGACYWINDOWSSTDIO

   If set to a non-empty string, does not use the new console reader and
   writer. This means that Unicode characters will be encoded according to
   the active console code page, rather than using utf-8.

   This variable is ignored if the standard streams are redirected (to files
   or pipes) rather than referring to console buffers.

   .. availability:: Windows.

   .. versionadded:: 3.6


.. envvar:: PYTHONCOERCECLOCALE

   If set to the value ``0``, causes the main Python command line application
   to skip coercing the legacy ASCII-based C and POSIX locales to a more
   capable UTF-8 based alternative.

   If this variable is *not* set (or is set to a value other than ``0``), the
   ``LC_ALL`` locale override environment variable is also not set, and the
   current locale reported for the ``LC_CTYPE`` category is either the default
   ``C`` locale, or else the explicitly ASCII-based ``POSIX`` locale, then the
   Python CLI will attempt to configure the following locales for the
   ``LC_CTYPE`` category in the order listed before loading the interpreter
   runtime:

   * ``C.UTF-8``
   * ``C.utf8``
   * ``UTF-8``

   If setting one of these locale categories succeeds, then the ``LC_CTYPE``
   environment variable will also be set accordingly in the current process
   environment before the Python runtime is initialized. This ensures that in
   addition to being seen by both the interpreter itself and other locale-aware
   components running in the same process (such as the GNU ``readline``
   library), the updated setting is also seen in subprocesses (regardless of
   whether or not those processes are running a Python interpreter), as well as
   in operations that query the environment rather than the current C locale
   (such as Python's own :func:`locale.getdefaultlocale`).

   Configuring one of these locales (either explicitly or via the above
   implicit locale coercion) automatically enables the ``surrogateescape``
   :ref:`error handler <error-handlers>` for :data:`sys.stdin` and
   :data:`sys.stdout` (:data:`sys.stderr` continues to use ``backslashreplace``
   as it does in any other locale). This stream handling behavior can be
   overridden using :envvar:`PYTHONIOENCODING` as usual.

   For debugging purposes, setting ``PYTHONCOERCECLOCALE=warn`` will cause
   Python to emit warning messages on ``stderr`` if either the locale coercion
   activates, or else if a locale that *would* have triggered coercion is
   still active when the Python runtime is initialized.

   Also note that even when locale coercion is disabled, or when it fails to
   find a suitable target locale, :envvar:`PYTHONUTF8` will still activate by
   default in legacy ASCII-based locales. Both features must be disabled in
   order to force the interpreter to use ``ASCII`` instead of ``UTF-8`` for
   system interfaces.

   .. availability:: \*nix.

   .. versionadded:: 3.7
      See :pep:`538` for more details.


.. envvar:: PYTHONDEVMODE

   If this environment variable is set to a non-empty string, enable the
   CPython "development mode". See the :option:`-X` ``dev`` option.

   .. versionadded:: 3.7

.. envvar:: PYTHONUTF8

   If set to ``1``, enables the interpreter's UTF-8 mode, where ``UTF-8`` is
   used as the text encoding for system interfaces, regardless of the
   current locale setting.

   This means that:

    * :func:`sys.getfilesystemencoding()` returns ``'UTF-8'`` (the locale
      encoding is ignored).
    * :func:`locale.getpreferredencoding()` returns ``'UTF-8'`` (the locale
      encoding is ignored, and the function's ``do_setlocale`` parameter has no
      effect).
    * :data:`sys.stdin`, :data:`sys.stdout`, and :data:`sys.stderr` all use
      UTF-8 as their text encoding, with the ``surrogateescape``
      :ref:`error handler <error-handlers>` being enabled for :data:`sys.stdin`
      and :data:`sys.stdout` (:data:`sys.stderr` continues to use
      ``backslashreplace`` as it does in the default locale-aware mode)

   As a consequence of the changes in those lower level APIs, other higher
   level APIs also exhibit different default behaviours:

    * Command line arguments, environment variables and filenames are decoded
      to text using the UTF-8 encoding.
    * :func:`os.fsdecode()` and :func:`os.fsencode()` use the UTF-8 encoding.
    * :func:`open()`, :func:`io.open()`, and :func:`codecs.open()` use the UTF-8
      encoding by default. However, they still use the strict error handler by
      default so that attempting to open a binary file in text mode is likely
      to raise an exception rather than producing nonsense data.

   Note that the standard stream settings in UTF-8 mode can be overridden by
   :envvar:`PYTHONIOENCODING` (just as they can be in the default locale-aware
   mode).

   If set to ``0``, the interpreter runs in its default locale-aware mode.

   Setting any other non-empty string causes an error during interpreter
   initialisation.

   If this environment variable is not set at all, then the interpreter defaults
   to using the current locale settings, *unless* the current locale is
   identified as a legacy ASCII-based locale
   (as described for :envvar:`PYTHONCOERCECLOCALE`), and locale coercion is
   either disabled or fails. In such legacy locales, the interpreter will
   default to enabling UTF-8 mode unless explicitly instructed not to do so.

   Also available as the :option:`-X` ``utf8`` option.

   .. availability:: \*nix.

   .. versionadded:: 3.7
      See :pep:`540` for more details.


Debug-mode variables
~~~~~~~~~~~~~~~~~~~~

Setting these variables only has an effect in a debug build of Python, that is,
if Python was configured with the ``--with-pydebug`` build option.

.. envvar:: PYTHONTHREADDEBUG

   If set, Python will print threading debug info.


.. envvar:: PYTHONDUMPREFS

   If set, Python will dump objects and reference counts still alive after
   shutting down the interpreter.
