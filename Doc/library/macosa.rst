
.. _mac-scripting:

*********************
MacPython OSA Modules
*********************

This chapter describes the current implementation of the Open Scripting
Architecure (OSA, also commonly referred to as AppleScript) for Python, allowing
you to control scriptable applications from your Python program, and with a
fairly pythonic interface. Development on this set of modules has stopped, and a
replacement is expected for Python 2.5.

For a description of the various components of AppleScript and OSA, and to get
an understanding of the architecture and terminology, you should read Apple's
documentation. The "Applescript Language Guide" explains the conceptual model
and the terminology, and documents the standard suite. The "Open Scripting
Architecture" document explains how to use OSA from an application programmers
point of view. In the Apple Help Viewer these books are located in the Developer
Documentation, Core Technologies section.

As an example of scripting an application, the following piece of AppleScript
will get the name of the frontmost :program:`Finder` window and print it::

   tell application "Finder"
       get name of window 1
   end tell

In Python, the following code fragment will do the same::

   import Finder

   f = Finder.Finder()
   print f.get(f.window(1).name)

As distributed the Python library includes packages that implement the standard
suites, plus packages that interface to a small number of common applications.

To send AppleEvents to an application you must first create the Python package
interfacing to the terminology of the application (what :program:`Script Editor`
calls the "Dictionary"). This can be done from within the :program:`PythonIDE`
or by running the :file:`gensuitemodule.py` module as a standalone program from
the command line.

The generated output is a package with a number of modules, one for every suite
used in the program plus an :mod:`__init__` module to glue it all together. The
Python inheritance graph follows the AppleScript inheritance graph, so if a
program's dictionary specifies that it includes support for the Standard Suite,
but extends one or two verbs with extra arguments then the output suite will
contain a module :mod:`Standard_Suite` that imports and re-exports everything
from :mod:`StdSuites.Standard_Suite` but overrides the methods that have extra
functionality. The output of :mod:`gensuitemodule` is pretty readable, and
contains the documentation that was in the original AppleScript dictionary in
Python docstrings, so reading it is a good source of documentation.

The output package implements a main class with the same name as the package
which contains all the AppleScript verbs as methods, with the direct object as
the first argument and all optional parameters as keyword arguments. AppleScript
classes are also implemented as Python classes, as are comparisons and all the
other thingies.

The main Python class implementing the verbs also allows access to the
properties and elements declared in the AppleScript class "application". In the
current release that is as far as the object orientation goes, so in the example
above we need to use ``f.get(f.window(1).name)`` instead of the more Pythonic
``f.window(1).name.get()``.

If an AppleScript identifier is not a Python identifier the name is mangled
according to a small number of rules:

* spaces are replaced with underscores

* other non-alphanumeric characters are replaced with ``_xx_`` where ``xx`` is
  the hexadecimal character value

* any Python reserved word gets an underscore appended

Python also has support for creating scriptable applications in Python, but The
following modules are relevant to MacPython AppleScript support:

.. toctree::

   gensuitemodule.rst
   aetools.rst
   aepack.rst
   aetypes.rst
   miniaeframe.rst


In addition, support modules have been pre-generated for :mod:`Finder`,
:mod:`Terminal`, :mod:`Explorer`, :mod:`Netscape`, :mod:`CodeWarrior`,
:mod:`SystemEvents` and :mod:`StdSuites`.
