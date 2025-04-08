.. _using:

========================================================
:mod:`!importlib.metadata` -- Accessing package metadata
========================================================

.. module:: importlib.metadata
   :synopsis: Accessing package metadata

.. versionadded:: 3.8
.. versionchanged:: 3.10
   ``importlib.metadata`` is no longer provisional.

**Source code:** :source:`Lib/importlib/metadata/__init__.py`

``importlib.metadata`` is a library that provides access to
the metadata of an installed `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_,
such as its entry points
or its top-level names (`Import Package <https://packaging.python.org/en/latest/glossary/#term-Import-Package>`_\s, modules, if any).
Built in part on Python's import system, this library
intends to replace similar functionality in the `entry point
API`_ and `metadata API`_ of ``pkg_resources``.  Along with
:mod:`importlib.resources`,
this package can eliminate the need to use the older and less efficient
``pkg_resources`` package.

``importlib.metadata`` operates on third-party *distribution packages*
installed into Python's ``site-packages`` directory via tools such as
:pypi:`pip`.
Specifically, it works with distributions with discoverable
``dist-info`` or ``egg-info`` directories,
and metadata defined by the `Core metadata specifications <https://packaging.python.org/en/latest/specifications/core-metadata/#core-metadata>`_.

.. important::

   These are *not* necessarily equivalent to or correspond 1:1 with
   the top-level *import package* names
   that can be imported inside Python code.
   One *distribution package* can contain multiple *import packages*
   (and single modules),
   and one top-level *import package*
   may map to multiple *distribution packages*
   if it is a namespace package.
   You can use :ref:`packages_distributions() <package-distributions>`
   to get a mapping between them.

By default, distribution metadata can live on the file system
or in zip archives on
:data:`sys.path`.  Through an extension mechanism, the metadata can live almost
anywhere.


.. seealso::

   https://importlib-metadata.readthedocs.io/
      The documentation for ``importlib_metadata``, which supplies a
      backport of ``importlib.metadata``.
      This includes an `API reference
      <https://importlib-metadata.readthedocs.io/en/latest/api.html>`__
      for this module's classes and functions,
      as well as a `migration guide
      <https://importlib-metadata.readthedocs.io/en/latest/migration.html>`__
      for existing users of ``pkg_resources``.


Overview
========

Let's say you wanted to get the version string for a
`Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_ you've installed
using ``pip``.  We start by creating a virtual environment and installing
something into it:

.. code-block:: shell-session

    $ python -m venv example
    $ source example/bin/activate
    (example) $ python -m pip install wheel

You can get the version string for ``wheel`` by running the following:

.. code-block:: pycon

    (example) $ python
    >>> from importlib.metadata import version  # doctest: +SKIP
    >>> version('wheel')  # doctest: +SKIP
    '0.32.3'

You can also get a collection of entry points selectable by properties of the EntryPoint (typically 'group' or 'name'), such as
``console_scripts``, ``distutils.commands`` and others.  Each group contains a
collection of :ref:`EntryPoint <entry-points>` objects.

You can get the :ref:`metadata for a distribution <metadata>`::

    >>> list(metadata('wheel'))  # doctest: +SKIP
    ['Metadata-Version', 'Name', 'Version', 'Summary', 'Home-page', 'Author', 'Author-email', 'Maintainer', 'Maintainer-email', 'License', 'Project-URL', 'Project-URL', 'Project-URL', 'Keywords', 'Platform', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Requires-Python', 'Provides-Extra', 'Requires-Dist', 'Requires-Dist']

You can also get a :ref:`distribution's version number <version>`, list its
:ref:`constituent files <files>`, and get a list of the distribution's
:ref:`requirements`.


.. exception:: PackageNotFoundError

   Subclass of :class:`ModuleNotFoundError` raised by several functions in this
   module when queried for a distribution package which is not installed in the
   current Python environment.


Functional API
==============

This package provides the following functionality via its public API.


.. _entry-points:

Entry points
------------

