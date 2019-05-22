:mod:`zipapp` --- Manage executable Python zip archives
=======================================================

.. module:: zipapp
   :synopsis: Manage executable Python zip archives

.. versionadded:: 3.5

**Source code:** :source:`Lib/zipapp.py`

.. index::
   single: Executable Zip Files

--------------

This module provides tools to manage the creation of zip files containing
Python code, which can be  :ref:`executed directly by the Python interpreter
<using-on-interface-options>`.  The module provides both a
:ref:`zipapp-command-line-interface` and a :ref:`zipapp-python-api`.


Basic Example
-------------

The following example shows how the :ref:`zipapp-command-line-interface`
can be used to create an executable archive from a directory containing
Python code.  When run, the archive will execute the ``main`` function from
the module ``myapp`` in the archive.

.. code-block:: shell-session

   $ python -m zipapp myapp -m "myapp:main"
   $ python myapp.pyz
   <output from myapp>


.. _zipapp-command-line-interface:

Command-Line Interface
----------------------

When called as a program from the command line, the following form is used:

.. code-block:: shell-session

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

.. cmdoption:: -c, --compress

   Compress files with the deflate method, reducing the size of the output
   file. By default, files are stored uncompressed in the archive.

   :option:`--compress` has no effect when copying an archive.

   .. versionadded:: 3.7

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


.. function:: create_archive(source, target=None, interpreter=None, main=None, filter=None, compressed=False)

   Create an application archive from *source*.  The source can be any
   of the following:

   * The name of a directory, or a :term:`path-like object` referring
     to a directory, in which case a new application archive will be
     created from the content of that directory.
   * The name of an existing application archive file, or a :term:`path-like object`
     referring to such a file, in which case the file is copied to
     the target (modifying it to reflect the value given for the *interpreter*
     argument).  The file name should include the ``.pyz`` extension, if required.
   * A file object open for reading in bytes mode.  The content of the
     file should be an application archive, and the file object is
     assumed to be positioned at the start of the archive.

   The *target* argument determines where the resulting archive will be
   written:

   * If it is the name of a file, or a :term:`path-like object`,
     the archive will be written to that file.
   * If it is an open file object, the archive will be written to that
     file object, which must be open for writing in bytes mode.
   * If the target is omitted (or ``None``), the source must be a directory
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

   The optional *filter* argument specifies a callback function that
   is passed a Path object representing the path to the file being added
   (relative to the source directory).  It should return ``True`` if the
   file is to be added.

   The optional *compressed* argument determines whether files are
   compressed.  If set to ``True``, files in the archive are compressed
   with the deflate method; otherwise, files are stored uncompressed.
   This argument has no effect when copying an existing archive.

   If a file object is specified for *source* or *target*, it is the
   caller's responsibility to close it after calling create_archive.

   When copying an existing archive, file objects supplied only need
   ``read`` and ``readline``, or ``write`` methods.  When creating an
   archive from a directory, if the target is a file object it will be
   passed to the ``zipfile.ZipFile`` class, and must supply the methods
   needed by that class.

   .. versionadded:: 3.7
      Added the *filter* and *compressed* arguments.

.. function:: get_interpreter(archive)

   Return the interpreter specified in the ``#!`` line at the start of the
   archive.  If there is no ``#!`` line, return :const:`None`.
   The *archive* argument can be a filename or a file-like object open
   for reading in bytes mode.  It is assumed to be at the start of the archive.


.. _zipapp-examples:

Examples
--------

Pack up a directory into an archive, and run it.

.. code-block:: shell-session

   $ python -m zipapp myapp
   $ python myapp.pyz
   <output from myapp>

The same can be done using the :func:`create_archive` function::

   >>> import zipapp
   >>> zipapp.create_archive('myapp.pyz', 'myapp')

To make the application directly executable on POSIX, specify an interpreter
to use.

.. code-block:: shell-session

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


.. _zipapp-specifying-the-interpreter:

Specifying the Interpreter
--------------------------

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

Typically, you should use an "/usr/bin/env python2" or "/usr/bin/env python3",
depending on whether your code is written for Python 2 or 3.


Creating Standalone Applications with zipapp
--------------------------------------------

Using the :mod:`zipapp` module, it is possible to create self-contained Python
programs, which can be distributed to end users who only need to have a
suitable version of Python installed on their system.  The key to doing this
is to bundle all of the application's dependencies into the archive, along
with the application code.

The steps to create a standalone archive are as follows:

1. Create your application in a directory as normal, so you have a ``myapp``
   directory containing a ``__main__.py`` file, and any supporting application
   code.

2. Install all of your application's dependencies into the ``myapp`` directory,
   using pip:

   .. code-block:: shell-session

      $ python -m pip install -r requirements.txt --target myapp

   (this assumes you have your project requirements in a ``requirements.txt``
   file - if not, you can just list the dependencies manually on the pip command
   line).

