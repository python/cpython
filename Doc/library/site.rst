
:mod:`site` --- Site-specific configuration hook
================================================

.. module:: site
   :synopsis: A standard way to reference site-specific modules.


**This module is automatically imported during initialization.** The automatic
import can be suppressed using the interpreter's :option:`-S` option.

.. index:: triple: module; search; path

Importing this module will append site-specific paths to the module search path.

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

.. versionchanged:: 2.6
   A space or tab is now required after the import keyword.

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

Then the following directories are added to ``sys.path``, in this order::

   /usr/local/lib/python2.3/site-packages/bar
   /usr/local/lib/python2.3/site-packages/foo

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

