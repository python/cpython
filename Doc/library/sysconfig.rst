:mod:`!sysconfig` --- Provide access to Python's configuration information
==========================================================================

.. module:: sysconfig
   :synopsis: Python's configuration information

.. moduleauthor:: Tarek Ziadé <tarek@ziade.org>
.. sectionauthor:: Tarek Ziadé <tarek@ziade.org>

.. versionadded:: 3.2

**Source code:** :source:`Lib/sysconfig`

.. index::
   single: configuration information

--------------

The :mod:`sysconfig` module provides access to Python's configuration
information like the list of installation paths and the configuration variables
relevant for the current platform.


Configuration variables
-----------------------

A Python distribution contains a :file:`Makefile` and a :file:`pyconfig.h`
header file that are necessary to build both the Python binary itself and
third-party C extensions compiled using ``setuptools``.

:mod:`sysconfig` puts all variables found in these files in a dictionary that
can be accessed using :func:`get_config_vars` or :func:`get_config_var`.

Notice that on Windows, it's a much smaller set.

.. function:: get_config_vars(*args)

   With no arguments, return a dictionary of all configuration variables
   relevant for the current platform.

   With arguments, return a list of values that result from looking up each
   argument in the configuration variable dictionary.

   For each argument, if the value is not found, return ``None``.


.. function:: get_config_var(name)

   Return the value of a single variable *name*. Equivalent to
   ``get_config_vars().get(name)``.

   If *name* is not found, return ``None``.

Example of usage::

   >>> import sysconfig
   >>> sysconfig.get_config_var('Py_ENABLE_SHARED')
   0
   >>> sysconfig.get_config_var('LIBDIR')
   '/usr/local/lib'
   >>> sysconfig.get_config_vars('AR', 'CXX')
   ['ar', 'g++']


.. _installation_paths:

Installation paths
------------------

Python uses an installation scheme that differs depending on the platform and on
the installation options.  These schemes are stored in :mod:`sysconfig` under
unique identifiers based on the value returned by :const:`os.name`.
The schemes are used by package installers to determine where to copy files to.

Python currently supports nine schemes:

- *posix_prefix*: scheme for POSIX platforms like Linux or macOS.  This is
  the default scheme used when Python or a component is installed.
- *posix_home*: scheme for POSIX platforms, when the *home* option is used.
  This scheme defines paths located under a specific home prefix.
- *posix_user*: scheme for POSIX platforms, when the *user* option is used.
  This scheme defines paths located under the user's home directory
  (:const:`site.USER_BASE`).
- *posix_venv*: scheme for :mod:`Python virtual environments <venv>` on POSIX
  platforms; by default it is the same as *posix_prefix*.
- *nt*: scheme for Windows.
  This is the default scheme used when Python or a component is installed.
- *nt_user*: scheme for Windows, when the *user* option is used.
- *nt_venv*: scheme for :mod:`Python virtual environments <venv>` on Windows;
  by default it is the same as *nt*.
- *venv*: a scheme with values from either *posix_venv* or *nt_venv* depending
  on the platform Python runs on.
- *osx_framework_user*: scheme for macOS, when the *user* option is used.

Each scheme is itself composed of a series of paths and each path has a unique
identifier.  Python currently uses eight paths:

- *stdlib*: directory containing the standard Python library files that are not
  platform-specific.
- *platstdlib*: directory containing the standard Python library files that are
  platform-specific.
- *platlib*: directory for site-specific, platform-specific files.
- *purelib*: directory for site-specific, non-platform-specific files ('pure' Python).
- *include*: directory for non-platform-specific header files for
  the Python C-API.
- *platinclude*: directory for platform-specific header files for
  the Python C-API.
- *scripts*: directory for script files.
- *data*: directory for data files.


.. _sysconfig-user-scheme:

User scheme
---------------

This scheme is designed to be the most convenient solution for users that don't
have write permission to the global site-packages directory or don't want to
install into it.

