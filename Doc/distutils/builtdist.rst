.. _built-dist:

****************************
Creating Built Distributions
****************************

.. include:: ./_setuptools_disclaimer.rst

A "built distribution" is what you're probably used to thinking of either as a
"binary package" or an "installer" (depending on your background).  It's not
necessarily binary, though, because it might contain only Python source code
and/or byte-code; and we don't call it a package, because that word is already
spoken for in Python.  (And "installer" is a term specific to the world of
mainstream desktop systems.)

A built distribution is how you make life as easy as possible for installers of
your module distribution: for users of RPM-based Linux systems, it's a binary
RPM; for Windows users, it's an executable installer; for Debian-based Linux
users, it's a Debian package; and so forth.  Obviously, no one person will be
able to create built distributions for every platform under the sun, so the
Distutils are designed to enable module developers to concentrate on their
specialty---writing code and creating source distributions---while an
intermediary species called *packagers* springs up to turn source distributions
into built distributions for as many platforms as there are packagers.

Of course, the module developer could be their own packager; or the packager could
be a volunteer "out there" somewhere who has access to a platform which the
original developer does not; or it could be software periodically grabbing new
source distributions and turning them into built distributions for as many
platforms as the software has access to.  Regardless of who they are, a packager
uses the setup script and the :command:`bdist` command family to generate built
distributions.

As a simple example, if I run the following command in the Distutils source
tree::

   python setup.py bdist

then the Distutils builds my module distribution (the Distutils itself in this
case), does a "fake" installation (also in the :file:`build` directory), and
creates the default type of built distribution for my platform.  The default
format for built distributions is a "dumb" tar file on Unix, and a simple
executable installer on Windows.  (That tar file is considered "dumb" because it
has to be unpacked in a specific location to work.)

Thus, the above command on a Unix system creates
:file:`Distutils-1.0.{plat}.tar.gz`; unpacking this tarball from the right place
installs the Distutils just as though you had downloaded the source distribution
and run ``python setup.py install``.  (The "right place" is either the root of
the filesystem or  Python's :file:`{prefix}` directory, depending on the options
given to the :command:`bdist_dumb` command; the default is to make dumb
distributions relative to :file:`{prefix}`.)

Obviously, for pure Python distributions, this isn't any simpler than just
running ``python setup.py install``\ ---but for non-pure distributions, which
include extensions that would need to be compiled, it can mean the difference
between someone being able to use your extensions or not.  And creating "smart"
built distributions, such as an RPM package or an executable installer for
Windows, is far more convenient for users even if your distribution doesn't
include any extensions.

The :command:`bdist` command has a :option:`!--formats` option, similar to the
:command:`sdist` command, which you can use to select the types of built
distribution to generate: for example, ::

   python setup.py bdist --format=zip

would, when run on a Unix system, create
:file:`Distutils-1.0.{plat}.zip`\ ---again, this archive would be unpacked
from the root directory to install the Distutils.

The available formats for built distributions are:

+-------------+------------------------------+---------+
| Format      | Description                  | Notes   |
+=============+==============================+=========+
| ``gztar``   | gzipped tar file             | \(1)    |
|             | (:file:`.tar.gz`)            |         |
+-------------+------------------------------+---------+
| ``bztar``   | bzipped tar file             |         |
|             | (:file:`.tar.bz2`)           |         |
+-------------+------------------------------+---------+
| ``xztar``   | xzipped tar file             |         |
|             | (:file:`.tar.xz`)            |         |
+-------------+------------------------------+---------+
| ``ztar``    | compressed tar file          | \(3)    |
|             | (:file:`.tar.Z`)             |         |
+-------------+------------------------------+---------+
| ``tar``     | tar file (:file:`.tar`)      |         |
+-------------+------------------------------+---------+
| ``zip``     | zip file (:file:`.zip`)      | (2),(4) |
+-------------+------------------------------+---------+
| ``rpm``     | RPM                          | \(5)    |
+-------------+------------------------------+---------+
| ``pkgtool`` | Solaris :program:`pkgtool`   |         |
+-------------+------------------------------+---------+
| ``sdux``    | HP-UX :program:`swinstall`   |         |
+-------------+------------------------------+---------+
| ``wininst`` | self-extracting ZIP file for | \(4)    |
|             | Windows                      |         |
+-------------+------------------------------+---------+
| ``msi``     | Microsoft Installer.         |         |
+-------------+------------------------------+---------+

.. versionchanged:: 3.5
   Added support for the ``xztar`` format.


Notes:

(1)
   default on Unix

