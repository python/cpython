import unittest
from test import test_support

import io

class IOTest(unittest.TestCase):

    def write_ops(self, f):
        f.write(b"blah.")
        f.seek(0)
        f.write(b"Hello.")
        self.assertEqual(f.tell(), 6)
        f.seek(-1, 1)
        self.assertEqual(f.tell(), 5)
        f.write(" world\n\n\n")
        f.seek(0)
        f.write("h")
        f.seek(-2, 2)
        f.truncate()

    def read_ops(self, f):
        data = f.read(5)
        self.assertEqual(data, b"hello")
        f.readinto(data)
        self.assertEqual(data, b" worl")
        f.readinto(data)
        self.assertEqual(data, b"d\n")
        f.seek(0)
        self.assertEqual(f.read(20), b"hello world\n")
        f.seek(-6, 2)
        self.assertEqual(f.read(5), b"world")
        f.seek(-6, 1)
        self.assertEqual(f.read(5), b" worl")
        self.assertEqual(f.tell(), 10)

    def test_raw_file_io(self):
        f = io.open(test_support.TESTFN, "wb", buffering=0)
        self.assertEqual(f.readable(), False)
        self.assertEqual(f.writable(), True)
        self.assertEqual(f.seekable(), True)
        self.write_ops(f)
        f.close()
        f = io.open(test_support.TESTFN, "rb", buffering=0)
        self.assertEqual(f.readable(), True)
        self.assertEqual(f.writable(), False)
        self.assertEqual(f.seekable(), True)
        self.read_ops(f)
        f.close()

    def test_raw_bytes_io(self):
        f = io.BytesIO()
        self.write_ops(f)
        data = f.getvalue()
        self.assertEqual(data, b"hello world\n")
        f = io.BytesIO(data)
        self.read_ops(f)

def test_main():
    test_support.run_unittest(IOTest)

if __name__ == "__main__":
    test_main()
