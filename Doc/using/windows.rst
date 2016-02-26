.. highlightlang:: none

.. _using-on-windows:

*************************
 Using Python on Windows
*************************

.. sectionauthor:: Robert Lehmann <lehmannro@gmail.com>

This document aims to give an overview of Windows-specific behaviour you should
know about when using Python on Microsoft Windows.


Installing Python
=================

Unlike most Unix systems and services, Windows does not require Python natively
and thus does not pre-install a version of Python.  However, the CPython team
has compiled Windows installers (MSI packages) with every `release
<https://www.python.org/download/releases/>`_ for many years.

With ongoing development of Python, some platforms that used to be supported
earlier are no longer supported (due to the lack of users or developers).
Check :pep:`11` for details on all unsupported platforms.

* DOS and Windows 3.x are deprecated since Python 2.0 and code specific to these
  systems was removed in Python 2.1.
* Up to 2.5, Python was still compatible with Windows 95, 98 and ME (but already
  raised a deprecation warning on installation).  For Python 2.6 (and all
  following releases), this support was dropped and new releases are just
  expected to work on the Windows NT family.
* `Windows CE <http://pythonce.sourceforge.net/>`_ is still supported.
* The `Cygwin <http://cygwin.com/>`_ installer offers to install the Python
  interpreter as well (cf. `Cygwin package source
  <ftp://ftp.uni-erlangen.de/pub/pc/gnuwin32/cygwin/mirrors/cygnus/
  release/python>`_, `Maintainer releases
  <http://www.tishler.net/jason/software/python/>`_)

See `Python for Windows (and DOS) <https://www.python.org/download/windows/>`_
for detailed information about platforms with precompiled installers.

.. seealso::

   `Python on XP <http://dooling.com/index.php/2006/03/14/python-on-xp-7-minutes-to-hello-world/>`_
      "7 Minutes to "Hello World!""
      by Richard Dooling, 2006

   `Installing on Windows <http://www.diveintopython.net/installing_python/windows.html>`_
      in "`Dive into Python: Python from novice to pro
      <http://www.diveintopython.net/>`_"
      by Mark Pilgrim, 2004,
      ISBN 1-59059-356-1

   `For Windows users <http://www.swaroopch.com/notes/python/#install_windows>`_
      in "Installing Python"
      in "`A Byte of Python <http://www.swaroopch.com/notes/python/>`_"
      by Swaroop C H, 2003


Alternative bundles
===================

Besides the standard CPython distribution, there are modified packages including
additional functionality.  The following is a list of popular versions and their
key features:

`ActivePython <https://www.activestate.com/activepython/>`_
    Installer with multi-platform compatibility, documentation, PyWin32

`Enthought Python Distribution <https://www.enthought.com/products/epd/>`_
    Popular modules (such as PyWin32) with their respective documentation, tool
    suite for building extensible Python applications

Notice that these packages are likely to install *older* versions of Python.



Configuring Python
==================

In order to run Python flawlessly, you might have to change certain environment
settings in Windows.


.. _setting-envvars:

Excursus: Setting environment variables
---------------------------------------

Windows has a built-in dialog for changing environment variables (following
guide applies to XP classical view): Right-click the icon for your machine
(usually located on your Desktop and called "My Computer") and choose
:menuselection:`Properties` there.  Then, open the :guilabel:`Advanced` tab
and click the :guilabel:`Environment Variables` button.

In short, your path is:

    :menuselection:`My Computer
    --> Properties
    --> Advanced
    --> Environment Variables`

In this dialog, you can add or modify User and System variables. To change
System variables, you need non-restricted access to your machine
(i.e. Administrator rights).

Another way of adding variables to your environment is using the :command:`set`
command::

    set PYTHONPATH=%PYTHONPATH%;C:\My_python_lib

To make this setting permanent, you could add the corresponding command line to
your :file:`autoexec.bat`. :program:`msconfig` is a graphical interface to this
file.

Viewing environment variables can also be done more straight-forward: The
command prompt will expand strings wrapped into percent signs automatically::

    echo %PATH%

Consult :command:`set /?` for details on this behaviour.

