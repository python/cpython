import unittest
import MacOS
import Carbon.File
from test import test_support
import os

TESTFN2 = test_support.TESTFN + '2'

class TestMacOS(unittest.TestCase):

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
            self.assertEquals(data, 'hello world\n')

            rfp = MacOS.openrf(test_support.TESTFN, '*rb')
            data = rfp.read(100)
            data2 = rfp.read(100)
            rfp.close()
            self.assertEquals(data, 'goodbye world\n')
            self.assertEquals(data2, '')


        finally:
            os.unlink(test_support.TESTFN)

def test_main():
    test_support.run_unittest(TestMacOS)


if __name__ == '__main__':
    test_main()
