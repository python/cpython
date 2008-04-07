.. _library-index:

###############################
  The Python Standard Library
###############################

:Release: |version|
:Date: |today|

While the :ref:`reference-index` describes the exact syntax and
semantics of the Python language, this library reference manual
describes the standard library that is distributed with Python. It also
describes some of the optional components that are commonly included
in Python distributions.

Python's standard library is very extensive, offering a wide range of
facilities as indicated by the long table of contents listed below. The
library contains built-in modules (written in C) that provide access to
system functionality such as file I/O that would otherwise be
inaccessible to Python programmers, as well as modules written in Python
that provide standardized solutions for many problems that occur in
everyday programming. Some of these modules are explicitly designed to
encourage and enhance the portability of Python programs by abstracting
away platform-specifics into platform-neutral APIs.

The Python installers for the Windows and Mac platforms usually include
the entire standard library and often also include many additional
components. For Unix-like operating systems Python is normally provided
as a collection of packages, so it may be necessary to use the packaging
tools provided with the operating system to obtain some or all of the
optional components.

In addition to the standard library, there is a growing collection of
several thousand components (from individual programs and modules to
packages and entire application development frameworks), available from
the `Python Package Index <http://pypi.python.org/pypi>`_.


.. toctree::
   :maxdepth: 2

   intro.rst
   functions.rst
   constants.rst
   objects.rst
   stdtypes.rst
   exceptions.rst

   strings.rst
   datatypes.rst
   numeric.rst
   filesys.rst
   persistence.rst
   archiving.rst
   fileformats.rst
   crypto.rst
   allos.rst
   someos.rst
   ipc.rst
   netdata.rst
   markup.rst
   internet.rst
   mm.rst
   i18n.rst
   frameworks.rst
   tk.rst
   development.rst
   debug.rst
   python.rst
   custominterp.rst
   restricted.rst
   modules.rst
   language.rst
   compiler.rst
   misc.rst
   windows.rst
   unix.rst
   mac.rst
   macosa.rst
   sgi.rst
   sun.rst
   undoc.rst
