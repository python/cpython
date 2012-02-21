.. highlightlang:: none

.. ATTENTION: You probably should update Misc/python.man, too, if you modify
.. this file.

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

    python [-BdEiOQsRStuUvVWxX3?] [-c command | -m module-name | script | - ] [args]

The most common use case is, of course, a simple invocation of a script::

    python myscript.py


.. _using-on-interface-options:

Interface options
~~~~~~~~~~~~~~~~~

The interpreter interface resembles that of the UNIX shell, but provides some
additional methods of invocation:

* When called with standard input connected to a tty device, it prompts for
  commands and executes them until an EOF (an end-of-file character, you can
  produce that with *Ctrl-D* on UNIX or *Ctrl-Z, Enter* on Windows) is read.
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
   (``.py``).  The ``module-name`` should be a valid Python module name, but
   the implementation may not always enforce this (e.g. it may allow you to
   use a name that includes a hyphen).

   Package names are also permitted. When a package name is supplied instead
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
   full path to the module file. As with the :option:`-c` option, the current
   directory will be added to the start of :data:`sys.path`.

   Many standard library modules contain code that is invoked on their execution
   as a script.  An example is the :mod:`timeit` module::

       python -mtimeit -s 'setup here' 'benchmarked code here'
       python -mtimeit -h # for details

   .. seealso::
      :func:`runpy.run_module`
         Equivalent functionality directly available to Python code

      :pep:`338` -- Executing modules as scripts

   .. versionadded:: 2.4

   .. versionchanged:: 2.5
      The named module can now be located inside a package.

   .. versionchanged:: 2.7
      Supply the package name to run a ``__main__`` submodule.
      sys.argv[0] is now set to ``"-m"`` while searching for the module
      (it was previously incorrectly set to ``"-c"``)


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

   .. versionchanged:: 2.5
      Directories and zipfiles containing a ``__main__.py`` file at the top
      level are now considered valid Python scripts.

If no interface option is given, :option:`-i` is implied, ``sys.argv[0]`` is
an empty string (``""``) and the current directory will be added to the
start of :data:`sys.path`.

.. seealso::  :ref:`tut-invoking`


Generic options
~~~~~~~~~~~~~~~

.. cmdoption:: -?
               -h
               --help

   Print a short description of all command line options.

   .. versionchanged:: 2.5
      The ``--help`` variant.


.. cmdoption:: -V
               --version

   Print the Python version number and exit.  Example output could be::

       Python 2.5.1

   .. versionchanged:: 2.5
      The ``--version`` variant.


Miscellaneous options
~~~~~~~~~~~~~~~~~~~~~

.. cmdoption:: -B

   If given, Python won't try to write ``.pyc`` or ``.pyo`` files on the
   import of source modules.  See also :envvar:`PYTHONDONTWRITEBYTECODE`.

   .. versionadded:: 2.6


.. cmdoption:: -d

   Turn on parser debugging output (for wizards only, depending on compilation
   options).  See also :envvar:`PYTHONDEBUG`.


.. cmdoption:: -E

   Ignore all :envvar:`PYTHON*` environment variables, e.g.
   :envvar:`PYTHONPATH` and :envvar:`PYTHONHOME`, that might be set.

   .. versionadded:: 2.2


.. cmdoption:: -i

   When a script is passed as first argument or the :option:`-c` option is used,
   enter interactive mode after executing the script or the command, even when
   :data:`sys.stdin` does not appear to be a terminal.  The
   :envvar:`PYTHONSTARTUP` file is not read.

   This can be useful to inspect global variables or a stack trace when a script
   raises an exception.  See also :envvar:`PYTHONINSPECT`.


.. cmdoption:: -O

   Turn on basic optimizations.  This changes the filename extension for
   compiled (:term:`bytecode`) files from ``.pyc`` to ``.pyo``.  See also
   :envvar:`PYTHONOPTIMIZE`.


.. cmdoption:: -OO

   Discard docstrings in addition to the :option:`-O` optimizations.


.. cmdoption:: -Q <arg>

   Division control. The argument must be one of the following:

   ``old``
     division of int/int and long/long return an int or long (*default*)
   ``new``
     new division semantics, i.e. division of int/int and long/long returns a
     float
   ``warn``
     old division semantics with a warning for int/int and long/long
   ``warnall``
     old division semantics with a warning for all uses of the division operator

   .. seealso::
      :file:`Tools/scripts/fixdiv.py`
         for a use of ``warnall``

      :pep:`238` -- Changing the division operator