.. function:: entry_points(**select_params)

   Returns a :class:`EntryPoints` instance describing entry points for the
   current environment. Any given keyword parameters are passed to the
   :meth:`!select` method for comparison to the attributes of
   the individual entry point definitions.

   Note: it is not currently possible to query for entry points based on
   their :attr:`!EntryPoint.dist` attribute (as different :class:`!Distribution`
   instances do not currently compare equal, even if they have the same attributes)

.. class:: EntryPoints

   Details of a collection of installed entry points.

   Also provides a ``.groups`` attribute that reports all identified entry
   point groups, and a ``.names`` attribute that reports all identified entry
   point names.

.. class:: EntryPoint

   Details of an installed entry point.

   Each :class:`!EntryPoint` instance has ``.name``, ``.group``, and ``.value``
   attributes and a ``.load()`` method to resolve the value. There are also
   ``.module``, ``.attr``, and ``.extras`` attributes for getting the
   components of the ``.value`` attribute, and ``.dist`` for obtaining
   information regarding the distribution package that provides the entry point.

Query all entry points::

    >>> eps = entry_points()  # doctest: +SKIP

The :func:`!entry_points` function returns a :class:`!EntryPoints` object,
a collection of all :class:`!EntryPoint` objects with ``names`` and ``groups``
attributes for convenience::

    >>> sorted(eps.groups)  # doctest: +SKIP
    ['console_scripts', 'distutils.commands', 'distutils.setup_keywords', 'egg_info.writers', 'setuptools.installation']

:class:`!EntryPoints` has a :meth:`!select` method to select entry points
matching specific properties. Select entry points in the
``console_scripts`` group::

    >>> scripts = eps.select(group='console_scripts')  # doctest: +SKIP

Equivalently, since :func:`!entry_points` passes keyword arguments
through to select::

    >>> scripts = entry_points(group='console_scripts')  # doctest: +SKIP

Pick out a specific script named "wheel" (found in the wheel project)::

    >>> 'wheel' in scripts.names  # doctest: +SKIP
    True
    >>> wheel = scripts['wheel']  # doctest: +SKIP

Equivalently, query for that entry point during selection::

    >>> (wheel,) = entry_points(group='console_scripts', name='wheel')  # doctest: +SKIP
    >>> (wheel,) = entry_points().select(group='console_scripts', name='wheel')  # doctest: +SKIP

Inspect the resolved entry point::

    >>> wheel  # doctest: +SKIP
    EntryPoint(name='wheel', value='wheel.cli:main', group='console_scripts')
    >>> wheel.module  # doctest: +SKIP
    'wheel.cli'
    >>> wheel.attr  # doctest: +SKIP
    'main'
    >>> wheel.extras  # doctest: +SKIP
    []
    >>> main = wheel.load()  # doctest: +SKIP
    >>> main  # doctest: +SKIP
    <function main at 0x103528488>

The ``group`` and ``name`` are arbitrary values defined by the package author
and usually a client will wish to resolve all entry points for a particular
group.  Read `the setuptools docs
<https://setuptools.pypa.io/en/latest/userguide/entry_point.html>`_
for more information on entry points, their definition, and usage.

.. versionchanged:: 3.12
   The "selectable" entry points were introduced in ``importlib_metadata``
   3.6 and Python 3.10. Prior to those changes, ``entry_points`` accepted
   no parameters and always returned a dictionary of entry points, keyed
   by group. With ``importlib_metadata`` 5.0 and Python 3.12,
   ``entry_points`` always returns an ``EntryPoints`` object. See
   :pypi:`backports.entry_points_selectable`
   for compatibility options.

.. versionchanged:: 3.13
   ``EntryPoint`` objects no longer present a tuple-like interface
   (:meth:`~object.__getitem__`).

.. _metadata:

Distribution metadata
---------------------

.. function:: metadata(distribution_name)

   Return the distribution metadata corresponding to the named
   distribution package as a :class:`PackageMetadata` instance.

   Raises :exc:`PackageNotFoundError` if the named distribution
   package is not installed in the current Python environment.

