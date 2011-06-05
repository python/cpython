.. highlightlang:: none

====================================
Installing Python projects: overwiew
====================================

.. _packaging_packaging-intro:

Introduction
============

Although Python's extensive standard library covers many programming needs,
there often comes a time when you need to add new functionality to your Python
installation in the form of third-party modules. This might be necessary to
support your own programming, or to support an application that you want to use
and that happens to be written in Python.

In the past, there was little support for adding third-party modules to an
existing Python installation.  With the introduction of the Python Distribution
Utilities (Distutils for short) in Python 2.0, this changed.  However, not all
problems were solved; end-users had to rely on ``easy_install`` or
``pip`` to download third-party modules from PyPI, uninstall distributions or do
other maintenance operations.  Packaging is a more complete replacement for
Distutils, in the standard library, with a backport named Distutils2 available
for older Python versions.

This document is aimed primarily at people who need to install third-party
Python modules: end-users and system administrators who just need to get some
Python application running, and existing Python programmers who want to add
new goodies to their toolbox. You don't need to know Python to read this
document; there will be some brief forays into using Python's interactive mode
to explore your installation, but that's it. If you're looking for information
on how to distribute your own Python modules so that others may use them, see
the :ref:`packaging-index` manual.


.. _packaging-trivial-install:

Best case: trivial installation
-------------------------------

In the best case, someone will have prepared a special version of the module
distribution you want to install that is targeted specifically at your platform
and can be installed just like any other software on your platform. For example,
the module's developer might make an executable installer available for Windows
users, an RPM package for users of RPM-based Linux systems (Red Hat, SuSE,
Mandrake, and many others), a Debian package for users of Debian and derivative
systems, and so forth.

In that case, you would use the standard system tools to download and install
the specific installer for your platform and its dependencies.

Of course, things will not always be that easy. You might be interested in a
module whose distribution doesn't have an easy-to-use installer for your
platform. In that case, you'll have to start with the source distribution
released by the module's author/maintainer. Installing from a source
distribution is not too hard, as long as the modules are packaged in the
standard way. The bulk of this document addresses the building and installing
of modules from standard source distributions.


.. _packaging-distutils:

The Python standard: Distutils
------------------------------

If you download a source distribution of a module, it will be obvious whether
it was packaged and distributed using Distutils.  First, the distribution's name
and version number will be featured prominently in the name of the downloaded
archive, e.g. :file:`foo-1.0.tar.gz` or :file:`widget-0.9.7.zip`.  Next, the
archive will unpack into a similarly-named directory: :file:`foo-1.0` or
:file:`widget-0.9.7`.  Additionally, the distribution may contain a
:file:`setup.cfg` file and a file named :file:`README.txt` ---or possibly just
:file:`README`--- explaining that building and installing the module
distribution is a simple matter of issuing the following command at your shell's
prompt::

   python setup.py install

Third-party projects have extended Distutils to work around its limitations or
add functionality.  After some years of near-inactivity in Distutils, a new
maintainer has started to standardize good ideas in PEPs and implement them in a
new, improved version of Distutils, called Distutils2 or Packaging.


.. _packaging-new-standard:

The new standard: Packaging
---------------------------

The rules described in the first paragraph above apply to Packaging-based
projects too: a source distribution will have a name like
:file:`widget-0.9.7.zip`.  One of the main differences with Distutils is that
distributions no longer have a :file:`setup.py` script; it used to cause a
number of issues.  Now there is a unique script installed with Python itself::

   pysetup install widget-0.9.7.zip

Running this command is enough to build and install projects (Python modules or
packages, scripts or whole applications), without even having to unpack the
archive.  It is also compatible with Distutils-based distributions.

Unless you have to perform non-standard installations or customize the build
process, you can stop reading this manual ---the above command is everything you
need to get out of it.

With :program:`pysetup`, you won't even have to manually download a distribution
before installing it; see :ref:`packaging-pysetup`.


.. _packaging-standard-install:

Standard build and install
==========================

As described in section :ref:`packaging-new-standard`, building and installing
a module distribution using Packaging usually comes down to one simple
command::

   pysetup run install_dist

How you actually run this command depends on the platform and the command line
interface it provides:

* **Unix**: Use a shell prompt.
* **Windows**: Open a command prompt ("DOS console") or use :command:`Powershell`.
* **OS X**: Open a :command:`Terminal`.


.. _packaging-platform-variations:

Platform variations
-------------------

The setup command is meant to be run from the root directory of the source
distribution, i.e. the top-level subdirectory that the module source
distribution unpacks into. For example, if you've just downloaded a module
source distribution :file:`foo-1.0.tar.gz` onto a Unix system, the normal
steps to follow are these::

   gunzip -c foo-1.0.tar.gz | tar xf -    # unpacks into directory foo-1.0
   cd foo-1.0
   pysetup run install_dist