(2)
   default on Windows

(3)
   requires external :program:`compress` utility.

(4)
   requires either external :program:`zip` utility or :mod:`zipfile` module (part
   of the standard Python library since Python 1.6)

(5)
   requires external :program:`rpm` utility, version 3.0.4 or better (use ``rpm
   --version`` to find out which version you have)

You don't have to use the :command:`bdist` command with the :option:`!--formats`
option; you can also use the command that directly implements the format you're
interested in.  Some of these :command:`bdist` "sub-commands" actually generate
several similar formats; for instance, the :command:`bdist_dumb` command
generates all the "dumb" archive formats (``tar``, ``gztar``, ``bztar``,
``xztar``, ``ztar``, and ``zip``), and :command:`bdist_rpm` generates both
binary and source RPMs.  The :command:`bdist` sub-commands, and the formats
generated by each, are:

+--------------------------+-------------------------------------+
| Command                  | Formats                             |
+==========================+=====================================+
| :command:`bdist_dumb`    | tar, gztar, bztar, xztar, ztar, zip |
+--------------------------+-------------------------------------+
| :command:`bdist_rpm`     | rpm, srpm                           |
+--------------------------+-------------------------------------+
| :command:`bdist_wininst` | wininst                             |
+--------------------------+-------------------------------------+
| :command:`bdist_msi`     | msi                                 |
+--------------------------+-------------------------------------+

.. note::
   bdist_wininst is deprecated since Python 3.8.

.. note::
   bdist_msi is deprecated since Python 3.9.

The following sections give details on the individual :command:`bdist_\*`
commands.


.. .. _creating-dumb:

.. Creating dumb built distributions
.. =================================

.. XXX Need to document absolute vs. prefix-relative packages here, but first
   I have to implement it!


.. _creating-rpms:

Creating RPM packages
=====================

The RPM format is used by many popular Linux distributions, including Red Hat,
SuSE, and Mandrake.  If one of these (or any of the other RPM-based Linux
distributions) is your usual environment, creating RPM packages for other users
of that same distribution is trivial. Depending on the complexity of your module
distribution and differences between Linux distributions, you may also be able
to create RPMs that work on different RPM-based distributions.

The usual way to create an RPM of your module distribution is to run the
:command:`bdist_rpm` command::

   python setup.py bdist_rpm

or the :command:`bdist` command with the :option:`!--format` option::

   python setup.py bdist --formats=rpm

The former allows you to specify RPM-specific options; the latter allows  you to
easily specify multiple formats in one run.  If you need to do both, you can
explicitly specify multiple :command:`bdist_\*` commands and their options::

   python setup.py bdist_rpm --packager="John Doe <jdoe@example.org>" \
                   bdist_wininst --target-version="2.0"

Creating RPM packages is driven by a :file:`.spec` file, much as using the
Distutils is driven by the setup script.  To make your life easier, the
:command:`bdist_rpm` command normally creates a :file:`.spec` file based on the
information you supply in the setup script, on the command line, and in any
Distutils configuration files.  Various options and sections in the
:file:`.spec` file are derived from options in the setup script as follows:

+------------------------------------------+----------------------------------------------+
| RPM :file:`.spec` file option or section | Distutils setup script option                |
+==========================================+==============================================+
| Name                                     | ``name``                                     |
+------------------------------------------+----------------------------------------------+
| Summary (in preamble)                    | ``description``                              |
+------------------------------------------+----------------------------------------------+
| Version                                  | ``version``                                  |
+------------------------------------------+----------------------------------------------+
| Vendor                                   | ``author`` and ``author_email``,             |
|                                          | or  --- & ``maintainer`` and                 |
|                                          | ``maintainer_email``                         |
+------------------------------------------+----------------------------------------------+
| Copyright                                | ``license``                                  |
+------------------------------------------+----------------------------------------------+
| Url                                      | ``url``                                      |
+------------------------------------------+----------------------------------------------+
| %description (section)                   | ``long_description``                         |
+------------------------------------------+----------------------------------------------+

Additionally, there are many options in :file:`.spec` files that don't have
corresponding options in the setup script.  Most of these are handled through
options to the :command:`bdist_rpm` command as follows:

