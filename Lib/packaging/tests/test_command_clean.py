"""Tests for distutils.command.clean."""
import os

from packaging.command.clean import clean
from packaging.tests import unittest, support


class cleanTestCase(support.TempdirManager, support.LoggingCatcher,
                    unittest.TestCase):

    def test_simple_run(self):
        pkg_dir, dist = self.create_dist()
        cmd = clean(dist)

        # let's add some elements clean should remove
        dirs = [(d, os.path.join(pkg_dir, d))
                for d in ('build_temp', 'build_lib', 'bdist_base',
                'build_scripts', 'build_base')]

        for name, path in dirs:
            os.mkdir(path)
            setattr(cmd, name, path)
            if name == 'build_base':
                continue
            for f in ('one', 'two', 'three'):
                self.write_file(os.path.join(path, f))

        # let's run the command
        cmd.all = True
        cmd.ensure_finalized()
        cmd.run()

        # make sure the files where removed
        for name, path in dirs:
            self.assertFalse(os.path.exists(path),
                             '%r was not removed' % path)

        # let's run the command again (should spit warnings but succeed)
        cmd.all = True
        cmd.ensure_finalized()
        cmd.run()


def test_suite():
    return unittest.makeSuite(cleanTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