.. cmdoption:: -R

   Turn on hash randomization, so that the :meth:`__hash__` values of str,
   bytes and datetime objects are "salted" with an unpredictable random value.
   Although they remain constant within an individual Python process, they are
   not predictable between repeated invocations of Python.

   This is intended to provide protection against a denial-of-service caused by
   carefully-chosen inputs that exploit the worst case performance of a dict
   construction, O(n^2) complexity.  See
   http://www.ocert.org/advisories/ocert-2011-003.html for details.

   Changing hash values affects the order in which keys are retrieved from a
   dict.  Although Python has never made guarantees about this ordering (and it
   typically varies between 32-bit and 64-bit builds), enough real-world code
   implicitly relies on this non-guaranteed behavior that the randomization is
   disabled by default.

   See also :envvar:`PYTHONHASHSEED`.

   .. versionadded:: 2.6.8


.. cmdoption:: -s

   Don't add the :data:`user site-packages directory <site.USER_SITE>` to
   :data:`sys.path`.

   .. versionadded:: 2.6

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. cmdoption:: -S

   Disable the import of the module :mod:`site` and the site-dependent
   manipulations of :data:`sys.path` that it entails.


.. cmdoption:: -t

   Issue a warning when a source file mixes tabs and spaces for indentation in a
   way that makes it depend on the worth of a tab expressed in spaces.  Issue an
   error when the option is given twice (:option:`-tt`).


.. cmdoption:: -u

   Force stdin, stdout and stderr to be totally unbuffered.  On systems where it
   matters, also put stdin, stdout and stderr in binary mode.

   Note that there is internal buffering in :meth:`file.readlines` and
   :ref:`bltin-file-objects` (``for line in sys.stdin``) which is not influenced
   by this option.  To work around this, you will want to use
   :meth:`file.readline` inside a ``while 1:`` loop.

   See also :envvar:`PYTHONUNBUFFERED`.


.. cmdoption:: -v

   Print a message each time a module is initialized, showing the place
   (filename or built-in module) from which it is loaded.  When given twice
   (:option:`-vv`), print a message for each file that is checked for when
   searching for a module.  Also provides information on module cleanup at exit.
   See also :envvar:`PYTHONVERBOSE`.


.. cmdoption:: -W arg

   Warning control.  Python's warning machinery by default prints warning
   messages to :data:`sys.stderr`.  A typical warning message has the following
   form::

       file:line: category: message

   By default, each warning is printed once for each source line where it
   occurs.  This option controls how often warnings are printed.

   Multiple :option:`-W` options may be given; when a warning matches more than
   one option, the action for the last matching option is performed.  Invalid
   :option:`-W` options are ignored (though, a warning message is printed about
   invalid options when the first warning is issued).

   Starting from Python 2.7, :exc:`DeprecationWarning` and its descendants
   are ignored by default.  The :option:`-Wd` option can be used to re-enable
   them.

   Warnings can also be controlled from within a Python program using the
   :mod:`warnings` module.

   The simplest form of argument is one of the following action strings (or a
   unique abbreviation) by themselves:

   ``ignore``
      Ignore all warnings.
   ``default``
      Explicitly request the default behavior (printing each warning once per
      source line).
   ``all``
      Print a warning each time it occurs (this may generate many messages if a
      warning is triggered repeatedly for the same source line, such as inside a
      loop).
   ``module``
      Print each warning only the first time it occurs in each module.
   ``once``
      Print each warning only the first time it occurs in the program.
   ``error``
      Raise an exception instead of printing a warning message.

   The full form of argument is::

       action:message:category:module:line

   Here, *action* is as explained above but only applies to messages that match
   the remaining fields.  Empty fields match all values; trailing empty fields
   may be omitted.  The *message* field matches the start of the warning message
   printed; this match is case-insensitive.  The *category* field matches the
   warning category.  This must be a class name; the match tests whether the
   actual warning category of the message is a subclass of the specified warning
   category.  The full class name must be given.  The *module* field matches the
   (fully-qualified) module name; this match is case-sensitive.  The *line*
   field matches the line number, where zero matches all line numbers and is
   thus equivalent to an omitted line number.

   .. seealso::
      :mod:`warnings` -- the warnings module

      :pep:`230` -- Warning framework

      :envvar:`PYTHONWARNINGS`


.. cmdoption:: -x

   Skip the first line of the source, allowing use of non-Unix forms of
   ``#!cmd``.  This is intended for a DOS specific hack only.

   .. note:: The line numbers in error messages will be off by one.

