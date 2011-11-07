"""Tests for packaging.command.install_data."""
import os
import sys
import sysconfig
import packaging.database
from sysconfig import _get_default_scheme
from packaging.tests import unittest, support
from packaging.command.install_data import install_data
from packaging.command.install_dist import install_dist
from packaging.command.install_distinfo import install_distinfo


class InstallDataTestCase(support.TempdirManager,
                          support.LoggingCatcher,
                          unittest.TestCase):

    def setUp(self):
        super(InstallDataTestCase, self).setUp()
        scheme = _get_default_scheme()
        old_items = sysconfig._SCHEMES.items(scheme)

        def restore():
            sysconfig._SCHEMES.remove_section(scheme)
            sysconfig._SCHEMES.add_section(scheme)
            for option, value in old_items:
                sysconfig._SCHEMES.set(scheme, option, value)

        self.addCleanup(restore)

    def test_simple_run(self):
        pkg_dir, dist = self.create_dist()
        cmd = install_data(dist)
        cmd.install_dir = inst = os.path.join(pkg_dir, 'inst')
        scheme = _get_default_scheme()

        sysconfig._SCHEMES.set(scheme, 'inst',
                               os.path.join(pkg_dir, 'inst'))
        sysconfig._SCHEMES.set(scheme, 'inst2',
                               os.path.join(pkg_dir, 'inst2'))

        one = os.path.join(pkg_dir, 'one')
        self.write_file(one, 'xxx')
        inst2 = os.path.join(pkg_dir, 'inst2')
        two = os.path.join(pkg_dir, 'two')
        self.write_file(two, 'xxx')

        # FIXME this creates a literal \{inst2\} directory!
        cmd.data_files = {one: '{inst}/one', two: '{inst2}/two'}
        self.assertCountEqual(cmd.get_inputs(), [one, two])

        # let's run the command
        cmd.ensure_finalized()
        cmd.run()

        # let's check the result
        self.assertEqual(len(cmd.get_outputs()), 2)
        rtwo = os.path.split(two)[-1]
        self.assertTrue(os.path.exists(os.path.join(inst2, rtwo)))
        rone = os.path.split(one)[-1]
        self.assertTrue(os.path.exists(os.path.join(inst, rone)))
        cmd.outfiles = []

        # let's try with warn_dir one
        cmd.warn_dir = True
        cmd.finalized = False
        cmd.ensure_finalized()
        cmd.run()

        # let's check the result
        self.assertEqual(len(cmd.get_outputs()), 2)
        self.assertTrue(os.path.exists(os.path.join(inst2, rtwo)))
        self.assertTrue(os.path.exists(os.path.join(inst, rone)))
        cmd.outfiles = []

        # now using root and empty dir
        cmd.root = os.path.join(pkg_dir, 'root')
        three = os.path.join(cmd.install_dir, 'three')
        self.write_file(three, 'xx')

        sysconfig._SCHEMES.set(scheme, 'inst3', cmd.install_dir)

        cmd.data_files = {one: '{inst}/one', two: '{inst2}/two',
                          three: '{inst3}/three'}
        cmd.finalized = False
        cmd.ensure_finalized()
        cmd.run()

        # let's check the result
        self.assertEqual(len(cmd.get_outputs()), 3)
        self.assertTrue(os.path.exists(os.path.join(inst2, rtwo)))
        self.assertTrue(os.path.exists(os.path.join(inst, rone)))

    def test_resources(self):
        install_dir = self.mkdtemp()
        scripts_dir = self.mkdtemp()
        project_dir, dist = self.create_dist(
            name='Spamlib', version='0.1',
            data_files={'spamd': '{scripts}/spamd'})

        os.chdir(project_dir)
        self.write_file('spamd', '# Python script')
        sysconfig._SCHEMES.set(_get_default_scheme(), 'scripts', scripts_dir)
        sys.path.insert(0, install_dir)
        packaging.database.disable_cache()
        self.addCleanup(sys.path.remove, install_dir)
        self.addCleanup(packaging.database.enable_cache)

        cmd = install_dist(dist)
        cmd.outputs = ['spamd']
        cmd.install_lib = install_dir
        dist.command_obj['install_dist'] = cmd

        cmd = install_data(dist)
        cmd.install_dir = install_dir
        cmd.ensure_finalized()
        dist.command_obj['install_data'] = cmd
        cmd.run()

        cmd = install_distinfo(dist)
        cmd.ensure_finalized()
        dist.command_obj['install_distinfo'] = cmd
        cmd.run()

        # first a few sanity checks
        self.assertEqual(os.listdir(scripts_dir), ['spamd'])
        self.assertEqual(os.listdir(install_dir), ['Spamlib-0.1.dist-info'])

        # now the real test
        fn = os.path.join(install_dir, 'Spamlib-0.1.dist-info', 'RESOURCES')
        with open(fn, encoding='utf-8') as fp:
            content = fp.read().strip()

        expected = 'spamd,%s' % os.path.join(scripts_dir, 'spamd')
        self.assertEqual(content, expected)

        # just to be sure, we also test that get_file works here, even though
        # packaging.database has its own test file
        with packaging.database.get_file('Spamlib', 'spamd') as fp:
            content = fp.read()

        self.assertEqual('# Python script', content)


def test_suite():
    return unittest.makeSuite(InstallDataTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