+-------------------------------+-----------------------------+-------------------------+
| RPM :file:`.spec` file option | :command:`bdist_rpm` option | default value           |
| or section                    |                             |                         |
+===============================+=============================+=========================+
| Release                       | ``release``                 | "1"                     |
+-------------------------------+-----------------------------+-------------------------+
| Group                         | ``group``                   | "Development/Libraries" |
+-------------------------------+-----------------------------+-------------------------+
| Vendor                        | ``vendor``                  | (see above)             |
+-------------------------------+-----------------------------+-------------------------+
| Packager                      | ``packager``                | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Provides                      | ``provides``                | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Requires                      | ``requires``                | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Conflicts                     | ``conflicts``               | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Obsoletes                     | ``obsoletes``               | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Distribution                  | ``distribution_name``       | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| BuildRequires                 | ``build_requires``          | (none)                  |
+-------------------------------+-----------------------------+-------------------------+
| Icon                          | ``icon``                    | (none)                  |
+-------------------------------+-----------------------------+-------------------------+

Obviously, supplying even a few of these options on the command-line would be
tedious and error-prone, so it's usually best to put them in the setup
configuration file, :file:`setup.cfg`\ ---see section :ref:`setup-config`.  If
you distribute or package many Python module distributions, you might want to
put options that apply to all of them in your personal Distutils configuration
file (:file:`~/.pydistutils.cfg`).  If you want to temporarily disable
this file, you can pass the :option:`!--no-user-cfg` option to :file:`setup.py`.

There are three steps to building a binary RPM package, all of which are
handled automatically by the Distutils:

#. create a :file:`.spec` file, which describes the package (analogous  to the
   Distutils setup script; in fact, much of the information in the  setup script
   winds up in the :file:`.spec` file)

#. create the source RPM

#. create the "binary" RPM (which may or may not contain binary code, depending
   on whether your module distribution contains Python extensions)

Normally, RPM bundles the last two steps together; when you use the Distutils,
all three steps are typically bundled together.

If you wish, you can separate these three steps.  You can use the
:option:`!--spec-only` option to make :command:`bdist_rpm` just create the
:file:`.spec` file and exit; in this case, the :file:`.spec` file will be
written to the "distribution directory"---normally :file:`dist/`, but
customizable with the :option:`!--dist-dir` option.  (Normally, the :file:`.spec`
file winds up deep in the "build tree," in a temporary directory created by
:command:`bdist_rpm`.)

