"""Tests for distutils.command.config."""
import unittest
import os

from distutils.command.config import dump_file
from distutils.tests import support
from distutils import log

class ConfigTestCase(support.LoggingSilencer,
                     support.TempdirManager,
                     unittest.TestCase):

    def _info(self, msg):
        for line in msg.splitlines():
            self._logs.append(line)

    def setUp(self):
        super(ConfigTestCase, self).setUp()
        self._logs = []
        self.old_log = log.info
        log.info = self._info

    def tearDown(self):
        log.info = self.old_log
        super(ConfigTestCase, self).tearDown()

    def test_dump_file(self):
        this_file = os.path.splitext(__file__)[0] + '.py'
        f = open(this_file)
        try:
            numlines = len(f.readlines())
        finally:
            f.close()

        dump_file(this_file, 'I am the header')
        self.assertEquals(len(self._logs), numlines+1)

def test_suite():
    return unittest.makeSuite(ConfigTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
