import unittest
from _apple_support import SystemLog
from test.support import is_apple
from unittest.mock import Mock, call

if not is_apple:
    raise unittest.SkipTest("Apple-specific")


# Test redirection of stdout and stderr to the Apple system log.
class TestAppleSystemLogOutput(unittest.TestCase):
    maxDiff = None

    def assert_writes(self, output):
        self.assertEqual(
            self.log_write.mock_calls,
            [
                call(self.log_level, line)
                for line in output
            ]
        )

        self.log_write.reset_mock()

    def setUp(self):
        self.log_write = Mock()
        self.log_level = 42
        self.log = SystemLog(self.log_write, self.log_level, errors="replace")

    def test_repr(self):
        self.assertEqual(repr(self.log), "<SystemLog (level 42)>")
        self.assertEqual(repr(self.log.buffer), "<LogStream (level 42)>")

    def test_log_config(self):
        self.assertIs(self.log.writable(), True)
        self.assertIs(self.log.readable(), False)

        self.assertEqual("UTF-8", self.log.encoding)
        self.assertEqual("replace", self.log.errors)

        self.assertIs(self.log.line_buffering, True)
        self.assertIs(self.log.write_through, False)

    def test_empty_str(self):
        self.log.write("")
        self.log.flush()

        self.assert_writes([])

    def test_simple_str(self):
        self.log.write("hello world\n")

        self.assert_writes([b"hello world\n"])

    def test_buffered_str(self):
        self.log.write("h")
        self.log.write("ello")
        self.log.write(" ")
        self.log.write("world\n")
        self.log.write("goodbye.")
        self.log.flush()

        self.assert_writes([b"hello world\n", b"goodbye."])

    def test_manual_flush(self):
        self.log.write("Hello")

        self.assert_writes([])

        self.log.write(" world\nHere for a while...\nGoodbye")
        self.assert_writes([b"Hello world\n", b"Here for a while...\n"])

        self.log.write(" world\nHello again")
        self.assert_writes([b"Goodbye world\n"])

        self.log.flush()
        self.assert_writes([b"Hello again"])

    def test_non_ascii(self):
        # Spanish
        self.log.write("ol\u00e9\n")
        self.assert_writes([b"ol\xc3\xa9\n"])

        # Chinese
        self.log.write("\u4e2d\u6587\n")
        self.assert_writes([b"\xe4\xb8\xad\xe6\x96\x87\n"])

        # Printing Non-BMP emoji
        self.log.write("\U0001f600\n")
        self.assert_writes([b"\xf0\x9f\x98\x80\n"])

        # Non-encodable surrogates are replaced
        self.log.write("\ud800\udc00\n")
        self.assert_writes([b"??\n"])

    def test_modified_null(self):
        # Null characters are logged using "modified UTF-8".
        self.log.write("\u0000\n")
        self.assert_writes([b"\xc0\x80\n"])
        self.log.write("a\u0000\n")
        self.assert_writes([b"a\xc0\x80\n"])
        self.log.write("\u0000b\n")
        self.assert_writes([b"\xc0\x80b\n"])
        self.log.write("a\u0000b\n")
        self.assert_writes([b"a\xc0\x80b\n"])

    def test_nonstandard_str(self):
        # String subclasses are accepted, but they should be converted
        # to a standard str without calling any of their methods.
        class CustomStr(str):
            def splitlines(self, *args, **kwargs):
                raise AssertionError()

            def __len__(self):
                raise AssertionError()

            def __str__(self):
                raise AssertionError()

        self.log.write(CustomStr("custom\n"))
        self.assert_writes([b"custom\n"])

    def test_non_str(self):
        # Non-string classes are not accepted.
        for obj in [b"", b"hello", None, 42]:
            with self.subTest(obj=obj):
                with self.assertRaisesRegex(
                    TypeError,
                    fr"write\(\) argument must be str, not "
                    fr"{type(obj).__name__}"
                ):
                    self.log.write(obj)

    def test_byteslike_in_buffer(self):
        # The underlying buffer *can* accept bytes-like objects
        self.log.buffer.write(bytearray(b"hello"))
        self.log.flush()

        self.log.buffer.write(b"")
        self.log.flush()

        self.log.buffer.write(b"goodbye")
        self.log.flush()

        self.assert_writes([b"hello", b"goodbye"])

    def test_non_byteslike_in_buffer(self):
        for obj in ["hello", None, 42]:
            with self.subTest(obj=obj):
                with self.assertRaisesRegex(
                    TypeError,
                    fr"write\(\) argument must be bytes-like, not "
                    fr"{type(obj).__name__}"
                ):
                    self.log.buffer.write(obj)
