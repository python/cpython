.. _packaging-command-reference:

*****************
Command Reference
*****************

This reference briefly documents all standard Packaging commands and some of
their options.

.. FIXME does not work: Use pysetup run --help-commands to list all
   standard and extra commands availavble on your system, with their
   description.  Use pysetup run <command> --help to get help about the options
   of one command.


Preparing distributions
=======================

:command:`check`
----------------

Perform some tests on the metadata of a distribution.

For example, it verifies that all required metadata fields are provided in the
:file:`setup.cfg` file.

.. TODO document reST checks


:command:`test`
---------------

Run a test suite.

When doing test-driven development, or running automated builds that need
testing before they are installed for downloading or use, it's often useful to
be able to run a project's unit tests without actually installing the project
anywhere.  The :command:`test` command runs project's unit tests without
actually installing it, by temporarily putting the project's source on
:data:`sys.path`, after first running :command:`build_ext -i` to ensure that any
C extensions are built.

You can use this command in one of two ways: either by specifying a
unittest-compatible test suite for your project (or any callable that returns
it) or by passing a test runner function that will run your tests and display
results in the console.  Both options take a Python dotted name in the form
``package.module.callable`` to specify the object to use.

If none of these options are specified, Packaging will try to perform test
discovery using either unittest (for Python 3.2 and higher) or unittest2 (for
older versions, if installed).

.. this is a pseudo-command name used to disambiguate the options in indexes and
   links
.. program:: packaging test

.. cmdoption:: --suite=NAME, -s NAME

   Specify the test suite (or module, class, or method) to be run.  The default
   for this option can be set by in the project's :file:`setup.cfg` file::

   .. code-block:: cfg

      [test]
      suite = mypackage.tests.get_all_tests

.. cmdoption:: --runner=NAME, -r NAME

   Specify the test runner to be called.


:command:`config`
-----------------

Perform distribution configuration.


The build step
==============

This step is mainly useful to compile C/C++ libraries or extension modules.  The
build commands can be run manually to check for syntax errors or packaging
issues (for example if the addition of a new source file was forgotten in the
:file:`setup.cfg` file), and is also run automatically by commands which need
it.  Packaging checks the mtime of source and built files to avoid re-building
if it's not necessary.


:command:`build`
----------------

Build all files of a distribution, delegating to the other :command:`build_*`
commands to do the work.


:command:`build_clib`
---------------------

Build C libraries.


:command:`build_ext`
--------------------

Build C/C++ extension modules.


:command:`build_py`
-------------------

Build the Python modules (just copy them to the build directory) and
byte-compile them to .pyc files.


:command:`build_scripts`
------------------------
Build the scripts (just copy them to the build directory and adjust their
shebang if they're Python scripts).


:command:`clean`
----------------

Clean the build tree of the release.

.. program:: packaging clean

.. cmdoption:: --all, -a

   Remove build directories for modules, scripts, etc., not only temporary build
   by-products.


Creating source and built distributions
=======================================

:command:`sdist`
----------------

Build a source distribution for a release.

It is recommended that you always build and upload a source distribution.  Users
of OSes with easy access to compilers and users of advanced packaging tools will
prefer to compile from source rather than using pre-built distributions.  For
Windows users, providing a binary installer is also recommended practice.


:command:`bdist`
----------------

Build a binary distribution for a release.

This command will call other :command:`bdist_*` commands to create one or more
distributions depending on the options given.  The default is to create a
.tar.gz archive on Unix and a zip archive on Windows or OS/2.

.. program:: packaging bdist

.. cmdoption:: --formats

   Binary formats to build (comma-separated list).

.. cmdoption:: --show-formats

   Dump list of available formats.


:command:`bdist_dumb`
---------------------

Build a "dumb" installer, a simple archive of files that could be unpacked under
``$prefix`` or ``$exec_prefix``.


:command:`bdist_wininst`
------------------------

Build a Windows installer.


:command:`bdist_msi`
--------------------

Build a `Microsoft Installer`_ (.msi) file.

.. _Microsoft Installer: http://msdn.microsoft.com/en-us/library/cc185688(VS.85).aspx

In most cases, the :command:`bdist_msi` installer is a better choice than the
:command:`bdist_wininst` installer, because it provides better support for Win64
platforms, allows administrators to perform non-interactive installations, and
allows installation through group policies.


Publishing distributions
========================

:command:`register`
-------------------

This command registers the current release with the Python Package Index.  This
is described in more detail in :PEP:`301`.

.. TODO explain user and project registration with the web UI


:command:`upload`
-----------------

Upload source and/or binary distributions to PyPI.

The distributions have to be built on the same command line as the
:command:`upload` command; see :ref:`packaging-package-upload` for more info.

.. program:: packaging upload

.. cmdoption:: --sign, -s

   Sign each uploaded file using GPG (GNU Privacy Guard).  The ``gpg`` program
   must be available for execution on the system ``PATH``.

.. cmdoption:: --identity=NAME, -i NAME

   Specify the identity or key name for GPG to use when signing.  The value of
   this option will be passed through the ``--local-user`` option of the
   ``gpg`` program.

.. cmdoption:: --show-response

   Display the full response text from server; this is useful for debugging
   PyPI problems.

.. cmdoption:: --repository=URL, -r URL

   The URL of the repository to upload to.  Defaults to
   http://pypi.python.org/pypi (i.e., the main PyPI installation).

.. cmdoption:: --upload-docs

   Also run :command:`upload_docs`.  Mainly useful as a default value in
   :file:`setup.cfg` (on the command line, it's shorter to just type both
   commands).


:command:`upload_docs`
----------------------

Upload HTML documentation to PyPI.

PyPI now supports publishing project documentation at a URI of the form
``http://packages.python.org/<project>``.  :command:`upload_docs`  will create
the necessary zip file out of a documentation directory and will post to the
repository.

Note that to upload the documentation of a project, the corresponding version
must already be registered with PyPI, using the :command:`register` command ---
just like with :command:`upload`.

Assuming there is an ``Example`` project with documentation in the subdirectory
:file:`docs`, for example::

   Example/
      example.py
      setup.cfg
      docs/
         build/
            html/
               index.html
               tips_tricks.html
         conf.py
         index.txt
         tips_tricks.txt

You can simply specify the directory with the HTML files in your
:file:`setup.cfg` file:

.. code-block:: cfg

   [upload_docs]
   upload-dir = docs/build/html


.. program:: packaging upload_docs

.. cmdoption:: --upload-dir

   The directory to be uploaded to the repository. By default documentation
   is searched for in ``docs`` (or ``doc``) directory in project root.

.. cmdoption:: --show-response

   Display the full response text from server; this is useful for debugging
   PyPI problems.

.. cmdoption:: --repository=URL, -r URL

   The URL of the repository to upload to.  Defaults to
   http://pypi.python.org/pypi (i.e., the main PyPI installation).


The install step
================

These commands are used by end-users of a project using :program:`pysetup` or
another compatible installer.  Each command will run the corresponding
:command:`build_*` command and then move the built files to their destination on
the target system.


:command:`install_dist`
-----------------------

Install a distribution, delegating to the other :command:`install_*` commands to
do the work.

.. program:: packaging install_dist

.. cmdoption:: --user

   Install in user site-packages directory (see :PEP:`370`).


:command:`install_data`
-----------------------

Install data files.


:command:`install_distinfo`
---------------------------

Install files recording details of the installation as specified in :PEP:`376`.


:command:`install_headers`
--------------------------

Install C/C++ header files.


:command:`install_lib`
----------------------

Install C library files.


:command:`install_scripts`
--------------------------

Install scripts.
