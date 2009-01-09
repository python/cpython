"""Tests for distutils.command.register."""
import sys
import os
import unittest
import getpass

from distutils.command.register import register
from distutils.core import Distribution

from distutils.tests import support
from distutils.tests.test_config import PYPIRC, PyPIRCCommandTestCase

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

class Inputs(object):
    """Fakes user inputs."""
    def __init__(self, *answers):
        self.answers = answers
        self.index = 0

    def __call__(self, prompt=''):
        try:
            return self.answers[self.index]
        finally:
            self.index += 1

class FakeServer(object):
    """Fakes a PyPI server"""
    def __init__(self):
        self.calls = []

    def __call__(self, *args):
        # we want to compare them, so let's store
        # something comparable
        els = list(args[0].items())
        els.sort()
        self.calls.append(tuple(els))
        return 200, 'OK'

class registerTestCase(PyPIRCCommandTestCase):

    def setUp(self):
        PyPIRCCommandTestCase.setUp(self)
        # patching the password prompt
        self._old_getpass = getpass.getpass
        def _getpass(prompt):
            return 'password'
        getpass.getpass = _getpass

    def tearDown(self):
        getpass.getpass = self._old_getpass
        PyPIRCCommandTestCase.tearDown(self)

    def test_create_pypirc(self):
        # this test makes sure a .pypirc file
        # is created when requested.

        # let's create a fake distribution
        # and a register instance
        dist = Distribution()
        dist.metadata.url = 'xxx'
        dist.metadata.author = 'xxx'
        dist.metadata.author_email = 'xxx'
        dist.metadata.name = 'xxx'
        dist.metadata.version =  'xxx'
        cmd = register(dist)

        # we shouldn't have a .pypirc file yet
        self.assert_(not os.path.exists(self.rc))

        # patching input and getpass.getpass
        # so register gets happy
        #
        # Here's what we are faking :
        # use your existing login (choice 1.)
        # Username : 'tarek'
        # Password : 'password'
        # Save your login (y/N)? : 'y'
        inputs = Inputs('1', 'tarek', 'y')
        from distutils.command import register as register_module
        register_module.input = inputs.__call__

        cmd.post_to_server = pypi_server = FakeServer()

        # let's run the command
        cmd.run()

        # we should have a brand new .pypirc file
        self.assert_(os.path.exists(self.rc))

        # with the content similar to WANTED_PYPIRC
        content = open(self.rc).read()
        self.assertEquals(content, WANTED_PYPIRC)

        # now let's make sure the .pypirc file generated
        # really works : we shouldn't be asked anything
        # if we run the command again
        def _no_way(prompt=''):
            raise AssertionError(prompt)
        register_module.raw_input = _no_way

        cmd.run()

        # let's see what the server received : we should
        # have 2 similar requests
        self.assert_(len(pypi_server.calls), 2)
        self.assert_(pypi_server.calls[0], pypi_server.calls[1])

    def test_password_not_in_file(self):

        f = open(self.rc, 'w')
        f.write(PYPIRC_NOPASSWORD)
        f.close()

        dist = Distribution()
        cmd = register(dist)
        cmd.post_to_server = FakeServer()

        cmd._set_config()
        cmd.finalize_options()
        cmd.send_metadata()

        # dist.password should be set
        # therefore used afterwards by other commands
        self.assertEquals(dist.password, 'password')

def test_suite():
    return unittest.makeSuite(registerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
