import os
import unittest
import shelve
import glob
from test import test_support

class TestCase(unittest.TestCase):

    fn = "shelftemp.db"

    def test_ascii_file_shelf(self):
        try:
            s = shelve.open(self.fn, binary=False)
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
            s.close()
        finally:
            for f in glob.glob(self.fn+"*"):
                os.unlink(f)

    def test_binary_file_shelf(self):
        try:
            s = shelve.open(self.fn, binary=True)
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
            s.close()
        finally:
            for f in glob.glob(self.fn+"*"):
                os.unlink(f)

    def test_in_memory_shelf(self):
        d1 = {}
        s = shelve.Shelf(d1, binary=False)
        s['key1'] = (1,2,3,4)
        self.assertEqual(s['key1'], (1,2,3,4))
        s.close()
        d2 = {}
        s = shelve.Shelf(d2, binary=True)
        s['key1'] = (1,2,3,4)
        self.assertEqual(s['key1'], (1,2,3,4))
        s.close()

        self.assertEqual(len(d1), 1)
        self.assertNotEqual(d1, d2)

from test_userdict import TestMappingProtocol

class TestShelveBase(TestMappingProtocol):
    fn = "shelftemp.db"
    counter = 0
    def __init__(self, *args, **kw):
        self._db = []
        TestMappingProtocol.__init__(self, *args, **kw)
    _tested_class = shelve.Shelf
    def _reference(self):
        return {"key1":"value1", "key2":2, "key3":(1,2,3)}
    def _empty_mapping(self):
        if self._in_mem:
            x= shelve.Shelf({}, binary = self._binary)
        else:
            self.counter+=1
            x= shelve.open(self.fn+str(self.counter), binary=self._binary)
        self._db.append(x)
        return x
    def tearDown(self):
        for db in self._db:
            db.close()
        self._db = []
        if not self._in_mem:
            for f in glob.glob(self.fn+"*"):
                os.unlink(f)

class TestAsciiFileShelve(TestShelveBase):
    _binary = False
    _in_mem = False
class TestBinaryFileShelve(TestShelveBase):
    _binary = True
    _in_mem = False
class TestAsciiMemShelve(TestShelveBase):
    _binary = False
    _in_mem = True
class TestBinaryMemShelve(TestShelveBase):
    _binary = True
    _in_mem = True

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestAsciiFileShelve))
    suite.addTest(unittest.makeSuite(TestBinaryFileShelve))
    suite.addTest(unittest.makeSuite(TestAsciiMemShelve))
    suite.addTest(unittest.makeSuite(TestBinaryMemShelve))
    suite.addTest(unittest.makeSuite(TestCase))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
