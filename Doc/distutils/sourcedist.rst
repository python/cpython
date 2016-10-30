.. _source-dist:

******************************
Creating a Source Distribution
******************************

As shown in section :ref:`distutils-simple-example`, you use the :command:`sdist` command
to create a source distribution.  In the simplest case, ::

   python setup.py sdist

(assuming you haven't specified any :command:`sdist` options in the setup script
or config file), :command:`sdist` creates the archive of the default format for
the current platform.  The default format is a gzip'ed tar file
(:file:`.tar.gz`) on Unix, and ZIP file on Windows.

You can specify as many formats as you like using the :option:`!--formats`
option, for example::

   python setup.py sdist --formats=gztar,zip

to create a gzipped tarball and a zip file.  The available formats are:

+-----------+-------------------------+---------+
| Format    | Description             | Notes   |
+===========+=========================+=========+
| ``zip``   | zip file (:file:`.zip`) | (1),(3) |
+-----------+-------------------------+---------+
| ``gztar`` | gzip'ed tar file        | \(2)    |
|           | (:file:`.tar.gz`)       |         |
+-----------+-------------------------+---------+
| ``bztar`` | bzip2'ed tar file       |         |
|           | (:file:`.tar.bz2`)      |         |
+-----------+-------------------------+---------+
| ``xztar`` | xz'ed tar file          |         |
|           | (:file:`.tar.xz`)       |         |
+-----------+-------------------------+---------+
| ``ztar``  | compressed tar file     | \(4)    |
|           | (:file:`.tar.Z`)        |         |
+-----------+-------------------------+---------+
| ``tar``   | tar file (:file:`.tar`) |         |
+-----------+-------------------------+---------+

.. versionchanged:: 3.5
   Added support for the ``xztar`` format.

Notes:

(1)
   default on Windows

(2)
   default on Unix

(3)
   requires either external :program:`zip` utility or :mod:`zipfile` module (part
   of the standard Python library since Python 1.6)

(4)
   requires the :program:`compress` program. Notice that this format is now
   pending for deprecation and will be removed in the future versions of Python.

When using any ``tar`` format (``gztar``, ``bztar``, ``xztar``, ``ztar`` or
``tar``), under Unix you can specify the ``owner`` and ``group`` names
that will be set for each member of the archive.

For example, if you want all files of the archive to be owned by root::

    python setup.py sdist --owner=root --group=root


.. _manifest:

Specifying the files to distribute
==================================

If you don't supply an explicit list of files (or instructions on how to
generate one), the :command:`sdist` command puts a minimal default set into the
source distribution:

* all Python source files implied by the ``py_modules`` and
  ``packages`` options

* all C source files mentioned in the ``ext_modules`` or
  ``libraries`` options

  .. XXX getting C library sources currently broken---no
         :meth:`get_source_files` method in :file:`build_clib.py`!

* scripts identified by the ``scripts`` option
  See :ref:`distutils-installing-scripts`.

* anything that looks like a test script: :file:`test/test\*.py` (currently, the
  Distutils don't do anything with test scripts except include them in source
  distributions, but in the future there will be a standard for testing Python
  module distributions)

* :file:`README.txt` (or :file:`README`), :file:`setup.py` (or whatever  you
  called your setup script), and :file:`setup.cfg`

* all files that matches the ``package_data`` metadata.
  See :ref:`distutils-installing-package-data`.

* all files that matches the ``data_files`` metadata.
  See :ref:`distutils-additional-files`.

Sometimes this is enough, but usually you will want to specify additional files
to distribute.  The typical way to do this is to write a *manifest template*,
called :file:`MANIFEST.in` by default.  The manifest template is just a list of
instructions for how to generate your manifest file, :file:`MANIFEST`, which is
the exact list of files to include in your source distribution.  The
:command:`sdist` command processes this template and generates a manifest based
on its instructions and what it finds in the filesystem.

If you prefer to roll your own manifest file, the format is simple: one filename
per line, regular files (or symlinks to them) only.  If you do supply your own
:file:`MANIFEST`, you must specify everything: the default set of files
described above does not apply in this case.

.. versionchanged:: 3.1
   An existing generated :file:`MANIFEST` will be regenerated without
   :command:`sdist` comparing its modification time to the one of
   :file:`MANIFEST.in` or :file:`setup.py`.

