:mod:`packaging.pypi.simple` --- Crawler using the PyPI "simple" interface
==========================================================================

.. module:: packaging.pypi.simple
   :synopsis: Crawler using the screen-scraping "simple" interface to fetch info
              and distributions.


The class provided by :mod:`packaging.pypi.simple` can access project indexes
and provide useful information about distributions.  PyPI, other indexes and
local indexes are supported.

You should use this module to search distributions by name and versions, process
index external pages and download distributions.  It is not suited for things
that will end up in too long index processing (like "finding all distributions
with a specific version, no matter the name"); use :mod:`packaging.pypi.xmlrpc`
for that.


API
---

.. class:: Crawler(index_url=DEFAULT_SIMPLE_INDEX_URL, \
                   prefer_final=False, prefer_source=True, \
                   hosts=('*',), follow_externals=False, \
                   mirrors_url=None, mirrors=None, timeout=15, \
                   mirrors_max_tries=0)

      *index_url* is the address of the index to use for requests.

      The first two parameters control the query results.  *prefer_final*
      indicates whether a final version (not alpha, beta or candidate) is to be
      prefered over a newer but non-final version (for example, whether to pick
      up 1.0 over 2.0a3).  It is used only for queries that don't give a version
      argument.  Likewise, *prefer_source* tells whether to prefer a source
      distribution over a binary one, if no distribution argument was prodived.

      Other parameters are related to external links (that is links that go
      outside the simple index): *hosts* is a list of hosts allowed to be
      processed if *follow_externals* is true (default behavior is to follow all
      hosts), *follow_externals* enables or disables following external links
      (default is false, meaning disabled).

      The remaining parameters are related to the mirroring infrastructure
      defined in :PEP:`381`.  *mirrors_url* gives a URL to look on for DNS
      records giving mirror adresses; *mirrors* is a list of mirror URLs (see
      the PEP).  If both *mirrors* and *mirrors_url* are given, *mirrors_url*
      will only be used if *mirrors* is set to ``None``.  *timeout* is the time
      (in seconds) to wait before considering a URL has timed out;
      *mirrors_max_tries"* is the number of times to try requesting informations
      on mirrors before switching.

      The following methods are defined:

      .. method:: get_distributions(project_name, version)

         Return the distributions found in the index for the given release.

      .. method:: get_metadata(project_name, version)

         Return the metadata found on the index for this project name and
         version.  Currently downloads and unpacks a distribution to read the
         PKG-INFO file.

      .. method:: get_release(requirements, prefer_final=None)

         Return one release that fulfills the given requirements.

      .. method:: get_releases(requirements, prefer_final=None, force_update=False)

         Search for releases and return a
         :class:`~packaging.pypi.dist.ReleasesList` object containing the
         results.

      .. method:: search_projects(name=None)

         Search the index for projects containing the given name and return a
         list of matching names.

   See also the base class :class:`packaging.pypi.base.BaseClient` for inherited
   methods.


.. data:: DEFAULT_SIMPLE_INDEX_URL

   The address used by default by the crawler class.  It is currently
   ``'http://a.pypi.python.org/simple/'``, the main PyPI installation.




Usage Exemples
---------------

To help you understand how using the `Crawler` class, here are some basic
usages.

Request the simple index to get a specific distribution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Supposing you want to scan an index to get a list of distributions for
the "foobar" project. You can use the "get_releases" method for that.
The get_releases method will browse the project page, and return
:class:`ReleaseInfo`  objects for each found link that rely on downloads. ::

   >>> from packaging.pypi.simple import Crawler
   >>> crawler = Crawler()
   >>> crawler.get_releases("FooBar")
   [<ReleaseInfo "Foobar 1.1">, <ReleaseInfo "Foobar 1.2">]


Note that you also can request the client about specific versions, using version
specifiers (described in `PEP 345
<http://www.python.org/dev/peps/pep-0345/#version-specifiers>`_)::

   >>> client.get_releases("FooBar < 1.2")
   [<ReleaseInfo "FooBar 1.1">, ]


`get_releases` returns a list of :class:`ReleaseInfo`, but you also can get the
best distribution that fullfil your requirements, using "get_release"::

   >>> client.get_release("FooBar < 1.2")
   <ReleaseInfo "FooBar 1.1">


Download distributions
^^^^^^^^^^^^^^^^^^^^^^

As it can get the urls of distributions provided by PyPI, the `Crawler`
client also can download the distributions and put it for you in a temporary
destination::

   >>> client.download("foobar")
   /tmp/temp_dir/foobar-1.2.tar.gz


You also can specify the directory you want to download to::

   >>> client.download("foobar", "/path/to/my/dir")
   /path/to/my/dir/foobar-1.2.tar.gz


While downloading, the md5 of the archive will be checked, if not matches, it
will try another time, then if fails again, raise `MD5HashDoesNotMatchError`.

Internally, that's not the Crawler which download the distributions, but the
`DistributionInfo` class. Please refer to this documentation for more details.


Following PyPI external links
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The default behavior for packaging is to *not* follow the links provided
by HTML pages in the "simple index", to find distributions related
downloads.

It's possible to tell the PyPIClient to follow external links by setting the
`follow_externals` attribute, on instantiation or after::

   >>> client = Crawler(follow_externals=True)

or ::

   >>> client = Crawler()
   >>> client.follow_externals = True


Working with external indexes, and mirrors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The default `Crawler` behavior is to rely on the Python Package index stored
on PyPI (http://pypi.python.org/simple).

As you can need to work with a local index, or private indexes, you can specify
it using the index_url parameter::

   >>> client = Crawler(index_url="file://filesystem/path/")

or ::

   >>> client = Crawler(index_url="http://some.specific.url/")


You also can specify mirrors to fallback on in case the first index_url you
provided doesnt respond, or not correctly. The default behavior for
`Crawler` is to use the list provided by Python.org DNS records, as
described in the :PEP:`381` about mirroring infrastructure.

If you don't want to rely on these, you could specify the list of mirrors you
want to try by specifying the `mirrors` attribute. It's a simple iterable::

   >>> mirrors = ["http://first.mirror","http://second.mirror"]
   >>> client = Crawler(mirrors=mirrors)


Searching in the simple index
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It's possible to search for projects with specific names in the package index.
Assuming you want to find all projects containing the "distutils" keyword::

   >>> c.search_projects("distutils")
   [<Project "collective.recipe.distutils">, <Project "Distutils">, <Project
   "Packaging">, <Project "distutilscross">, <Project "lpdistutils">, <Project
   "taras.recipe.distutils">, <Project "zerokspot.recipe.distutils">]


You can also search the projects starting with a specific text, or ending with
that text, using a wildcard::

   >>> c.search_projects("distutils*")
   [<Project "Distutils">, <Project "Packaging">, <Project "distutilscross">]

   >>> c.search_projects("*distutils")
   [<Project "collective.recipe.distutils">, <Project "Distutils">, <Project
   "lpdistutils">, <Project "taras.recipe.distutils">, <Project
   "zerokspot.recipe.distutils">]
