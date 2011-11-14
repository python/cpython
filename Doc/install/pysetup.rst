.. _packaging-pysetup:

================
Pysetup Tutorial
================

Getting started
---------------

Pysetup is a simple script that supports the following features:

- install, remove, list, and verify Python packages;
- search for available packages on PyPI or any *Simple Index*;
- verify installed packages (md5sum, installed files, version).


Finding out what's installed
----------------------------

Pysetup makes it easy to find out what Python packages are installed::

   $ pysetup list virtualenv
   'virtualenv' 1.6 at '/opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info'

   $ pysetup list
   'pyverify' 0.8.1 at '/opt/python3.3/lib/python3.3/site-packages/pyverify-0.8.1.dist-info'
   'virtualenv' 1.6 at '/opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info'
   ...


Installing a distribution
-------------------------

Pysetup can install a Python project from the following sources:

- PyPI and Simple Indexes;
- source directories containing a valid :file:`setup.py` or :file:`setup.cfg`;
- distribution source archives (:file:`project-1.0.tar.gz`, :file:`project-1.0.zip`);
- HTTP (http://host/packages/project-1.0.tar.gz).


Installing from PyPI and Simple Indexes::

   $ pysetup install project
   $ pysetup install project==1.0

Installing from a distribution source archive::

   $ pysetup install project-1.0.tar.gz

Installing from a source directory containing a valid :file:`setup.py` or
:file:`setup.cfg`::

   $ cd path/to/source/directory
   $ pysetup install

   $ pysetup install path/to/source/directory

Installing from HTTP::

   $ pysetup install http://host/packages/project-1.0.tar.gz


Retrieving metadata
-------------------

You can gather metadata from two sources, a project's source directory or an
installed distribution. The `pysetup metadata` command can retrieve one or
more metadata fields using the `-f` option and a metadata field as the
argument. ::

   $ pysetup metadata virtualenv -f version -f name
   Version:
       1.6
   Name:
       virtualenv

   $ pysetup metadata virtualenv
   Metadata-Version:
       1.0
   Name:
       virtualenv
   Version:
       1.6
   Platform:
       UNKNOWN
   Summary:
       Virtual Python Environment builder
   ...

.. seealso::

   There are three metadata versions, 1.0, 1.1, and 1.2. The following PEPs
   describe specifics of the field names, and their semantics and usage.  1.0
   :PEP:`241`, 1.1 :PEP:`314`, and 1.2 :PEP:`345`


Removing a distribution
-----------------------

You can remove one or more installed distributions using the `pysetup remove`
command::

   $ pysetup remove virtualenv
   removing 'virtualenv':
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/dependency_links.txt
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/entry_points.txt
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/not-zip-safe
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/PKG-INFO
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/SOURCES.txt
     /opt/python3.3/lib/python3.3/site-packages/virtualenv-1.6-py3.3.egg-info/top_level.txt
   Proceed (y/n)? y
   success: removed 6 files and 1 dirs

The optional '-y' argument auto confirms, skipping the conformation prompt::

  $ pysetup remove virtualenv -y


Getting help
------------

All pysetup actions take the `-h` and `--help` options which prints the commands
help string to stdout. ::

   $ pysetup remove -h
   Usage: pysetup remove dist [-y]
      or: pysetup remove --help

   Uninstall a Python package.

   positional arguments:
      dist  installed distribution name

   optional arguments:
      -y  auto confirm package removal

Getting a list of all pysetup actions and global options::

   $ pysetup --help
   Usage: pysetup [options] action [action_options]

   Actions:
       run: Run one or several commands
       metadata: Display the metadata of a project
       install: Install a project
       remove: Remove a project
       search: Search for a project in the indexes
       list: List installed projects
       graph: Display a graph
       create: Create a project
       generate-setup: Generate a backward-compatible setup.py

   To get more help on an action, use:

       pysetup action --help

   Global options:
       --verbose (-v)  run verbosely (default)
       --quiet (-q)    run quietly (turns verbosity off)
       --dry-run (-n)  don't actually do anything
       --help (-h)     show detailed help message
       --no-user-cfg   ignore pydistutils.cfg in your home directory
       --version       Display the version
