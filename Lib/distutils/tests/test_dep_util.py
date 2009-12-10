"""Tests for distutils.dep_util."""
import unittest
import os
import time

from distutils.dep_util import newer, newer_pairwise, newer_group
from distutils.errors import DistutilsFileError
from distutils.tests import support

# XXX needs to be tuned for the various platforms
_ST_MIME_TIMER = 1

class DepUtilTestCase(support.TempdirManager, unittest.TestCase):

    def test_newer(self):
        tmpdir = self.mkdtemp()
        target = os.path.join(tmpdir, 'target')
        source = os.path.join(tmpdir, 'source')

        # Raise DistutilsFileError if 'source' does not exist.
        self.assertRaises(DistutilsFileError, newer, target, source)

        # Return true if 'source' exists and is more recently modified than
        # 'target', or if 'source' exists and 'target' doesn't.
        self.write_file(target)
        self.assertTrue(newer(target, source))
        self.write_file(source, 'xox')
        time.sleep(_ST_MIME_TIMER)  # ensures ST_MTIME differs
        self.write_file(target, 'xhx')
        self.assertTrue(newer(target, source))

        # Return false if both exist and 'target' is the same age or younger
        # than 'source'.
        self.write_file(source, 'xox'); self.write_file(target, 'xhx')
        self.assertFalse(newer(target, source))
        self.write_file(target, 'xox')
        time.sleep(_ST_MIME_TIMER)
        self.write_file(source, 'xhx')
        self.assertFalse(newer(target, source))

    def test_newer_pairwise(self):
        tmpdir = self.mkdtemp()
        sources = os.path.join(tmpdir, 'sources')
        targets = os.path.join(tmpdir, 'targets')
        os.mkdir(sources)
        os.mkdir(targets)
        one = os.path.join(sources, 'one')
        two = os.path.join(sources, 'two')
        three = os.path.join(targets, 'three')
        four = os.path.join(targets, 'four')

        self.write_file(one)
        self.write_file(three)
        self.write_file(four)
        time.sleep(_ST_MIME_TIMER)
        self.write_file(two)

        self.assertEquals(newer_pairwise([one, two], [three, four]),
                          ([two],[four]))

    def test_newer_group(self):
        tmpdir = self.mkdtemp()
        sources = os.path.join(tmpdir, 'sources')
        os.mkdir(sources)
        one = os.path.join(sources, 'one')
        two = os.path.join(sources, 'two')
        three = os.path.join(sources, 'three')
        target = os.path.join(tmpdir, 'target')

        # return true if 'target' is out-of-date with respect to any file
        # listed in 'sources'.
        self.write_file(target)
        time.sleep(_ST_MIME_TIMER)
        self.write_file(one)
        self.write_file(two)
        self.write_file(three)
        self.assertTrue(newer_group([one, two, three], target))

        self.write_file(one)
        self.write_file(three)
        self.write_file(two)
        time.sleep(0.1)
        self.write_file(target)
        self.assertFalse(newer_group([one, two, three], target))

        # missing handling
        os.remove(one)
        self.assertRaises(OSError, newer_group, [one, two, three], target)

        self.assertFalse(newer_group([one, two, three], target,
                                     missing='ignore'))

        self.assertTrue(newer_group([one, two, three], target,
                                    missing='newer'))


def test_suite():
    return unittest.makeSuite(DepUtilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
