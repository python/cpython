"""Tests for distutils.command.upload."""
import sys
import os
import unittest
import http.client as httpclient

from distutils.command.upload import upload
from distutils.core import Distribution

from distutils.tests import support
from distutils.tests.test_config import PYPIRC, PyPIRCCommandTestCase

PYPIRC_LONG_PASSWORD = """\
[distutils]

index-servers =
    server1
    server2

[server1]
username:me
password:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa

[server2]
username:meagain
password: secret
realm:acme
repository:http://another.pypi/
"""


PYPIRC_NOPASSWORD = """\
[distutils]

index-servers =
    server1

[server1]
username:me
"""
class Response(object):
    def __init__(self, status=200, reason='OK'):
        self.status = status
        self.reason = reason

class FakeConnection(object):

    def __init__(self):
        self.requests = []
        self.headers = []
        self.body = ''

    def __call__(self, netloc):
        return self

    def connect(self):
        pass
    endheaders = connect

    def putrequest(self, method, url):
        self.requests.append((method, url))

    def putheader(self, name, value):
        self.headers.append((name, value))

    def send(self, body):
        self.body = body

    def getresponse(self):
        return Response()

class uploadTestCase(PyPIRCCommandTestCase):

    def setUp(self):
        super(uploadTestCase, self).setUp()
        self.old_class = httpclient.HTTPConnection
        self.conn = httpclient.HTTPConnection = FakeConnection()

    def tearDown(self):
        httpclient.HTTPConnection = self.old_class
        super(uploadTestCase, self).tearDown()

    def test_finalize_options(self):

        # new format
        self.write_file(self.rc, PYPIRC)
        dist = Distribution()
        cmd = upload(dist)
        cmd.finalize_options()
        for attr, waited in (('username', 'me'), ('password', 'secret'),
                             ('realm', 'pypi'),
                             ('repository', 'http://pypi.python.org/pypi')):
            self.assertEqual(getattr(cmd, attr), waited)

    def test_saved_password(self):
        # file with no password
        self.write_file(self.rc, PYPIRC_NOPASSWORD)

        # make sure it passes
        dist = Distribution()
        cmd = upload(dist)
        cmd.finalize_options()
        self.assertEqual(cmd.password, None)

        # make sure we get it as well, if another command
        # initialized it at the dist level
        dist.password = 'xxx'
        cmd = upload(dist)
        cmd.finalize_options()
        self.assertEqual(cmd.password, 'xxx')

    def test_upload(self):
        tmp = self.mkdtemp()
        path = os.path.join(tmp, 'xxx')
        self.write_file(path)
        command, pyversion, filename = 'xxx', '2.6', path
        dist_files = [(command, pyversion, filename)]
        self.write_file(self.rc, PYPIRC_LONG_PASSWORD)

        # lets run it
        pkg_dir, dist = self.create_dist(dist_files=dist_files)
        cmd = upload(dist)
        cmd.ensure_finalized()
        cmd.run()

        # what did we send ?
        headers = dict(self.conn.headers)
        self.assertEqual(headers['Content-length'], '2087')
        self.assertTrue(headers['Content-type'].startswith('multipart/form-data'))
        self.assertFalse('\n' in headers['Authorization'])

        self.assertEqual(self.conn.requests, [('POST', '/pypi')])
        self.assertTrue((b'xxx') in self.conn.body)

def test_suite():
    return unittest.makeSuite(uploadTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
