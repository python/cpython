:mod:`packaging.pypi` --- Interface to projects indexes
=======================================================

.. module:: packaging.pypi
   :synopsis: Low-level and high-level APIs to query projects indexes.


Packaging queries PyPI to get information about projects or download them.  The
low-level facilities used internally are also part of the public API designed to
be used by other tools.

The :mod:`packaging.pypi` package provides those facilities, which can be
used to access information about Python projects registered at indexes, the
main one being PyPI, located ad http://pypi.python.org/.

There is two ways to retrieve data from these indexes: a screen-scraping
interface called the "simple API", and XML-RPC.  The first one uses HTML pages
located under http://pypi.python.org/simple/, the second one makes XML-RPC
requests to http://pypi.python.org/pypi/.  All functions and classes also work
with other indexes such as mirrors, which typically implement only the simple
interface.

Packaging provides a class that wraps both APIs to provide full query and
download functionality: :class:`packaging.pypi.client.ClientWrapper`.  If you
want more control, you can use the underlying classes
:class:`packaging.pypi.simple.Crawler` and :class:`packaging.pypi.xmlrpc.Client`
to connect to one specific interface.


:mod:`packaging.pypi.client` --- High-level query API
=====================================================

.. module:: packaging.pypi.client
   :synopsis: Wrapper around :mod;`packaging.pypi.xmlrpc` and
              :mod:`packaging.pypi.simple` to query indexes.


This module provides a high-level API to query indexes and search
for releases and distributions. The aim of this module is to choose the best
way to query the API automatically, either using XML-RPC or the simple index,
with a preference toward the latter.

.. class:: ClientWrapper

   Instances of this class will use the simple interface or XML-RPC requests to
   query indexes and return :class:`packaging.pypi.dist.ReleaseInfo` and
   :class:`packaging.pypi.dist.ReleasesList` objects.

   .. method:: find_projects

   .. method:: get_release

   .. method:: get_releases


:mod:`packaging.pypi.base` --- Base class for index crawlers
============================================================

.. module:: packaging.pypi.base
   :synopsis: Base class used to implement crawlers.


.. class:: BaseClient(prefer_final, prefer_source)

   Base class containing common methods for the index crawlers or clients.  One
   method is currently defined:

   .. method:: download_distribution(requirements, temp_path=None, \
                                     prefer_source=None, prefer_final=None)

      Download a distribution from the last release according to the
      requirements.  If *temp_path* is provided, download to this path,
      otherwise, create a temporary directory for the download.  If a release is
      found, the full path to the downloaded file is returned.
