"""Tests for distutils.command.install_scripts."""

import os
import unittest

from distutils.command.install_scripts import install_scripts
from distutils.core import Distribution


class InstallScriptsTestCase(unittest.TestCase):

    def test_default_settings(self):
        dist = Distribution()
        dist.command_obj["build"] = DummyCommand(build_scripts="/foo/bar")
        dist.command_obj["install"] = DummyCommand(
            install_scripts="/splat/funk",
            force=1,
            skip_build=1,
            )
        cmd = install_scripts(dist)
        self.assert_(not cmd.force)
        self.assert_(not cmd.skip_build)
        self.assert_(cmd.build_dir is None)
        self.assert_(cmd.install_dir is None)

        cmd.finalize_options()

        self.assert_(cmd.force)
        self.assert_(cmd.skip_build)
        self.assertEqual(cmd.build_dir, "/foo/bar")
        self.assertEqual(cmd.install_dir, "/splat/funk")


class DummyCommand:

    def __init__(self, **kwargs):
        for kw, val in kwargs.items():
            setattr(self, kw, val)

    def ensure_finalized(self):
        pass



def test_suite():
    return unittest.makeSuite(InstallScriptsTestCase)