.. seealso::

   https://support.microsoft.com/kb/100843
      Environment variables in Windows NT

   https://support.microsoft.com/kb/310519
      How To Manage Environment Variables in Windows XP

   http://www.chem.gla.ac.uk/~louis/software/faq/q1.html
      Setting Environment variables, Louis J. Farrugia


Finding the Python executable
-----------------------------

Besides using the automatically created start menu entry for the Python
interpreter, you might want to start Python in the DOS prompt.  To make this
work, you need to set your :envvar:`%PATH%` environment variable to include the
directory of your Python distribution, delimited by a semicolon from other
entries.  An example variable could look like this (assuming the first two
entries are Windows' default)::

    C:\WINDOWS\system32;C:\WINDOWS;C:\Python25

Typing :command:`python` on your command prompt will now fire up the Python
interpreter.  Thus, you can also execute your scripts with command line options,
see :ref:`using-on-cmdline` documentation.


Finding modules
---------------

Python usually stores its library (and thereby your site-packages folder) in the
installation directory.  So, if you had installed Python to
:file:`C:\\Python\\`, the default library would reside in
:file:`C:\\Python\\Lib\\` and third-party modules should be stored in
:file:`C:\\Python\\Lib\\site-packages\\`.

This is how :data:`sys.path` is populated on Windows:

* An empty entry is added at the start, which corresponds to the current
  directory.

* If the environment variable :envvar:`PYTHONPATH` exists, as described in
  :ref:`using-on-envvars`, its entries are added next.  Note that on Windows,
  paths in this variable must be separated by semicolons, to distinguish them
  from the colon used in drive identifiers (``C:\`` etc.).

* Additional "application paths" can be added in the registry as subkeys of
  :samp:`\\SOFTWARE\\Python\\PythonCore\\{version}\\PythonPath` under both the
  ``HKEY_CURRENT_USER`` and ``HKEY_LOCAL_MACHINE`` hives.  Subkeys which have
  semicolon-delimited path strings as their default value will cause each path
  to be added to :data:`sys.path`.  (Note that all known installers only use
  HKLM, so HKCU is typically empty.)

* If the environment variable :envvar:`PYTHONHOME` is set, it is assumed as
  "Python Home".  Otherwise, the path of the main Python executable is used to
  locate a "landmark file" (``Lib\os.py``) to deduce the "Python Home".  If a
  Python home is found, the relevant sub-directories added to :data:`sys.path`
  (``Lib``, ``plat-win``, etc) are based on that folder.  Otherwise, the core
  Python path is constructed from the PythonPath stored in the registry.

* If the Python Home cannot be located, no :envvar:`PYTHONPATH` is specified in
  the environment, and no registry entries can be found, a default path with
  relative entries is used (e.g. ``.\Lib;.\plat-win``, etc).

The end result of all this is:

* When running :file:`python.exe`, or any other .exe in the main Python
  directory (either an installed version, or directly from the PCbuild
  directory), the core path is deduced, and the core paths in the registry are
  ignored.  Other "application paths" in the registry are always read.

* When Python is hosted in another .exe (different directory, embedded via COM,
  etc), the "Python Home" will not be deduced, so the core path from the
  registry is used.  Other "application paths" in the registry are always read.

* If Python can't find its home and there is no registry (eg, frozen .exe, some
  very strange installation setup) you get a path with some default, but
  relative, paths.


Executing scripts
-----------------

Python scripts (files with the extension ``.py``) will be executed by
:program:`python.exe` by default.  This executable opens a terminal, which stays
open even if the program uses a GUI.  If you do not want this to happen, use the
extension ``.pyw`` which will cause the script to be executed by
:program:`pythonw.exe` by default (both executables are located in the top-level
of your Python installation directory).  This suppresses the terminal window on
startup.

You can also make all ``.py`` scripts execute with :program:`pythonw.exe`,
setting this through the usual facilities, for example (might require
administrative rights):

#. Launch a command prompt.
#. Associate the correct file group with ``.py`` scripts::

      assoc .py=Python.File

#. Redirect all Python files to the new executable::

      ftype Python.File=C:\Path\to\pythonw.exe "%1" %*


