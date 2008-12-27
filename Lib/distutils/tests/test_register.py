"""Tests for distutils.command.register."""
import sys
import os
import unittest

from distutils.command.register import register
from distutils.core import Distribution

from distutils.tests import support
from distutils.tests.test_config import PYPIRC, PyPIRCCommandTestCase

class RawInputs(object):
    """Fakes user inputs."""
    def __init__(self, *answers):
        self.answers = answers
        self.index = 0

    def __call__(self, prompt=''):
        try:
            return self.answers[self.index]
        finally:
            self.index += 1

WANTED_PYPIRC = """\
[distutils]
index-servers =
    pypi

[pypi]
username:tarek
password:xxx
"""

class registerTestCase(PyPIRCCommandTestCase):

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

        # patching raw_input and getpass.getpass
        # so register gets happy
        #
        # Here's what we are faking :
        # use your existing login (choice 1.)
        # Username : 'tarek'
        # Password : 'xxx'
        # Save your login (y/N)? : 'y'
        inputs = RawInputs('1', 'tarek', 'y')
        from distutils.command import register as register_module
        register_module.raw_input = inputs.__call__
        def _getpass(prompt):
            return 'xxx'
        register_module.getpass.getpass = _getpass
        class FakeServer(object):
            def __init__(self):
                self.calls = []

            def __call__(self, *args):
                # we want to compare them, so let's store
                # something comparable
                els = args[0].items()
                els.sort()
                self.calls.append(tuple(els))
                return 200, 'OK'

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

def test_suite():
    return unittest.makeSuite(registerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
