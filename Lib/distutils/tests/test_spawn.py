"""Tests for distutils.spawn."""
import os
import stat
import sys
import time
import unittest
from test.support import captured_stdout, run_unittest
from test import support as test_support

from distutils.spawn import _nt_quote_args
from distutils.spawn import spawn, find_executable
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
        if os.name == 'posix':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!/bin/sh\nexit 1')
            os.chmod(exe, 0777)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 1')

        os.chmod(exe, 0777)
        self.assertRaises(DistutilsExecError, spawn, [exe])

        # now something that works
        if os.name == 'posix':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!/bin/sh\nexit 0')
            os.chmod(exe, 0777)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 0')

        os.chmod(exe, 0777)
        spawn([exe])  # should work without any error

    def test_find_executable(self):
        with test_support.temp_dir() as tmp_dir:
            # use TESTFN to get a pseudo-unique filename
            program_noeext = test_support.TESTFN
            # Give the temporary program an ".exe" suffix for all.
            # It's needed on Windows and not harmful on other platforms.
            program = program_noeext + ".exe"

            filename = os.path.join(tmp_dir, program)
            with open(filename, "wb"):
                pass
            os.chmod(filename, stat.S_IXUSR)

            # test path parameter
            rv = find_executable(program, path=tmp_dir)
            self.assertEqual(rv, filename)

            if sys.platform == 'win32':
                # test without ".exe" extension
                rv = find_executable(program_noeext, path=tmp_dir)
                self.assertEqual(rv, filename)

            # test find in the current directory
            with test_support.change_cwd(tmp_dir):
                rv = find_executable(program)
                self.assertEqual(rv, program)

            # test non-existent program
            dont_exist_program = "dontexist_" + program
            rv = find_executable(dont_exist_program , path=tmp_dir)
            self.assertIsNone(rv)

            # test os.defpath: missing PATH environment variable
            with test_support.EnvironmentVarGuard() as env:
                from distutils import spawn
                with test_support.swap_attr(spawn.os, 'defpath', tmp_dir):
                    env.pop('PATH')

                    rv = find_executable(program)
                    self.assertEqual(rv, filename)


def test_suite():
    return unittest.makeSuite(SpawnTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