Files will be installed into subdirectories of :const:`site.USER_BASE` (written
as :file:`{userbase}` hereafter).  This scheme installs pure Python modules and
extension modules in the same location (also known as :const:`site.USER_SITE`).

``posix_user``
^^^^^^^^^^^^^^

============== ===========================================================
Path           Installation directory
============== ===========================================================
*stdlib*       :file:`{userbase}/lib/python{X.Y}`
*platstdlib*   :file:`{userbase}/lib/python{X.Y}`
*platlib*      :file:`{userbase}/lib/python{X.Y}/site-packages`
*purelib*      :file:`{userbase}/lib/python{X.Y}/site-packages`
*include*      :file:`{userbase}/include/python{X.Y}`
*scripts*      :file:`{userbase}/bin`
*data*         :file:`{userbase}`
============== ===========================================================

``nt_user``
^^^^^^^^^^^

============== ===========================================================
Path           Installation directory
============== ===========================================================
*stdlib*       :file:`{userbase}\\Python{XY}`
*platstdlib*   :file:`{userbase}\\Python{XY}`
*platlib*      :file:`{userbase}\\Python{XY}\\site-packages`
*purelib*      :file:`{userbase}\\Python{XY}\\site-packages`
*include*      :file:`{userbase}\\Python{XY}\\Include`
*scripts*      :file:`{userbase}\\Python{XY}\\Scripts`
*data*         :file:`{userbase}`
============== ===========================================================

``osx_framework_user``
^^^^^^^^^^^^^^^^^^^^^^

============== ===========================================================
Path           Installation directory
============== ===========================================================
*stdlib*       :file:`{userbase}/lib/python`
*platstdlib*   :file:`{userbase}/lib/python`
*platlib*      :file:`{userbase}/lib/python/site-packages`
*purelib*      :file:`{userbase}/lib/python/site-packages`
*include*      :file:`{userbase}/include/python{X.Y}`
*scripts*      :file:`{userbase}/bin`
*data*         :file:`{userbase}`
============== ===========================================================


.. _sysconfig-home-scheme:

Home scheme
-----------

The idea behind the "home scheme" is that you build and maintain a personal
stash of Python modules.  This scheme's name is derived from the idea of a
"home" directory on Unix, since it's not unusual for a Unix user to make their
home directory have a layout similar to :file:`/usr/` or :file:`/usr/local/`.
This scheme can be used by anyone, regardless of the operating system they
are installing for.

``posix_home``
^^^^^^^^^^^^^^

============== ===========================================================
Path           Installation directory
============== ===========================================================
*stdlib*       :file:`{home}/lib/python`
*platstdlib*   :file:`{home}/lib/python`
*platlib*      :file:`{home}/lib/python`
*purelib*      :file:`{home}/lib/python`
*include*      :file:`{home}/include/python`
*platinclude*  :file:`{home}/include/python`
*scripts*      :file:`{home}/bin`
*data*         :file:`{home}`
============== ===========================================================


.. _sysconfig-prefix-scheme:

Prefix scheme
-------------

The "prefix scheme" is useful when you wish to use one Python installation to
perform the build/install (i.e., to run the setup script), but install modules
into the third-party module directory of a different Python installation (or
something that looks like a different Python installation).  If this sounds a
trifle unusual, it is---that's why the user and home schemes come before.  However,
there are at least two known cases where the prefix scheme will be useful.

First, consider that many Linux distributions put Python in :file:`/usr`, rather
than the more traditional :file:`/usr/local`.  This is entirely appropriate,
since in those cases Python is part of "the system" rather than a local add-on.
However, if you are installing Python modules from source, you probably want
them to go in :file:`/usr/local/lib/python2.{X}` rather than
:file:`/usr/lib/python2.{X}`.

