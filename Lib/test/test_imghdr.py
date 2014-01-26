import imghdr
import io
import sys
import unittest
from test.test_support import findfile, TESTFN, unlink, run_unittest

TEST_FILES = (
    ('python.png', 'png'),
    ('python.gif', 'gif'),
    ('python.bmp', 'bmp'),
    ('python.ppm', 'ppm'),
    ('python.pgm', 'pgm'),
    ('python.pbm', 'pbm'),
    ('python.jpg', 'jpeg'),
    ('python.ras', 'rast'),
    ('python.sgi', 'rgb'),
    ('python.tiff', 'tiff'),
    ('python.xbm', 'xbm')
)

class UnseekableIO(io.FileIO):
    def tell(self):
        raise io.UnsupportedOperation

    def seek(self, *args, **kwargs):
        raise io.UnsupportedOperation

class TestImghdr(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.testfile = findfile('python.png', subdir='imghdrdata')
        with open(cls.testfile, 'rb') as stream:
            cls.testdata = stream.read()

    def tearDown(self):
        unlink(TESTFN)

    def test_data(self):
        for filename, expected in TEST_FILES:
            filename = findfile(filename, subdir='imghdrdata')
            self.assertEqual(imghdr.what(filename), expected)
            ufilename = filename.decode(sys.getfilesystemencoding())
            self.assertEqual(imghdr.what(ufilename), expected)
            with open(filename, 'rb') as stream:
                self.assertEqual(imghdr.what(stream), expected)
            with open(filename, 'rb') as stream:
                data = stream.read()
            self.assertEqual(imghdr.what(None, data), expected)

    def test_register_test(self):
        def test_jumbo(h, file):
            if h.startswith(b'eggs'):
                return 'ham'
        imghdr.tests.append(test_jumbo)
        self.addCleanup(imghdr.tests.pop)
        self.assertEqual(imghdr.what(None, b'eggs'), 'ham')

    def test_file_pos(self):
        with open(TESTFN, 'wb') as stream:
            stream.write(b'ababagalamaga')
            pos = stream.tell()
            stream.write(self.testdata)
        with open(TESTFN, 'rb') as stream:
            stream.seek(pos)
            self.assertEqual(imghdr.what(stream), 'png')
            self.assertEqual(stream.tell(), pos)

    def test_bad_args(self):
        with self.assertRaises(TypeError):
            imghdr.what()
        with self.assertRaises(AttributeError):
            imghdr.what(None)
        with self.assertRaises(TypeError):
            imghdr.what(self.testfile, 1)
        with open(self.testfile, 'rb') as f:
            with self.assertRaises(AttributeError):
                imghdr.what(f.fileno())

    def test_invalid_headers(self):
        for header in (b'\211PN\r\n',
                       b'\001\331',
                       b'\x59\xA6',
                       b'cutecat',
                       b'000000JFI',
                       b'GIF80'):
            self.assertIsNone(imghdr.what(None, header))

    def test_missing_file(self):
        with self.assertRaises(IOError):
            imghdr.what('missing')

    def test_closed_file(self):
        stream = open(self.testfile, 'rb')
        stream.close()
        with self.assertRaises(ValueError) as cm:
            imghdr.what(stream)
        stream = io.BytesIO(self.testdata)
        stream.close()
        with self.assertRaises(ValueError) as cm:
            imghdr.what(stream)

    def test_unseekable(self):
        with open(TESTFN, 'wb') as stream:
            stream.write(self.testdata)
        with UnseekableIO(TESTFN, 'rb') as stream:
            with self.assertRaises(io.UnsupportedOperation):
                imghdr.what(stream)

    def test_output_stream(self):
        with open(TESTFN, 'wb') as stream:
            stream.write(self.testdata)
            stream.seek(0)
            with self.assertRaises(IOError) as cm:
                imghdr.what(stream)

def test_main():
    run_unittest(TestImghdr)

if __name__ == '__main__':
    test_main()
