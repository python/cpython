Incremental
===========

|travis|
|pypi|
|coverage|

Incremental is a small library that versions your Python projects.

API documentation can be found `here <https://hawkowl.github.io/incremental/docs/>`_.


Quick Start
-----------

Add this to your ``setup.py``\ 's ``setup()`` call, removing any other versioning arguments:

.. code::

   setup(
       use_incremental=True,
       setup_requires=['incremental'],
       install_requires=['incremental'], # along with any other install dependencies
       ...
   }


Then run ``python -m incremental.update <projectname> --create`` (you will need ``click`` installed from PyPI).
It will create a file in your package named ``_version.py`` and look like this:

.. code::

   from incremental import Version

   __version__ = Version("widgetbox", 17, 1, 0)
   __all__ = ["__version__"]


Then, so users of your project can find your version, in your root package's ``__init__.py`` add:

.. code::

   from ._version import __version__


Subsequent installations of your project will then use Incremental for versioning.


Incremental Versions
--------------------

``incremental.Version`` is a class that represents a version of a given project.
It is made up of the following elements (which are given during instantiation):

- ``package`` (required), the name of the package this ``Version`` represents.
- ``major``, ``minor``, ``micro`` (all required), the X.Y.Z of your project's ``Version``.
- ``release_candidate`` (optional), set to 0 or higher to mark this ``Version`` being of a release candidate (also sometimes called a "prerelease").
- ``dev`` (optional), set to 0 or higher to mark this ``Version`` as a development release.

You can extract a PEP-440 compatible version string by using the following methods:

- ``.local()``, which returns a ``str`` containing the full version plus any Git or SVN information, if available. An example output would be ``"17.1.1rc1+r123"`` or ``"3.7.0+rb2e812003b5d5fcf08efd1dffed6afa98d44ac8c"``.
- ``.public()``, which returns a ``str`` containing the full version, without any Git or SVN information. This is the version you should provide to users, or publicly use. An example output would be ``"13.2.0"``, ``"17.1.2dev1"``, or ``"18.8.0rc2"``.

Calling ``repr()`` with a ``Version`` will give a Python-source-code representation of it, and calling ``str()`` with a ``Version`` will provide a string similar to ``'[Incremental, version 16.10.1]'``.


Updating
--------

Incremental includes a tool to automate updating your Incremental-using project's version called ``incremental.update``.
It updates the ``_version.py`` file and automatically updates some uses of Incremental versions from an indeterminate version to the current one.
It requires ``click`` from PyPI.

``python -m incremental.update <projectname>`` will perform updates on that package.
The commands that can be given after that will determine what the next version is.

- ``--newversion=<version>``, to set the project version to a fully-specified version (like 1.2.3, or 17.1.0dev1).
- ``--rc``, to set the project version to ``<year-2000>.<month>.0rc1`` if the current version is not a release candidate, or bump the release candidate number by 1 if it is.
- ``--dev``, to set the project development release number to 0 if it is not a development release, or bump the development release number by 1 if it is.
- ``--patch``, to increment the patch number of the release. This will also reset the release candidate number, pass ``--rc`` at the same time to increment the patch number and make it a release candidate.

If you give no arguments, it will strip the release candidate number, making it a "full release".

Incremental supports "indeterminate" versions, as a stand-in for the next "full" version. This can be used when the version which will be displayed to the end-user is unknown (for example "introduced in" or "deprecated in"). Incremental supports the following indeterminate versions:

- ``Version("<projectname>", "NEXT", 0, 0)``
- ``<projectname> NEXT``

When you run ``python -m incremental.update <projectname> --rc``, these will be updated to real versions (assuming the target final version is 17.1.0):

- ``Version("<projectname>", 17, 1, 0, release_candidate=1)``
- ``<projectname> 17.1.0rc1``

Once the final version is made, it will become:

- ``Version("<projectname>", 17, 1, 0)``
- ``<projectname> 17.1.0``


.. |coverage| image:: https://codecov.io/github/hawkowl/incremental/coverage.svg?branch=master
.. _coverage: https://codecov.io/github/hawkowl/incremental

.. |travis| image:: https://travis-ci.org/hawkowl/incremental.svg?branch=master
.. _travis: http://travis-ci.org/hawkowl/incremental

.. |pypi| image:: http://img.shields.io/pypi/v/incremental.svg
.. _pypi: https://pypi.python.org/pypi/incremental


