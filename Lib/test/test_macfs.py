# Copyright (C) 2003 Python Software Foundation

import unittest
import warnings
warnings.filterwarnings("ignore", "macfs.*", DeprecationWarning, __name__)
import macfs
import os
import sys
import tempfile
from test import test_support

class TestMacfs(unittest.TestCase):

    def setUp(self):
        fp = open(test_support.TESTFN, 'w')
        fp.write('hello world\n')
        fp.close()

    def tearDown(self):
        try:
            os.unlink(test_support.TESTFN)
        except:
            pass

    def test_fsspec(self):
        fss = macfs.FSSpec(test_support.TESTFN)
        self.assertEqual(os.path.realpath(test_support.TESTFN), fss.as_pathname())

    def test_fsref(self):
        fsr = macfs.FSRef(test_support.TESTFN)
        self.assertEqual(os.path.realpath(test_support.TESTFN), fsr.as_pathname())

    def test_fsref_unicode(self):
        if sys.getfilesystemencoding():
            testfn_unicode = unicode(test_support.TESTFN)
            fsr = macfs.FSRef(testfn_unicode)
            self.assertEqual(os.path.realpath(test_support.TESTFN), fsr.as_pathname())

    def test_coercion(self):
        fss = macfs.FSSpec(test_support.TESTFN)
        fsr = macfs.FSRef(test_support.TESTFN)
        fss2 = fsr.as_fsspec()
        fsr2 = fss.as_fsref()
        self.assertEqual(fss.as_pathname(), fss2.as_pathname())
        self.assertEqual(fsr.as_pathname(), fsr2.as_pathname())

    def test_dates(self):
        import time
        fss = macfs.FSSpec(test_support.TESTFN)
        now = int(time.time())
        fss.SetDates(now, now-1, now-2)
        dates = fss.GetDates()
        self.assertEqual(dates, (now, now-1, now-2))

    def test_ctor_type(self):
        fss = macfs.FSSpec(test_support.TESTFN)
        fss.SetCreatorType('Pyth', 'TEXT')
        filecr, filetp = fss.GetCreatorType()
        self.assertEqual((filecr, filetp), ('Pyth', 'TEXT'))

    def test_alias(self):
        fss = macfs.FSSpec(test_support.TESTFN)
        alias = fss.NewAlias()
        fss2, changed = alias.Resolve()
        self.assertEqual(changed, 0)
        self.assertEqual(fss.as_pathname(), fss2.as_pathname())


    def test_fss_alias(self):
        fss = macfs.FSSpec(test_support.TESTFN)


def test_main():
    test_support.run_unittest(TestMacfs)


if __name__ == '__main__':
    test_main()