On Windows, you'd probably download :file:`foo-1.0.zip`. If you downloaded the
archive file to :file:`C:\\Temp`, then it would unpack into
:file:`C:\\Temp\\foo-1.0`. To actually unpack the archive, you can use either
an archive manipulator with a graphical user interface (such as WinZip or 7-Zip)
or a command-line tool (such as :program:`unzip`, :program:`pkunzip` or, again,
:program:`7z`). Then, open a command prompt window ("DOS box" or
Powershell), and run::

   cd c:\Temp\foo-1.0
   pysetup run install_dist


.. _packaging-splitting-up:

Splitting the job up
--------------------

Running ``pysetup run install_dist`` builds and installs all modules in one go. If you
prefer to work incrementally ---especially useful if you want to customize the
build process, or if things are going wrong--- you can use the setup script to
do one thing at a time. This is a valuable tool when different users will perform
separately the build and install steps. For example, you might want to build a
module distribution and hand it off to a system administrator for installation
(or do it yourself, but with super-user or admin privileges).

For example, to build everything in one step and then install everything
in a second step, you aptly invoke two distinct Packaging commands::

   pysetup run build
   pysetup run install_dist

If you do this, you will notice that invoking the :command:`install_dist` command
first runs the :command:`build` command, which ---in this case--- quickly
notices it can spare itself the work, since everything in the :file:`build`
directory is up-to-date.

You may often ignore this ability to divide the process in steps if all you do
is installing modules downloaded from the Internet, but it's very handy for
more advanced tasks. If you find yourself in the need for distributing your own
Python modules and extensions, though, you'll most likely run many individual
Packaging commands.


.. _packaging-how-build-works:

How building works
------------------

As implied above, the :command:`build` command is responsible for collecting
and placing the files to be installed into a *build directory*. By default,
this is :file:`build`, under the distribution root. If you're excessively
concerned with speed, or want to keep the source tree pristine, you can specify
a different build directory with the :option:`--build-base` option. For example::

   pysetup run build --build-base /tmp/pybuild/foo-1.0

(Or you could do this permanently with a directive in your system or personal
Packaging configuration file; see section :ref:`packaging-config-files`.)
In the usual case, however, all this is unnecessary.

The build tree's default layout looks like so::

   --- build/ --- lib/
   or
   --- build/ --- lib.<plat>/
                  temp.<plat>/

where ``<plat>`` expands to a brief description of the current OS/hardware
platform and Python version. The first form, with just a :file:`lib` directory,
is used for pure module distributions (module distributions that
include only pure Python modules). If a module distribution contains any
extensions (modules written in C/C++), then the second form, with two ``<plat>``
directories, is used. In that case, the :file:`temp.{plat}` directory holds
temporary files generated during the compile/link process which are not intended
to be installed. In either case, the :file:`lib` (or :file:`lib.{plat}`) directory
contains all Python modules (pure Python and extensions) to be installed.

In the future, more directories will be added to handle Python scripts,
documentation, binary executables, and whatever else is required to install
Python modules and applications.


.. _packaging-how-install-works:

How installation works
----------------------

After the :command:`build` command is run (whether explicitly or by the
:command:`install_dist` command on your behalf), the work of the :command:`install_dist`
command is relatively simple: all it has to do is copy the contents of
:file:`build/lib` (or :file:`build/lib.{plat}`) to the installation directory
of your choice.

If you don't choose an installation directory ---i.e., if you just run
``pysetup run install_dist``\ --- then the :command:`install_dist` command
installs to the standard location for third-party Python modules. This location
varies by platform and depending on how you built/installed Python itself. On
Unix (and Mac OS X, which is also Unix-based), it also depends on whether the
module distribution being installed is pure Python or contains extensions
("non-pure"):

+-----------------+-----------------------------------------------------+--------------------------------------------------+-------+
| Platform        | Standard installation location                      | Default value                                    | Notes |
+=================+=====================================================+==================================================+=======+
| Unix (pure)     | :file:`{prefix}/lib/python{X.Y}/site-packages`      | :file:`/usr/local/lib/python{X.Y}/site-packages` | \(1)  |
+-----------------+-----------------------------------------------------+--------------------------------------------------+-------+
| Unix (non-pure) | :file:`{exec-prefix}/lib/python{X.Y}/site-packages` | :file:`/usr/local/lib/python{X.Y}/site-packages` | \(1)  |
+-----------------+-----------------------------------------------------+--------------------------------------------------+-------+
| Windows         | :file:`{prefix}\\Lib\\site-packages`                | :file:`C:\\Python{XY}\\Lib\\site-packages`       | \(2)  |
+-----------------+-----------------------------------------------------+--------------------------------------------------+-------+

Notes:

(1)
   Most Linux distributions include Python as a standard part of the system, so
   :file:`{prefix}` and :file:`{exec-prefix}` are usually both :file:`/usr` on
   Linux. If you build Python yourself on Linux (or any Unix-like system), the
   default :file:`{prefix}` and :file:`{exec-prefix}` are :file:`/usr/local`.

