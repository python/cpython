"""Tests for packaging.command.upload."""
import os
import sys

from packaging.command.upload import upload
from packaging.dist import Distribution
from packaging.errors import PackagingOptionError

from packaging.tests import unittest, support
try:
    import threading
    from packaging.tests.pypi_server import PyPIServerTestCase
except ImportError:
    threading = None
    PyPIServerTestCase = unittest.TestCase


PYPIRC_NOPASSWORD = """\
[distutils]

index-servers =
    server1

[server1]
username:me
"""

PYPIRC = """\
[distutils]

index-servers =
    server1
    server2

[server1]
username:me
password:secret

[server2]
username:meagain
password: secret
realm:acme
repository:http://another.pypi/
"""


@unittest.skipIf(threading is None, 'needs threading')
class UploadTestCase(support.TempdirManager, support.EnvironRestorer,
                     support.LoggingCatcher, PyPIServerTestCase):

    restore_environ = ['HOME']

    def setUp(self):
        super(UploadTestCase, self).setUp()
        self.tmp_dir = self.mkdtemp()
        self.rc = os.path.join(self.tmp_dir, '.pypirc')
        os.environ['HOME'] = self.tmp_dir

    def test_finalize_options(self):
        # new format
        self.write_file(self.rc, PYPIRC)
        dist = Distribution()
        cmd = upload(dist)
        cmd.finalize_options()
        for attr, expected in (('username', 'me'), ('password', 'secret'),
                               ('realm', 'pypi'),
                               ('repository', 'http://pypi.python.org/pypi')):
            self.assertEqual(getattr(cmd, attr), expected)

    def test_finalize_options_unsigned_identity_raises_exception(self):
        self.write_file(self.rc, PYPIRC)
        dist = Distribution()
        cmd = upload(dist)
        cmd.identity = True
        cmd.sign = False
        self.assertRaises(PackagingOptionError, cmd.finalize_options)

    def test_saved_password(self):
        # file with no password
        self.write_file(self.rc, PYPIRC_NOPASSWORD)

        # make sure it passes
        dist = Distribution()
        cmd = upload(dist)
        cmd.ensure_finalized()
        self.assertEqual(cmd.password, None)

        # make sure we get it as well, if another command
        # initialized it at the dist level
        dist.password = 'xxx'
        cmd = upload(dist)
        cmd.finalize_options()
        self.assertEqual(cmd.password, 'xxx')

    def test_upload_without_files_raises_exception(self):
        dist = Distribution()
        cmd = upload(dist)
        self.assertRaises(PackagingOptionError, cmd.run)

    def test_upload(self):
        path = os.path.join(self.tmp_dir, 'xxx')
        self.write_file(path)
        command, pyversion, filename = 'xxx', '3.3', path
        dist_files = [(command, pyversion, filename)]

        # lets run it
        pkg_dir, dist = self.create_dist(dist_files=dist_files, author='dédé')
        cmd = upload(dist)
        cmd.ensure_finalized()
        cmd.repository = self.pypi.full_address
        cmd.run()

        # what did we send ?
        handler, request_data = self.pypi.requests[-1]
        headers = handler.headers
        #self.assertIn('dédé', str(request_data))
        self.assertIn(b'xxx', request_data)

        self.assertEqual(int(headers['content-length']), len(request_data))
        self.assertLess(int(headers['content-length']), 2500)
        self.assertTrue(headers['content-type'].startswith('multipart/form-data'))
        self.assertEqual(handler.command, 'POST')
        self.assertNotIn('\n', headers['authorization'])

    def test_upload_docs(self):
        path = os.path.join(self.tmp_dir, 'xxx')
        self.write_file(path)
        command, pyversion, filename = 'xxx', '3.3', path
        dist_files = [(command, pyversion, filename)]
        docs_path = os.path.join(self.tmp_dir, "build", "docs")
        os.makedirs(docs_path)
        self.write_file(os.path.join(docs_path, "index.html"), "yellow")
        self.write_file(self.rc, PYPIRC)

        # lets run it
        pkg_dir, dist = self.create_dist(dist_files=dist_files, author='dédé')

        cmd = upload(dist)
        cmd.get_finalized_command("build").run()
        cmd.upload_docs = True
        cmd.ensure_finalized()
        cmd.repository = self.pypi.full_address
        try:
            prev_dir = os.getcwd()
            os.chdir(self.tmp_dir)
            cmd.run()
        finally:
            os.chdir(prev_dir)

        handler, request_data = self.pypi.requests[-1]
        action, name, content = request_data.split(
            "----------------GHSKFJDLGDS7543FJKLFHRE75642756743254"
            .encode())[1:4]

        self.assertIn(b'name=":action"', action)
        self.assertIn(b'doc_upload', action)


def test_suite():
    return unittest.makeSuite(UploadTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
