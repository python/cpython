:mod:`packaging.database` --- Database of installed distributions
=================================================================

.. module:: packaging.database
   :synopsis: Functions to query and manipulate installed distributions.


This module provides an implementation of :PEP:`376`.  It was originally
intended to land in :mod:`pkgutil`, but with the inclusion of Packaging in the
standard library, it was thought best to include it in a submodule of
:mod:`packaging`, leaving :mod:`pkgutil` to deal with imports.

Installed Python distributions are represented by instances of
:class:`Distribution`, or :class:`EggInfoDistribution` for legacy egg formats.
Most functions also provide an extra argument ``use_egg_info`` to take legacy
distributions into account.


Classes representing installed distributions
--------------------------------------------

.. class:: Distribution(path)

   Class representing an installed distribution.  It is different from
   :class:`packaging.dist.Distribution` which holds the list of files, the
   metadata and options during the run of a Packaging command.

   Instantiate with the *path* to a ``.dist-info`` directory.  Instances can be
   compared and sorted.  Other available methods are:

   .. XXX describe how comparison works

   .. method:: get_distinfo_file(path, binary=False)

      Return a read-only file object for a file located at
      :file:`{project}-{version}.dist-info/{path}`.  *path* should be a
      ``'/'``-separated path relative to the ``.dist-info`` directory or an
      absolute path; if it is an absolute path and doesn't start with the path
      to the :file:`.dist-info` directory, a :class:`PackagingError` is raised.

      If *binary* is ``True``, the file is opened in binary mode.

   .. method:: get_resource_path(relative_path)

      .. TODO

   .. method:: list_distinfo_files(local=False)

      Return an iterator over all files located in the :file:`.dist-info`
      directory.  If *local* is ``True``, each returned path is transformed into
      a local absolute path, otherwise the raw value found in the :file:`RECORD`
      file is returned.

   .. method::  list_installed_files(local=False)

      Iterate over the files installed with the distribution and registered in
      the :file:`RECORD` file and yield a tuple ``(path, md5, size)`` for each
      line.  If *local* is ``True``, the returned path is transformed into a
      local absolute path, otherwise the raw value is returned.

      A local absolute path is an absolute path in which occurrences of ``'/'``
      have been replaced by :data:`os.sep`.

   .. method:: uses(path)

      Check whether *path* was installed by this distribution (i.e. if the path
      is present in the :file:`RECORD` file).  *path* can be a local absolute
      path or a relative ``'/'``-separated path.  Returns a boolean.

   Available attributes:

   .. attribute:: metadata

      Instance of :class:`packaging.metadata.Metadata` filled with the contents
      of the :file:`{project}-{version}.dist-info/METADATA` file.

   .. attribute:: name

      Shortcut for ``metadata['Name']``.

   .. attribute:: version

      Shortcut for ``metadata['Version']``.

   .. attribute:: requested

      Boolean indicating whether this distribution was requested by the user of
      automatically installed as a dependency.


.. class:: EggInfoDistribution(path)

   Class representing a legacy distribution.  It is compatible with distutils'
   and setuptools' :file:`.egg-info` and :file:`.egg` files and directories.

   .. FIXME should be named EggDistribution

   Instantiate with the *path* to an egg file or directory.  Instances can be
   compared and sorted.  Other available methods are:

   .. method:: list_installed_files(local=False)

   .. method:: uses(path)

   Available attributes:

   .. attribute:: metadata

      Instance of :class:`packaging.metadata.Metadata` filled with the contents
      of the :file:`{project-version}.egg-info/PKG-INFO` or
      :file:`{project-version}.egg` file.

   .. attribute:: name

      Shortcut for ``metadata['Name']``.

   .. attribute:: version

      Shortcut for ``metadata['Version']``.


Functions to work with the database
-----------------------------------

.. function:: get_distribution(name, use_egg_info=False, paths=None)

   Return an instance of :class:`Distribution` or :class:`EggInfoDistribution`
   for the first installed distribution matching *name*.  Egg distributions are
   considered only if *use_egg_info* is true; if both a dist-info and an egg
   file are found, the dist-info prevails.  The directories to be searched are
   given in *paths*, which defaults to :data:`sys.path`.  Return ``None`` if no
   matching distribution is found.

   .. FIXME param should be named use_egg


.. function:: get_distributions(use_egg_info=False, paths=None)

   Return an iterator of :class:`Distribution` instances for all installed
   distributions found in *paths* (defaults to :data:`sys.path`).  If
   *use_egg_info* is true, also return instances of :class:`EggInfoDistribution`
   for legacy distributions found.


.. function:: get_file_users(path)

   Return an iterator over all distributions using *path*, a local absolute path
   or a relative ``'/'``-separated path.

   .. XXX does this work with prefixes or full file path only?


