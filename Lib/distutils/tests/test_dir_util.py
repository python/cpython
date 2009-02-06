"""Tests for distutils.dir_util."""
import unittest
import os
import shutil

from distutils.dir_util import mkpath
from distutils.dir_util import remove_tree
from distutils.dir_util import create_tree
from distutils.dir_util import copy_tree

from distutils import log

class DirUtilTestCase(unittest.TestCase):

    def _log(self, msg, *args):
        if len(args) > 0:
            self._logs.append(msg % args)
        else:
            self._logs.append(msg)

    def setUp(self):
        self._logs = []
        self.root_target = os.path.join(os.path.dirname(__file__), 'deep')
        self.target = os.path.join(self.root_target, 'here')
        self.target2 = os.path.join(os.path.dirname(__file__), 'deep2')
        self.old_log = log.info
        log.info = self._log

    def tearDown(self):
        for target in (self.target, self.target2):
            if os.path.exists(target):
                shutil.rmtree(target)
        log.info = self.old_log

    def test_mkpath_remove_tree_verbosity(self):

        mkpath(self.target, verbose=0)
        wanted = []
        self.assertEquals(self._logs, wanted)
        remove_tree(self.root_target, verbose=0)

        mkpath(self.target, verbose=1)
        wanted = ['creating %s' % self.root_target,
                  'creating %s' % self.target]
        self.assertEquals(self._logs, wanted)
        self._logs = []

        remove_tree(self.root_target, verbose=1)
        wanted = ["removing '%s' (and everything under it)" % self.root_target]
        self.assertEquals(self._logs, wanted)

    def test_create_tree_verbosity(self):

        create_tree(self.root_target, ['one', 'two', 'three'], verbose=0)
        self.assertEquals(self._logs, [])
        remove_tree(self.root_target, verbose=0)

        wanted = ['creating %s' % self.root_target]
        create_tree(self.root_target, ['one', 'two', 'three'], verbose=1)
        self.assertEquals(self._logs, wanted)

        remove_tree(self.root_target, verbose=0)


    def test_copy_tree_verbosity(self):

        mkpath(self.target, verbose=0)

        copy_tree(self.target, self.target2, verbose=0)
        self.assertEquals(self._logs, [])

        remove_tree(self.root_target, verbose=0)

        mkpath(self.target, verbose=0)
        a_file = os.path.join(self.target, 'ok.txt')
        f = open(a_file, 'w')
        f.write('some content')
        f.close()

        wanted = ['copying %s -> %s' % (a_file, self.target2)]
        copy_tree(self.target, self.target2, verbose=1)
        self.assertEquals(self._logs, wanted)

        remove_tree(self.root_target, verbose=0)
        remove_tree(self.target2, verbose=0)

def test_suite():
    return unittest.makeSuite(DirUtilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