.. class:: PackageMetadata

   A concrete implementation of the
   `PackageMetadata protocol <https://importlib-metadata.readthedocs.io/en/latest/api.html#importlib_metadata.PackageMetadata>`_.

   In addition to providing the defined protocol methods and attributes, subscripting
   the instance is equivalent to calling the :meth:`!get` method.

Every `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_
includes some metadata, which you can extract using the :func:`!metadata` function::

    >>> wheel_metadata = metadata('wheel')  # doctest: +SKIP

The keys of the returned data structure name the metadata keywords, and
the values are returned unparsed from the distribution metadata::

    >>> wheel_metadata['Requires-Python']  # doctest: +SKIP
    '>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*'

:class:`PackageMetadata` also presents a :attr:`!json` attribute that returns
all the metadata in a JSON-compatible form per :PEP:`566`::

    >>> wheel_metadata.json['requires_python']
    '>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*'

The full set of available metadata is not described here.
See the PyPA `Core metadata specification <https://packaging.python.org/en/latest/specifications/core-metadata/#core-metadata>`_ for additional details.

.. versionchanged:: 3.10
   The ``Description`` is now included in the metadata when presented
   through the payload. Line continuation characters have been removed.

   The ``json`` attribute was added.


.. _version:

Distribution versions
---------------------

.. function:: version(distribution_name)

   Return the installed distribution package
   `version <https://packaging.python.org/en/latest/specifications/core-metadata/#version>`__
   for the named distribution package.

   Raises :exc:`PackageNotFoundError` if the named distribution
   package is not installed in the current Python environment.

The :func:`!version` function is the quickest way to get a
`Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_'s version
number, as a string::

    >>> version('wheel')  # doctest: +SKIP
    '0.32.3'


.. _files:

Distribution files
------------------

.. function:: files(distribution_name)

   Return the full set of files contained within the named
   distribution package.

   Raises :exc:`PackageNotFoundError` if the named distribution
   package is not installed in the current Python environment.

   Returns :const:`None` if the distribution is found but the installation
   database records reporting the files associated with the distribuion package
   are missing.

.. class:: PackagePath

    A :class:`pathlib.PurePath` derived object with additional ``dist``,
    ``size``, and ``hash`` properties corresponding to the distribution
    package's installation metadata for that file.

The :func:`!files` function takes a
`Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_
name and returns all of the files installed by this distribution. Each file is reported
as a :class:`PackagePath` instance. For example::

    >>> util = [p for p in files('wheel') if 'util.py' in str(p)][0]  # doctest: +SKIP
    >>> util  # doctest: +SKIP
    PackagePath('wheel/util.py')
    >>> util.size  # doctest: +SKIP
    859
    >>> util.dist  # doctest: +SKIP
    <importlib.metadata._hooks.PathDistribution object at 0x101e0cef0>
    >>> util.hash  # doctest: +SKIP
    <FileHash mode: sha256 value: bYkw5oMccfazVCoYQwKkkemoVyMAFoR34mmKBx8R1NI>

Once you have the file, you can also read its contents::

    >>> print(util.read_text())  # doctest: +SKIP
    import base64
    import sys
    ...
    def as_bytes(s):
        if isinstance(s, text_type):
            return s.encode('utf-8')
        return s

You can also use the :meth:`!locate` method to get the absolute
path to the file::

    >>> util.locate()  # doctest: +SKIP
    PosixPath('/home/gustav/example/lib/site-packages/wheel/util.py')

In the case where the metadata file listing files
(``RECORD`` or ``SOURCES.txt``) is missing, :func:`!files` will
return :const:`None`. The caller may wish to wrap calls to
:func:`!files` in `always_iterable
<https://more-itertools.readthedocs.io/en/stable/api.html#more_itertools.always_iterable>`_
or otherwise guard against this condition if the target
distribution is not known to have the metadata present.

.. _requirements:

Distribution requirements
-------------------------

.. function:: requires(distribution_name)

   Return the declared dependency specifiers for the named
   distribution package.

   Raises :exc:`PackageNotFoundError` if the named distribution
   package is not installed in the current Python environment.