.. function:: obsoletes_distribution(name, version=None, use_egg_info=False)

   Return an iterator over all distributions that declare they obsolete *name*.
   *version* is an optional argument to match only specific releases (see
   :mod:`packaging.version`).  If *use_egg_info* is true, legacy egg
   distributions will be considered as well.


.. function:: provides_distribution(name, version=None, use_egg_info=False)

   Return an iterator over all distributions that declare they provide *name*.
   *version* is an optional argument to match only specific releases (see
   :mod:`packaging.version`).  If *use_egg_info* is true, legacy egg
   distributions will be considered as well.


Utility functions
-----------------

.. function:: distinfo_dirname(name, version)

   Escape *name* and *version* into a filename-safe form and return the
   directory name built from them, for example
   :file:`{safename}-{safeversion}.dist-info.`  In *name*, runs of
   non-alphanumeric characters are replaced with one ``'_'``; in *version*,
   spaces become dots, and runs of other non-alphanumeric characters (except
   dots) a replaced by one ``'-'``.

   .. XXX wth spaces in version numbers?

For performance purposes, the list of distributions is being internally
cached.   Caching is enabled by default, but you can control it with these
functions:

.. function:: clear_cache()

   Clear the cache.

.. function:: disable_cache()

   Disable the cache, without clearing it.

.. function:: enable_cache()

   Enable the internal cache, without clearing it.


Examples
--------

Print all information about a distribution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Given a path to a ``.dist-info`` distribution, we shall print out all
information that can be obtained using functions provided in this module::

   import sys
   import packaging.database

   path = input()
   # first create the Distribution instance
   try:
       dist = packaging.database.Distribution(path)
   except IOError:
       sys.exit('No such distribution')

   print('Information about %r' % dist.name)
   print()

   print('Files')
   print('=====')
   for path, md5, size in dist.list_installed_files():
       print('* Path: %s' % path)
       print('  Hash %s, Size: %s bytes' % (md5, size))
   print()

   print('Metadata')
   print('========')
   for key, value in dist.metadata.items():
       print('%20s: %s' % (key, value))
   print()

   print('Extra')
   print('=====')
   if dist.requested:
       print('* It was installed by user request')
   else:
       print('* It was installed as a dependency')

If we save the script above as ``print_info.py``, we can use it to extract
information from a :file:`.dist-info` directory.  By typing in the console:

.. code-block:: sh

   $ echo /tmp/choxie/choxie-2.0.0.9.dist-info | python3 print_info.py

we get the following output:

.. code-block:: none

   Information about 'choxie'

   Files
   =====
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9/truffles.py
     Hash 5e052db6a478d06bad9ae033e6bc08af, Size: 111 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9/choxie/chocolate.py
     Hash ac56bf496d8d1d26f866235b95f31030, Size: 214 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9/choxie/__init__.py
     Hash 416aab08dfa846f473129e89a7625bbc, Size: 25 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9.dist-info/INSTALLER
     Hash d41d8cd98f00b204e9800998ecf8427e, Size: 0 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9.dist-info/METADATA
     Hash 696a209967fef3c8b8f5a7bb10386385, Size: 225 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9.dist-info/REQUESTED
     Hash d41d8cd98f00b204e9800998ecf8427e, Size: 0 bytes
   * Path: ../tmp/distutils2/tests/fake_dists/choxie-2.0.0.9.dist-info/RECORD
     Hash None, Size: None bytes

   Metadata
   ========
       Metadata-Version: 1.2
                   Name: choxie
                Version: 2.0.0.9
               Platform: []
     Supported-Platform: UNKNOWN
                Summary: Chocolate with a kick!
            Description: UNKNOWN
               Keywords: []
              Home-page: UNKNOWN
                 Author: UNKNOWN
           Author-email: UNKNOWN
             Maintainer: UNKNOWN
       Maintainer-email: UNKNOWN
                License: UNKNOWN
             Classifier: []
           Download-URL: UNKNOWN
         Obsoletes-Dist: ['truffles (<=0.8,>=0.5)', 'truffles (<=0.9,>=0.6)']
            Project-URL: []
          Provides-Dist: ['truffles (1.0)']
          Requires-Dist: ['towel-stuff (0.1)']
        Requires-Python: UNKNOWN
      Requires-External: []

  Extra
  =====
  * It was installed as a dependency


Find out obsoleted distributions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now, we take tackle a different problem, we are interested in finding out
which distributions have been obsoleted. This can be easily done as follows::

  import packaging.database

  # iterate over all distributions in the system
  for dist in packaging.database.get_distributions():
      name, version = dist.name, dist.version
      # find out which distributions obsolete this name/version combination
      replacements = packaging.database.obsoletes_distribution(name, version)
      if replacements:
          print('%r %s is obsoleted by' % (name, version),
                ', '.join(repr(r.name) for r in replacements))

This is how the output might look like:

.. code-block:: none

  'strawberry' 0.6 is obsoleted by 'choxie'
  'grammar' 1.0a4 is obsoleted by 'towel-stuff'
