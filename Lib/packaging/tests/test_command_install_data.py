"""Tests for packaging.command.install_data."""
import os
import sysconfig
from sysconfig import _get_default_scheme
from packaging.tests import unittest, support
from packaging.command.install_data import install_data


class InstallDataTestCase(support.TempdirManager,
                          support.LoggingCatcher,
                          unittest.TestCase):

    def test_simple_run(self):
        scheme = _get_default_scheme()
        old_items = sysconfig._SCHEMES.items(scheme)
        def restore():
            sysconfig._SCHEMES.remove_section(scheme)
            sysconfig._SCHEMES.add_section(scheme)
            for option, value in old_items:
                sysconfig._SCHEMES.set(scheme, option, value)
        self.addCleanup(restore)

        pkg_dir, dist = self.create_dist()
        cmd = install_data(dist)
        cmd.install_dir = inst = os.path.join(pkg_dir, 'inst')

        sysconfig._SCHEMES.set(scheme, 'inst',
                               os.path.join(pkg_dir, 'inst'))
        sysconfig._SCHEMES.set(scheme, 'inst2',
                               os.path.join(pkg_dir, 'inst2'))

        one = os.path.join(pkg_dir, 'one')
        self.write_file(one, 'xxx')
        inst2 = os.path.join(pkg_dir, 'inst2')
        two = os.path.join(pkg_dir, 'two')
        self.write_file(two, 'xxx')

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

        sysconfig._SCHEMES.set(scheme, 'inst3',
                               cmd.install_dir)

        cmd.data_files = {one: '{inst}/one', two: '{inst2}/two',
                          three: '{inst3}/three'}
        cmd.ensure_finalized()
        cmd.run()

        # let's check the result
        self.assertEqual(len(cmd.get_outputs()), 3)
        self.assertTrue(os.path.exists(os.path.join(inst2, rtwo)))
        self.assertTrue(os.path.exists(os.path.join(inst, rone)))


def test_suite():
    return unittest.makeSuite(InstallDataTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