(2)
   The default installation directory on Windows was :file:`C:\\Program
   Files\\Python` under Python 1.6a1, 1.5.2, and earlier.

:file:`{prefix}` and :file:`{exec-prefix}` stand for the directories that Python
is installed to, and where it finds its libraries at run-time. They are always
the same under Windows, and very often the same under Unix and Mac OS X. You
can find out what your Python installation uses for :file:`{prefix}` and
:file:`{exec-prefix}` by running Python in interactive mode and typing a few
simple commands.

.. TODO link to Doc/using instead of duplicating

To start the interactive Python interpreter, you need to follow a slightly
different recipe for each platform. Under Unix, just type :command:`python` at
the shell prompt. Under Windows (assuming the Python executable is on your
:envvar:`PATH`, which is the usual case), you can choose :menuselection:`Start --> Run`,
type ``python`` and press ``enter``. Alternatively, you can simply execute
:command:`python` at a command prompt ("DOS console" or Powershell).

Once the interpreter is started, you type Python code at the prompt. For
example, on my Linux system, I type the three Python statements shown below,
and get the output as shown, to find out my :file:`{prefix}` and :file:`{exec-prefix}`::

   Python 3.3 (r32:88445, Apr  2 2011, 10:43:54)
   Type "help", "copyright", "credits" or "license" for more information.
   >>> import sys
   >>> sys.prefix
   '/usr'
   >>> sys.exec_prefix
   '/usr'

If you don't want to install modules to the standard location, or if you don't
have permission to write there, then you need to read about alternate
installations in section :ref:`packaging-alt-install`. If you want to customize your
installation directories more heavily, see section :ref:`packaging-custom-install`.


.. _packaging-alt-install:

Alternate installation
======================

Often, it is necessary or desirable to install modules to a location other than
the standard location for third-party Python modules. For example, on a Unix
system you might not have permission to write to the standard third-party module
directory. Or you might wish to try out a module before making it a standard
part of your local Python installation. This is especially true when upgrading
a distribution already present: you want to make sure your existing base of
scripts still works with the new version before actually upgrading.

The Packaging :command:`install_dist` command is designed to make installing module
distributions to an alternate location simple and painless. The basic idea is
that you supply a base directory for the installation, and the
:command:`install_dist` command picks a set of directories (called an *installation
scheme*) under this base directory in which to install files. The details
differ across platforms, so read whichever of the following sections applies to
you.


.. _packaging-alt-install-prefix:

Alternate installation: the home scheme
---------------------------------------

The idea behind the "home scheme" is that you build and maintain a personal
stash of Python modules. This scheme's name is derived from the concept of a
"home" directory on Unix, since it's not unusual for a Unix user to make their
home directory have a layout similar to :file:`/usr/` or :file:`/usr/local/`.
In spite of its name's origin, this scheme can be used by anyone, regardless
of the operating system.

Installing a new module distribution in this way is as simple as ::

   pysetup run install_dist --home <dir>

where you can supply any directory you like for the :option:`--home` option. On
Unix, lazy typists can just type a tilde (``~``); the :command:`install_dist` command
will expand this to your home directory::

   pysetup run install_dist --home ~

The :option:`--home` option defines the base directory for the installation.
Under it, files are installed to the following directories:

+------------------------------+---------------------------+-----------------------------+
| Type of file                 | Installation Directory    | Override option             |
+==============================+===========================+=============================+
| pure module distribution     | :file:`{home}/lib/python` | :option:`--install-purelib` |
+------------------------------+---------------------------+-----------------------------+
| non-pure module distribution | :file:`{home}/lib/python` | :option:`--install-platlib` |
+------------------------------+---------------------------+-----------------------------+
| scripts                      | :file:`{home}/bin`        | :option:`--install-scripts` |
+------------------------------+---------------------------+-----------------------------+
| data                         | :file:`{home}/share`      | :option:`--install-data`    |
+------------------------------+---------------------------+-----------------------------+


.. _packaging-alt-install-home:

Alternate installation: Unix (the prefix scheme)
------------------------------------------------

The "prefix scheme" is useful when you wish to use one Python installation to
run the build command, but install modules into the third-party module directory
of a different Python installation (or something that looks like a different
Python installation). If this sounds a trifle unusual, it is ---that's why the
"home scheme" comes first. However, there are at least two known cases where the
prefix scheme will be useful.

First, consider that many Linux distributions put Python in :file:`/usr`, rather
than the more traditional :file:`/usr/local`. This is entirely appropriate,
since in those cases Python is part of "the system" rather than a local add-on.
However, if you are installing Python modules from source, you probably want
them to go in :file:`/usr/local/lib/python2.{X}` rather than
:file:`/usr/lib/python2.{X}`. This can be done with ::

   pysetup run install_dist --prefix /usr/local

