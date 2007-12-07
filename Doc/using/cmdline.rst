.. highlightlang:: none

.. _using-on-general:

Command line and environment
============================

The CPython interpreter scans the command line and the environment for various
settings.

.. note:: 
   
   Other implementations' command line schemes may differ.  See
   :ref:`implementations` for further resources.


.. _using-on-cmdline:

Command line
------------

When invoking Python, you may specify any of these options::

    python [-bdEiOStuUvxX?] [-c command | -m module-name | script | - ] [args]

The most common use case is, of course, a simple invocation of a script::

    python myscript.py


Interface options
~~~~~~~~~~~~~~~~~

The interpreter interface resembles that of the UNIX shell:

* When called with standard input connected to a tty device, it prompts for
  commands and executes them until an EOF (an end-of-file character, you can
  produce that with *Ctrl-D* on UNIX or *Ctrl-Z, Enter* on Windows) is read.
* When called with a file name argument or with a file as standard input, it
  reads and executes a script from that file.
* When called with ``-c command``, it executes the Python statement(s) given as
  *command*.  Here *command* may contain multiple statements separated by
  newlines. Leading whitespace is significant in Python statements!
* When called with ``-m module-name``, the given module is searched on the
  Python module path and executed as a script.

In non-interactive mode, the entire input is parsed before it is executed.

An interface option terminates the list of options consumed by the interpreter,
all consecutive arguments will end up in :data:`sys.argv` -- note that the first
element, subscript zero (``sys.argv[0]``), is a string reflecting the program's
source.

.. cmdoption:: -c <command>

   Execute the Python code in *command*.  *command* can be one ore more
   statements separated by newlines, with significant leading whitespace as in
   normal module code.
   
   If this option is given, the first element of :data:`sys.argv` will be
   ``"-c"``.


.. cmdoption:: -m <module-name>

   Search :data:`sys.path` for the named module and run the corresponding module
   file as if it were executed with ``python modulefile.py`` as a script.
   
   Since the argument is a *module* name, you must not give a file extension
   (``.py``).  However, the ``module-name`` does not have to be a valid Python
   identifer (e.g. you can use a file name including a hyphen).

   .. note::

      This option cannot be used with builtin modules and extension modules
      written in C, since they do not have Python module files.
   
   If this option is given, the first element of :data:`sys.argv` will be the
   full path to the module file.
   
   Many standard library modules contain code that is invoked on their execution
   as a script.  An example is the :mod:`timeit` module::

       python -mtimeit -s 'setup here' 'benchmarked code here'
       python -mtimeit -h # for details

   .. seealso:: 
      :func:`runpy.run_module`
         The actual implementation of this feature.

      :pep:`338` -- Executing modules as scripts


.. describe:: <script>

   Execute the Python code contained in *script*, which must be an (absolute or
   relative) file name.

   If this option is given, the first element of :data:`sys.argv` will be the
   script file name as given on the command line.


.. describe:: -

   Read commands from standard input (:data:`sys.stdin`).  If standard input is
   a terminal, :option:`-i` is implied.

   If this option is given, the first element of :data:`sys.argv` will be
   ``"-"``.

   .. seealso:: 
      :ref:`tut-invoking`


If no script name is given, ``sys.argv[0]`` is an empty string (``""``).


Generic options
~~~~~~~~~~~~~~~

.. cmdoption:: -?
               -h
               --help

   Print a short description of all command line options.


.. cmdoption:: -V
               --version

   Print the Python version number and exit.  Example output could be::
    
       Python 2.5.1


Miscellaneous options
~~~~~~~~~~~~~~~~~~~~~

.. cmdoption:: -b

   Issue a warning when comparing str and bytes. Issue an error when the
   option is given twice (:option:`-bb`).


.. cmdoption:: -d

   Turn on parser debugging output (for wizards only, depending on compilation
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


.. cmdoption:: -O

   Turn on basic optimizations.  This changes the filename extension for
   compiled (:term:`bytecode`) files from ``.pyc`` to ``.pyo``.  See also
   :envvar:`PYTHONOPTIMIZE`.


.. cmdoption:: -OO

   Discard docstrings in addition to the :option:`-O` optimizations.




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


.. XXX should the -U option be documented?

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
   
   Warnings can also be controlled from within a Python program using the
   :mod:`warnings` module.

   The simplest form of argument is one of the following action strings (or a
   unique abbreviation):
    
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
      Print each warning only only the first time it occurs in each module.
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
   warning category.  This must be a class name; the match test whether the
   actual warning category of the message is a subclass of the specified warning
   category.  The full class name must be given.  The *module* field matches the
   (fully-qualified) module name; this match is case-sensitive.  The *line*
   field matches the line number, where zero matches all line numbers and is
   thus equivalent to an omitted line number.

   .. seealso::

      :pep:`230` -- Warning framework


.. cmdoption:: -x
   
   Skip the first line of the source, allowing use of non-Unix forms of
   ``#!cmd``.  This is intended for a DOS specific hack only.
   
   .. warning:: The line numbers in error messages will be off by one!

.. _using-on-envvars:

Environment variables
---------------------

.. envvar:: PYTHONHOME
   
   Change the location of the standard Python libraries.  By default, the
   libraries are searched in :file:`{prefix}/lib/python<version>` and
   :file:`{exec_prefix}/lib/python<version>`, where :file:`{prefix}` and
   :file:`{exec_prefix}` are installation-dependent directories, both defaulting
   to :file:`/usr/local`.
   
   When :envvar:`PYTHONHOME` is set to a single directory, its value replaces
   both :file:`{prefix}` and :file:`{exec_prefix}`.  To specify different values
   for these, set :envvar:`PYTHONHOME` to :file:`{prefix}:{exec_prefix}``.


.. envvar:: PYTHONPATH

   Augments the default search path for module files.  The format is the same as
   the shell's :envvar:`PATH`: one or more directory pathnames separated by
   colons.  Non-existent directories are silently ignored.
   
   The default search path is installation dependent, but generally begins with
   :file:`{prefix}/lib/python<version>`` (see :envvar:`PYTHONHOME` above).  It
   is *always* appended to :envvar:`PYTHONPATH`.
   
   If a script argument is given, the directory containing the script is
   inserted in the path in front of :envvar:`PYTHONPATH`.  The search path can
   be manipulated from within a Python program as the variable :data:`sys.path`.


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


.. envvar:: PYTHONUNBUFFERED
   
   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-u` option.


.. envvar:: PYTHONVERBOSE
   
   If this is set to a non-empty string it is equivalent to specifying the
   :option:`-v` option.  If set to an integer, it is equivalent to specifying
   :option:`-v` multiple times.


.. envvar:: PYTHONCASEOK
   
   If this is set, Python ignores case in :keyword:`import` statements.  This
   only works on Windows.

