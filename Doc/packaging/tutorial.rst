==================
Packaging tutorial
==================

Welcome to the Packaging tutorial! We will learn how to use Packaging
to package your project.

.. TODO merge with introduction.rst


Getting started
---------------

Packaging works with the *setup.cfg* file. It contains all the metadata for
your project, as defined in PEP 345, but also declare what your project
contains.

Let's say you have a project called *CLVault* containing one package called
*clvault*, and a few scripts inside. You can use the *pysetup* script to create
a *setup.cfg* file for the project. The script will ask you a few questions::

   $ mkdir CLVault
   $ cd CLVault
   $ pysetup create
   Project name [CLVault]:
   Current version number: 0.1
   Package description:
   >Command-line utility to store and retrieve passwords
   Author name: Tarek Ziade
   Author e-mail address: tarek@ziade.org
   Project Home Page: http://bitbucket.org/tarek/clvault
   Do you want to add a package ? (y/n): y
   Package name: clvault
   Do you want to add a package ? (y/n): n
   Do you want to set Trove classifiers? (y/n): y
   Please select the project status:

   1 - Planning
   2 - Pre-Alpha
   3 - Alpha
   4 - Beta
   5 - Production/Stable
   6 - Mature
   7 - Inactive

   Status: 3
   What license do you use: GPL
   Matching licenses:

   1) License :: OSI Approved :: GNU General Public License (GPL)
   2) License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)

   Type the number of the license you wish to use or ? to try again:: 1
   Do you want to set other trove identifiers (y/n) [n]: n
   Wrote "setup.cfg".


A setup.cfg file is created, containing the metadata of your project and the
list of the packages it contains::

   $ cat setup.cfg
   [metadata]
   name = CLVault
   version = 0.1
   author = Tarek Ziade
   author_email = tarek@ziade.org
   description = Command-line utility to store and retrieve passwords
   home_page = http://bitbucket.org/tarek/clvault

   classifier = Development Status :: 3 - Alpha
       License :: OSI Approved :: GNU General Public License (GPL)

   [files]
   packages = clvault


Our project will depend on the *keyring* project. Let's add it in the
[metadata] section::

   [metadata]
   ...
   requires_dist =
       keyring


Running commands
----------------

You can run useful commands on your project once the setup.cfg file is ready:

- sdist: creates a source distribution
- register: register your project to PyPI
- upload: upload the distribution to PyPI
- install_dist: install it

All commands are run using the run script::

   $ pysetup run install_dist
   $ pysetup run sdist
   $ pysetup run upload

If you want to push a source distribution of your project to PyPI, do::

   $ pysetup run sdist register upload


Installing the project
----------------------

The project can be installed by manually running the packaging install command::

   $ pysetup run install_dist