Another possibility is a network filesystem where the name used to write to a
remote directory is different from the name used to read it: for example, the
Python interpreter accessed as :file:`/usr/local/bin/python` might search for
modules in :file:`/usr/local/lib/python2.{X}`, but those modules would have to
be installed to, say, :file:`/mnt/{@server}/export/lib/python2.{X}`. This could
be done with ::

   pysetup run install_dist --prefix=/mnt/@server/export

In either case, the :option:`--prefix` option defines the installation base, and
the :option:`--exec-prefix` option defines the platform-specific installation
base, which is used for platform-specific files. (Currently, this just means
non-pure module distributions, but could be expanded to C libraries, binary
executables, etc.) If :option:`--exec-prefix` is not supplied, it defaults to
:option:`--prefix`. Files are installed as follows:

+------------------------------+-----------------------------------------------------+-----------------------------+
| Type of file                 | Installation Directory                              | Override option             |
+==============================+=====================================================+=============================+
| pure module distribution     | :file:`{prefix}/lib/python{X.Y}/site-packages`      | :option:`--install-purelib` |
+------------------------------+-----------------------------------------------------+-----------------------------+
| non-pure module distribution | :file:`{exec-prefix}/lib/python{X.Y}/site-packages` | :option:`--install-platlib` |
+------------------------------+-----------------------------------------------------+-----------------------------+
| scripts                      | :file:`{prefix}/bin`                                | :option:`--install-scripts` |
+------------------------------+-----------------------------------------------------+-----------------------------+
| data                         | :file:`{prefix}/share`                              | :option:`--install-data`    |
+------------------------------+-----------------------------------------------------+-----------------------------+

There is no requirement that :option:`--prefix` or :option:`--exec-prefix`
actually point to an alternate Python installation; if the directories listed
above do not already exist, they are created at installation time.

Incidentally, the real reason the prefix scheme is important is simply that a
standard Unix installation uses the prefix scheme, but with :option:`--prefix`
and :option:`--exec-prefix` supplied by Python itself as ``sys.prefix`` and
``sys.exec_prefix``. Thus, you might think you'll never use the prefix scheme,
but every time you run ``pysetup run install_dist`` without any other
options, you're using it.

Note that installing extensions to an alternate Python installation doesn't have
anything to do with how those extensions are built: in particular, extensions
will be compiled using the Python header files (:file:`Python.h` and friends)
installed with the Python interpreter used to run the build command. It is
therefore your responsibility to ensure compatibility between the interpreter
intended to run extensions installed in this way and the interpreter used to
build these same extensions. To avoid problems, it is best to make sure that
the two interpreters are the same version of Python (possibly different builds,
or possibly copies of the same build). (Of course, if your :option:`--prefix`
and :option:`--exec-prefix` don't even point to an alternate Python installation,
this is immaterial.)


.. _packaging-alt-install-windows:

Alternate installation: Windows (the prefix scheme)
---------------------------------------------------

Windows has a different and vaguer notion of home directories than Unix, and
since its standard Python installation is simpler, the :option:`--prefix` option
has traditionally been used to install additional packages to arbitrary
locations. ::

   pysetup run install_dist --prefix "\Temp\Python"

to install modules to the :file:`\\Temp\\Python` directory on the current drive.

The installation base is defined by the :option:`--prefix` option; the
:option:`--exec-prefix` option is unsupported under Windows. Files are
installed as follows:

+------------------------------+---------------------------+-----------------------------+
| Type of file                 | Installation Directory    | Override option             |
+==============================+===========================+=============================+
| pure module distribution     | :file:`{prefix}`          | :option:`--install-purelib` |
+------------------------------+---------------------------+-----------------------------+
| non-pure module distribution | :file:`{prefix}`          | :option:`--install-platlib` |
+------------------------------+---------------------------+-----------------------------+
| scripts                      | :file:`{prefix}\\Scripts` | :option:`--install-scripts` |
+------------------------------+---------------------------+-----------------------------+
| data                         | :file:`{prefix}\\Data`    | :option:`--install-data`    |
+------------------------------+---------------------------+-----------------------------+


.. _packaging-custom-install:

Custom installation
===================

Sometimes, the alternate installation schemes described in section
:ref:`packaging-alt-install` just don't do what you want. You might want to tweak
just one or two directories while keeping everything under the same base
directory, or you might want to completely redefine the installation scheme.
In either case, you're creating a *custom installation scheme*.

You probably noticed the column of "override options" in the tables describing
the alternate installation schemes above. Those options are how you define a
custom installation scheme. These override options can be relative, absolute,
or explicitly defined in terms of one of the installation base directories.
(There are two installation base directories, and they are normally the same
---they only differ when you use the Unix "prefix scheme" and supply different
:option:`--prefix` and :option:`--exec-prefix` options.)

For example, say you're installing a module distribution to your home directory
under Unix, but you want scripts to go in :file:`~/scripts` rather than
:file:`~/bin`. As you might expect, you can override this directory with the
:option:`--install-scripts` option and, in this case, it makes most sense to supply
a relative path, which will be interpreted relative to the installation base
directory (in our example, your home directory)::

   pysetup run install_dist --home ~ --install-scripts scripts

