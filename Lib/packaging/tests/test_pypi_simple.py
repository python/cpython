"""Tests for the packaging.pypi.simple module."""
import re
import os
import sys
import http.client
import urllib.error
import urllib.parse
import urllib.request

from packaging.pypi.simple import Crawler

from packaging.tests import unittest
from packaging.tests.support import TempdirManager, LoggingCatcher
from packaging.tests.pypi_server import (use_pypi_server, PyPIServer,
                                         PYPI_DEFAULT_STATIC_PATH)


class SimpleCrawlerTestCase(TempdirManager,
                            LoggingCatcher,
                            unittest.TestCase):

    def _get_simple_crawler(self, server, base_url="/simple/", hosts=None,
                            *args, **kwargs):
        """Build and return a SimpleIndex with the test server urls"""
        if hosts is None:
            hosts = (server.full_address.replace("http://", ""),)
        kwargs['hosts'] = hosts
        return Crawler(server.full_address + base_url, *args,
                       **kwargs)

    @use_pypi_server()
    def test_bad_urls(self, server):
        crawler = Crawler()
        url = 'http://127.0.0.1:0/nonesuch/test_simple'
        try:
            v = crawler._open_url(url)
        except Exception as v:
            self.assertIn(url, str(v))
        else:
            v.close()
            self.assertIsInstance(v, urllib.error.HTTPError)

        # issue 16
        # easy_install inquant.contentmirror.plone breaks because of a typo
        # in its home URL
        crawler = Crawler(hosts=('example.org',))
        url = ('url:%20https://svn.plone.org/svn/collective/'
               'inquant.contentmirror.plone/trunk')
        try:
            v = crawler._open_url(url)
        except Exception as v:
            self.assertIn(url, str(v))
        else:
            v.close()
            self.assertIsInstance(v, urllib.error.HTTPError)

        def _urlopen(*args):
            raise http.client.BadStatusLine('line')

        old_urlopen = urllib.request.urlopen
        urllib.request.urlopen = _urlopen
        url = 'http://example.org'
        try:
            v = crawler._open_url(url)
        except Exception as v:
            self.assertIn('line', str(v))
        else:
            v.close()
            # TODO use self.assertRaises
            raise AssertionError('Should have raise here!')
        finally:
            urllib.request.urlopen = old_urlopen

        # issue 20
        url = 'http://http://svn.pythonpaste.org/Paste/wphp/trunk'
        try:
            crawler._open_url(url)
        except Exception as v:
            self.assertIn('nonnumeric port', str(v))

        # issue #160
        url = server.full_address
        page = ('<a href="http://www.famfamfam.com]('
                'http://www.famfamfam.com/">')
        crawler._process_url(url, page)

    @use_pypi_server("test_found_links")
    def test_found_links(self, server):
        # Browse the index, asking for a specified release version
        # The PyPI index contains links for version 1.0, 1.1, 2.0 and 2.0.1
        crawler = self._get_simple_crawler(server)
        last_release = crawler.get_release("foobar")

        # we have scanned the index page
        self.assertIn(server.full_address + "/simple/foobar/",
            crawler._processed_urls)

        # we have found 4 releases in this page
        self.assertEqual(len(crawler._projects["foobar"]), 4)

        # and returned the most recent one
        self.assertEqual("%s" % last_release.version, '2.0.1')

    def test_is_browsable(self):
        crawler = Crawler(follow_externals=False)
        self.assertTrue(crawler._is_browsable(crawler.index_url + "test"))

        # Now, when following externals, we can have a list of hosts to trust.
        # and don't follow other external links than the one described here.
        crawler = Crawler(hosts=["pypi.python.org", "example.org"],
                          follow_externals=True)
        good_urls = (
            "http://pypi.python.org/foo/bar",
            "http://pypi.python.org/simple/foobar",
            "http://example.org",
            "http://example.org/",
            "http://example.org/simple/",
        )
        bad_urls = (
            "http://python.org",
            "http://example.tld",
        )

        for url in good_urls:
            self.assertTrue(crawler._is_browsable(url))

        for url in bad_urls:
            self.assertFalse(crawler._is_browsable(url))

        # allow all hosts
        crawler = Crawler(follow_externals=True, hosts=("*",))
        self.assertTrue(crawler._is_browsable("http://an-external.link/path"))
        self.assertTrue(crawler._is_browsable("pypi.example.org/a/path"))

        # specify a list of hosts we want to allow
        crawler = Crawler(follow_externals=True,
                          hosts=("*.example.org",))
        self.assertFalse(crawler._is_browsable("http://an-external.link/path"))
        self.assertTrue(
            crawler._is_browsable("http://pypi.example.org/a/path"))

    @use_pypi_server("with_externals")
    def test_follow_externals(self, server):
        # Include external pages
        # Try to request the package index, wich contains links to "externals"
        # resources. They have to  be scanned too.
        crawler = self._get_simple_crawler(server, follow_externals=True)
        crawler.get_release("foobar")
        self.assertIn(server.full_address + "/external/external.html",
            crawler._processed_urls)

    @use_pypi_server("with_real_externals")
    def test_restrict_hosts(self, server):
        # Only use a list of allowed hosts is possible
        # Test that telling the simple pyPI client to not retrieve external
        # works
        crawler = self._get_simple_crawler(server, follow_externals=False)
        crawler.get_release("foobar")
        self.assertNotIn(server.full_address + "/external/external.html",
            crawler._processed_urls)

    @use_pypi_server(static_filesystem_paths=["with_externals"],
        static_uri_paths=["simple", "external"])
    def test_links_priority(self, server):
        # Download links from the pypi simple index should be used before
        # external download links.
        # http://bitbucket.org/tarek/distribute/issue/163/md5-validation-error
        #
        # Usecase :
        # - someone uploads a package on pypi, a md5 is generated
        # - someone manually coindexes this link (with the md5 in the url) onto
        #   an external page accessible from the package page.
        # - someone reuploads the package (with a different md5)
        # - while easy_installing, an MD5 error occurs because the external
        # link is used
        # -> The index should use the link from pypi, not the external one.

        # start an index server
        index_url = server.full_address + '/simple/'

        # scan a test index
        crawler = Crawler(index_url, follow_externals=True)
        releases = crawler.get_releases("foobar")
        server.stop()

        # we have only one link, because links are compared without md5
        self.assertEqual(1, len(releases))
        self.assertEqual(1, len(releases[0].dists))
        # the link should be from the index
        self.assertEqual(2, len(releases[0].dists['sdist'].urls))
        self.assertEqual('12345678901234567',
                         releases[0].dists['sdist'].url['hashval'])
        self.assertEqual('md5', releases[0].dists['sdist'].url['hashname'])

    @use_pypi_server(static_filesystem_paths=["with_norel_links"],
        static_uri_paths=["simple", "external"])
    def test_not_scan_all_links(self, server):
        # Do not follow all index page links.
        # The links not tagged with rel="download" and rel="homepage" have
        # to not be processed by the package index, while processing "pages".

        # process the pages
        crawler = self._get_simple_crawler(server, follow_externals=True)
        crawler.get_releases("foobar")
        # now it should have processed only pages with links rel="download"
        # and rel="homepage"
        self.assertIn("%s/simple/foobar/" % server.full_address,
            crawler._processed_urls)  # it's the simple index page
        self.assertIn("%s/external/homepage.html" % server.full_address,
            crawler._processed_urls)  # the external homepage is rel="homepage"
        self.assertNotIn("%s/external/nonrel.html" % server.full_address,
            crawler._processed_urls)  # this link contains no rel=*
        self.assertNotIn("%s/unrelated-0.2.tar.gz" % server.full_address,
            crawler._processed_urls)  # linked from simple index (no rel)
        self.assertIn("%s/foobar-0.1.tar.gz" % server.full_address,
            crawler._processed_urls)  # linked from simple index (rel)
        self.assertIn("%s/foobar-2.0.tar.gz" % server.full_address,
            crawler._processed_urls)  # linked from external homepage (rel)

    def test_uses_mirrors(self):
        # When the main repository seems down, try using the given mirrors"""
        server = PyPIServer("foo_bar_baz")
        mirror = PyPIServer("foo_bar_baz")
        mirror.start()  # we dont start the server here

        try:
            # create the index using both servers
            crawler = Crawler(server.full_address + "/simple/", hosts=('*',),
                              # set the timeout to 1s for the tests
                              timeout=1, mirrors=[mirror.full_address])

            # this should not raise a timeout
            self.assertEqual(4, len(crawler.get_releases("foo")))
        finally:
            mirror.stop()
            server.stop()

    def test_simple_link_matcher(self):
        # Test that the simple link matcher finds the right links"""
        crawler = Crawler(follow_externals=False)

        # Here, we define:
        #   1. one link that must be followed, cause it's a download one
        #   2. one link that must *not* be followed, cause the is_browsable
        #      returns false for it.
        #   3. one link that must be followed cause it's a homepage that is
        #      browsable
        #   4. one link that must be followed, because it contain a md5 hash
        self.assertTrue(crawler._is_browsable("%stest" % crawler.index_url))
        self.assertFalse(crawler._is_browsable("http://dl-link2"))
        content = """
        <a href="http://dl-link1" rel="download">download_link1</a>
        <a href="http://dl-link2" rel="homepage">homepage_link1</a>
        <a href="%(index_url)stest" rel="homepage">homepage_link2</a>
        <a href="%(index_url)stest/foobar-1.tar.gz#md5=abcdef>download_link2</a>
        """ % {'index_url': crawler.index_url}

        # Test that the simple link matcher yield the good links.
        generator = crawler._simple_link_matcher(content, crawler.index_url)
        self.assertEqual(('%stest/foobar-1.tar.gz#md5=abcdef' %
                          crawler.index_url, True), next(generator))
        self.assertEqual(('http://dl-link1', True), next(generator))
        self.assertEqual(('%stest' % crawler.index_url, False),
                         next(generator))
        self.assertRaises(StopIteration, generator.__next__)

        # Follow the external links is possible (eg. homepages)
        crawler.follow_externals = True
        generator = crawler._simple_link_matcher(content, crawler.index_url)
        self.assertEqual(('%stest/foobar-1.tar.gz#md5=abcdef' %
                          crawler.index_url, True), next(generator))
        self.assertEqual(('http://dl-link1', True), next(generator))
        self.assertEqual(('http://dl-link2', False), next(generator))
        self.assertEqual(('%stest' % crawler.index_url, False),
                         next(generator))
        self.assertRaises(StopIteration, generator.__next__)

    def test_browse_local_files(self):
        # Test that we can browse local files"""
        index_url = "file://" + PYPI_DEFAULT_STATIC_PATH
        if sys.platform == 'win32':
            # under windows the correct syntax is:
            #   file:///C|\the\path\here
            # instead of
            #   file://C:\the\path\here
            fix = re.compile(r'^(file://)([A-Za-z])(:)')
            index_url = fix.sub('\\1/\\2|', index_url)

        index_path = os.sep.join([index_url, "test_found_links", "simple"])
        crawler = Crawler(index_path)
        dists = crawler.get_releases("foobar")
        self.assertEqual(4, len(dists))

    def test_get_link_matcher(self):
        crawler = Crawler("http://example.org")
        self.assertEqual('_simple_link_matcher', crawler._get_link_matcher(
                         "http://example.org/some/file").__name__)
        self.assertEqual('_default_link_matcher', crawler._get_link_matcher(
                         "http://other-url").__name__)

    def test_default_link_matcher(self):
        crawler = Crawler("http://example.org", mirrors=[])
        crawler.follow_externals = True
        crawler._is_browsable = lambda *args: True
        base_url = "http://example.org/some/file/"
        content = """
<a href="../homepage" rel="homepage">link</a>
<a href="../download" rel="download">link2</a>
<a href="../simpleurl">link2</a>
        """
        found_links = set(uri for uri, _ in
                          crawler._default_link_matcher(content, base_url))
        self.assertIn('http://example.org/some/homepage', found_links)
        self.assertIn('http://example.org/some/simpleurl', found_links)
        self.assertIn('http://example.org/some/download', found_links)

    @use_pypi_server("project_list")
    def test_search_projects(self, server):
        # we can search the index for some projects, on their names
        # the case used no matters here
        crawler = self._get_simple_crawler(server)
        tests = (('Foobar', ['FooBar-bar', 'Foobar-baz', 'Baz-FooBar']),
                 ('foobar*', ['FooBar-bar', 'Foobar-baz']),
                 ('*foobar', ['Baz-FooBar']))

        for search, expected in tests:
            projects = [p.name for p in crawler.search_projects(search)]
            self.assertListEqual(expected, projects)


def test_suite():
    return unittest.makeSuite(SimpleCrawlerTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest="test_suite")
