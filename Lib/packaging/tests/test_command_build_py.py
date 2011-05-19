"""Tests for distutils.command.build_py."""

import os
import sys

from packaging.command.build_py import build_py
from packaging.dist import Distribution
from packaging.errors import PackagingFileError

from packaging.tests import unittest, support


class BuildPyTestCase(support.TempdirManager,
                      support.LoggingCatcher,
                      unittest.TestCase):

    def test_package_data(self):
        sources = self.mkdtemp()
        pkg_dir = os.path.join(sources, 'pkg')
        os.mkdir(pkg_dir)
        f = open(os.path.join(pkg_dir, "__init__.py"), "w")
        try:
            f.write("# Pretend this is a package.")
        finally:
            f.close()
        f = open(os.path.join(pkg_dir, "README.txt"), "w")
        try:
            f.write("Info about this package")
        finally:
            f.close()

        destination = self.mkdtemp()

        dist = Distribution({"packages": ["pkg"],
                             "package_dir": sources})
        # script_name need not exist, it just need to be initialized

        dist.script_name = os.path.join(sources, "setup.py")
        dist.command_obj["build"] = support.DummyCommand(
            force=False,
            build_lib=destination,
            use_2to3_fixers=None,
            convert_2to3_doctests=None,
            use_2to3=False)
        dist.packages = ["pkg"]
        dist.package_data = {"pkg": ["README.txt"]}
        dist.package_dir = sources

        cmd = build_py(dist)
        cmd.compile = True
        cmd.ensure_finalized()
        self.assertEqual(cmd.package_data, dist.package_data)

        cmd.run()

        # This makes sure the list of outputs includes byte-compiled
        # files for Python modules but not for package data files
        # (there shouldn't *be* byte-code files for those!).
        #
        self.assertEqual(len(cmd.get_outputs()), 3)
        pkgdest = os.path.join(destination, "pkg")
        files = os.listdir(pkgdest)
        self.assertIn("__init__.py", files)
        self.assertIn("__init__.pyc", files)
        self.assertIn("README.txt", files)

    def test_empty_package_dir(self):
        # See SF 1668596/1720897.
        cwd = os.getcwd()

        # create the distribution files.
        sources = self.mkdtemp()
        pkg = os.path.join(sources, 'pkg')
        os.mkdir(pkg)
        open(os.path.join(pkg, "__init__.py"), "wb").close()
        testdir = os.path.join(pkg, "doc")
        os.mkdir(testdir)
        open(os.path.join(testdir, "testfile"), "wb").close()

        os.chdir(sources)
        old_stdout = sys.stdout
        #sys.stdout = StringIO.StringIO()

        try:
            dist = Distribution({"packages": ["pkg"],
                                 "package_dir": sources,
                                 "package_data": {"pkg": ["doc/*"]}})
            # script_name need not exist, it just need to be initialized
            dist.script_name = os.path.join(sources, "setup.py")
            dist.script_args = ["build"]
            dist.parse_command_line()

            try:
                dist.run_commands()
            except PackagingFileError as e:
                self.fail("failed package_data test when package_dir is ''")
        finally:
            # Restore state.
            os.chdir(cwd)
            sys.stdout = old_stdout

    @unittest.skipUnless(hasattr(sys, 'dont_write_bytecode'),
                         'sys.dont_write_bytecode not supported')
    def test_dont_write_bytecode(self):
        # makes sure byte_compile is not used
        pkg_dir, dist = self.create_dist()
        cmd = build_py(dist)
        cmd.compile = True
        cmd.optimize = 1

        old_dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True
        try:
            cmd.byte_compile([])
        finally:
            sys.dont_write_bytecode = old_dont_write_bytecode

        self.assertIn('byte-compiling is disabled', self.get_logs()[0])

def test_suite():
    return unittest.makeSuite(BuildPyTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
