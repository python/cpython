"""Tests for distutils.command.build_py."""

import os
import sys
import imp

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
        pycache_dir = os.path.join(pkgdest, "__pycache__")
        self.assertIn("__init__.py", files)
        self.assertIn("README.txt", files)
        pyc_files = os.listdir(pycache_dir)
        self.assertIn("__init__.%s.pyc" % imp.get_tag(), pyc_files)

    def test_empty_package_dir(self):
        # See SF 1668596/1720897.
        # create the distribution files.
        sources = self.mkdtemp()
        pkg = os.path.join(sources, 'pkg')
        os.mkdir(pkg)
        open(os.path.join(pkg, "__init__.py"), "wb").close()
        testdir = os.path.join(pkg, "doc")
        os.mkdir(testdir)
        open(os.path.join(testdir, "testfile"), "wb").close()

        os.chdir(sources)
        dist = Distribution({"packages": ["pkg"],
                             "package_dir": sources,
                             "package_data": {"pkg": ["doc/*"]}})
        dist.script_args = ["build"]
        dist.parse_command_line()

        try:
            dist.run_commands()
        except PackagingFileError:
            self.fail("failed package_data test when package_dir is ''")

    def test_byte_compile(self):
        project_dir, dist = self.create_dist(py_modules=['boiledeggs'])
        os.chdir(project_dir)
        self.write_file('boiledeggs.py', 'import antigravity')
        cmd = build_py(dist)
        cmd.compile = True
        cmd.build_lib = 'here'
        cmd.finalize_options()
        cmd.run()

        found = os.listdir(cmd.build_lib)
        self.assertEqual(sorted(found), ['__pycache__', 'boiledeggs.py'])
        found = os.listdir(os.path.join(cmd.build_lib, '__pycache__'))
        self.assertEqual(found, ['boiledeggs.%s.pyc' % imp.get_tag()])

    def test_byte_compile_optimized(self):
        project_dir, dist = self.create_dist(py_modules=['boiledeggs'])
        os.chdir(project_dir)
        self.write_file('boiledeggs.py', 'import antigravity')
        cmd = build_py(dist)
        cmd.compile = True
        cmd.optimize = 1
        cmd.build_lib = 'here'
        cmd.finalize_options()
        cmd.run()

        found = os.listdir(cmd.build_lib)
        self.assertEqual(sorted(found), ['__pycache__', 'boiledeggs.py'])
        found = os.listdir(os.path.join(cmd.build_lib, '__pycache__'))
        self.assertEqual(sorted(found), ['boiledeggs.%s.pyc' % imp.get_tag(),
                                         'boiledeggs.%s.pyo' % imp.get_tag()])

    def test_byte_compile_under_B(self):
        # make sure byte compilation works under -B (dont_write_bytecode)
        self.addCleanup(setattr, sys, 'dont_write_bytecode',
                        sys.dont_write_bytecode)
        sys.dont_write_bytecode = True
        self.test_byte_compile()
        self.test_byte_compile_optimized()


def test_suite():
    return unittest.makeSuite(BuildPyTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
