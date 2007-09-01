
:mod:`modulefinder` --- Find modules used by a script
=====================================================

.. sectionauthor:: A.M. Kuchling <amk@amk.ca>


.. module:: modulefinder
   :synopsis: Find modules used by a script.


This module provides a :class:`ModuleFinder` class that can be used to determine
the set of modules imported by a script. ``modulefinder.py`` can also be run as
a script, giving the filename of a Python script as its argument, after which a
report of the imported modules will be printed.


.. function:: AddPackagePath(pkg_name, path)

   Record that the package named *pkg_name* can be found in the specified *path*.


.. function:: ReplacePackage(oldname, newname)

   Allows specifying that the module named *oldname* is in fact the package named
   *newname*.  The most common usage would be  to handle how the :mod:`_xmlplus`
   package replaces the :mod:`xml` package.


.. class:: ModuleFinder([path=None, debug=0, excludes=[], replace_paths=[]])

   This class provides :meth:`run_script` and :meth:`report` methods to determine
   the set of modules imported by a script. *path* can be a list of directories to
   search for modules; if not specified, ``sys.path`` is used.  *debug* sets the
   debugging level; higher values make the class print  debugging messages about
   what it's doing. *excludes* is a list of module names to exclude from the
   analysis. *replace_paths* is a list of ``(oldpath, newpath)`` tuples that will
   be replaced in module paths.


.. method:: ModuleFinder.report()

   Print a report to standard output that lists the modules imported by the script
   and their paths, as well as modules that are missing or seem to be missing.


.. method:: ModuleFinder.run_script(pathname)

   Analyze the contents of the *pathname* file, which must contain  Python code.

