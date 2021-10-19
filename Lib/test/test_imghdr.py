import imghdr
import io
import os
import pathlib
import unittest
import warnings
from test.support import findfile
from test.support.os_helper import TESTFN, unlink


TEST_FILES = (
    ('python.png', 'png'),
    ('python.gif', 'gif'),
    ('python.bmp', 'bmp'),
    ('python.ppm', 'ppm'),
    ('python.pgm', 'pgm'),
    ('python.pbm', 'pbm'),
    ('python.jpg', 'jpeg'),
    ('python-raw.jpg', 'jpeg'),  # raw JPEG without JFIF/EXIF markers
    ('python.ras', 'rast'),
    ('python.sgi', 'rgb'),
    ('python.tiff', 'tiff'),
    ('python.xbm', 'xbm'),
    ('python.webp', 'webp'),
    ('python.exr', 'exr'),
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
            with open(filename, 'rb') as stream:
                self.assertEqual(imghdr.what(stream), expected)
            with open(filename, 'rb') as stream:
                data = stream.read()
            self.assertEqual(imghdr.what(None, data), expected)
            self.assertEqual(imghdr.what(None, bytearray(data)), expected)

    def test_pathlike_filename(self):
        for filename, expected in TEST_FILES:
            with self.subTest(filename=filename):
                filename = findfile(filename, subdir='imghdrdata')
                self.assertEqual(imghdr.what(pathlib.Path(filename)), expected)

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
        with self.assertRaises(AttributeError):
            imghdr.what(os.fsencode(self.testfile))
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

    def test_string_data(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", BytesWarning)
            for filename, _ in TEST_FILES:
                filename = findfile(filename, subdir='imghdrdata')
                with open(filename, 'rb') as stream:
                    data = stream.read().decode('latin1')
                with self.assertRaises(TypeError):
                    imghdr.what(io.StringIO(data))
                with self.assertRaises(TypeError):
                    imghdr.what(None, data)

    def test_missing_file(self):
        with self.assertRaises(FileNotFoundError):
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
            with self.assertRaises(OSError) as cm:
                imghdr.what(stream)

if __name__ == '__main__':
    unittest.main()
