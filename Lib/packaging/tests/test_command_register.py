"""Tests for packaging.command.register."""
import os
import getpass
import urllib.request
import urllib.error
import urllib.parse

try:
    import docutils
    DOCUTILS_SUPPORT = True
except ImportError:
    DOCUTILS_SUPPORT = False

from packaging.tests import unittest, support
from packaging.command import register as register_module
from packaging.command.register import register
from packaging.errors import PackagingSetupError


PYPIRC_NOPASSWORD = """\
[distutils]

index-servers =
    server1

[server1]
username:me
"""

WANTED_PYPIRC = """\
[distutils]
index-servers =
    pypi

[pypi]
username:tarek
password:password
"""


class Inputs:
    """Fakes user inputs."""
    def __init__(self, *answers):
        self.answers = answers
        self.index = 0

    def __call__(self, prompt=''):
        try:
            return self.answers[self.index]
        finally:
            self.index += 1


class FakeOpener:
    """Fakes a PyPI server"""
    def __init__(self):
        self.reqs = []

    def __call__(self, *args):
        return self

    def open(self, req):
        self.reqs.append(req)
        return self

    def read(self):
        return 'xxx'


class RegisterTestCase(support.TempdirManager,
                       support.EnvironRestorer,
                       support.LoggingCatcher,
                       unittest.TestCase):

    restore_environ = ['HOME']

    def setUp(self):
        super(RegisterTestCase, self).setUp()
        self.tmp_dir = self.mkdtemp()
        self.rc = os.path.join(self.tmp_dir, '.pypirc')
        os.environ['HOME'] = self.tmp_dir

        # patching the password prompt
        self._old_getpass = getpass.getpass

        def _getpass(prompt):
            return 'password'

        getpass.getpass = _getpass
        self.old_opener = urllib.request.build_opener
        self.conn = urllib.request.build_opener = FakeOpener()

    def tearDown(self):
        getpass.getpass = self._old_getpass
        urllib.request.build_opener = self.old_opener
        if hasattr(register_module, 'input'):
            del register_module.input
        super(RegisterTestCase, self).tearDown()

    def _get_cmd(self, metadata=None):
        if metadata is None:
            metadata = {'url': 'xxx', 'author': 'xxx',
                        'author_email': 'xxx',
                        'name': 'xxx', 'version': 'xxx'}
        pkg_info, dist = self.create_dist(**metadata)
        return register(dist)

    def test_create_pypirc(self):
        # this test makes sure a .pypirc file
        # is created when requested.

        # let's create a register instance
        cmd = self._get_cmd()

        # we shouldn't have a .pypirc file yet
        self.assertFalse(os.path.exists(self.rc))

        # patching input and getpass.getpass
        # so register gets happy
        # Here's what we are faking :
        # use your existing login (choice 1.)
        # Username : 'tarek'
        # Password : 'password'
        # Save your login (y/N)? : 'y'
        inputs = Inputs('1', 'tarek', 'y')
        register_module.input = inputs
        cmd.ensure_finalized()
        cmd.run()

        # we should have a brand new .pypirc file
        self.assertTrue(os.path.exists(self.rc))

        # with the content similar to WANTED_PYPIRC
        with open(self.rc) as fp:
            content = fp.read()
        self.assertEqual(content, WANTED_PYPIRC)

        # now let's make sure the .pypirc file generated
        # really works : we shouldn't be asked anything
        # if we run the command again
        def _no_way(prompt=''):
            raise AssertionError(prompt)

        register_module.input = _no_way
        cmd.show_response = True
        cmd.ensure_finalized()
        cmd.run()

        # let's see what the server received : we should
        # have 2 similar requests
        self.assertEqual(len(self.conn.reqs), 2)
        req1 = dict(self.conn.reqs[0].headers)
        req2 = dict(self.conn.reqs[1].headers)
        self.assertEqual(req2['Content-length'], req1['Content-length'])
        self.assertIn('xxx', self.conn.reqs[1].data)

    def test_password_not_in_file(self):

        self.write_file(self.rc, PYPIRC_NOPASSWORD)
        cmd = self._get_cmd()
        cmd.finalize_options()
        cmd._set_config()
        cmd.send_metadata()

        # dist.password should be set
        # therefore used afterwards by other commands
        self.assertEqual(cmd.distribution.password, 'password')

    def test_registration(self):
        # this test runs choice 2
        cmd = self._get_cmd()
        inputs = Inputs('2', 'tarek', 'tarek@ziade.org')
        register_module.input = inputs
        # let's run the command
        # FIXME does this send a real request? use a mock server
        cmd.ensure_finalized()
        cmd.run()

        # we should have send a request
        self.assertEqual(len(self.conn.reqs), 1)
        req = self.conn.reqs[0]
        headers = dict(req.headers)
        self.assertEqual(headers['Content-length'], '608')
        self.assertIn('tarek', req.data)

    def test_password_reset(self):
        # this test runs choice 3
        cmd = self._get_cmd()
        inputs = Inputs('3', 'tarek@ziade.org')
        register_module.input = inputs
        cmd.ensure_finalized()
        cmd.run()

        # we should have send a request
        self.assertEqual(len(self.conn.reqs), 1)
        req = self.conn.reqs[0]
        headers = dict(req.headers)
        self.assertEqual(headers['Content-length'], '290')
        self.assertIn('tarek', req.data)

    @unittest.skipUnless(DOCUTILS_SUPPORT, 'needs docutils')
    def test_strict(self):
        # testing the script option
        # when on, the register command stops if
        # the metadata is incomplete or if
        # long_description is not reSt compliant

        # empty metadata
        cmd = self._get_cmd({'name': 'xxx', 'version': 'xxx'})
        cmd.ensure_finalized()
        cmd.strict = True
        inputs = Inputs('1', 'tarek', 'y')
        register_module.input = inputs
        self.assertRaises(PackagingSetupError, cmd.run)

        # metadata is OK but long_description is broken
        metadata = {'home_page': 'xxx', 'author': 'xxx',
                    'author_email': 'éxéxé',
                    'name': 'xxx', 'version': 'xxx',
                    'description': 'title\n==\n\ntext'}

        cmd = self._get_cmd(metadata)
        cmd.ensure_finalized()
        cmd.strict = True

        self.assertRaises(PackagingSetupError, cmd.run)

        # now something that works
        metadata['description'] = 'title\n=====\n\ntext'
        cmd = self._get_cmd(metadata)
        cmd.ensure_finalized()
        cmd.strict = True
        inputs = Inputs('1', 'tarek', 'y')
        register_module.input = inputs
        cmd.ensure_finalized()
        cmd.run()

        # strict is not by default
        cmd = self._get_cmd()
        cmd.ensure_finalized()
        inputs = Inputs('1', 'tarek', 'y')
        register_module.input = inputs
        cmd.ensure_finalized()
        cmd.run()

    def test_register_pep345(self):
        cmd = self._get_cmd({})
        cmd.ensure_finalized()
        cmd.distribution.metadata['Requires-Dist'] = ['lxml']
        data = cmd.build_post_data('submit')
        self.assertEqual(data['metadata_version'], '1.2')
        self.assertEqual(data['requires_dist'], ['lxml'])


def test_suite():
    return unittest.makeSuite(RegisterTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
