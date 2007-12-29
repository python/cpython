.. _tut-modules:

*******
Modules
*******

If you quit from the Python interpreter and enter it again, the definitions you
have made (functions and variables) are lost. Therefore, if you want to write a
somewhat longer program, you are better off using a text editor to prepare the
input for the interpreter and running it with that file as input instead.  This
is known as creating a *script*.  As your program gets longer, you may want to
split it into several files for easier maintenance.  You may also want to use a
handy function that you've written in several programs without copying its
definition into each program.

To support this, Python has a way to put definitions in a file and use them in a
script or in an interactive instance of the interpreter. Such a file is called a
*module*; definitions from a module can be *imported* into other modules or into
the *main* module (the collection of variables that you have access to in a
script executed at the top level and in calculator mode).

A module is a file containing Python definitions and statements.  The file name
is the module name with the suffix :file:`.py` appended.  Within a module, the
module's name (as a string) is available as the value of the global variable
``__name__``.  For instance, use your favorite text editor to create a file
called :file:`fibo.py` in the current directory with the following contents::

   # Fibonacci numbers module

   def fib(n):    # write Fibonacci series up to n
       a, b = 0, 1
       while b < n:
           print b,
           a, b = b, a+b

   def fib2(n): # return Fibonacci series up to n
       result = []
       a, b = 0, 1
       while b < n:
           result.append(b)
           a, b = b, a+b
       return result

Now enter the Python interpreter and import this module with the following
command::

   >>> import fibo

This does not enter the names of the functions defined in ``fibo``  directly in
the current symbol table; it only enters the module name ``fibo`` there. Using
the module name you can access the functions::

   >>> fibo.fib(1000)
   1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987
   >>> fibo.fib2(100)
   [1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89]
   >>> fibo.__name__
   'fibo'

If you intend to use a function often you can assign it to a local name::

   >>> fib = fibo.fib
   >>> fib(500)
   1 1 2 3 5 8 13 21 34 55 89 144 233 377


.. _tut-moremodules:

More on Modules
===============