To get the full set of requirements for a `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_,
use the :func:`!requires`
function::

    >>> requires('wheel')  # doctest: +SKIP
    ["pytest (>=3.0.0) ; extra == 'test'", "pytest-cov ; extra == 'test'"]


.. _package-distributions:
.. _import-distribution-package-mapping:

Mapping import to distribution packages
---------------------------------------

.. function:: packages_distributions()

   Return a mapping from the top level module and import package
   names found via :data:`sys.meta_path` to the names of the distribution
   packages (if any) that provide the corresponding files.

   To allow for namespace packages (which may have members provided by
   multiple distribution packages), each top level import name maps to a
   list of distribution names rather than mapping directly to a single name.

A convenience method to resolve the `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_
name (or names, in the case of a namespace package)
that provide each importable top-level
Python module or `Import Package <https://packaging.python.org/en/latest/glossary/#term-Import-Package>`_::

    >>> packages_distributions()
    {'importlib_metadata': ['importlib-metadata'], 'yaml': ['PyYAML'], 'jaraco': ['jaraco.classes', 'jaraco.functools'], ...}

Some editable installs, `do not supply top-level names
<https://github.com/pypa/packaging-problems/issues/609>`_, and thus this
function is not reliable with such installs.

.. versionadded:: 3.10

.. _distributions:

Distributions
=============

.. function:: distribution(distribution_name)

   Return a :class:`Distribution` instance describing the named
   distribution package.

   Raises :exc:`PackageNotFoundError` if the named distribution
   package is not installed in the current Python environment.

.. class:: Distribution

   Details of an installed distribution package.

   Note: different :class:`!Distribution` instances do not currently compare
   equal, even if they relate to the same installed distribution and
   accordingly have the same attributes.

While the module level API described above is the most common and convenient usage,
you can get all of that information from the :class:`!Distribution` class.
:class:`!Distribution` is an abstract object that represents the metadata for
a Python `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_.
You can get the concrete :class:`!Distribution` subclass instance for an installed
distribution package by calling the :func:`distribution` function::

    >>> from importlib.metadata import distribution  # doctest: +SKIP
    >>> dist = distribution('wheel')  # doctest: +SKIP
    >>> type(dist)  # doctest: +SKIP
    <class 'importlib.metadata.PathDistribution'>

Thus, an alternative way to get the version number is through the
:class:`!Distribution` instance::

    >>> dist.version  # doctest: +SKIP
    '0.32.3'

There are all kinds of additional metadata available on :class:`!Distribution`
instances::

    >>> dist.metadata['Requires-Python']  # doctest: +SKIP
    '>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*'
    >>> dist.metadata['License']  # doctest: +SKIP
    'MIT'

For editable packages, an ``origin`` property may present :pep:`610`
metadata::

    >>> dist.origin.url
    'file:///path/to/wheel-0.32.3.editable-py3-none-any.whl'

The full set of available metadata is not described here.
See the PyPA `Core metadata specification <https://packaging.python.org/en/latest/specifications/core-metadata/#core-metadata>`_ for additional details.

.. versionadded:: 3.13
   The ``.origin`` property was added.

Distribution Discovery
======================

By default, this package provides built-in support for discovery of metadata
for file system and zip file `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_\s.
This metadata finder search defaults to ``sys.path``, but varies slightly in how it interprets those values from how other import machinery does. In particular:

- ``importlib.metadata`` does not honor :class:`bytes` objects on ``sys.path``.
- ``importlib.metadata`` will incidentally honor :py:class:`pathlib.Path` objects on ``sys.path`` even though such values will be ignored for imports.


Extending the search algorithm
==============================

Because `Distribution Package <https://packaging.python.org/en/latest/glossary/#term-Distribution-Package>`_ metadata
is not available through :data:`sys.path` searches, or
package loaders directly,
the metadata for a distribution is found through import
system :ref:`finders <finders-and-loaders>`.  To find a distribution package's metadata,
``importlib.metadata`` queries the list of :term:`meta path finders <meta path finder>` on
:data:`sys.meta_path`.