Another Unix example: suppose your Python installation was built and installed
with a prefix of :file:`/usr/local/python`. Thus, in a standard installation,
scripts will wind up in :file:`/usr/local/python/bin`. If you want them in
:file:`/usr/local/bin` instead, you would supply this absolute directory for
the :option:`--install-scripts` option::

   pysetup run install_dist --install-scripts /usr/local/bin

This command performs an installation using the "prefix scheme", where the
prefix is whatever your Python interpreter was installed with ---in this case,
:file:`/usr/local/python`.

If you maintain Python on Windows, you might want third-party modules to live in
a subdirectory of :file:`{prefix}`, rather than right in :file:`{prefix}`
itself. This is almost as easy as customizing the script installation directory
---you just have to remember that there are two types of modules to worry about,
pure modules and non-pure modules (i.e., modules from a non-pure distribution).
For example::

   pysetup run install_dist --install-purelib Site --install-platlib Site

.. XXX Nothing is installed right under prefix in windows, is it??

The specified installation directories are relative to :file:`{prefix}`. Of
course, you also have to ensure that these directories are in Python's module
search path, such as by putting a :file:`.pth` file in :file:`{prefix}`. See
section :ref:`packaging-search-path` to find out how to modify Python's search path.

If you want to define an entire installation scheme, you just have to supply all
of the installation directory options. Using relative paths is recommended here.
For example, if you want to maintain all Python module-related files under
:file:`python` in your home directory, and you want a separate directory for
each platform that you use your home directory from, you might define the
following installation scheme::

   pysetup run install_dist --home ~ \
       --install-purelib python/lib \
       --install-platlib python/'lib.$PLAT' \
       --install-scripts python/scripts \
       --install-data python/data

or, equivalently, ::

   pysetup run install_dist --home ~/python \
       --install-purelib lib \
       --install-platlib 'lib.$PLAT' \
       --install-scripts scripts \
       --install-data data

``$PLAT`` doesn't need to be defined as an environment variable ---it will also
be expanded by Packaging as it parses your command line options, just as it
does when parsing your configuration file(s). (More on that later.)

Obviously, specifying the entire installation scheme every time you install a
new module distribution would be very tedious. To spare you all that work, you
can store it in a Packaging configuration file instead (see section
:ref:`packaging-config-files`), like so::

   [install_dist]
   install-base = $HOME
   install-purelib = python/lib
   install-platlib = python/lib.$PLAT
   install-scripts = python/scripts
   install-data = python/data

or, equivalently, ::

   [install_dist]
   install-base = $HOME/python
   install-purelib = lib
   install-platlib = lib.$PLAT
   install-scripts = scripts
   install-data = data

Note that these two are *not* equivalent if you override their installation
base directory when running the setup script. For example, ::

   pysetup run install_dist --install-base /tmp

would install pure modules to :file:`/tmp/python/lib` in the first case, and
to :file:`/tmp/lib` in the second case. (For the second case, you'd probably
want to supply an installation base of :file:`/tmp/python`.)

You may have noticed the use of ``$HOME`` and ``$PLAT`` in the sample
configuration file. These are Packaging configuration variables, which
bear a strong resemblance to environment variables. In fact, you can use
environment variables in configuration files on platforms that have such a notion, but
Packaging additionally defines a few extra variables that may not be in your
environment, such as ``$PLAT``. Of course, on systems that don't have
environment variables, such as Mac OS 9, the configuration variables supplied by
the Packaging are the only ones you can use. See section :ref:`packaging-config-files`
for details.

.. XXX which vars win out eventually in case of clash env or Packaging?

.. XXX need some Windows examples---when would custom installation schemes be
   needed on those platforms?


.. XXX Move this section to Doc/using

.. _packaging-search-path:

Modifying Python's search path
------------------------------

