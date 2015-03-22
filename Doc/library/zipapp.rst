:mod:`zipapp` --- Manage executable python zip archives
=======================================================

.. module:: zipapp
   :synopsis: Manage executable python zip archives


.. index::
   single: Executable Zip Files

.. versionadded:: 3.5

**Source code:** :source:`Lib/zipapp.py`

--------------

This module provides tools to manage the creation of zip files containing
Python code, which can be  :ref:`executed directly by the Python interpreter
<using-on-interface-options>`.  The module provides both a
:ref:`zipapp-command-line-interface` and a :ref:`zipapp-python-api`.


Basic Example
-------------

The following example shows how the :ref:`command-line-interface`
can be used to create an executable archive from a directory containing
Python code.  When run, the archive will execute the ``main`` function from
the module ``myapp`` in the archive.

.. code-block:: sh

   $ python -m zipapp myapp -m "myapp:main"
   $ python myapp.pyz
   <output from myapp>


.. _zipapp-command-line-interface:

Command-Line Interface
----------------------

When called as a program from the command line, the following form is used:

.. code-block:: sh

   $ python -m zipapp source [options]

If *source* is a directory, this will create an archive from the contents of
*source*.  If *source* is a file, it should be an archive, and it will be
copied to the target archive (or the contents of its shebang line will be
displayed if the --info option is specified).

The following options are understood:

.. program:: zipapp

.. cmdoption:: -o <output>, --output=<output>

   Write the output to a file named *output*.  If this option is not specified,
   the output filename will be the same as the input *source*, with the
   extension ``.pyz`` added.  If an explicit filename is given, it is used as
   is (so a ``.pyz`` extension should be included if required).

   An output filename must be specified if the *source* is an archive (and in
   that case, *output* must not be the same as *source*).

.. cmdoption:: -p <interpreter>, --python=<interpreter>

   Add a ``#!`` line to the archive specifying *interpreter* as the command
   to run.  Also, on POSIX, make the archive executable.  The default is to
   write no ``#!`` line, and not make the file executable.

.. cmdoption:: -m <mainfn>, --main=<mainfn>

   Write a ``__main__.py`` file to the archive that executes *mainfn*.  The
   *mainfn* argument should have the form "pkg.mod:fn", where "pkg.mod" is a
   package/module in the archive, and "fn" is a callable in the given module.
   The ``__main__.py`` file will execute that callable.

   :option:`--main` cannot be specified when copying an archive.

.. cmdoption:: --info

   Display the interpreter embedded in the archive, for diagnostic purposes.  In
   this case, any other options are ignored and SOURCE must be an archive, not a
   directory.

.. cmdoption:: -h, --help

   Print a short usage message and exit.


.. _zipapp-python-api:

Python API
----------

The module defines two convenience functions:


.. function:: create_archive(source, target=None, interpreter=None, main=None)

   Create an application archive from *source*.  The source can be any
   of the following:

   * The name of a directory, or a :class:`pathlib.Path` object referring
     to a directory, in which case a new application archive will be
     created from the content of that directory.
   * The name of an existing application archive file, or a :class:`pathlib.Path`
     object referring to such a file, in which case the file is copied to
     the target (modifying it to reflect the value given for the *interpreter*
     argument).  The file name should include the ``.pyz`` extension, if required.
   * A file object open for reading in bytes mode.  The content of the
     file should be an application archive, and the file object is
     assumed to be positioned at the start of the archive.

   The *target* argument determines where the resulting archive will be
   written:

   * If it is the name of a file, or a :class:`pathlb.Path` object,
     the archive will be written to that file.
   * If it is an open file object, the archive will be written to that
     file object, which must be open for writing in bytes mode.
   * If the target is omitted (or None), the source must be a directory
     and the target will be a file with the same name as the source, with
     a ``.pyz`` extension added.

   The *interpreter* argument specifies the name of the Python
   interpreter with which the archive will be executed.  It is written as
   a "shebang" line at the start of the archive.  On POSIX, this will be
   interpreted by the OS, and on Windows it will be handled by the Python
   launcher.  Omitting the *interpreter* results in no shebang line being
   written.  If an interpreter is specified, and the target is a
   filename, the executable bit of the target file will be set.

   The *main* argument specifies the name of a callable which will be
   used as the main program for the archive.  It can only be specified if
   the source is a directory, and the source does not already contain a
   ``__main__.py`` file.  The *main* argument should take the form
   "pkg.module:callable" and the archive will be run by importing
   "pkg.module" and executing the given callable with no arguments.  It
   is an error to omit *main* if the source is a directory and does not
   contain a ``__main__.py`` file, as otherwise the resulting archive
   would not be executable.

   If a file object is specified for *source* or *target*, it is the
   caller's responsibility to close it after calling create_archive.

   When copying an existing archive, file objects supplied only need
   ``read`` and ``readline``, or ``write`` methods.  When creating an
   archive from a directory, if the target is a file object it will be
   passed to the ``zipfile.ZipFile`` class, and must supply the methods
   needed by that class.

