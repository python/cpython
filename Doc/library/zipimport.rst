:mod:`zipimport` --- Import modules from Zip archives
=====================================================

.. module:: zipimport
   :synopsis: Support for importing Python modules from ZIP archives.

.. moduleauthor:: Just van Rossum <just@letterror.com>

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

Any files may be present in the ZIP archive, but only files :file:`.py` and
:file:`.pyc` are available for import.  ZIP import of dynamic modules
(:file:`.pyd`, :file:`.so`) is disallowed. Note that if an archive only contains
:file:`.py` files, Python will not attempt to modify the archive by adding the
corresponding :file:`.pyc` file, meaning that if a ZIP archive
doesn't contain :file:`.pyc` files, importing may be rather slow.

ZIP archives with an archive comment are currently not supported.

.. seealso::

   `PKZIP Application Note <https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT>`_
      Documentation on the ZIP file format by Phil Katz, the creator of the format and
      algorithms used.

   :pep:`273` - Import Modules from Zip Archives
      Written by James C. Ahlstrom, who also provided an implementation. Python 2.3
      follows the specification in PEP 273, but uses an implementation written by Just
      van Rossum that uses the import hooks described in PEP 302.

   :pep:`302` - New Import Hooks
      The PEP to add the import hooks that help this module work.


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

   .. method:: find_module(fullname[, path])

      Search for a module specified by *fullname*. *fullname* must be the fully
      qualified (dotted) module name. It returns the zipimporter instance itself
      if the module was found, or :const:`None` if it wasn't. The optional
      *path* argument is ignored---it's there for compatibility with the
      importer protocol.


   .. method:: get_code(fullname)

      Return the code object for the specified module. Raise
      :exc:`ZipImportError` if the module couldn't be found.


   .. method:: get_data(pathname)

      Return the data associated with *pathname*. Raise :exc:`OSError` if the
      file wasn't found.

      .. versionchanged:: 3.3
         :exc:`IOError` used to be raised instead of :exc:`OSError`.


   .. method:: get_filename(fullname)

      Return the value ``__file__`` would be set to if the specified module
      was imported. Raise :exc:`ZipImportError` if the module couldn't be
      found.

      .. versionadded:: 3.1


   .. method:: get_source(fullname)

      Return the source code for the specified module. Raise
      :exc:`ZipImportError` if the module couldn't be found, return
      :const:`None` if the archive does contain the module, but has no source
      for it.


   .. method:: is_package(fullname)

      Return ``True`` if the module specified by *fullname* is a package. Raise
      :exc:`ZipImportError` if the module couldn't be found.


   .. method:: load_module(fullname)

      Load the module specified by *fullname*. *fullname* must be the fully
      qualified (dotted) module name. It returns the imported module, or raises
      :exc:`ZipImportError` if it wasn't found.


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

   $ unzip -l example.zip
   Archive:  example.zip
     Length     Date   Time    Name
    --------    ----   ----    ----
        8467  11-26-02 22:30   jwzthreading.py
    --------                   -------
        8467                   1 file
   $ ./python
   Python 2.3 (#1, Aug 1 2003, 19:54:32)
   >>> import sys
   >>> sys.path.insert(0, 'example.zip')  # Add .zip file to front of path
   >>> import jwzthreading
   >>> jwzthreading.__file__
   'example.zip/jwzthreading.py'