When the Python interpreter executes an :keyword:`import` statement, it searches
for both Python code and extension modules along a search path. A default value
for this path is configured into the Python binary when the interpreter is built.
You can obtain the search path by importing the :mod:`sys` module and printing
the value of ``sys.path``. ::

   $ python
   Python 2.2 (#11, Oct  3 2002, 13:31:27)
   [GCC 2.96 20000731 (Red Hat Linux 7.3 2.96-112)] on linux2
   Type "help", "copyright", "credits" or "license" for more information.
   >>> import sys
   >>> sys.path
   ['', '/usr/local/lib/python2.3', '/usr/local/lib/python2.3/plat-linux2',
    '/usr/local/lib/python2.3/lib-tk', '/usr/local/lib/python2.3/lib-dynload',
    '/usr/local/lib/python2.3/site-packages']
   >>>

The null string in ``sys.path`` represents the current working directory.

The expected convention for locally installed packages is to put them in the
:file:`{...}/site-packages/` directory, but you may want to choose a different
location for some reason. For example, if your site kept by convention all web
server-related software under :file:`/www`. Add-on Python modules might then
belong in :file:`/www/python`, and in order to import them, this directory would
have to be added to ``sys.path``. There are several ways to solve this problem.

The most convenient way is to add a path configuration file to a directory
that's already on Python's path, usually to the :file:`.../site-packages/`
directory. Path configuration files have an extension of :file:`.pth`, and each
line must contain a single path that will be appended to ``sys.path``. (Because
the new paths are appended to ``sys.path``, modules in the added directories
will not override standard modules. This means you can't use this mechanism for
installing fixed versions of standard modules.)

Paths can be absolute or relative, in which case they're relative to the
directory containing the :file:`.pth` file. See the documentation of
the :mod:`site` module for more information.

A slightly less convenient way is to edit the :file:`site.py` file in Python's
standard library, and modify ``sys.path``. :file:`site.py` is automatically
imported when the Python interpreter is executed, unless the :option:`-S` switch
is supplied to suppress this behaviour. So you could simply edit
:file:`site.py` and add two lines to it::

   import sys
   sys.path.append('/www/python/')

However, if you reinstall the same major version of Python (perhaps when
upgrading from 3.3 to 3.3.1, for example) :file:`site.py` will be overwritten by
the stock version. You'd have to remember that it was modified and save a copy
before doing the installation.

Alternatively, there are two environment variables that can modify ``sys.path``.
:envvar:`PYTHONHOME` sets an alternate value for the prefix of the Python
installation. For example, if :envvar:`PYTHONHOME` is set to ``/www/python``,
the search path will be set to ``['', '/www/python/lib/pythonX.Y/',
'/www/python/lib/pythonX.Y/plat-linux2', ...]``.

The :envvar:`PYTHONPATH` variable can be set to a list of paths that will be
added to the beginning of ``sys.path``. For example, if :envvar:`PYTHONPATH` is
set to ``/www/python:/opt/py``, the search path will begin with
``['/www/python', '/opt/py']``. (Note that directories must exist in order to
be added to ``sys.path``; the :mod:`site` module removes non-existent paths.)

Finally, ``sys.path`` is just a regular Python list, so any Python application
can modify it by adding or removing entries.


.. _packaging-config-files:

Configuration files for Packaging
=================================

As mentioned above, you can use configuration files to store personal or site
preferences for any option supported by any Packaging command. Depending on your
platform, you can use one of two or three possible configuration files. These
files will be read before parsing the command-line, so they take precedence over
default values. In turn, the command-line will override configuration files.
Lastly, if there are multiple configuration files, values from files read
earlier will be overridden by values from files read later.

.. XXX "one of two or three possible..." seems wrong info. Below always 3 files
   are indicated in the tables.


.. _packaging-config-filenames:

Location and names of configuration files
-----------------------------------------

The name and location of the configuration files vary slightly across
platforms. On Unix and Mac OS X, these are the three configuration files listed
in the order they are processed:

+--------------+----------------------------------------------------------+-------+
| Type of file | Location and filename                                    | Notes |
+==============+==========================================================+=======+
| system       | :file:`{prefix}/lib/python{ver}/packaging/packaging.cfg` | \(1)  |
+--------------+----------------------------------------------------------+-------+
| personal     | :file:`$HOME/.pydistutils.cfg`                           | \(2)  |
+--------------+----------------------------------------------------------+-------+
| local        | :file:`setup.cfg`                                        | \(3)  |
+--------------+----------------------------------------------------------+-------+

Similarly, the configuration files on Windows ---also listed in the order they
are processed--- are these:

+--------------+-------------------------------------------------+-------+
| Type of file | Location and filename                           | Notes |
+==============+=================================================+=======+
| system       | :file:`{prefix}\\Lib\\packaging\\packaging.cfg` | \(4)  |
+--------------+-------------------------------------------------+-------+
| personal     | :file:`%HOME%\\pydistutils.cfg`                 | \(5)  |
+--------------+-------------------------------------------------+-------+
| local        | :file:`setup.cfg`                               | \(3)  |
+--------------+-------------------------------------------------+-------+

On all platforms, the *personal* file can be temporarily disabled by
means of the `--no-user-cfg` option.

Notes:

(1)
   Strictly speaking, the system-wide configuration file lives in the directory
   where Packaging is installed.

(2)
   On Unix, if the :envvar:`HOME` environment variable is not defined, the
   user's home directory will be determined with the :func:`getpwuid` function
   from the standard :mod:`pwd` module. Packaging uses the
   :func:`os.path.expanduser` function to do this.

(3)
   I.e., in the current directory (usually the location of the setup script).

(4)
   (See also note (1).) Python's default installation prefix is
   :file:`C:\\Python`, so the system configuration file is normally
   :file:`C:\\Python\\Lib\\packaging\\packaging.cfg`.

(5)
   On Windows, if the :envvar:`HOME` environment variable is not defined,
   :envvar:`USERPROFILE` then :envvar:`HOMEDRIVE` and :envvar:`HOMEPATH` will
   be tried. Packaging uses the :func:`os.path.expanduser` function to do this.


.. _packaging-config-syntax:

Syntax of configuration files
-----------------------------

All Packaging configuration files share the same syntax. Options defined in
them are grouped into sections, and each Packaging command gets its own section.
Additionally, there's a ``global`` section for options that affect every command.
Sections consist of one or more lines containing a single option specified as
``option = value``.

For example, here's a complete configuration file that forces all commands to
run quietly by default::

   [global]
   verbose = 0

If this was the system configuration file, it would affect all processing
of any Python module distribution by any user on the current system. If it was
installed as your personal configuration file (on systems that support them),
it would affect only module distributions processed by you. Lastly, if it was
used as the :file:`setup.cfg` for a particular module distribution, it would
affect that distribution only.

.. XXX "(on systems that support them)" seems wrong info

If you wanted to, you could override the default "build base" directory and
make the :command:`build\*` commands always forcibly rebuild all files with
the following::

   [build]
   build-base = blib
   force = 1

which corresponds to the command-line arguments::

   pysetup run build --build-base blib --force

except that including the :command:`build` command on the command-line means
that command will be run. Including a particular command in configuration files
has no such implication; it only means that if the command is run, the options
for it in the configuration file will apply. (This is also true if you run
other commands that derive values from it.)

You can find out the complete list of options for any command using the
:option:`--help` option, e.g.::

   pysetup run build --help

and you can find out the complete list of global options by using
:option:`--help` without a command::

   pysetup run --help

See also the "Reference" section of the "Distributing Python Modules" manual.

.. XXX no links to the relevant section exist.


.. _packaging-building-ext:

Building extensions: tips and tricks
====================================

Whenever possible, Packaging tries to use the configuration information made
available by the Python interpreter used to run `pysetup`.
For example, the same compiler and linker flags used to compile Python will also
be used for compiling extensions. Usually this will work well, but in
complicated situations this might be inappropriate. This section discusses how
to override the usual Packaging behaviour.


.. _packaging-tweak-flags:

Tweaking compiler/linker flags
------------------------------

Compiling a Python extension written in C or C++ will sometimes require
specifying custom flags for the compiler and linker in order to use a particular
library or produce a special kind of object code. This is especially true if the
extension hasn't been tested on your platform, or if you're trying to
cross-compile Python.

.. TODO update to new setup.cfg

In the most general case, the extension author might have foreseen that
compiling the extensions would be complicated, and provided a :file:`Setup` file
for you to edit. This will likely only be done if the module distribution
contains many separate extension modules, or if they often require elaborate
sets of compiler flags in order to work.

A :file:`Setup` file, if present, is parsed in order to get a list of extensions
to build. Each line in a :file:`Setup` describes a single module. Lines have
the following structure::

   module ... [sourcefile ...] [cpparg ...] [library ...]


Let's examine each of the fields in turn.

* *module* is the name of the extension module to be built, and should be a
  valid Python identifier. You can't just change this in order to rename a module
  (edits to the source code would also be needed), so this should be left alone.

* *sourcefile* is anything that's likely to be a source code file, at least
  judging by the filename. Filenames ending in :file:`.c` are assumed to be
  written in C, filenames ending in :file:`.C`, :file:`.cc`, and :file:`.c++` are
  assumed to be C++, and filenames ending in :file:`.m` or :file:`.mm` are assumed
  to be in Objective C.

* *cpparg* is an argument for the C preprocessor,  and is anything starting with
  :option:`-I`, :option:`-D`, :option:`-U` or :option:`-C`.

* *library* is anything ending in :file:`.a` or beginning with :option:`-l` or
  :option:`-L`.

If a particular platform requires a special library on your platform, you can
add it by editing the :file:`Setup` file and running ``pysetup run build``.
For example, if the module defined by the line ::

   foo foomodule.c

must be linked with the math library :file:`libm.a` on your platform, simply add
:option:`-lm` to the line::

   foo foomodule.c -lm

Arbitrary switches intended for the compiler or the linker can be supplied with
the :option:`-Xcompiler` *arg* and :option:`-Xlinker` *arg* options::

   foo foomodule.c -Xcompiler -o32 -Xlinker -shared -lm

The next option after :option:`-Xcompiler` and :option:`-Xlinker` will be
appended to the proper command line, so in the above example the compiler will
be passed the :option:`-o32` option, and the linker will be passed
:option:`-shared`. If a compiler option requires an argument, you'll have to
supply multiple :option:`-Xcompiler` options; for example, to pass ``-x c++``
the :file:`Setup` file would have to contain ``-Xcompiler -x -Xcompiler c++``.

Compiler flags can also be supplied through setting the :envvar:`CFLAGS`
environment variable. If set, the contents of :envvar:`CFLAGS` will be added to
the compiler flags specified in the  :file:`Setup` file.


.. _packaging-non-ms-compilers:

Using non-Microsoft compilers on Windows
----------------------------------------

.. sectionauthor:: Rene Liebscher <R.Liebscher@gmx.de>



Borland/CodeGear C++
^^^^^^^^^^^^^^^^^^^^

This subsection describes the necessary steps to use Packaging with the Borland
C++ compiler version 5.5. First you have to know that Borland's object file
format (OMF) is different from the format used by the Python version you can
download from the Python or ActiveState Web site. (Python is built with
Microsoft Visual C++, which uses COFF as the object file format.) For this
reason, you have to convert Python's library :file:`python25.lib` into the
Borland format. You can do this as follows:

.. Should we mention that users have to create cfg-files for the compiler?
.. see also http://community.borland.com/article/0,1410,21205,00.html

::

   coff2omf python25.lib python25_bcpp.lib

The :file:`coff2omf` program comes with the Borland compiler. The file
:file:`python25.lib` is in the :file:`Libs` directory of your Python
installation. If your extension uses other libraries (zlib, ...) you have to
convert them too.

The converted files have to reside in the same directories as the normal
libraries.

How does Packaging manage to use these libraries with their changed names?  If
the extension needs a library (eg. :file:`foo`) Packaging checks first if it
finds a library with suffix :file:`_bcpp` (eg. :file:`foo_bcpp.lib`) and then
uses this library. In the case it doesn't find such a special library it uses
the default name (:file:`foo.lib`.) [#]_

To let Packaging compile your extension with Borland, C++ you now have to
type::

   pysetup run build --compiler bcpp

If you want to use the Borland C++ compiler as the default, you could specify
this in your personal or system-wide configuration file for Packaging (see
section :ref:`packaging-config-files`.)


.. seealso::

   `C++Builder Compiler <http://www.codegear.com/downloads/free/cppbuilder>`_
      Information about the free C++ compiler from Borland, including links to the
      download pages.

   `Creating Python Extensions Using Borland's Free Compiler <http://www.cyberus.ca/~g_will/pyExtenDL.shtml>`_
      Document describing how to use Borland's free command-line C++ compiler to build
      Python.


GNU C / Cygwin / MinGW
^^^^^^^^^^^^^^^^^^^^^^

This section describes the necessary steps to use Packaging with the GNU C/C++
compilers in their Cygwin and MinGW distributions. [#]_ For a Python interpreter
that was built with Cygwin, everything should work without any of these
following steps.

Not all extensions can be built with MinGW or Cygwin, but many can. Extensions
most likely to not work are those that use C++ or depend on Microsoft Visual C
extensions.

To let Packaging compile your extension with Cygwin, you have to type::

   pysetup run build --compiler=cygwin

and for Cygwin in no-cygwin mode [#]_ or for MinGW, type::

   pysetup run build --compiler=mingw32

If you want to use any of these options/compilers as default, you should
consider writing it in your personal or system-wide configuration file for
Packaging (see section :ref:`packaging-config-files`.)

Older Versions of Python and MinGW
""""""""""""""""""""""""""""""""""
The following instructions only apply if you're using a version of Python
inferior to 2.4.1 with a MinGW inferior to 3.0.0 (with
:file:`binutils-2.13.90-20030111-1`).

These compilers require some special libraries. This task is more complex than
for Borland's C++, because there is no program to convert the library. First
you have to create a list of symbols which the Python DLL exports. (You can find
a good program for this task at
http://www.emmestech.com/software/pexports-0.43/download_pexports.html).

.. I don't understand what the next line means. --amk
   (inclusive the references on data structures.)

::

   pexports python25.dll > python25.def

The location of an installed :file:`python25.dll` will depend on the
installation options and the version and language of Windows. In a "just for
me" installation, it will appear in the root of the installation directory. In
a shared installation, it will be located in the system directory.

Then you can create from these information an import library for gcc. ::

   /cygwin/bin/dlltool --dllname python25.dll --def python25.def --output-lib libpython25.a

The resulting library has to be placed in the same directory as
:file:`python25.lib`. (Should be the :file:`libs` directory under your Python
installation directory.)

If your extension uses other libraries (zlib,...) you might have to convert
them too. The converted files have to reside in the same directories as the
normal libraries do.


.. seealso::

   `Building Python modules on MS Windows platform with MinGW <http://www.zope.org/Members/als/tips/win32_mingw_modules>`_
      Information about building the required libraries for the MinGW
      environment.


.. rubric:: Footnotes

.. [#] This also means you could replace all existing COFF-libraries with
   OMF-libraries of the same name.

.. [#] Check http://sources.redhat.com/cygwin/ and http://www.mingw.org/ for
   more information.

.. [#] Then you have no POSIX emulation available, but you also don't need
   :file:`cygwin1.dll`.
