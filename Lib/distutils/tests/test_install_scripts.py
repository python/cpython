"""Tests for distutils.command.install_scripts."""

import os
import shutil
import tempfile
import unittest

from distutils.command.install_scripts import install_scripts
from distutils.core import Distribution


class InstallScriptsTestCase(unittest.TestCase):

    def setUp(self):
        self.tempdirs = []

    def tearDown(self):
        while self.tempdirs:
            d = self.tempdirs.pop()
            shutil.rmtree(d)

    def mkdtemp(self):
        """Create a temporary directory that will be cleaned up.

        Returns the path of the directory.
        """
        d = tempfile.mkdtemp()
        self.tempdirs.append(d)
        return d

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

    def test_installation(self):
        source = self.mkdtemp()
        expected = []

        def write_script(name, text):
            expected.append(name)
            f = open(os.path.join(source, name), "w")
            f.write(text)
            f.close()

        write_script("script1.py", ("#! /usr/bin/env python2.3\n"
                                    "# bogus script w/ Python sh-bang\n"
                                    "pass\n"))
        write_script("script2.py", ("#!/usr/bin/python\n"
                                    "# bogus script w/ Python sh-bang\n"
                                    "pass\n"))
        write_script("shell.sh", ("#!/bin/sh\n"
                                  "# bogus shell script w/ sh-bang\n"
                                  "exit 0\n"))

        target = self.mkdtemp()
        dist = Distribution()
        dist.command_obj["build"] = DummyCommand(build_scripts=source)
        dist.command_obj["install"] = DummyCommand(
            install_scripts=target,
            force=1,
            skip_build=1,
            )
        cmd = install_scripts(dist)
        cmd.finalize_options()
        cmd.run()

        installed = os.listdir(target)
        for name in expected:
            self.assert_(name in installed)


class DummyCommand:
    """Class to store options for retrieval via set_undefined_options()."""

    def __init__(self, **kwargs):
        for kw, val in kwargs.items():
            setattr(self, kw, val)

    def ensure_finalized(self):
        pass



def test_suite():
    return unittest.makeSuite(InstallScriptsTestCase)
