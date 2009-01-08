"""Tests for distutils.command.upload."""
import sys
import os
import unittest

from distutils.command.upload import upload
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


class uploadTestCase(PyPIRCCommandTestCase):

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

    def test_saved_password(self):
        # file with no password
        f = open(self.rc, 'w')
        f.write(PYPIRC_NOPASSWORD)
        f.close()

        # make sure it passes
        dist = Distribution()
        cmd = upload(dist)
        cmd.finalize_options()
        self.assertEquals(cmd.password, None)

        # make sure we get it as well, if another command
        # initialized it at the dist level
        dist.password = 'xxx'
        cmd = upload(dist)
        cmd.finalize_options()
        self.assertEquals(cmd.password, 'xxx')

def test_suite():
    return unittest.makeSuite(uploadTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
