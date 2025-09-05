'''Tests for WindowsConsoleIO
'''

import io
import os
import sys
import tempfile
import unittest
from test.support import os_helper, requires_resource

if sys.platform != 'win32':
    raise unittest.SkipTest("test only relevant on win32")

from _testconsole import write_input

ConIO = io._WindowsConsoleIO

class WindowsConsoleIOTests(unittest.TestCase):
    def test_abc(self):
        self.assertIsSubclass(ConIO, io.RawIOBase)
        self.assertNotIsSubclass(ConIO, io.BufferedIOBase)
        self.assertNotIsSubclass(ConIO, io.TextIOBase)

    def test_open_fd(self):
        self.assertRaisesRegex(ValueError,
            "negative file descriptor", ConIO, -1)

        with tempfile.TemporaryFile() as tmpfile:
            fd = tmpfile.fileno()
            # Windows 10: "Cannot open non-console file"
            # Earlier: "Cannot open console output buffer for reading"
            self.assertRaisesRegex(ValueError,
                "Cannot open (console|non-console file)", ConIO, fd)

        try:
            f = ConIO(0)
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertTrue(f.readable())
            self.assertFalse(f.writable())
            self.assertEqual(0, f.fileno())
            f.close()   # multiple close should not crash
            f.close()
            with self.assertWarns(RuntimeWarning):
                with ConIO(False):
                    pass

        try:
            f = ConIO(1, 'w')
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertFalse(f.readable())
            self.assertTrue(f.writable())
            self.assertEqual(1, f.fileno())
            f.close()
            f.close()
            with self.assertWarns(RuntimeWarning):
                with ConIO(False):
                    pass

        try:
            f = ConIO(2, 'w')
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertFalse(f.readable())
            self.assertTrue(f.writable())
            self.assertEqual(2, f.fileno())
            f.close()
            f.close()

    def test_open_name(self):
        self.assertRaises(ValueError, ConIO, sys.executable)

        f = ConIO("CON")
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()   # multiple close should not crash
        f.close()

        f = ConIO('CONIN$')
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()
        f.close()

        f = ConIO('CONOUT$', 'w')
        self.assertFalse(f.readable())
        self.assertTrue(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()
        f.close()

        # bpo-45354: Windows 11 changed MS-DOS device name handling
        if sys.getwindowsversion()[:3] < (10, 0, 22000):
            f = open('C:/con', 'rb', buffering=0)
            self.assertIsInstance(f, ConIO)
            f.close()

    def test_subclass_repr(self):
        class TestSubclass(ConIO):
            pass

        f = TestSubclass("CON")
        with f:
            self.assertIn(TestSubclass.__name__, repr(f))

        self.assertIn(TestSubclass.__name__, repr(f))

    @unittest.skipIf(sys.getwindowsversion()[:2] <= (6, 1),
        "test does not work on Windows 7 and earlier")
    def test_conin_conout_names(self):
        f = open(r'\\.\conin$', 'rb', buffering=0)
        self.assertIsInstance(f, ConIO)
        f.close()

        f = open('//?/conout$', 'wb', buffering=0)
        self.assertIsInstance(f, ConIO)
        f.close()

    def test_conout_path(self):
        temp_path = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, temp_path)

        conout_path = os.path.join(temp_path, 'CONOUT$')

        with open(conout_path, 'wb', buffering=0) as f:
            # bpo-45354: Windows 11 changed MS-DOS device name handling
            if (6, 1) < sys.getwindowsversion()[:3] < (10, 0, 22000):
                self.assertIsInstance(f, ConIO)
            else:
                self.assertNotIsInstance(f, ConIO)

    def test_write_empty_data(self):
        with ConIO('CONOUT$', 'w') as f:
            self.assertEqual(f.write(b''), 0)

    @requires_resource('console')
    def test_write(self):
        testcases = []
        with ConIO('CONOUT$', 'w') as f:
            for a in [
                b'',
                b'abc',
                b'\xc2\xa7\xe2\x98\x83\xf0\x9f\x90\x8d',
                b'\xff'*10,
            ]:
                for b in b'\xc2\xa7', b'\xe2\x98\x83', b'\xf0\x9f\x90\x8d':
                    testcases.append(a + b)
                    for i in range(1, len(b)):
                        data = a + b[:i]
                        testcases.append(data + b'z')
                        testcases.append(data + b'\xff')
                        # incomplete multibyte sequence
                        with self.subTest(data=data):
                            self.assertEqual(f.write(data), len(a))
            for data in testcases:
                with self.subTest(data=data):
                    self.assertEqual(f.write(data), len(data))

    def assertStdinRoundTrip(self, text):
        stdin = open('CONIN$', 'r')
        old_stdin = sys.stdin
        try:
            sys.stdin = stdin
            write_input(
                stdin.buffer.raw,
                (text + '\r\n').encode('utf-16-le', 'surrogatepass')
            )
            actual = input()
        finally:
            sys.stdin = old_stdin
        self.assertEqual(actual, text)

    @requires_resource('console')
    def test_input(self):
        # ASCII
        self.assertStdinRoundTrip('abc123')
        # Non-ASCII
        self.assertStdinRoundTrip('ϼўТλФЙ')
        # Combining characters
        self.assertStdinRoundTrip('A͏B ﬖ̳AA̝')

    # bpo-38325
    @unittest.skipIf(True, "Handling Non-BMP characters is broken")
    def test_input_nonbmp(self):
        # Non-BMP
        self.assertStdinRoundTrip('\U00100000\U0010ffff\U0010fffd')

    @requires_resource('console')
    def test_partial_reads(self):
        # Test that reading less than 1 full character works when stdin
        # contains multibyte UTF-8 sequences
        source = 'ϼўТλФЙ\r\n'.encode('utf-16-le')
        expected = 'ϼўТλФЙ\r\n'.encode('utf-8')
        for read_count in range(1, 16):
            with open('CONIN$', 'rb', buffering=0) as stdin:
                write_input(stdin, source)

                actual = b''
                while not actual.endswith(b'\n'):
                    b = stdin.read(read_count)
                    actual += b

                self.assertEqual(actual, expected, 'stdin.read({})'.format(read_count))

    # bpo-38325
    @unittest.skipIf(True, "Handling Non-BMP characters is broken")
    def test_partial_surrogate_reads(self):
        # Test that reading less than 1 full character works when stdin
        # contains surrogate pairs that cannot be decoded to UTF-8 without
        # reading an extra character.
        source = '\U00101FFF\U00101001\r\n'.encode('utf-16-le')
        expected = '\U00101FFF\U00101001\r\n'.encode('utf-8')
        for read_count in range(1, 16):
            with open('CONIN$', 'rb', buffering=0) as stdin:
                write_input(stdin, source)

                actual = b''
                while not actual.endswith(b'\n'):
                    b = stdin.read(read_count)
                    actual += b

                self.assertEqual(actual, expected, 'stdin.read({})'.format(read_count))

    @requires_resource('console')
    def test_ctrl_z(self):
        with open('CONIN$', 'rb', buffering=0) as stdin:
            source = '\xC4\x1A\r\n'.encode('utf-16-le')
            expected = '\xC4'.encode('utf-8')
            write_input(stdin, source)
            a, b = stdin.read(1), stdin.readall()
            self.assertEqual(expected[0:1], a)
            self.assertEqual(expected[1:], b)

if __name__ == "__main__":
    unittest.main()