.. versionchanged:: 3.1.3
   :file:`MANIFEST` files start with a comment indicating they are generated.
   Files without this comment are not overwritten or removed.

.. versionchanged:: 3.2.2
   :command:`sdist` will read a :file:`MANIFEST` file if no :file:`MANIFEST.in`
   exists, like it used to do.


The manifest template has one command per line, where each command specifies a
set of files to include or exclude from the source distribution.  For an
example, again we turn to the Distutils' own manifest template:

.. code-block:: none

   include *.txt
   recursive-include examples *.txt *.py
   prune examples/sample?/build

The meanings should be fairly clear: include all files in the distribution root
matching :file:`\*.txt`, all files anywhere under the :file:`examples` directory
matching :file:`\*.txt` or :file:`\*.py`, and exclude all directories matching
:file:`examples/sample?/build`.  All of this is done *after* the standard
include set, so you can exclude files from the standard set with explicit
instructions in the manifest template.  (Or, you can use the
:option:`!--no-defaults` option to disable the standard set entirely.)  There are
several other commands available in the manifest template mini-language; see
section :ref:`sdist-cmd`.

The order of commands in the manifest template matters: initially, we have the
list of default files as described above, and each command in the template adds
to or removes from that list of files.  Once we have fully processed the
manifest template, we remove files that should not be included in the source
distribution:

* all files in the Distutils "build" tree (default :file:`build/`)

* all files in directories named :file:`RCS`, :file:`CVS`, :file:`.svn`,
  :file:`.hg`, :file:`.git`, :file:`.bzr` or :file:`_darcs`

Now we have our complete list of files, which is written to the manifest for
future reference, and then used to build the source distribution archive(s).

You can disable the default set of included files with the
:option:`!--no-defaults` option, and you can disable the standard exclude set
with :option:`!--no-prune`.

Following the Distutils' own manifest template, let's trace how the
:command:`sdist` command builds the list of files to include in the Distutils
source distribution:

#. include all Python source files in the :file:`distutils` and
   :file:`distutils/command` subdirectories (because packages corresponding to
   those two directories were mentioned in the ``packages`` option in the
   setup script---see section :ref:`setup-script`)

#. include :file:`README.txt`, :file:`setup.py`, and :file:`setup.cfg` (standard
   files)

#. include :file:`test/test\*.py` (standard files)

#. include :file:`\*.txt` in the distribution root (this will find
   :file:`README.txt` a second time, but such redundancies are weeded out later)

#. include anything matching :file:`\*.txt` or :file:`\*.py` in the sub-tree
   under :file:`examples`,

#. exclude all files in the sub-trees starting at directories matching
   :file:`examples/sample?/build`\ ---this may exclude files included by the
   previous two steps, so it's important that the ``prune`` command in the manifest
   template comes after the ``recursive-include`` command

#. exclude the entire :file:`build` tree, and any :file:`RCS`, :file:`CVS`,
   :file:`.svn`, :file:`.hg`, :file:`.git`, :file:`.bzr` and :file:`_darcs`
   directories

Just like in the setup script, file and directory names in the manifest template
should always be slash-separated; the Distutils will take care of converting
them to the standard representation on your platform. That way, the manifest
template is portable across operating systems.


.. _manifest-options:

Manifest-related options
========================

The normal course of operations for the :command:`sdist` command is as follows:

* if the manifest file (:file:`MANIFEST` by default) exists and the first line
  does not have a comment indicating it is generated from :file:`MANIFEST.in`,
  then it is used as is, unaltered

* if the manifest file doesn't exist or has been previously automatically
  generated, read :file:`MANIFEST.in` and create the manifest

* if neither :file:`MANIFEST` nor :file:`MANIFEST.in` exist, create a manifest
  with just the default file set

* use the list of files now in :file:`MANIFEST` (either just generated or read
  in) to create the source distribution archive(s)

There are a couple of options that modify this behaviour.  First, use the
:option:`!--no-defaults` and :option:`!--no-prune` to disable the standard
"include" and "exclude" sets.

Second, you might just want to (re)generate the manifest, but not create a source
distribution::

   python setup.py sdist --manifest-only

:option:`!-o` is a shortcut for :option:`!--manifest-only`.