.. function:: get_interpreter(archive)

   Return the interpreter specified in the ``#!`` line at the start of the
   archive.  If there is no ``#!`` line, return :const:`None`.
   The *archive* argument can be a filename or a file-like object open
   for reading in bytes mode.  It is assumed to be at the start of the archive.


.. _zipapp-examples:

Examples
--------

Pack up a directory into an archive, and run it.

.. code-block:: sh

   $ python -m zipapp myapp
   $ python myapp.pyz
   <output from myapp>

The same can be done using the :func:`create_archive` functon::

   >>> import zipapp
   >>> zipapp.create_archive('myapp.pyz', 'myapp')

To make the application directly executable on POSIX, specify an interpreter
to use.

.. code-block:: sh

   $ python -m zipapp myapp -p "/usr/bin/env python"
   $ ./myapp.pyz
   <output from myapp>

To replace the shebang line on an existing archive, create a modified archive
using the :func:`create_archive` function::

   >>> import zipapp
   >>> zipapp.create_archive('old_archive.pyz', 'new_archive.pyz', '/usr/bin/python3')

To update the file in place, do the replacement in memory using a :class:`BytesIO`
object, and then overwrite the source afterwards.  Note that there is a risk
when overwriting a file in place that an error will result in the loss of
the original file.  This code does not protect against such errors, but
production code should do so.  Also, this method will only work if the archive
fits in memory::

   >>> import zipapp
   >>> import io
   >>> temp = io.BytesIO()
   >>> zipapp.create_archive('myapp.pyz', temp, '/usr/bin/python2')
   >>> with open('myapp.pyz', 'wb') as f:
   >>>     f.write(temp.getvalue())

Note that if you specify an interpreter and then distribute your application
archive, you need to ensure that the interpreter used is portable.  The Python
launcher for Windows supports most common forms of POSIX ``#!`` line, but there
are other issues to consider:

* If you use "/usr/bin/env python" (or other forms of the "python" command,
  such as "/usr/bin/python"), you need to consider that your users may have
  either Python 2 or Python 3 as their default, and write your code to work
  under both versions.
* If you use an explicit version, for example "/usr/bin/env python3" your
  application will not work for users who do not have that version.  (This
  may be what you want if you have not made your code Python 2 compatible).
* There is no way to say "python X.Y or later", so be careful of using an
  exact version like "/usr/bin/env python3.4" as you will need to change your
  shebang line for users of Python 3.5, for example.

The Python Zip Application Archive Format
-----------------------------------------

Python has been able to execute zip files which contain a ``__main__.py`` file
since version 2.6.  In order to be executed by Python, an application archive
simply has to be a standard zip file containing a ``__main__.py`` file which
will be run as the entry point for the application.  As usual for any Python
script, the parent of the script (in this case the zip file) will be placed on
:data:`sys.path` and thus further modules can be imported from the zip file.

The zip file format allows arbitrary data to be prepended to a zip file.  The
zip application format uses this ability to prepend a standard POSIX "shebang"
line to the file (``#!/path/to/interpreter``).

Formally, the Python zip application format is therefore:

1. An optional shebang line, containing the characters ``b'#!'`` followed by an
   interpreter name, and then a newline (``b'\n'``) character.  The interpreter
   name can be anything acceptable to the OS "shebang" processing, or the Python
   launcher on Windows.  The interpreter should be encoded in UTF-8 on Windows,
   and in :func:`sys.getfilesystemencoding()` on POSIX.
2. Standard zipfile data, as generated by the :mod:`zipfile` module.  The
   zipfile content *must* include a file called ``__main__.py`` (which must be
   in the "root" of the zipfile - i.e., it cannot be in a subdirectory).  The
   zipfile data can be compressed or uncompressed.

If an application archive has a shebang line, it may have the executable bit set
on POSIX systems, to allow it to be executed directly.

There is no requirement that the tools in this module are used to create
application archives - the module is a convenience, but archives in the above
format created by any means are acceptable to Python.