.. % \XXX{this isn't implemented yet---is it needed?!}
.. % You can also specify a custom \file{.spec} file with the
.. % \longprogramopt{spec-file} option; used in conjunction with
.. % \longprogramopt{spec-only}, this gives you an opportunity to customize
.. % the \file{.spec} file manually:
.. %
.. % \ begin{verbatim}
.. % > python setup.py bdist_rpm --spec-only
.. % # ...edit dist/FooBar-1.0.spec
.. % > python setup.py bdist_rpm --spec-file=dist/FooBar-1.0.spec
.. % \ end{verbatim}
.. %
.. % (Although a better way to do this is probably to override the standard
.. % \command{bdist\_rpm} command with one that writes whatever else you want
.. % to the \file{.spec} file.)


.. _creating-wininst:

Creating Windows Installers
===========================

.. warning::
   bdist_wininst is deprecated since Python 3.8.

.. warning::
   bdist_msi is deprecated since Python 3.9.

Executable installers are the natural format for binary distributions on
Windows.  They display a nice graphical user interface, display some information
about the module distribution to be installed taken from the metadata in the
setup script, let the user select a few options, and start or cancel the
installation.

Since the metadata is taken from the setup script, creating Windows installers
is usually as easy as running::

   python setup.py bdist_wininst

or the :command:`bdist` command with the :option:`!--formats` option::

   python setup.py bdist --formats=wininst

If you have a pure module distribution (only containing pure Python modules and
packages), the resulting installer will be version independent and have a name
like :file:`foo-1.0.win32.exe`. Note that creating ``wininst`` binary
distributions in only supported on Windows systems.

If you have a non-pure distribution, the extensions can only be created on a
Windows platform, and will be Python version dependent. The installer filename
will reflect this and now has the form :file:`foo-1.0.win32-py2.0.exe`.  You
have to create a separate installer for every Python version you want to
support.

The installer will try to compile pure modules into :term:`bytecode` after installation
on the target system in normal and optimizing mode.  If you don't want this to
happen for some reason, you can run the :command:`bdist_wininst` command with
the :option:`!--no-target-compile` and/or the :option:`!--no-target-optimize`
option.

By default the installer will display the cool "Python Powered" logo when it is
run, but you can also supply your own 152x261 bitmap which must be a Windows
:file:`.bmp` file with the :option:`!--bitmap` option.

The installer will also display a large title on the desktop background window
when it is run, which is constructed from the name of your distribution and the
version number.  This can be changed to another text by using the
:option:`!--title` option.

The installer file will be written to the "distribution directory" --- normally
:file:`dist/`, but customizable with the :option:`!--dist-dir` option.

.. _cross-compile-windows:

Cross-compiling on Windows
==========================

Starting with Python 2.6, distutils is capable of cross-compiling between
Windows platforms.  In practice, this means that with the correct tools
installed, you can use a 32bit version of Windows to create 64bit extensions
and vice-versa.

To build for an alternate platform, specify the :option:`!--plat-name` option
to the build command.  Valid values are currently 'win32', and  'win-amd64'.
For example, on a 32bit version of Windows, you could execute::

   python setup.py build --plat-name=win-amd64

to build a 64bit version of your extension.  The Windows Installers also
support this option, so the command::

   python setup.py build --plat-name=win-amd64 bdist_wininst

would create a 64bit installation executable on your 32bit version of Windows.

To cross-compile, you must download the Python source code and cross-compile
Python itself for the platform you are targeting - it is not possible from a
binary installation of Python (as the .lib etc file for other platforms are
not included.)  In practice, this means the user of a 32 bit operating
system will need to use Visual Studio 2008 to open the
:file:`PCbuild/PCbuild.sln` solution in the Python source tree and build the
"x64" configuration of the 'pythoncore' project before cross-compiling
extensions is possible.

Note that by default, Visual Studio 2008 does not install 64bit compilers or
tools.  You may need to reexecute the Visual Studio setup process and select
these tools (using Control Panel->[Add/Remove] Programs is a convenient way to
check or modify your existing install.)

.. _postinstallation-script:

The Postinstallation script
---------------------------

Starting with Python 2.3, a postinstallation script can be specified with the
:option:`!--install-script` option.  The basename of the script must be
specified, and the script filename must also be listed in the scripts argument
to the setup function.

This script will be run at installation time on the target system after all the
files have been copied, with ``argv[1]`` set to :option:`!-install`, and again at
uninstallation time before the files are removed with ``argv[1]`` set to
:option:`!-remove`.

The installation script runs embedded in the windows installer, every output
(``sys.stdout``, ``sys.stderr``) is redirected into a buffer and will be
displayed in the GUI after the script has finished.

Some functions especially useful in this context are available as additional
built-in functions in the installation script.


.. function:: directory_created(path)
              file_created(path)

   These functions should be called when a directory or file is created by the
   postinstall script at installation time.  It will register *path* with the
   uninstaller, so that it will be removed when the distribution is uninstalled.
   To be safe, directories are only removed if they are empty.


.. function:: get_special_folder_path(csidl_string)

   This function can be used to retrieve special folder locations on Windows like
   the Start Menu or the Desktop.  It returns the full path to the folder.
   *csidl_string* must be one of the following strings::

      "CSIDL_APPDATA"

      "CSIDL_COMMON_STARTMENU"
      "CSIDL_STARTMENU"

      "CSIDL_COMMON_DESKTOPDIRECTORY"
      "CSIDL_DESKTOPDIRECTORY"

      "CSIDL_COMMON_STARTUP"
      "CSIDL_STARTUP"

      "CSIDL_COMMON_PROGRAMS"
      "CSIDL_PROGRAMS"

      "CSIDL_FONTS"

   If the folder cannot be retrieved, :exc:`OSError` is raised.

   Which folders are available depends on the exact Windows version, and probably
   also the configuration.  For details refer to Microsoft's documentation of the
   :c:func:`SHGetSpecialFolderPath` function.


.. function:: create_shortcut(target, description, filename[, arguments[, workdir[, iconpath[, iconindex]]]])

   This function creates a shortcut. *target* is the path to the program to be
   started by the shortcut. *description* is the description of the shortcut.
   *filename* is the title of the shortcut that the user will see. *arguments*
   specifies the command line arguments, if any. *workdir* is the working directory
   for the program. *iconpath* is the file containing the icon for the shortcut,
   and *iconindex* is the index of the icon in the file *iconpath*.  Again, for
   details consult the Microsoft documentation for the :class:`IShellLink`
   interface.


Vista User Access Control (UAC)
===============================

Starting with Python 2.6, bdist_wininst supports a :option:`!--user-access-control`
option.  The default is 'none' (meaning no UAC handling is done), and other
valid values are 'auto' (meaning prompt for UAC elevation if Python was
installed for all users) and 'force' (meaning always prompt for elevation).

.. note::
   bdist_wininst is deprecated since Python 3.8.

.. note::
   bdist_msi is deprecated since Python 3.9.
