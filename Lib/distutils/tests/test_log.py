"""Tests for distutils.log"""

import sys
import unittest
from tempfile import NamedTemporaryFile
from test.support import run_unittest

from distutils import log

class TestLog(unittest.TestCase):
    def test_non_ascii(self):
        # Issue #8663: test that non-ASCII text is escaped with
        # backslashreplace error handler (stream use ASCII encoding and strict
        # error handler)
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        try:
            log.set_threshold(log.DEBUG)
            with NamedTemporaryFile(mode="w+", encoding='ascii') as stdout, \
                 NamedTemporaryFile(mode="w+", encoding='ascii') as stderr:
                sys.stdout = stdout
                sys.stderr = stderr
                log.debug("debug:\xe9")
                log.fatal("fatal:\xe9")
                stdout.seek(0)
                self.assertEquals(stdout.read().rstrip(), "debug:\\xe9")
                stderr.seek(0)
                self.assertEquals(stderr.read().rstrip(), "fatal:\\xe9")
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr

def test_suite():
    return unittest.makeSuite(TestLog)

if __name__ == "__main__":
    run_unittest(test_suite())