A module can contain executable statements as well as function definitions.
These statements are intended to initialize the module. They are executed only
the *first* time the module is imported somewhere. [#]_

Each module has its own private symbol table, which is used as the global symbol
table by all functions defined in the module. Thus, the author of a module can
use global variables in the module without worrying about accidental clashes
with a user's global variables. On the other hand, if you know what you are
doing you can touch a module's global variables with the same notation used to
refer to its functions, ``modname.itemname``.

Modules can import other modules.  It is customary but not required to place all
:keyword:`import` statements at the beginning of a module (or script, for that
matter).  The imported module names are placed in the importing module's global
symbol table.

There is a variant of the :keyword:`import` statement that imports names from a
module directly into the importing module's symbol table.  For example::

   >>> from fibo import fib, fib2
   >>> fib(500)
   1 1 2 3 5 8 13 21 34 55 89 144 233 377

This does not introduce the module name from which the imports are taken in the
local symbol table (so in the example, ``fibo`` is not defined).

There is even a variant to import all names that a module defines::

   >>> from fibo import *
   >>> fib(500)
   1 1 2 3 5 8 13 21 34 55 89 144 233 377

This imports all names except those beginning with an underscore (``_``).


.. _tut-modulesasscripts:

Executing modules as scripts
----------------------------

When you run a Python module with ::

   python fibo.py <arguments>

the code in the module will be executed, just as if you imported it, but with
the ``__name__`` set to ``"__main__"``.  That means that by adding this code at
the end of your module::

   if __name__ == "__main__":
       import sys
       fib(int(sys.argv[1]))

you can make the file usable as a script as well as an importable module,
because the code that parses the command line only runs if the module is
executed as the "main" file::

   $ python fibo.py 50
   1 1 2 3 5 8 13 21 34

If the module is imported, the code is not run::

   >>> import fibo
   >>>

This is often used either to provide a convenient user interface to a module, or
for testing purposes (running the module as a script executes a test suite).


.. _tut-searchpath:

The Module Search Path
----------------------

.. index:: triple: module; search; path

When a module named :mod:`spam` is imported, the interpreter searches for a file
named :file:`spam.py` in the current directory, and then in the list of
directories specified by the environment variable :envvar:`PYTHONPATH`.  This
has the same syntax as the shell variable :envvar:`PATH`, that is, a list of
directory names.  When :envvar:`PYTHONPATH` is not set, or when the file is not
found there, the search continues in an installation-dependent default path; on
Unix, this is usually :file:`.:/usr/local/lib/python`.

Actually, modules are searched in the list of directories given by the variable
``sys.path`` which is initialized from the directory containing the input script
(or the current directory), :envvar:`PYTHONPATH` and the installation- dependent
default.  This allows Python programs that know what they're doing to modify or
replace the module search path.  Note that because the directory containing the
script being run is on the search path, it is important that the script not have
the same name as a standard module, or Python will attempt to load the script as
a module when that module is imported. This will generally be an error.  See
section :ref:`tut-standardmodules` for more information.


"Compiled" Python files
-----------------------

As an important speed-up of the start-up time for short programs that use a lot
of standard modules, if a file called :file:`spam.pyc` exists in the directory
where :file:`spam.py` is found, this is assumed to contain an
already-"byte-compiled" version of the module :mod:`spam`. The modification time
of the version of :file:`spam.py` used to create :file:`spam.pyc` is recorded in
:file:`spam.pyc`, and the :file:`.pyc` file is ignored if these don't match.

Normally, you don't need to do anything to create the :file:`spam.pyc` file.
Whenever :file:`spam.py` is successfully compiled, an attempt is made to write
the compiled version to :file:`spam.pyc`.  It is not an error if this attempt
fails; if for any reason the file is not written completely, the resulting
:file:`spam.pyc` file will be recognized as invalid and thus ignored later.  The
contents of the :file:`spam.pyc` file are platform independent, so a Python
module directory can be shared by machines of different architectures.

Some tips for experts:

* When the Python interpreter is invoked with the :option:`-O` flag, optimized
  code is generated and stored in :file:`.pyo` files.  The optimizer currently
  doesn't help much; it only removes :keyword:`assert` statements.  When
  :option:`-O` is used, *all* :term:`bytecode` is optimized; ``.pyc`` files are
  ignored and ``.py`` files are compiled to optimized bytecode.

* Passing two :option:`-O` flags to the Python interpreter (:option:`-OO`) will
  cause the bytecode compiler to perform optimizations that could in some rare
  cases result in malfunctioning programs.  Currently only ``__doc__`` strings are
  removed from the bytecode, resulting in more compact :file:`.pyo` files.  Since
  some programs may rely on having these available, you should only use this
  option if you know what you're doing.

* A program doesn't run any faster when it is read from a :file:`.pyc` or
  :file:`.pyo` file than when it is read from a :file:`.py` file; the only thing
  that's faster about :file:`.pyc` or :file:`.pyo` files is the speed with which
  they are loaded.

* When a script is run by giving its name on the command line, the bytecode for
  the script is never written to a :file:`.pyc` or :file:`.pyo` file.  Thus, the
  startup time of a script may be reduced by moving most of its code to a module
  and having a small bootstrap script that imports that module.  It is also
  possible to name a :file:`.pyc` or :file:`.pyo` file directly on the command
  line.

* It is possible to have a file called :file:`spam.pyc` (or :file:`spam.pyo`
  when :option:`-O` is used) without a file :file:`spam.py` for the same module.
  This can be used to distribute a library of Python code in a form that is
  moderately hard to reverse engineer.

  .. index:: module: compileall

* The module :mod:`compileall` can create :file:`.pyc` files (or :file:`.pyo`
  files when :option:`-O` is used) for all modules in a directory.


.. _tut-standardmodules:

Standard Modules
================

.. index:: module: sys

Python comes with a library of standard modules, described in a separate
document, the Python Library Reference ("Library Reference" hereafter).  Some
modules are built into the interpreter; these provide access to operations that
are not part of the core of the language but are nevertheless built in, either
for efficiency or to provide access to operating system primitives such as
system calls.  The set of such modules is a configuration option which also
depends on the underlying platform For example, the :mod:`winreg` module is only
provided on Windows systems. One particular module deserves some attention:
:mod:`sys`, which is built into every Python interpreter.  The variables
``sys.ps1`` and ``sys.ps2`` define the strings used as primary and secondary
prompts::

   >>> import sys
   >>> sys.ps1
   '>>> '
   >>> sys.ps2
   '... '
   >>> sys.ps1 = 'C> '
   C> print 'Yuck!'
   Yuck!
   C>


These two variables are only defined if the interpreter is in interactive mode.

The variable ``sys.path`` is a list of strings that determines the interpreter's
search path for modules. It is initialized to a default path taken from the
environment variable :envvar:`PYTHONPATH`, or from a built-in default if
:envvar:`PYTHONPATH` is not set.  You can modify it using standard list
operations::

   >>> import sys
   >>> sys.path.append('/ufs/guido/lib/python')


.. _tut-dir:

The :func:`dir` Function
========================

The built-in function :func:`dir` is used to find out which names a module
defines.  It returns a sorted list of strings::

   >>> import fibo, sys
   >>> dir(fibo)
   ['__name__', 'fib', 'fib2']
   >>> dir(sys)
   ['__displayhook__', '__doc__', '__excepthook__', '__name__', '__stderr__',
    '__stdin__', '__stdout__', '_getframe', 'api_version', 'argv', 
    'builtin_module_names', 'byteorder', 'callstats', 'copyright',
    'displayhook', 'exc_clear', 'exc_info', 'exc_type', 'excepthook',
    'exec_prefix', 'executable', 'exit', 'getdefaultencoding', 'getdlopenflags',
    'getrecursionlimit', 'getrefcount', 'hexversion', 'maxint', 'maxunicode',
    'meta_path', 'modules', 'path', 'path_hooks', 'path_importer_cache',
    'platform', 'prefix', 'ps1', 'ps2', 'setcheckinterval', 'setdlopenflags',
    'setprofile', 'setrecursionlimit', 'settrace', 'stderr', 'stdin', 'stdout',
    'version', 'version_info', 'warnoptions']

Without arguments, :func:`dir` lists the names you have defined currently::

   >>> a = [1, 2, 3, 4, 5]
   >>> import fibo
   >>> fib = fibo.fib
   >>> dir()
   ['__builtins__', '__doc__', '__file__', '__name__', 'a', 'fib', 'fibo', 'sys']

Note that it lists all types of names: variables, modules, functions, etc.

.. index:: module: __builtin__

:func:`dir` does not list the names of built-in functions and variables.  If you
want a list of those, they are defined in the standard module
:mod:`__builtin__`::

   >>> import __builtin__
   >>> dir(__builtin__)
   ['ArithmeticError', 'AssertionError', 'AttributeError', 'DeprecationWarning',
    'EOFError', 'Ellipsis', 'EnvironmentError', 'Exception', 'False',
    'FloatingPointError', 'FutureWarning', 'IOError', 'ImportError',
    'IndentationError', 'IndexError', 'KeyError', 'KeyboardInterrupt',
    'LookupError', 'MemoryError', 'NameError', 'None', 'NotImplemented',
    'NotImplementedError', 'OSError', 'OverflowError', 
    'PendingDeprecationWarning', 'ReferenceError', 'RuntimeError',
    'RuntimeWarning', 'StandardError', 'StopIteration', 'SyntaxError',
    'SyntaxWarning', 'SystemError', 'SystemExit', 'TabError', 'True',
    'TypeError', 'UnboundLocalError', 'UnicodeDecodeError',
    'UnicodeEncodeError', 'UnicodeError', 'UnicodeTranslateError',
    'UserWarning', 'ValueError', 'Warning', 'WindowsError',
    'ZeroDivisionError', '_', '__debug__', '__doc__', '__import__',
    '__name__', 'abs', 'apply', 'basestring', 'bool', 'buffer',
    'callable', 'chr', 'classmethod', 'cmp', 'coerce', 'compile',
    'complex', 'copyright', 'credits', 'delattr', 'dict', 'dir', 'divmod',
    'enumerate', 'eval', 'execfile', 'exit', 'file', 'filter', 'float',
    'frozenset', 'getattr', 'globals', 'hasattr', 'hash', 'help', 'hex',
    'id', 'input', 'int', 'intern', 'isinstance', 'issubclass', 'iter',
    'len', 'license', 'list', 'locals', 'long', 'map', 'max', 'min',
    'object', 'oct', 'open', 'ord', 'pow', 'property', 'quit', 'range',
    'raw_input', 'reduce', 'reload', 'repr', 'reversed', 'round', 'set',
    'setattr', 'slice', 'sorted', 'staticmethod', 'str', 'sum', 'super',
    'tuple', 'type', 'unichr', 'unicode', 'vars', 'xrange', 'zip']


.. _tut-packages:

Packages
========

Packages are a way of structuring Python's module namespace by using "dotted
module names".  For example, the module name :mod:`A.B` designates a submodule
named ``B`` in a package named ``A``.  Just like the use of modules saves the
authors of different modules from having to worry about each other's global
variable names, the use of dotted module names saves the authors of multi-module
packages like NumPy or the Python Imaging Library from having to worry about
each other's module names.

Suppose you want to design a collection of modules (a "package") for the uniform
handling of sound files and sound data.  There are many different sound file
formats (usually recognized by their extension, for example: :file:`.wav`,
:file:`.aiff`, :file:`.au`), so you may need to create and maintain a growing
collection of modules for the conversion between the various file formats.
There are also many different operations you might want to perform on sound data
(such as mixing, adding echo, applying an equalizer function, creating an
artificial stereo effect), so in addition you will be writing a never-ending
stream of modules to perform these operations.  Here's a possible structure for
your package (expressed in terms of a hierarchical filesystem)::

   sound/                          Top-level package
         __init__.py               Initialize the sound package
         formats/                  Subpackage for file format conversions
                 __init__.py
                 wavread.py
                 wavwrite.py
                 aiffread.py
                 aiffwrite.py
                 auread.py
                 auwrite.py
                 ...
         effects/                  Subpackage for sound effects
                 __init__.py
                 echo.py
                 surround.py
                 reverse.py
                 ...
         filters/                  Subpackage for filters
                 __init__.py
                 equalizer.py
                 vocoder.py
                 karaoke.py
                 ...

When importing the package, Python searches through the directories on
``sys.path`` looking for the package subdirectory.

The :file:`__init__.py` files are required to make Python treat the directories
as containing packages; this is done to prevent directories with a common name,
such as ``string``, from unintentionally hiding valid modules that occur later
on the module search path. In the simplest case, :file:`__init__.py` can just be
an empty file, but it can also execute initialization code for the package or
set the ``__all__`` variable, described later.

Users of the package can import individual modules from the package, for
example::

   import sound.effects.echo

This loads the submodule :mod:`sound.effects.echo`.  It must be referenced with
its full name. ::

   sound.effects.echo.echofilter(input, output, delay=0.7, atten=4)

An alternative way of importing the submodule is::

   from sound.effects import echo

This also loads the submodule :mod:`echo`, and makes it available without its
package prefix, so it can be used as follows::

   echo.echofilter(input, output, delay=0.7, atten=4)

Yet another variation is to import the desired function or variable directly::

   from sound.effects.echo import echofilter

Again, this loads the submodule :mod:`echo`, but this makes its function
:func:`echofilter` directly available::

   echofilter(input, output, delay=0.7, atten=4)

Note that when using ``from package import item``, the item can be either a
submodule (or subpackage) of the package, or some  other name defined in the
package, like a function, class or variable.  The ``import`` statement first
tests whether the item is defined in the package; if not, it assumes it is a
module and attempts to load it.  If it fails to find it, an :exc:`ImportError`
exception is raised.

Contrarily, when using syntax like ``import item.subitem.subsubitem``, each item
except for the last must be a package; the last item can be a module or a
package but can't be a class or function or variable defined in the previous
item.


.. _tut-pkg-import-star:

Importing \* From a Package
---------------------------

.. index:: single: __all__

Now what happens when the user writes ``from sound.effects import *``?  Ideally,
one would hope that this somehow goes out to the filesystem, finds which
submodules are present in the package, and imports them all.  Unfortunately,
this operation does not work very well on Windows platforms, where the
filesystem does not always have accurate information about the case of a
filename!  On these platforms, there is no guaranteed way to know whether a file
:file:`ECHO.PY` should be imported as a module :mod:`echo`, :mod:`Echo` or
:mod:`ECHO`.  (For example, Windows 95 has the annoying practice of showing all
file names with a capitalized first letter.)  The DOS 8+3 filename restriction
adds another interesting problem for long module names.

The only solution is for the package author to provide an explicit index of the
package.  The import statement uses the following convention: if a package's
:file:`__init__.py` code defines a list named ``__all__``, it is taken to be the
list of module names that should be imported when ``from package import *`` is
encountered.  It is up to the package author to keep this list up-to-date when a
new version of the package is released.  Package authors may also decide not to
support it, if they don't see a use for importing \* from their package.  For
example, the file :file:`sounds/effects/__init__.py` could contain the following
code::

   __all__ = ["echo", "surround", "reverse"]

This would mean that ``from sound.effects import *`` would import the three
named submodules of the :mod:`sound` package.

If ``__all__`` is not defined, the statement ``from sound.effects import *``
does *not* import all submodules from the package :mod:`sound.effects` into the
current namespace; it only ensures that the package :mod:`sound.effects` has
been imported (possibly running any initialization code in :file:`__init__.py`)
and then imports whatever names are defined in the package.  This includes any
names defined (and submodules explicitly loaded) by :file:`__init__.py`.  It
also includes any submodules of the package that were explicitly loaded by
previous import statements.  Consider this code::

   import sound.effects.echo
   import sound.effects.surround
   from sound.effects import *

In this example, the echo and surround modules are imported in the current
namespace because they are defined in the :mod:`sound.effects` package when the
``from...import`` statement is executed.  (This also works when ``__all__`` is
defined.)

Note that in general the practice of importing ``*`` from a module or package is
frowned upon, since it often causes poorly readable code. However, it is okay to
use it to save typing in interactive sessions, and certain modules are designed
to export only names that follow certain patterns.

Remember, there is nothing wrong with using ``from Package import
specific_submodule``!  In fact, this is the recommended notation unless the
importing module needs to use submodules with the same name from different
packages.


Intra-package References
------------------------

The submodules often need to refer to each other.  For example, the
:mod:`surround` module might use the :mod:`echo` module.  In fact, such
references are so common that the :keyword:`import` statement first looks in the
containing package before looking in the standard module search path. Thus, the
:mod:`surround` module can simply use ``import echo`` or ``from echo import
echofilter``.  If the imported module is not found in the current package (the
package of which the current module is a submodule), the :keyword:`import`
statement looks for a top-level module with the given name.

When packages are structured into subpackages (as with the :mod:`sound` package
in the example), you can use absolute imports to refer to submodules of siblings
packages.  For example, if the module :mod:`sound.filters.vocoder` needs to use
the :mod:`echo` module in the :mod:`sound.effects` package, it can use ``from
sound.effects import echo``.

Starting with Python 2.5, in addition to the implicit relative imports described
above, you can write explicit relative imports with the ``from module import
name`` form of import statement. These explicit relative imports use leading
dots to indicate the current and parent packages involved in the relative
import. From the :mod:`surround` module for example, you might use::

   from . import echo
   from .. import formats
   from ..filters import equalizer

Note that both explicit and implicit relative imports are based on the name of
the current module. Since the name of the main module is always ``"__main__"``,
modules intended for use as the main module of a Python application should
always use absolute imports.


Packages in Multiple Directories
--------------------------------

Packages support one more special attribute, :attr:`__path__`.  This is
initialized to be a list containing the name of the directory holding the
package's :file:`__init__.py` before the code in that file is executed.  This
variable can be modified; doing so affects future searches for modules and
subpackages contained in the package.

While this feature is not often needed, it can be used to extend the set of
modules found in a package.


.. rubric:: Footnotes

.. [#] In fact function definitions are also 'statements' that are 'executed'; the
   execution enters the function name in the module's global symbol table.

