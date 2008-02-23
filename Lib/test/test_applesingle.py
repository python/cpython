# Copyright (C) 2003 Python Software Foundation

import unittest
import macostools
import Carbon.File
import MacOS
import os
from test import test_support
import struct
import applesingle

AS_MAGIC=0x00051600
AS_VERSION=0x00020000
dataforkdata = 'hello\r\0world\n'
resourceforkdata = 'goodbye\ncruel\0world\r'

applesingledata = struct.pack(">ll16sh", AS_MAGIC, AS_VERSION, "foo", 2) + \
    struct.pack(">llllll", 1, 50, len(dataforkdata),
        2, 50+len(dataforkdata), len(resourceforkdata)) + \
    dataforkdata + \
    resourceforkdata
TESTFN2 = test_support.TESTFN + '2'

class TestApplesingle(unittest.TestCase):

    def setUp(self):
        fp = open(test_support.TESTFN, 'w')
        fp.write(applesingledata)
        fp.close()

    def tearDown(self):
        try:
            os.unlink(test_support.TESTFN)
        except:
            pass
        try:
            os.unlink(TESTFN2)
        except:
            pass

    def compareData(self, isrf, data):
        if isrf:
            fp = MacOS.openrf(TESTFN2, '*rb')
        else:
            fp = open(TESTFN2, 'rb')
        filedata = fp.read(1000)
        self.assertEqual(data, filedata)

    def test_applesingle(self):
        try:
            os.unlink(TESTFN2)
        except:
            pass
        applesingle.decode(test_support.TESTFN, TESTFN2)
        self.compareData(False, dataforkdata)
        self.compareData(True, resourceforkdata)

    def test_applesingle_resonly(self):
        try:
            os.unlink(TESTFN2)
        except:
            pass
        applesingle.decode(test_support.TESTFN, TESTFN2, resonly=True)
        self.compareData(False, resourceforkdata)

def test_main():
    test_support.run_unittest(TestApplesingle)


if __name__ == '__main__':
    test_main()