Another possibility is a network filesystem where the name used to write to a
remote directory is different from the name used to read it: for example, the
Python interpreter accessed as :file:`/usr/local/bin/python` might search for
modules in :file:`/usr/local/lib/python2.{X}`, but those modules would have to
be installed to, say, :file:`/mnt/{@server}/export/lib/python2.{X}`.

``posix_prefix``
^^^^^^^^^^^^^^^^

============== ==========================================================
Path           Installation directory
============== ==========================================================
*stdlib*       :file:`{prefix}/lib/python{X.Y}`
*platstdlib*   :file:`{prefix}/lib/python{X.Y}`
*platlib*      :file:`{prefix}/lib/python{X.Y}/site-packages`
*purelib*      :file:`{prefix}/lib/python{X.Y}/site-packages`
*include*      :file:`{prefix}/include/python{X.Y}`
*platinclude*  :file:`{prefix}/include/python{X.Y}`
*scripts*      :file:`{prefix}/bin`
*data*         :file:`{prefix}`
============== ==========================================================

``nt``
^^^^^^

============== ==========================================================
Path           Installation directory
============== ==========================================================
*stdlib*       :file:`{prefix}\\Lib`
*platstdlib*   :file:`{prefix}\\Lib`
*platlib*      :file:`{prefix}\\Lib\\site-packages`
*purelib*      :file:`{prefix}\\Lib\\site-packages`
*include*      :file:`{prefix}\\Include`
*platinclude*  :file:`{prefix}\\Include`
*scripts*      :file:`{prefix}\\Scripts`
*data*         :file:`{prefix}`
============== ==========================================================


Installation path functions
---------------------------

:mod:`sysconfig` provides some functions to determine these installation paths.

.. function:: get_scheme_names()

   Return a tuple containing all schemes currently supported in
   :mod:`sysconfig`.


.. function:: get_default_scheme()

   Return the default scheme name for the current platform.

   .. versionadded:: 3.10
      This function was previously named ``_get_default_scheme()`` and
      considered an implementation detail.

   .. versionchanged:: 3.11
      When Python runs from a virtual environment,
      the *venv* scheme is returned.

.. function:: get_preferred_scheme(key)

   Return a preferred scheme name for an installation layout specified by *key*.

   *key* must be either ``"prefix"``, ``"home"``, or ``"user"``.

   The return value is a scheme name listed in :func:`get_scheme_names`. It
   can be passed to :mod:`sysconfig` functions that take a *scheme* argument,
   such as :func:`get_paths`.

   .. versionadded:: 3.10

   .. versionchanged:: 3.11
      When Python runs from a virtual environment and ``key="prefix"``,
      the *venv* scheme is returned.


.. function:: _get_preferred_schemes()

   Return a dict containing preferred scheme names on the current platform.
   Python implementers and redistributors may add their preferred schemes to
   the ``_INSTALL_SCHEMES`` module-level global value, and modify this function
   to return those scheme names, to e.g. provide different schemes for system
   and language package managers to use, so packages installed by either do not
   mix with those by the other.

   End users should not use this function, but :func:`get_default_scheme` and
   :func:`get_preferred_scheme` instead.

   .. versionadded:: 3.10


.. function:: get_path_names()

   Return a tuple containing all path names currently supported in
   :mod:`sysconfig`.


.. function:: get_path(name, [scheme, [vars, [expand]]])

   Return an installation path corresponding to the path *name*, from the
   install scheme named *scheme*.

   *name* has to be a value from the list returned by :func:`get_path_names`.

   :mod:`sysconfig` stores installation paths corresponding to each path name,
   for each platform, with variables to be expanded.  For instance the *stdlib*
   path for the *nt* scheme is: ``{base}/Lib``.

   :func:`get_path` will use the variables returned by :func:`get_config_vars`
   to expand the path.  All variables have default values for each platform so
   one may call this function and get the default value.

   If *scheme* is provided, it must be a value from the list returned by
   :func:`get_scheme_names`.  Otherwise, the default scheme for the current
   platform is used.

   If *vars* is provided, it must be a dictionary of variables that will update
   the dictionary returned by :func:`get_config_vars`.

   If *expand* is set to ``False``, the path will not be expanded using the
   variables.

   If *name* is not found, raise a :exc:`KeyError`.


