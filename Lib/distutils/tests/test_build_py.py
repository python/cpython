"""Tests for distutils.command.build_py."""

import os
import unittest

from distutils.command.build_py import build_py
from distutils.core import Distribution

from distutils.tests import support


class BuildPyTestCase(support.TempdirManager, unittest.TestCase):

    def test_package_data(self):
        sources = self.mkdtemp()
        f = open(os.path.join(sources, "__init__.py"), "w")
        f.write("# Pretend this is a package.")
        f.close()
        f = open(os.path.join(sources, "README.txt"), "w")
        f.write("Info about this package")
        f.close()

        destination = self.mkdtemp()

        dist = Distribution({"packages": ["pkg"],
                             "package_dir": {"pkg": sources}})
        # script_name need not exist, it just need to be initialized
        dist.script_name = os.path.join(sources, "setup.py")
        dist.command_obj["build"] = support.DummyCommand(
            force=0,
            build_lib=destination)
        dist.packages = ["pkg"]
        dist.package_data = {"pkg": ["README.txt"]}
        dist.package_dir = {"pkg": sources}

        cmd = build_py(dist)
        cmd.ensure_finalized()
        self.assertEqual(cmd.package_data, dist.package_data)

        cmd.run()

        self.assertEqual(len(cmd.get_outputs()), 2)
        pkgdest = os.path.join(destination, "pkg")
        files = os.listdir(pkgdest)
        self.assert_("__init__.py" in files)
        self.assert_("README.txt" in files)


def test_suite():
    return unittest.makeSuite(BuildPyTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
