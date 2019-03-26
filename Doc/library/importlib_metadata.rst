.. _using:

==========================
 Using importlib_metadata
==========================

``importlib_metadata`` is a library that provides for access to installed
package metadata.  Built in part on Python's import system, this library
intends to replace similar functionality in the `entry point
API`_ and `metadata API`_ of ``pkg_resources``.  Along with
``importlib.resources`` in `Python 3.7
and newer`_ (backported as `importlib_resources`_ for older versions of
Python), this can eliminate the need to use the older and less efficient
``pkg_resources`` package.

By "installed package" we generally mean a third-party package installed into
Python's ``site-packages`` directory via tools such as `pip
<https://pypi.org/project/pip/>`_.  Specifically,
it means a package with either a discoverable ``dist-info`` or ``egg-info``
directory, and metadata defined by `PEP 566`_ or its older specifications.
By default, package metadata can live on the file system or in wheels on
``sys.path``.  Through an extension mechanism, the metadata can live almost
anywhere.


Overview
========

Let's say you wanted to get the version string for a package you've installed
using ``pip``.  We start by creating a virtual environment and installing
something into it::

    $ python3 -m venv example
    $ source example/bin/activate
    (example) $ pip install importlib_metadata
    (example) $ pip install wheel

You can get the version string for ``wheel`` by running the following::

    (example) $ python
    >>> from importlib_metadata import version
    >>> version('wheel')
    '0.32.3'

You can also get the set of entry points keyed by group, such as
``console_scripts``, ``distutils.commands`` and others.  Each group contains a
sequence of :ref:`EntryPoint <entry-points>` objects.

