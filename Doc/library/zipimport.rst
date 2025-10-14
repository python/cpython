:mod:`!zipimport` --- Import modules from Zip archives
======================================================

.. module:: zipimport
   :synopsis: Support for importing Python modules from ZIP archives.

.. moduleauthor:: Just van Rossum <just@letterror.com>

**Source code:** :source:`Lib/zipimport.py`

--------------

This module adds the ability to import Python modules (:file:`\*.py`,
:file:`\*.pyc`) and packages from ZIP-format archives. It is usually not
needed to use the :mod:`zipimport` module explicitly; it is automatically used
by the built-in :keyword:`import` mechanism for :data:`sys.path` items that are paths
to ZIP archives.

Typically, :data:`sys.path` is a list of directory names as strings.  This module
also allows an item of :data:`sys.path` to be a string naming a ZIP file archive.
The ZIP archive can contain a subdirectory structure to support package imports,
and a path within the archive can be specified to only import from a
subdirectory.  For example, the path :file:`example.zip/lib/` would only
import from the :file:`lib/` subdirectory within the archive.

Any files may be present in the ZIP archive, but importers are only invoked for
:file:`.py` and :file:`.pyc` files.  ZIP import of dynamic modules
(:file:`.pyd`, :file:`.so`) is disallowed. Note that if an archive only contains
:file:`.py` files, Python will not attempt to modify the archive by adding the
corresponding :file:`.pyc` file, meaning that if a ZIP archive
doesn't contain :file:`.pyc` files, importing may be rather slow.

.. versionchanged:: 3.15
   Zstandard (*zstd*) compressed zip file entries are supported.

.. versionchanged:: 3.13
   ZIP64 is supported

.. versionchanged:: 3.8
   Previously, ZIP archives with an archive comment were not supported.

.. seealso::

   `PKZIP Application Note <https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT>`_
      Documentation on the ZIP file format by Phil Katz, the creator of the format and
      algorithms used.

   :pep:`273` - Import Modules from Zip Archives
      Written by James C. Ahlstrom, who also provided an implementation. Python 2.3
      follows the specification in :pep:`273`, but uses an implementation written by Just
      van Rossum that uses the import hooks described in :pep:`302`.

   :mod:`importlib` - The implementation of the import machinery
      Package providing the relevant protocols for all importers to
      implement.


This module defines an exception:

.. exception:: ZipImportError

   Exception raised by zipimporter objects. It's a subclass of :exc:`ImportError`,
   so it can be caught as :exc:`ImportError`, too.


.. _zipimporter-objects:

zipimporter Objects
-------------------

:class:`zipimporter` is the class for importing ZIP files.

.. class:: zipimporter(archivepath)

   Create a new zipimporter instance. *archivepath* must be a path to a ZIP
   file, or to a specific path within a ZIP file.  For example, an *archivepath*
   of :file:`foo/bar.zip/lib` will look for modules in the :file:`lib` directory
   inside the ZIP file :file:`foo/bar.zip` (provided that it exists).

   :exc:`ZipImportError` is raised if *archivepath* doesn't point to a valid ZIP
   archive.

   .. versionchanged:: 3.12

      Methods ``find_loader()`` and ``find_module()``, deprecated in 3.10 are
      now removed.  Use :meth:`find_spec` instead.

   .. method:: create_module(spec)

      Implementation of :meth:`importlib.abc.Loader.create_module` that returns
      :const:`None` to explicitly request the default semantics.

      .. versionadded:: 3.10


   .. method:: exec_module(module)

      Implementation of :meth:`importlib.abc.Loader.exec_module`.

      .. versionadded:: 3.10


   .. method:: find_spec(fullname, target=None)

      An implementation of :meth:`importlib.abc.PathEntryFinder.find_spec`.

      .. versionadded:: 3.10


   .. method:: get_code(fullname)

      Return the code object for the specified module. Raise
      :exc:`ZipImportError` if the module couldn't be imported.


   .. method:: get_data(pathname)

      Return the data associated with *pathname*. Raise :exc:`OSError` if the
      file wasn't found.

      .. versionchanged:: 3.3
         :exc:`IOError` used to be raised, it is now an alias of :exc:`OSError`.


   .. method:: get_filename(fullname)

      Return the value ``__file__`` would be set to if the specified module
      was imported. Raise :exc:`ZipImportError` if the module couldn't be
      imported.

      .. versionadded:: 3.1


   .. method:: get_source(fullname)

      Return the source code for the specified module. Raise
      :exc:`ZipImportError` if the module couldn't be found, return
      :const:`None` if the archive does contain the module, but has no source
      for it.


   .. method:: is_package(fullname)

      Return ``True`` if the module specified by *fullname* is a package. Raise
      :exc:`ZipImportError` if the module couldn't be found.


   .. method:: invalidate_caches()

      Clear out the internal cache of information about files found within
      the ZIP archive.

      .. versionadded:: 3.10


   .. attribute:: archive

      The file name of the importer's associated ZIP file, without a possible
      subpath.


   .. attribute:: prefix

      The subpath within the ZIP file where modules are searched.  This is the
      empty string for zipimporter objects which point to the root of the ZIP
      file.

   The :attr:`archive` and :attr:`prefix` attributes, when combined with a
   slash, equal the original *archivepath* argument given to the
   :class:`zipimporter` constructor.


.. _zipimport-examples:

Examples
--------

Here is an example that imports a module from a ZIP archive - note that the
:mod:`zipimport` module is not explicitly used.

.. code-block:: shell-session

   $ unzip -l example_archive.zip
   Archive:  example_archive.zip
     Length     Date   Time    Name
    --------    ----   ----    ----
        8467  01-01-00 12:30   example.py
    --------                   -------
        8467                   1 file

.. code-block:: pycon

   >>> import sys
   >>> # Add the archive to the front of the module search path
   >>> sys.path.insert(0, 'example_archive.zip')
   >>> import example
   >>> example.__file__
   'example_archive.zip/example.py'

