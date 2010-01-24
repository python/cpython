"""Tests for distutils.command.upload."""
# -*- encoding: utf8 -*-
import sys
import os
import unittest
import httplib

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

class _Resp(object):
    def __init__(self, status):
        self.status = status
        self.reason = 'OK'

_CONNECTIONS = []

class _FakeHTTPConnection(object):
    def __init__(self, netloc):
        self.requests = []
        self.headers = {}
        self.body = None
        self.netloc = netloc
        _CONNECTIONS.append(self)

    def connect(self):
        pass
    endheaders = connect

    def send(self, body):
        self.body = body

    def putrequest(self, type_, data):
        self.requests.append((type_, data))

    def putheader(self, name, value):
        self.headers[name] = value

    def getresponse(self):
        return _Resp(200)

class uploadTestCase(PyPIRCCommandTestCase):

    def setUp(self):
        super(uploadTestCase, self).setUp()
        self.old_klass = httplib.HTTPConnection
        httplib.HTTPConnection = _FakeHTTPConnection

    def tearDown(self):
        httplib.HTTPConnection = self.old_klass
        super(uploadTestCase, self).tearDown()

    def test_finalize_options(self):

        # new format
        f = open(self.rc, 'w')
        f.write(PYPIRC)
        f.close()

        dist = Distribution()
        cmd = upload(dist)
        cmd.finalize_options()
        for attr, waited in (('username', 'me'), ('password', 'secret'),
                             ('realm', 'pypi'),
                             ('repository', 'http://pypi.python.org/pypi')):
            self.assertEquals(getattr(cmd, attr), waited)


    def test_upload(self):
        tmp = self.mkdtemp()
        path = os.path.join(tmp, 'xxx')
        self.write_file(path)
        command, pyversion, filename = 'xxx', '2.6', path
        dist_files = [(command, pyversion, filename)]
        self.write_file(self.rc, PYPIRC_LONG_PASSWORD)

        # lets run it
        pkg_dir, dist = self.create_dist(dist_files=dist_files, author=u'dédé')
        cmd = upload(dist)
        cmd.ensure_finalized()
        cmd.run()

        # what did we send ?
        res = _CONNECTIONS[-1]

        headers = res.headers
        self.assert_('dédé' in res.body)
        self.assertEquals(headers['Content-length'], '2085')
        self.assertTrue(headers['Content-type'].startswith('multipart/form-data'))

        method, request = res.requests[-1]
        self.assertEquals(method, 'POST')
        self.assertEquals(res.netloc, 'pypi.python.org')
        self.assertTrue('xxx' in res.body)
        self.assertFalse('\n' in headers['Authorization'])

def test_suite():
    return unittest.makeSuite(uploadTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
