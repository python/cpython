"""Tests for distutils.file_util."""
import unittest
import os
import shutil

from distutils.file_util import move_file
from distutils import log
from distutils.tests import support

class FileUtilTestCase(support.TempdirManager, unittest.TestCase):

    def _log(self, msg, *args):
        if len(args) > 0:
            self._logs.append(msg % args)
        else:
            self._logs.append(msg)

    def setUp(self):
        super(FileUtilTestCase, self).setUp()
        self._logs = []
        self.old_log = log.info
        log.info = self._log
        tmp_dir = self.mkdtemp()
        self.source = os.path.join(tmp_dir, 'f1')
        self.target = os.path.join(tmp_dir, 'f2')
        self.target_dir = os.path.join(tmp_dir, 'd1')

    def tearDown(self):
        log.info = self.old_log
        super(FileUtilTestCase, self).tearDown()

    def test_move_file_verbosity(self):
        f = open(self.source, 'w')
        try:
            f.write('some content')
        finally:
            f.close()

        move_file(self.source, self.target, verbose=0)
        wanted = []
        self.assertEquals(self._logs, wanted)

        # back to original state
        move_file(self.target, self.source, verbose=0)

        move_file(self.source, self.target, verbose=1)
        wanted = ['moving %s -> %s' % (self.source, self.target)]
        self.assertEquals(self._logs, wanted)

        # back to original state
        move_file(self.target, self.source, verbose=0)

        self._logs = []
        # now the target is a dir
        os.mkdir(self.target_dir)
        move_file(self.source, self.target_dir, verbose=1)
        wanted = ['moving %s -> %s' % (self.source, self.target_dir)]
        self.assertEquals(self._logs, wanted)


def test_suite():
    return unittest.makeSuite(FileUtilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
