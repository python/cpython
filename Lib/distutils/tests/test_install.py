"""Tests for distutils.command.install."""

import os
import os.path
import sys
import unittest
import site

from distutils.command.install import install
from distutils.command import install as install_module
from distutils.command.install import INSTALL_SCHEMES
from distutils.core import Distribution

from distutils.tests import support


class InstallTestCase(support.TempdirManager, unittest.TestCase):

    def test_home_installation_scheme(self):
        # This ensure two things:
        # - that --home generates the desired set of directory names
        # - test --home is supported on all platforms
        builddir = self.mkdtemp()
        destination = os.path.join(builddir, "installation")

        dist = Distribution({"name": "foopkg"})
        # script_name need not exist, it just need to be initialized
        dist.script_name = os.path.join(builddir, "setup.py")
        dist.command_obj["build"] = support.DummyCommand(
            build_base=builddir,
            build_lib=os.path.join(builddir, "lib"),
            )

        cmd = install(dist)
        cmd.home = destination
        cmd.ensure_finalized()

        self.assertEqual(cmd.install_base, destination)
        self.assertEqual(cmd.install_platbase, destination)

        def check_path(got, expected):
            got = os.path.normpath(got)
            expected = os.path.normpath(expected)
            self.assertEqual(got, expected)

        libdir = os.path.join(destination, "lib", "python")
        check_path(cmd.install_lib, libdir)
        check_path(cmd.install_platlib, libdir)
        check_path(cmd.install_purelib, libdir)
        check_path(cmd.install_headers,
                   os.path.join(destination, "include", "python", "foopkg"))
        check_path(cmd.install_scripts, os.path.join(destination, "bin"))
        check_path(cmd.install_data, destination)

    def test_user_site(self):
        # site.USER_SITE was introduced in 2.6
        if sys.version < '2.6':
            return

        # preparing the environement for the test
        self.old_user_base = site.USER_BASE
        self.old_user_site = site.USER_SITE
        self.tmpdir = self.mkdtemp()
        self.user_base = os.path.join(self.tmpdir, 'B')
        self.user_site = os.path.join(self.tmpdir, 'S')
        site.USER_BASE = self.user_base
        site.USER_SITE = self.user_site
        install_module.USER_BASE = self.user_base
        install_module.USER_SITE = self.user_site

        def _expanduser(path):
            return self.tmpdir
        self.old_expand = os.path.expanduser
        os.path.expanduser = _expanduser

        try:
            # this is the actual test
            self._test_user_site()
        finally:
            site.USER_BASE = self.old_user_base
            site.USER_SITE = self.old_user_site
            install_module.USER_BASE = self.old_user_base
            install_module.USER_SITE = self.old_user_site
            os.path.expanduser = self.old_expand

    def _test_user_site(self):
        for key in ('nt_user', 'unix_user', 'os2_home'):
            self.assert_(key in INSTALL_SCHEMES)

        dist = Distribution({'name': 'xx'})
        cmd = install(dist)

        # making sure the user option is there
        options = [name for name, short, lable in
                   cmd.user_options]
        self.assert_('user' in options)

        # setting a value
        cmd.user = 1

        # user base and site shouldn't be created yet
        self.assert_(not os.path.exists(self.user_base))
        self.assert_(not os.path.exists(self.user_site))

        # let's run finalize
        cmd.ensure_finalized()

        # now they should
        self.assert_(os.path.exists(self.user_base))
        self.assert_(os.path.exists(self.user_site))

        self.assert_('userbase' in cmd.config_vars)
        self.assert_('usersite' in cmd.config_vars)

def test_suite():
    return unittest.makeSuite(InstallTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
