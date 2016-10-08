'''Tests for WindowsConsoleIO
'''

import io
import unittest
import sys

if sys.platform != 'win32':
    raise unittest.SkipTest("test only relevant on win32")

from _testconsole import write_input

ConIO = io._WindowsConsoleIO

class WindowsConsoleIOTests(unittest.TestCase):
    def test_abc(self):
        self.assertTrue(issubclass(ConIO, io.RawIOBase))
        self.assertFalse(issubclass(ConIO, io.BufferedIOBase))
        self.assertFalse(issubclass(ConIO, io.TextIOBase))

    def test_open_fd(self):
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

    def test_input(self):
        # ASCII
        self.assertStdinRoundTrip('abc123')
        # Non-ASCII
        self.assertStdinRoundTrip('ϼўТλФЙ')
        # Combining characters
        self.assertStdinRoundTrip('A͏B ﬖ̳AA̝')
        # Non-BMP
        self.assertStdinRoundTrip('\U00100000\U0010ffff\U0010fffd')

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
