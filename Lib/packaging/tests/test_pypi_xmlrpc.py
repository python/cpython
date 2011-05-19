"""Tests for the packaging.pypi.xmlrpc module."""

from packaging.pypi.xmlrpc import Client, InvalidSearchField, ProjectNotFound

from packaging.tests import unittest

try:
    import threading
    from packaging.tests.pypi_server import use_xmlrpc_server
except ImportError:
    threading = None
    use_xmlrpc_server = None


@unittest.skipIf(threading is None, "Needs threading")
class TestXMLRPCClient(unittest.TestCase):
    def _get_client(self, server, *args, **kwargs):
        return Client(server.full_address, *args, **kwargs)

    @use_xmlrpc_server()
    def test_search_projects(self, server):
        client = self._get_client(server)
        server.xmlrpc.set_search_result(['FooBar', 'Foo', 'FooFoo'])
        results = [r.name for r in client.search_projects(name='Foo')]
        self.assertEqual(3, len(results))
        self.assertIn('FooBar', results)
        self.assertIn('Foo', results)
        self.assertIn('FooFoo', results)

    def test_search_projects_bad_fields(self):
        client = Client()
        self.assertRaises(InvalidSearchField, client.search_projects,
                          invalid="test")

    @use_xmlrpc_server()
    def test_get_releases(self, server):
        client = self._get_client(server)
        server.xmlrpc.set_distributions([
            {'name': 'FooBar', 'version': '1.1'},
            {'name': 'FooBar', 'version': '1.2', 'url': 'http://some/url/'},
            {'name': 'FooBar', 'version': '1.3', 'url': 'http://other/url/'},
        ])

        # use a lambda here to avoid an useless mock call
        server.xmlrpc.list_releases = lambda *a, **k: ['1.1', '1.2', '1.3']

        releases = client.get_releases('FooBar (<=1.2)')
        # dont call release_data and release_url; just return name and version.
        self.assertEqual(2, len(releases))
        versions = releases.get_versions()
        self.assertIn('1.1', versions)
        self.assertIn('1.2', versions)
        self.assertNotIn('1.3', versions)

        self.assertRaises(ProjectNotFound, client.get_releases, 'Foo')

    @use_xmlrpc_server()
    def test_get_distributions(self, server):
        client = self._get_client(server)
        server.xmlrpc.set_distributions([
            {'name': 'FooBar', 'version': '1.1',
             'url': 'http://example.org/foobar-1.1-sdist.tar.gz',
             'digest': '1234567',
             'type': 'sdist', 'python_version': 'source'},
            {'name':'FooBar', 'version': '1.1',
             'url': 'http://example.org/foobar-1.1-bdist.tar.gz',
             'digest': '8912345', 'type': 'bdist'},
        ])

        releases = client.get_releases('FooBar', '1.1')
        client.get_distributions('FooBar', '1.1')
        release = releases.get_release('1.1')
        self.assertTrue('http://example.org/foobar-1.1-sdist.tar.gz',
                        release['sdist'].url['url'])
        self.assertTrue('http://example.org/foobar-1.1-bdist.tar.gz',
                release['bdist'].url['url'])
        self.assertEqual(release['sdist'].python_version, 'source')

    @use_xmlrpc_server()
    def test_get_metadata(self, server):
        client = self._get_client(server)
        server.xmlrpc.set_distributions([
            {'name': 'FooBar',
             'version': '1.1',
             'keywords': '',
             'obsoletes_dist': ['FooFoo'],
             'requires_external': ['Foo'],
            }])
        release = client.get_metadata('FooBar', '1.1')
        self.assertEqual(['Foo'], release.metadata['requires_external'])
        self.assertEqual(['FooFoo'], release.metadata['obsoletes_dist'])


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestXMLRPCClient))
    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
