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

def test_main():
    test_support.run_unittest(TestCase)


if __name__ == "__main__":
    test_main()