Additional modules
==================

Even though Python aims to be portable among all platforms, there are features
that are unique to Windows.  A couple of modules, both in the standard library
and external, and snippets exist to use these features.

The Windows-specific standard modules are documented in
:ref:`mswin-specific-services`.


PyWin32
-------

The `PyWin32 <https://pypi.python.org/pypi/pywin32>`_ module by Mark Hammond
is a collection of modules for advanced Windows-specific support.  This includes
utilities for:

* `Component Object Model <https://www.microsoft.com/com/>`_ (COM)
* Win32 API calls
* Registry
* Event log
* `Microsoft Foundation Classes <https://msdn.microsoft.com/en-us/library/fe1cf721%28VS.80%29.aspx>`_ (MFC)
  user interfaces

`PythonWin <https://web.archive.org/web/20060524042422/
https://www.python.org/windows/pythonwin/>`_ is a sample MFC application
shipped with PyWin32.  It is an embeddable IDE with a built-in debugger.

.. seealso::

   `Win32 How Do I...? <http://timgolden.me.uk/python/win32_how_do_i.html>`_
      by Tim Golden

   `Python and COM <http://www.boddie.org.uk/python/COM.html>`_
      by David and Paul Boddie


Py2exe
------

`Py2exe <http://www.py2exe.org/>`_ is a :mod:`distutils` extension (see
:ref:`extending-distutils`) which wraps Python scripts into executable Windows
programs (:file:`{*}.exe` files).  When you have done this, you can distribute
your application without requiring your users to install Python.


WConio
------

Since Python's advanced terminal handling layer, :mod:`curses`, is restricted to
Unix-like systems, there is a library exclusive to Windows as well: Windows
Console I/O for Python.

`WConio <http://newcenturycomputers.net/projects/wconio.html>`_ is a wrapper for
Turbo-C's :file:`CONIO.H`, used to create text user interfaces.



Compiling Python on Windows
===========================

If you want to compile CPython yourself, first thing you should do is get the
`source <https://www.python.org/downloads/source/>`_. You can download either the
latest release's source or just grab a fresh `checkout
<https://docs.python.org/devguide/setup.html#getting-the-source-code>`_.

For Microsoft Visual C++, which is the compiler with which official Python
releases are built, the source tree contains solutions/project files.  View the
:file:`readme.txt` in their respective directories:

+--------------------+--------------+-----------------------+
| Directory          | MSVC version | Visual Studio version |
+====================+==============+=======================+
| :file:`PC/VC6/`    | 6.0          | 97                    |
+--------------------+--------------+-----------------------+
| :file:`PC/VS7.1/`  | 7.1          | 2003                  |
+--------------------+--------------+-----------------------+
| :file:`PC/VS8.0/`  | 8.0          | 2005                  |
+--------------------+--------------+-----------------------+
| :file:`PCbuild/`   | 9.0          | 2008                  |
+--------------------+--------------+-----------------------+

Note that not all of these build directories are fully supported.  Read the
release notes to see which compiler version the official releases for your
version are built with.

Check :file:`PC/readme.txt` for general information on the build process.


For extension modules, consult :ref:`building-on-windows`.

.. seealso::

   `Python + Windows + distutils + SWIG + gcc MinGW <http://sebsauvage.net/python/mingw.html>`_
      or "Creating Python extensions in C/C++ with SWIG and compiling them with
      MinGW gcc under Windows" or "Installing Python extension with distutils
      and without Microsoft Visual C++" by Sébastien Sauvage, 2003

   `MingW -- Python extensions <http://oldwiki.mingw.org/index.php/Python%20extensions>`_
      by Trent Apted et al, 2007


Other resources
===============

.. seealso::

   `Python Programming On Win32 <http://shop.oreilly.com/product/9781565926219.do>`_
      "Help for Windows Programmers"
      by Mark Hammond and Andy Robinson, O'Reilly Media, 2000,
      ISBN 1-56592-621-8

   `A Python for Windows Tutorial <http://www.imladris.com/Scripts/PythonForWindows.html>`_
      by Amanda Birmingham, 2004

