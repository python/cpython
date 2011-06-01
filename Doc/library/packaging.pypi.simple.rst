:mod:`packaging.pypi.simple` --- Crawler using the PyPI "simple" interface
==========================================================================

.. module:: packaging.pypi.simple
   :synopsis: Crawler using the screen-scraping "simple" interface to fetch info
              and distributions.


`packaging.pypi.simple` can process Python Package Indexes  and provides
useful information about distributions. It also can crawl local indexes, for
instance.

You should use `packaging.pypi.simple` for:

    * Search distributions by name and versions.
    * Process index external pages.
    * Download distributions by name and versions.

And should not be used for:

    * Things that will end up in too long index processing (like "finding all
      distributions with a specific version, no matters the name")


API
---

.. class:: Crawler


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
