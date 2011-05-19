"""Tests for packaging.command.bdist."""
import sys

import urllib.request
import urllib.parse
import urllib.error

from packaging.tests.pypi_server import PyPIServer, PYPI_DEFAULT_STATIC_PATH
from packaging.tests import unittest


class PyPIServerTest(unittest.TestCase):

    def test_records_requests(self):
        # We expect that PyPIServer can log our requests
        server = PyPIServer()
        server.default_response_status = 200

        try:
            server.start()
            self.assertEqual(len(server.requests), 0)

            data = b'Rock Around The Bunker'

            headers = {"X-test-header": "Mister Iceberg"}

            request = urllib.request.Request(server.full_address, data, headers)
            urllib.request.urlopen(request)
            self.assertEqual(len(server.requests), 1)
            handler, request_data = server.requests[-1]
            self.assertIn(data, request_data)
            self.assertIn("x-test-header", handler.headers)
            self.assertEqual(handler.headers["x-test-header"], "Mister Iceberg")

        finally:
            server.stop()


    def test_serve_static_content(self):
        # PYPI Mocked server can serve static content from disk.

        def uses_local_files_for(server, url_path):
            """Test that files are served statically (eg. the output from the
            server is the same than the one made by a simple file read.
            """
            url = server.full_address + url_path
            request = urllib.request.Request(url)
            response = urllib.request.urlopen(request)
            file = open(PYPI_DEFAULT_STATIC_PATH + "/test_pypi_server" +
               url_path)
            answer = response.read().decode() == file.read()
            file.close()
            return answer

        server = PyPIServer(static_uri_paths=["simple", "external"],
            static_filesystem_paths=["test_pypi_server"])
        server.start()
        try:
            # the file does not exists on the disc, so it might not be served
            url = server.full_address + "/simple/unexisting_page"
            request = urllib.request.Request(url)
            try:
                urllib.request.urlopen(request)
            except urllib.error.HTTPError as e:
                self.assertEqual(e.code, 404)

            # now try serving a content that do exists
            self.assertTrue(uses_local_files_for(server, "/simple/index.html"))

            # and another one in another root path
            self.assertTrue(uses_local_files_for(server, "/external/index.html"))

        finally:
            server.stop()


def test_suite():
    return unittest.makeSuite(PyPIServerTest)

if __name__ == '__main__':
    unittest.main(defaultTest="test_suite")
