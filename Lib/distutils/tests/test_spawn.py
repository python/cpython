"""Tests for distutils.spawn."""
import unittest
import sys
import os
from test.support import run_unittest, unix_shell

from distutils.spawn import _nt_quote_args
from distutils.spawn import spawn
from distutils.errors import DistutilsExecError
from distutils.tests import support

class SpawnTestCase(support.TempdirManager,
                    support.LoggingSilencer,
                    unittest.TestCase):

    def test_nt_quote_args(self):

        for (args, wanted) in ((['with space', 'nospace'],
                                ['"with space"', 'nospace']),
                               (['nochange', 'nospace'],
                                ['nochange', 'nospace'])):
            res = _nt_quote_args(args)
            self.assertEqual(res, wanted)


    @unittest.skipUnless(os.name in ('nt', 'posix'),
                         'Runs only under posix or nt')
    def test_spawn(self):
        tmpdir = self.mkdtemp()

        # creating something executable
        # through the shell that returns 1
        if sys.platform != 'win32':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!%s\nexit 1' % unix_shell)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 1')

        os.chmod(exe, 0o777)
        self.assertRaises(DistutilsExecError, spawn, [exe])

        # now something that works
        if sys.platform != 'win32':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!%s\nexit 0' % unix_shell)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 0')

        os.chmod(exe, 0o777)
        spawn([exe])  # should work without any error

def test_suite():
    return unittest.makeSuite(SpawnTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