.. cmdoption:: -3

   Warn about Python 3.x incompatibilities which cannot be fixed trivially by
   :ref:`2to3 <2to3-reference>`. Among these are:

   * :meth:`dict.has_key`
   * :func:`apply`
   * :func:`callable`
   * :func:`coerce`
   * :func:`execfile`
   * :func:`reduce`
   * :func:`reload`

   Using these will emit a :exc:`DeprecationWarning`.

   .. versionadded:: 2.6

Options you shouldn't use
~~~~~~~~~~~~~~~~~~~~~~~~~

.. cmdoption:: -J

   Reserved for use by Jython_.

.. _Jython: http://jython.org

.. cmdoption:: -U

   Turns all string literals into unicodes globally.  Do not be tempted to use
   this option as it will probably break your world.  It also produces
   ``.pyc`` files with a different magic number than normal.  Instead, you can
   enable unicode literals on a per-module basis by using::

        from __future__ import unicode_literals

   at the top of the file.  See :mod:`__future__` for details.

.. cmdoption:: -X

    Reserved for alternative implementations of Python to use for their own
    purposes.

.. _using-on-envvars:

Environment variables
---------------------

These environment variables influence Python's behavior.

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
   :data:`sys.ps2` in this file.


.. envvar:: PYTHONY2K

   Set this to a non-empty string to cause the :mod:`time` module to require
   dates specified as strings to include 4-digit years, otherwise 2-digit years
   are converted based on rules described in the :mod:`time` module
   documentation.


.. envvar:: PYTHONOPTIMIZE

   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-O` option.  If set to an integer, it is equivalent to specifying
   :option:`-O` multiple times.


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
   only works on Windows, OS X, OS/2, and RiscOS.


.. envvar:: PYTHONDONTWRITEBYTECODE

   If this is set, Python won't try to write ``.pyc`` or ``.pyo`` files on the
   import of source modules.

   .. versionadded:: 2.6

.. envvar:: PYTHONHASHSEED

   If this variable is set to ``random``, the effect is the same as specifying
   the :option:`-R` option: a random value is used to seed the hashes of str,
   bytes and datetime objects.

   If :envvar:`PYTHONHASHSEED` is set to an integer value, it is used as a
   fixed seed for generating the hash() of the types covered by the hash
   randomization.

   Its purpose is to allow repeatable hashing, such as for selftests for the
   interpreter itself, or to allow a cluster of python processes to share hash
   values.

   The integer must be a decimal number in the range [0,4294967295].
   Specifying the value 0 will lead to the same hash values as when hash
   randomization is disabled.

   .. versionadded:: 2.6.8


.. envvar:: PYTHONIOENCODING

   Overrides the encoding used for stdin/stdout/stderr, in the syntax
   ``encodingname:errorhandler``.  The ``:errorhandler`` part is optional and
   has the same meaning as in :func:`str.encode`.

   .. versionadded:: 2.6


.. envvar:: PYTHONNOUSERSITE

   If this is set, Python won't add the :data:`user site-packages directory
   <site.USER_SITE>` to :data:`sys.path`.

   .. versionadded:: 2.6

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. envvar:: PYTHONUSERBASE

   Defines the :data:`user base directory <site.USER_BASE>`, which is used to
   compute the path of the :data:`user site-packages directory <site.USER_SITE>`
   and :ref:`Distutils installation paths <inst-alt-install-user>` for ``python
   setup.py install --user``.

   .. versionadded:: 2.6

   .. seealso::

      :pep:`370` -- Per user site-packages directory


.. envvar:: PYTHONEXECUTABLE

   If this environment variable is set, ``sys.argv[0]`` will be set to its
   value instead of the value got through the C runtime.  Only works on
   Mac OS X.

.. envvar:: PYTHONWARNINGS

   This is equivalent to the :option:`-W` option. If set to a comma
   separated string, it is equivalent to specifying :option:`-W` multiple
   times.


Debug-mode variables
~~~~~~~~~~~~~~~~~~~~

Setting these variables only has an effect in a debug build of Python, that is,
if Python was configured with the ``--with-pydebug`` build option.

.. envvar:: PYTHONTHREADDEBUG

   If set, Python will print threading debug info.

   .. versionchanged:: 2.6
      Previously, this variable was called ``THREADDEBUG``.

.. envvar:: PYTHONDUMPREFS

   If set, Python will dump objects and reference counts still alive after
   shutting down the interpreter.


.. envvar:: PYTHONMALLOCSTATS

   If set, Python will print memory allocation statistics every time a new
   object arena is created, and on shutdown.

