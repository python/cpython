:mod:`site` --- Site-specific configuration hook
================================================

.. module:: site
   :synopsis: A standard way to reference site-specific modules.

**Source code:** :source:`Lib/site.py`

--------------

**This module is automatically imported during initialization.** The automatic
import can be suppressed using the interpreter's :option:`-S` option.

.. index:: triple: module; search; path

Importing this module will append site-specific paths to the module search
path, unless :option:`-S` was used.  In that case, this module can be safely
imported with no automatic modifications to the module search path.  To
explicitly trigger the usual site-specific additions, call the
:func:`site.main` function.

.. versionchanged:: 3.3
   Importing the module used to trigger paths manipulation even when using
   :option:`-S`.

.. index::
   pair: site-python; directory
   pair: site-packages; directory

It starts by constructing up to four directories from a head and a tail part.
For the head part, it uses ``sys.prefix`` and ``sys.exec_prefix``; empty heads
are skipped.  For the tail part, it uses the empty string and then
:file:`lib/site-packages` (on Windows) or
:file:`lib/python|version|/site-packages` and then :file:`lib/site-python` (on
Unix and Macintosh).  For each of the distinct head-tail combinations, it sees
if it refers to an existing directory, and if so, adds it to ``sys.path`` and
also inspects the newly added path for configuration files.

A path configuration file is a file whose name has the form :file:`package.pth`
and exists in one of the four directories mentioned above; its contents are
additional items (one per line) to be added to ``sys.path``.  Non-existing items
are never added to ``sys.path``, but no check is made that the item refers to a
directory (rather than a file).  No item is added to ``sys.path`` more than
once.  Blank lines and lines beginning with ``#`` are skipped.  Lines starting
with ``import`` (followed by space or tab) are executed.

.. index::
   single: package
   triple: path; configuration; file

For example, suppose ``sys.prefix`` and ``sys.exec_prefix`` are set to
:file:`/usr/local`.  The Python X.Y library is then installed in
:file:`/usr/local/lib/python{X.Y}` (where only the first three characters of
``sys.version`` are used to form the installation path name).  Suppose this has
a subdirectory :file:`/usr/local/lib/python{X.Y}/site-packages` with three
subsubdirectories, :file:`foo`, :file:`bar` and :file:`spam`, and two path
configuration files, :file:`foo.pth` and :file:`bar.pth`.  Assume
:file:`foo.pth` contains the following::

   # foo package configuration

   foo
   bar
   bletch

and :file:`bar.pth` contains::

   # bar package configuration

   bar

Then the following version-specific directories are added to
``sys.path``, in this order::

   /usr/local/lib/pythonX.Y/site-packages/bar
   /usr/local/lib/pythonX.Y/site-packages/foo

Note that :file:`bletch` is omitted because it doesn't exist; the :file:`bar`
directory precedes the :file:`foo` directory because :file:`bar.pth` comes
alphabetically before :file:`foo.pth`; and :file:`spam` is omitted because it is
not mentioned in either path configuration file.

.. index:: module: sitecustomize

After these path manipulations, an attempt is made to import a module named
:mod:`sitecustomize`, which can perform arbitrary site-specific customizations.
If this import fails with an :exc:`ImportError` exception, it is silently
ignored.

.. index:: module: sitecustomize

Note that for some non-Unix systems, ``sys.prefix`` and ``sys.exec_prefix`` are
empty, and the path manipulations are skipped; however the import of
:mod:`sitecustomize` is still attempted.


.. data:: PREFIXES

   A list of prefixes for site package directories


.. data:: ENABLE_USER_SITE

   Flag showing the status of the user site directory. True means the
   user site directory is enabled and added to sys.path. When the flag
   is None the user site directory is disabled for security reasons.


.. data:: USER_SITE

   Path to the user site directory for the current Python version or None


.. data:: USER_BASE

   Path to the base directory for user site directories


.. envvar:: PYTHONNOUSERSITE


.. envvar:: PYTHONUSERBASE


.. function:: main()

   Adds all the standard site-specific directories to the module search
   path.  This function is called automatically when this module is imported,
   unless the :program:`python` interpreter was started with the :option:`-S`
   flag.

   .. versionchanged:: 3.3
      This function used to be called unconditionnally.


.. function:: addsitedir(sitedir, known_paths=None)

   Adds a directory to sys.path and processes its pth files.

.. function:: getsitepackages()

   Returns a list containing all global site-packages directories
   (and possibly site-python).

   .. versionadded:: 3.2

.. function:: getuserbase()

   Returns the "user base" directory path.

   The "user base" directory can be used to store data. If the global
   variable ``USER_BASE`` is not initialized yet, this function will also set
   it.

   .. versionadded:: 3.2

.. function:: getusersitepackages()

   Returns the user-specific site-packages directory path.

   If the global variable ``USER_SITE`` is not initialized yet, this
   function will also set it.

   .. versionadded:: 3.2

.. XXX Update documentation
.. XXX document python -m site --user-base --user-site

