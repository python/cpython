# Copyright (C) 2003 Python Software Foundation

import unittest
import macostools
import Carbon.File
import MacOS
import os
import sys
from test import test_support

TESTFN2 = test_support.TESTFN + '2'

class TestMacostools(unittest.TestCase):

    def setUp(self):
        fp = open(test_support.TESTFN, 'w')
        fp.write('hello world\n')
        fp.close()
        rfp = MacOS.openrf(test_support.TESTFN, '*wb')
        rfp.write('goodbye world\n')
        rfp.close()

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)
        test_support.unlink(TESTFN2)

    def compareData(self):
        fp = open(test_support.TESTFN, 'r')
        data1 = fp.read()
        fp.close()
        fp = open(TESTFN2, 'r')
        data2 = fp.read()
        fp.close()
        if data1 != data2:
            return 'Data forks differ'
        rfp = MacOS.openrf(test_support.TESTFN, '*rb')
        data1 = rfp.read(1000)
        rfp.close()
        rfp = MacOS.openrf(TESTFN2, '*rb')
        data2 = rfp.read(1000)
        rfp.close()
        if data1 != data2:
            return 'Resource forks differ'
        return ''

    def test_touched(self):
        # This really only tests that nothing unforeseen happens.
        with test_support.check_warnings(('macostools.touched*',
                                          DeprecationWarning), quiet=True):
            macostools.touched(test_support.TESTFN)

    if sys.maxint < 2**32:
        def test_copy(self):
            test_support.unlink(TESTFN2)
            macostools.copy(test_support.TESTFN, TESTFN2)
            self.assertEqual(self.compareData(), '')

    if sys.maxint < 2**32:
        def test_mkalias(self):
            test_support.unlink(TESTFN2)
            macostools.mkalias(test_support.TESTFN, TESTFN2)
            fss, _, _ = Carbon.File.ResolveAliasFile(TESTFN2, 0)
            self.assertEqual(fss.as_pathname(), os.path.realpath(test_support.TESTFN))

        def test_mkalias_relative(self):
            test_support.unlink(TESTFN2)
            # If the directory doesn't exist, then chances are this is a new
            # install of Python so don't create it since the user might end up
            # running ``sudo make install`` and creating the directory here won't
            # leave it with the proper permissions.
            if not os.path.exists(sys.prefix):
                return
            macostools.mkalias(test_support.TESTFN, TESTFN2, sys.prefix)
            fss, _, _ = Carbon.File.ResolveAliasFile(TESTFN2, 0)
            self.assertEqual(fss.as_pathname(), os.path.realpath(test_support.TESTFN))


def test_main():
    # Skip on wide unicode
    if len(u'\0'.encode('unicode-internal')) == 4:
        raise test_support.TestSkipped("test_macostools is broken in USC4")
    test_support.run_unittest(TestMacostools)


if __name__ == '__main__':
    test_main()
