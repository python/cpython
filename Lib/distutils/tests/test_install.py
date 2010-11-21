"""Tests for distutils.command.install."""

import os
import os.path
import sys
import unittest
import site

from test.support import captured_stdout

from distutils.command.install import install
from distutils.command import install as install_module
from distutils.command.install import INSTALL_SCHEMES
from distutils.core import Distribution
from distutils.errors import DistutilsOptionError

from distutils.tests import support

class InstallTestCase(support.TempdirManager,
                      support.EnvironGuard,
                      support.LoggingSilencer,
                      unittest.TestCase):

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
            self.assertTrue(key in INSTALL_SCHEMES)

        dist = Distribution({'name': 'xx'})
        cmd = install(dist)

        # making sure the user option is there
        options = [name for name, short, lable in
                   cmd.user_options]
        self.assertTrue('user' in options)

        # setting a value
        cmd.user = 1

        # user base and site shouldn't be created yet
        self.assertTrue(not os.path.exists(self.user_base))
        self.assertTrue(not os.path.exists(self.user_site))

        # let's run finalize
        cmd.ensure_finalized()

        # now they should
        self.assertTrue(os.path.exists(self.user_base))
        self.assertTrue(os.path.exists(self.user_site))

        self.assertTrue('userbase' in cmd.config_vars)
        self.assertTrue('usersite' in cmd.config_vars)

    def test_handle_extra_path(self):
        dist = Distribution({'name': 'xx', 'extra_path': 'path,dirs'})
        cmd = install(dist)

        # two elements
        cmd.handle_extra_path()
        self.assertEqual(cmd.extra_path, ['path', 'dirs'])
        self.assertEqual(cmd.extra_dirs, 'dirs')
        self.assertEqual(cmd.path_file, 'path')

        # one element
        cmd.extra_path = ['path']
        cmd.handle_extra_path()
        self.assertEqual(cmd.extra_path, ['path'])
        self.assertEqual(cmd.extra_dirs, 'path')
        self.assertEqual(cmd.path_file, 'path')

        # none
        dist.extra_path = cmd.extra_path = None
        cmd.handle_extra_path()
        self.assertEqual(cmd.extra_path, None)
        self.assertEqual(cmd.extra_dirs, '')
        self.assertEqual(cmd.path_file, None)

        # three elements (no way !)
        cmd.extra_path = 'path,dirs,again'
        self.assertRaises(DistutilsOptionError, cmd.handle_extra_path)

    def test_finalize_options(self):
        dist = Distribution({'name': 'xx'})
        cmd = install(dist)

        # must supply either prefix/exec-prefix/home or
        # install-base/install-platbase -- not both
        cmd.prefix = 'prefix'
        cmd.install_base = 'base'
        self.assertRaises(DistutilsOptionError, cmd.finalize_options)

        # must supply either home or prefix/exec-prefix -- not both
        cmd.install_base = None
        cmd.home = 'home'
        self.assertRaises(DistutilsOptionError, cmd.finalize_options)

        # can't combine user with with prefix/exec_prefix/home or
        # install_(plat)base
        cmd.prefix = None
        cmd.user = 'user'
        self.assertRaises(DistutilsOptionError, cmd.finalize_options)

    def test_record(self):

        install_dir = self.mkdtemp()
        pkgdir, dist = self.create_dist()

        dist = Distribution()
        cmd = install(dist)
        dist.command_obj['install'] = cmd
        cmd.root = install_dir
        cmd.record = os.path.join(pkgdir, 'RECORD')
        cmd.ensure_finalized()

        cmd.run()

        # let's check the RECORD file was created with one
        # line (the egg info file)
        f = open(cmd.record)
        try:
            self.assertEqual(len(f.readlines()), 1)
        finally:
            f.close()

    def test_debug_mode(self):
        # this covers the code called when DEBUG is set
        old_logs_len = len(self.logs)
        install_module.DEBUG = True
        try:
            with captured_stdout() as stdout:
                self.test_record()
        finally:
            install_module.DEBUG = False
        self.assertTrue(len(self.logs) > old_logs_len)

def test_suite():
    return unittest.makeSuite(InstallTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
