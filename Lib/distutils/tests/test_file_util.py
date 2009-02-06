"""Tests for distutils.file_util."""
import unittest
import os
import shutil

from distutils.file_util import move_file
from distutils import log

class FileUtilTestCase(unittest.TestCase):

    def _log(self, msg, *args):
        if len(args) > 0:
            self._logs.append(msg % args)
        else:
            self._logs.append(msg)

    def setUp(self):
        self._logs = []
        self.old_log = log.info
        log.info = self._log
        self.source = os.path.join(os.path.dirname(__file__), 'f1')
        self.target = os.path.join(os.path.dirname(__file__), 'f2')
        self.target_dir = os.path.join(os.path.dirname(__file__), 'd1')

    def tearDown(self):
        log.info = self.old_log
        for f in (self.source, self.target, self.target_dir):
            if os.path.exists(f):
                if os.path.isfile(f):
                    os.remove(f)
                else:
                    shutil.rmtree(f)

    def test_move_file_verbosity(self):

        f = open(self.source, 'w')
        f.write('some content')
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