By default ``importlib.metadata`` installs a finder for distribution packages
found on the file system.
This finder doesn't actually find any *distributions*,
but it can find their metadata.

The abstract class :py:class:`importlib.abc.MetaPathFinder` defines the
interface expected of finders by Python's import system.
``importlib.metadata`` extends this protocol by looking for an optional
``find_distributions`` callable on the finders from
:data:`sys.meta_path` and presents this extended interface as the
``DistributionFinder`` abstract base class, which defines this abstract
method::

    @abc.abstractmethod
    def find_distributions(context=DistributionFinder.Context()):
        """Return an iterable of all Distribution instances capable of
        loading the metadata for packages for the indicated ``context``.
        """

The ``DistributionFinder.Context`` object provides ``.path`` and ``.name``
properties indicating the path to search and name to match and may
supply other relevant context.

What this means in practice is that to support finding distribution package
metadata in locations other than the file system, subclass
``Distribution`` and implement the abstract methods. Then from
a custom finder, return instances of this derived ``Distribution`` in the
``find_distributions()`` method.

Example
-------

Consider for example a custom finder that loads Python
modules from a database::

    class DatabaseImporter(importlib.abc.MetaPathFinder):
        def __init__(self, db):
            self.db = db

        def find_spec(self, fullname, target=None) -> ModuleSpec:
            return self.db.spec_from_name(fullname)

    sys.meta_path.append(DatabaseImporter(connect_db(...)))

That importer now presumably provides importable modules from a
database, but it provides no metadata or entry points. For this
custom importer to provide metadata, it would also need to implement
``DistributionFinder``::

    from importlib.metadata import DistributionFinder

    class DatabaseImporter(DistributionFinder):
        ...

        def find_distributions(self, context=DistributionFinder.Context()):
            query = dict(name=context.name) if context.name else {}
            for dist_record in self.db.query_distributions(query):
                yield DatabaseDistribution(dist_record)

In this way, ``query_distributions`` would return records for
each distribution served by the database matching the query. For
example, if ``requests-1.0`` is in the database, ``find_distributions``
would yield a ``DatabaseDistribution`` for ``Context(name='requests')``
or ``Context(name=None)``.

For the sake of simplicity, this example ignores ``context.path``\. The
``path`` attribute defaults to ``sys.path`` and is the set of import paths to
be considered in the search. A ``DatabaseImporter`` could potentially function
without any concern for a search path. Assuming the importer does no
partitioning, the "path" would be irrelevant. In order to illustrate the
purpose of ``path``, the example would need to illustrate a more complex
``DatabaseImporter`` whose behavior varied depending on
``sys.path``/``PYTHONPATH``. In that case, the ``find_distributions`` should
honor the ``context.path`` and only yield ``Distribution``\ s pertinent to that
path.

``DatabaseDistribution``, then, would look something like::

    class DatabaseDistribution(importlib.metadata.Distribution):
        def __init__(self, record):
            self.record = record

        def read_text(self, filename):
            """
            Read a file like "METADATA" for the current distribution.
            """
            if filename == "METADATA":
                return f"""Name: {self.record.name}
    Version: {self.record.version}
    """
            if filename == "entry_points.txt":
                return "\n".join(
                  f"""[{ep.group}]\n{ep.name}={ep.value}"""
                  for ep in self.record.entry_points)

        def locate_file(self, path):
            raise RuntimeError("This distribution has no file system")

This basic implementation should provide metadata and entry points for
packages served by the ``DatabaseImporter``, assuming that the
``record`` supplies suitable ``.name``, ``.version``, and
``.entry_points`` attributes.

The ``DatabaseDistribution`` may also provide other metadata files, like
``RECORD`` (required for ``Distribution.files``) or override the
implementation of ``Distribution.files``. See the source for more inspiration.


.. _`entry point API`: https://setuptools.readthedocs.io/en/latest/pkg_resources.html#entry-points
.. _`metadata API`: https://setuptools.readthedocs.io/en/latest/pkg_resources.html#metadata-api
