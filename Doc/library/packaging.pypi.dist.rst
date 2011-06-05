:mod:`packaging.pypi.dist` --- Classes representing query results
=================================================================

.. module:: packaging.pypi.dist
   :synopsis: Classes representing the results of queries to indexes.


Information coming from the indexes is held in instances of the classes defined
in this module.

Keep in mind that each project (eg. FooBar) can have several releases
(eg. 1.1, 1.2, 1.3), and each of these releases can be provided in multiple
distributions (eg. a source distribution, a binary one, etc).


ReleaseInfo
-----------

Each release has a project name, version, metadata, and related distributions.

This information is stored in :class:`ReleaseInfo`
objects.

.. class:: ReleaseInfo


DistInfo
---------

:class:`DistInfo` is a simple class that contains
information related to distributions; mainly the URLs where distributions
can be found.

.. class:: DistInfo


ReleasesList
------------

The :mod:`~packaging.pypi.dist` module provides a class which works
with lists of :class:`ReleaseInfo` classes;
used to filter and order results.

.. class:: ReleasesList


Example usage
-------------

Build a list of releases and order them
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Assuming we have a list of releases::

   >>> from packaging.pypi.dist import ReleasesList, ReleaseInfo
   >>> fb10 = ReleaseInfo("FooBar", "1.0")
   >>> fb11 = ReleaseInfo("FooBar", "1.1")
   >>> fb11a = ReleaseInfo("FooBar", "1.1a1")
   >>> ReleasesList("FooBar", [fb11, fb11a, fb10])
   >>> releases.sort_releases()
   >>> releases.get_versions()
   ['1.1', '1.1a1', '1.0']
   >>> releases.add_release("1.2a1")
   >>> releases.get_versions()
   ['1.1', '1.1a1', '1.0', '1.2a1']
   >>> releases.sort_releases()
   ['1.2a1', '1.1', '1.1a1', '1.0']
   >>> releases.sort_releases(prefer_final=True)
   >>> releases.get_versions()
   ['1.1', '1.0', '1.2a1', '1.1a1']


Add distribution related information to releases
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It's easy to add distribution information to releases::

   >>> from packaging.pypi.dist import ReleasesList, ReleaseInfo
   >>> r = ReleaseInfo("FooBar", "1.0")
   >>> r.add_distribution("sdist", url="http://example.org/foobar-1.0.tar.gz")
   >>> r.dists
   {'sdist': FooBar 1.0 sdist}
   >>> r['sdist'].url
   {'url': 'http://example.org/foobar-1.0.tar.gz', 'hashname': None, 'hashval':
   None, 'is_external': True}


Getting attributes from the dist objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To abstract querying information returned from the indexes, attributes and
release information can be retrieved directly from dist objects.

For instance, if you have a release instance that does not contain the metadata
attribute, it can be fetched by using the "fetch_metadata" method::

   >>> r = Release("FooBar", "1.1")
   >>> print r.metadata
   None # metadata field is actually set to "None"
   >>> r.fetch_metadata()
   <Metadata for FooBar 1.1>

.. XXX add proper roles to these constructs


It's possible to retrieve a project's releases (`fetch_releases`),
metadata (`fetch_metadata`) and distributions (`fetch_distributions`) using
a similar work flow.

.. XXX what is possible?

Internally, this is possible because while retrieving information about
projects, releases or distributions, a reference to the client used is
stored which can be accessed using the objects `_index` attribute.