3. Optionally, delete the ``.dist-info`` directories created by pip in the
   ``myapp`` directory. These hold metadata for pip to manage the packages, and
   as you won't be making any further use of pip they aren't required -
   although it won't do any harm if you leave them.

4. Package the application using:

   .. code-block:: shell-session

      $ python -m zipapp -p "interpreter" myapp

This will produce a standalone executable, which can be run on any machine with
the appropriate interpreter available. See :ref:`zipapp-specifying-the-interpreter`
for details. It can be shipped to users as a single file.

On Unix, the ``myapp.pyz`` file is executable as it stands.  You can rename the
file to remove the ``.pyz`` extension if you prefer a "plain" command name.  On
Windows, the ``myapp.pyz[w]`` file is executable by virtue of the fact that
the Python interpreter registers the ``.pyz`` and ``.pyzw`` file extensions
when installed.


Making a Windows executable
~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Windows, registration of the ``.pyz`` extension is optional, and
furthermore, there are certain places that don't recognise registered
extensions "transparently" (the simplest example is that
``subprocess.run(['myapp'])`` won't find your application - you need to
explicitly specify the extension).

On Windows, therefore, it is often preferable to create an executable from the
zipapp.  This is relatively easy, although it does require a C compiler.  The
basic approach relies on the fact that zipfiles can have arbitrary data
prepended, and Windows exe files can have arbitrary data appended.  So by
creating a suitable launcher and tacking the ``.pyz`` file onto the end of it,
you end up with a single-file executable that runs your application.

A suitable launcher can be as simple as the following::

   #define Py_LIMITED_API 1
   #include "Python.h"

   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>

   #ifdef WINDOWS
   int WINAPI wWinMain(
       HINSTANCE hInstance,      /* handle to current instance */
       HINSTANCE hPrevInstance,  /* handle to previous instance */
       LPWSTR lpCmdLine,         /* pointer to command line */
       int nCmdShow              /* show state of window */
   )
   #else
   int wmain()
   #endif
   {
       wchar_t **myargv = _alloca((__argc + 1) * sizeof(wchar_t*));
       myargv[0] = __wargv[0];
       memcpy(myargv + 1, __wargv, __argc * sizeof(wchar_t *));
       return Py_Main(__argc+1, myargv);
   }

If you define the ``WINDOWS`` preprocessor symbol, this will generate a
GUI executable, and without it, a console executable.

To compile the executable, you can either just use the standard MSVC
command line tools, or you can take advantage of the fact that distutils
knows how to compile Python source::

   >>> from distutils.ccompiler import new_compiler
   >>> import distutils.sysconfig
   >>> import sys
   >>> import os
   >>> from pathlib import Path

   >>> def compile(src):
   >>>     src = Path(src)
   >>>     cc = new_compiler()
   >>>     exe = src.stem
   >>>     cc.add_include_dir(distutils.sysconfig.get_python_inc())
   >>>     cc.add_library_dir(os.path.join(sys.base_exec_prefix, 'libs'))
   >>>     # First the CLI executable
   >>>     objs = cc.compile([str(src)])
   >>>     cc.link_executable(objs, exe)
   >>>     # Now the GUI executable
   >>>     cc.define_macro('WINDOWS')
   >>>     objs = cc.compile([str(src)])
   >>>     cc.link_executable(objs, exe + 'w')

   >>> if __name__ == "__main__":
   >>>     compile("zastub.c")

The resulting launcher uses the "Limited ABI", so it will run unchanged with
any version of Python 3.x.  All it needs is for Python (``python3.dll``) to be
on the user's ``PATH``.

For a fully standalone distribution, you can distribute the launcher with your
application appended, bundled with the Python "embedded" distribution.  This
will run on any PC with the appropriate architecture (32 bit or 64 bit).


Caveats
~~~~~~~

There are some limitations to the process of bundling your application into
a single file.  In most, if not all, cases they can be addressed without
needing major changes to your application.

1. If your application depends on a package that includes a C extension, that
   package cannot be run from a zip file (this is an OS limitation, as executable
   code must be present in the filesystem for the OS loader to load it). In this
   case, you can exclude that dependency from the zipfile, and either require
   your users to have it installed, or ship it alongside your zipfile and add code
   to your ``__main__.py`` to include the directory containing the unzipped
   module in ``sys.path``. In this case, you will need to make sure to ship
   appropriate binaries for your target architecture(s) (and potentially pick the
   correct version to add to ``sys.path`` at runtime, based on the user's machine).

2. If you are shipping a Windows executable as described above, you either need to
   ensure that your users have ``python3.dll`` on their PATH (which is not the
   default behaviour of the installer) or you should bundle your application with
   the embedded distribution.

3. The suggested launcher above uses the Python embedding API.  This means that in
   your application, ``sys.executable`` will be your application, and *not* a
   conventional Python interpreter.  Your code and its dependencies need to be
   prepared for this possibility.  For example, if your application uses the
   :mod:`multiprocessing` module, it will need to call
   :func:`multiprocessing.set_executable` to let the module know where to find the
   standard Python interpreter.


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

