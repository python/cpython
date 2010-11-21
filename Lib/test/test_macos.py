import unittest
from test import test_support
import os
import subprocess

MacOS = test_support.import_module('MacOS')

TESTFN2 = test_support.TESTFN + '2'

class TestMacOS(unittest.TestCase):

    def testGetCreatorAndType(self):
        if not os.path.exists('/Developer/Tools/SetFile'):
            return

        try:
            fp = open(test_support.TESTFN, 'w')
            fp.write('\n')
            fp.close()

            subprocess.call(
                    ['/Developer/Tools/SetFile', '-t', 'ABCD', '-c', 'EFGH',
                        test_support.TESTFN])

            cr, tp = MacOS.GetCreatorAndType(test_support.TESTFN)
            self.assertEqual(tp, 'ABCD')
            self.assertEqual(cr, 'EFGH')

        finally:
            os.unlink(test_support.TESTFN)

    def testSetCreatorAndType(self):
        if not os.path.exists('/Developer/Tools/GetFileInfo'):
            return

        try:
            fp = open(test_support.TESTFN, 'w')
            fp.write('\n')
            fp.close()

            MacOS.SetCreatorAndType(test_support.TESTFN,
                    'ABCD', 'EFGH')

            cr, tp = MacOS.GetCreatorAndType(test_support.TESTFN)
            self.assertEqual(cr, 'ABCD')
            self.assertEqual(tp, 'EFGH')

            data = subprocess.Popen(["/Developer/Tools/GetFileInfo", test_support.TESTFN],
                    stdout=subprocess.PIPE).communicate()[0]

            tp = None
            cr = None
            for  ln in data.splitlines():
                if ln.startswith('type:'):
                    tp = ln.split()[-1][1:-1]
                if ln.startswith('creator:'):
                    cr = ln.split()[-1][1:-1]

            self.assertEqual(cr, 'ABCD')
            self.assertEqual(tp, 'EFGH')

        finally:
            os.unlink(test_support.TESTFN)


    def testOpenRF(self):
        try:
            fp = open(test_support.TESTFN, 'w')
            fp.write('hello world\n')
            fp.close()

            rfp = MacOS.openrf(test_support.TESTFN, '*wb')
            rfp.write('goodbye world\n')
            rfp.close()


            fp = open(test_support.TESTFN, 'r')
            data = fp.read()
            fp.close()
            self.assertEqual(data, 'hello world\n')

            rfp = MacOS.openrf(test_support.TESTFN, '*rb')
            data = rfp.read(100)
            data2 = rfp.read(100)
            rfp.close()
            self.assertEqual(data, 'goodbye world\n')
            self.assertEqual(data2, '')


        finally:
            os.unlink(test_support.TESTFN)

def test_main():
    test_support.run_unittest(TestMacOS)


if __name__ == '__main__':
    test_main()