You can get the :ref:`metadata for a distribution <metadata>`::

    >>> list(metadata('wheel'))
    ['Metadata-Version', 'Name', 'Version', 'Summary', 'Home-page', 'Author', 'Author-email', 'Maintainer', 'Maintainer-email', 'License', 'Project-URL', 'Project-URL', 'Project-URL', 'Keywords', 'Platform', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Classifier', 'Requires-Python', 'Provides-Extra', 'Requires-Dist', 'Requires-Dist']

You can also get a :ref:`distribution's version number <version>`, list its
:ref:`constituent files <files>`_, and get a list of the distribution's
:ref:`requirements`_.


Distributions
=============

.. CAUTION:: The ``Distribution`` class described here may or may not end up
             in the final stable public API.  Consider this class `provisional
             <https://www.python.org/dev/peps/pep-0411/>`_ until the 1.0
             release.

While the above API is the most common and convenient usage, you can get all
of that information from the ``Distribution`` class.  A ``Distribution`` is an
abstract object that represents the metadata for a Python package.  You can
get the ``Distribution`` instance::

    >>> from importlib_metadata import distribution
    >>> dist = distribution('wheel')

Thus, an alternative way to get the version number is through the
``Distribution`` instance::

    >>> dist.version
    '0.32.3'

There are all kinds of additional metadata available on the ``Distribution``
instance::

    >>> d.metadata['Requires-Python']
    '>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*'
    >>> d.metadata['License']
    'MIT'

The full set of available metadata is not described here.  See `PEP 566
<https://www.python.org/dev/peps/pep-0566/>`_ for additional details.


Functional API
==============

This package provides the following functionality via its public API.


.. _entry-points::

Entry points
------------

The ``entry_points()`` function returns a dictionary of all entry points,
keyed by group.  Entry points are represented by ``EntryPoint`` instances;
each ``EntryPoint`` has a ``.name``, ``.group``, and ``.value`` attributes and
a ``.load()`` method to resolve the value.

    >>> eps = entry_points()
    >>> list(eps)
    ['console_scripts', 'distutils.commands', 'distutils.setup_keywords', 'egg_info.writers', 'setuptools.installation']
    >>> scripts = eps['console_scripts']
    >>> wheel = [ep for ep in scripts if ep.name == 'wheel'][0]
    >>> wheel
    EntryPoint(name='wheel', value='wheel.cli:main', group='console_scripts')
    >>> main = wheel.load()
    >>> main
    <function main at 0x103528488>

The ``group`` and ``name`` are arbitrary values defined by the package author
and usually a client will wish to resolve all entry points for a particular
group.  Read `the setuptools docs
<https://setuptools.readthedocs.io/en/latest/setuptools.html#dynamic-discovery-of-services-and-plugins>`_
for more information on entrypoints, their definition, and usage.


.. _metadata::

Distribution metadata
---------------------

Every distribution includes some metadata, which you can extract using the
``metadata()`` function::

    >>> wheel_metadata = metadata('wheel')

The keys of the returned data structure [#f1]_ name the metadata keywords, and
their values are returned unparsed from the distribution metadata::

    >>> wheel_metadata['Requires-Python']
    '>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*'


.. _version::

Distribution versions
---------------------

The ``version()`` function is the quickest way to get a distribution's version
number, as a string::

    >>> version('wheel')
    '0.32.3'


.. _files::

Distribution files
------------------

You can also get the full set of files contained within a distribution.  The
``files()`` function takes a distribution package name and returns all of the
files installed by this distribution.  Each file object returned is a
``PackagePath``, a `pathlib.Path`_ derived object with additional ``dist``,
``size``, and ``hash`` properties as indicated by the metadata.  For example::

    >>> util = [p for p in files('wheel') if 'util.py' in str(p)][0]
    >>> util
    PackagePath('wheel/util.py')
    >>> util.size
    859
    >>> util.dist
    <importlib_metadata._hooks.PathDistribution object at 0x101e0cef0>
    >>> util.hash
    <FileHash mode: sha256 value: bYkw5oMccfazVCoYQwKkkemoVyMAFoR34mmKBx8R1NI>

Once you have the file, you can also read its contents::

    >>> print(util.read_text())
    import base64
    import sys
    ...
    def as_bytes(s):
        if isinstance(s, text_type):
            return s.encode('utf-8')
        return s


.. _requirements::

Distribution requirements
-------------------------

To get the full set of requirements for a distribution, use the ``requires()``
function.  Note that this returns an iterator::

    >>> list(requires('wheel'))
    ["pytest (>=3.0.0) ; extra == 'test'"]



Extending the search algorithm
==============================

Because package metadata is not available through ``sys.path`` searches, or
package loaders directly, the metadata for a package is found through import
system `finders`_.  To find a distribution package's metadata,
``importlib_metadata`` queries the list of `meta path finders`_ on
`sys.meta_path`_.

By default ``importlib_metadata`` installs a finder for distribution packages
found on the file system.  This finder doesn't actually find any *packages*,
but it can find the packages' metadata.

The abstract class :py:class:`importlib.abc.MetaPathFinder` defines the
interface expected of finders by Python's import system.
``importlib_metadata`` extends this protocol by looking for an optional
``find_distributions`` callable on the finders from
``sys.meta_path``.  If the finder has this method, it must return
an iterator over instances of the ``Distribution`` abstract class. This
method must have the signature::

    def find_distributions(name=None, path=sys.path):
        """Return an iterable of all Distribution instances capable of
        loading the metadata for packages matching the name
        (or all names if not supplied) along the paths in the list
        of directories ``path`` (defaults to sys.path).
        """

What this means in practice is that to support finding distribution package
metadata in locations other than the file system, you should derive from
``Distribution`` and implement the ``load_metadata()`` method.  This takes a
single argument which is the name of the package whose metadata is being
found.  This instance of the ``Distribution`` base abstract class is what your
finder's ``find_distributions()`` method should return.


.. _`entry point API`: https://setuptools.readthedocs.io/en/latest/pkg_resources.html#entry-points
.. _`metadata API`: https://setuptools.readthedocs.io/en/latest/pkg_resources.html#metadata-api
.. _`Python 3.7 and newer`: https://docs.python.org/3/library/importlib.html#module-importlib.resources
.. _`importlib_resources`: https://importlib-resources.readthedocs.io/en/latest/index.html
.. _`PEP 566`: https://www.python.org/dev/peps/pep-0566/
.. _`finders`: https://docs.python.org/3/reference/import.html#finders-and-loaders
.. _`meta path finders`: https://docs.python.org/3/glossary.html#term-meta-path-finder
.. _`sys.meta_path`: https://docs.python.org/3/library/sys.html#sys.meta_path
.. _`pathlib.Path`: https://docs.python.org/3/library/pathlib.html#pathlib.Path


.. rubric:: Footnotes

.. [#f1] Technically, the returned distribution metadata object is an
         `email.message.Message
         <https://docs.python.org/3/library/email.message.html#email.message.EmailMessage>`_
         instance, but this is an implementation detail, and not part of the
         stable API.  You should only use dictionary-like methods and syntax
         to access the metadata contents.