.. function:: get_paths([scheme, [vars, [expand]]])

   Return a dictionary containing all installation paths corresponding to an
   installation scheme. See :func:`get_path` for more information.

   If *scheme* is not provided, will use the default scheme for the current
   platform.

   If *vars* is provided, it must be a dictionary of variables that will
   update the dictionary used to expand the paths.

   If *expand* is set to false, the paths will not be expanded.

   If *scheme* is not an existing scheme, :func:`get_paths` will raise a
   :exc:`KeyError`.


Other functions
---------------

.. function:: get_python_version()

   Return the ``MAJOR.MINOR`` Python version number as a string.  Similar to
   ``'%d.%d' % sys.version_info[:2]``.


.. function:: get_platform()

   Return a string that identifies the current platform.

   This is used mainly to distinguish platform-specific build directories and
   platform-specific built distributions.  Typically includes the OS name and
   version and the architecture (as supplied by :func:`os.uname`), although the
   exact information included depends on the OS; e.g., on Linux, the kernel
   version isn't particularly important.

   Examples of returned values:

   - linux-x86_64
   - linux-aarch64
   - solaris-2.6-sun4u

   Windows:

   - win-amd64 (64-bit Windows on AMD64, aka x86_64, Intel64, and EM64T)
   - win-arm64 (64-bit Windows on ARM64, aka AArch64)
   - win32 (all others - specifically, sys.platform is returned)

   POSIX based OS:

   - linux-x86_64
   - macosx-15.5-arm64
   - macosx-26.0-universal2 (macOS on Apple Silicon or Intel)
   - android-24-arm64_v8a

   For other non-POSIX platforms, currently just returns :data:`sys.platform`.


.. function:: is_python_build()

   Return ``True`` if the running Python interpreter was built from source and
   is being run from its built location, and not from a location resulting from
   e.g. running ``make install`` or installing via a binary installer.


.. function:: parse_config_h(fp[, vars])

   Parse a :file:`config.h`\-style file.

   *fp* is a file-like object pointing to the :file:`config.h`\-like file.

   A dictionary containing name/value pairs is returned.  If an optional
   dictionary is passed in as the second argument, it is used instead of a new
   dictionary, and updated with the values read in the file.


.. function:: get_config_h_filename()

   Return the path of :file:`pyconfig.h`.

.. function:: get_makefile_filename()

   Return the path of :file:`Makefile`.

.. _sysconfig-cli:
.. _using-sysconfig-as-a-script:

Command-line usage
------------------

You can use :mod:`sysconfig` as a script with Python's *-m* option:

.. code-block:: shell-session

    $ python -m sysconfig
    Platform: "macosx-10.4-i386"
    Python version: "3.2"
    Current installation scheme: "posix_prefix"

    Paths:
            data = "/usr/local"
            include = "/Users/tarek/Dev/svn.python.org/py3k/Include"
            platinclude = "."
            platlib = "/usr/local/lib/python3.2/site-packages"
            platstdlib = "/usr/local/lib/python3.2"
            purelib = "/usr/local/lib/python3.2/site-packages"
            scripts = "/usr/local/bin"
            stdlib = "/usr/local/lib/python3.2"

    Variables:
            AC_APPLE_UNIVERSAL_BUILD = "0"
            AIX_GENUINE_CPLUSPLUS = "0"
            AR = "ar"
            ARFLAGS = "rc"
            ...

This call will print in the standard output the information returned by
:func:`get_platform`, :func:`get_python_version`, :func:`get_path` and
:func:`get_config_vars`.
