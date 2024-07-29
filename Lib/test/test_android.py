import platform
import queue
import re
import subprocess
import sys
import unittest
from array import array
from contextlib import contextmanager
from threading import Thread
from test.support import LOOPBACK_TIMEOUT
from time import time


if sys.platform != "android":
    raise unittest.SkipTest("Android-specific")

api_level = platform.android_ver().api_level


# Test redirection of stdout and stderr to the Android log.
@unittest.skipIf(
    api_level < 23 and platform.machine() == "aarch64",
    "SELinux blocks reading logs on older ARM64 emulators"
)
class TestAndroidOutput(unittest.TestCase):
    maxDiff = None

    def setUp(self):
        self.logcat_process = subprocess.Popen(
            ["logcat", "-v", "tag"], stdout=subprocess.PIPE,
            errors="backslashreplace"
        )
        self.logcat_queue = queue.Queue()

        def logcat_thread():
            for line in self.logcat_process.stdout:
                self.logcat_queue.put(line.rstrip("\n"))
            self.logcat_process.stdout.close()
        Thread(target=logcat_thread).start()

        from ctypes import CDLL, c_char_p, c_int
        android_log_write = getattr(CDLL("liblog.so"), "__android_log_write")
        android_log_write.argtypes = (c_int, c_char_p, c_char_p)
        ANDROID_LOG_INFO = 4

        # Separate tests using a marker line with a different tag.
        tag, message = "python.test", f"{self.id()} {time()}"
        android_log_write(
            ANDROID_LOG_INFO, tag.encode("UTF-8"), message.encode("UTF-8"))
        self.assert_log("I", tag, message, skip=True, timeout=5)

    def assert_logs(self, level, tag, expected, **kwargs):
        for line in expected:
            self.assert_log(level, tag, line, **kwargs)

    def assert_log(self, level, tag, expected, *, skip=False, timeout=0.5):
        deadline = time() + timeout
        while True:
            try:
                line = self.logcat_queue.get(timeout=(deadline - time()))
            except queue.Empty:
                self.fail(f"line not found: {expected!r}")
            if match := re.fullmatch(fr"(.)/{tag}: (.*)", line):
                try:
                    self.assertEqual(level, match[1])
                    self.assertEqual(expected, match[2])
                    break
                except AssertionError:
                    if not skip:
                        raise

    def tearDown(self):
        self.logcat_process.terminate()
        self.logcat_process.wait(LOOPBACK_TIMEOUT)

    @contextmanager
    def unbuffered(self, stream):
        stream.reconfigure(write_through=True)
        try:
            yield
        finally:
            stream.reconfigure(write_through=False)

    def test_str(self):
        for stream_name, level in [("stdout", "I"), ("stderr", "W")]:
            with self.subTest(stream=stream_name):
                stream = getattr(sys, stream_name)
                tag = f"python.{stream_name}"
                self.assertEqual(f"<TextLogStream '{tag}'>", repr(stream))

                self.assertTrue(stream.writable())
                self.assertFalse(stream.readable())
                self.assertEqual("UTF-8", stream.encoding)
                self.assertTrue(stream.line_buffering)
                self.assertFalse(stream.write_through)

                # stderr is backslashreplace by default; stdout is configured
                # that way by libregrtest.main.
                self.assertEqual("backslashreplace", stream.errors)

                def write(s, lines=None, *, write_len=None):
                    if write_len is None:
                        write_len = len(s)
                    self.assertEqual(write_len, stream.write(s))
                    if lines is None:
                        lines = [s]
                    self.assert_logs(level, tag, lines)

                # Single-line messages,
                with self.unbuffered(stream):
                    write("", [])

                    write("a")
                    write("Hello")
                    write("Hello world")
                    write(" ")
                    write("  ")

                    # Non-ASCII text
                    write("ol\u00e9")  # Spanish
                    write("\u4e2d\u6587")  # Chinese

                    # Non-BMP emoji
                    write("\U0001f600")

                    # Non-encodable surrogates
                    write("\ud800\udc00", [r"\ud800\udc00"])

                    # Code used by surrogateescape (which isn't enabled here)
                    write("\udc80", [r"\udc80"])

                    # Null characters are logged using "modified UTF-8".
                    write("\u0000", [r"\xc0\x80"])
                    write("a\u0000", [r"a\xc0\x80"])
                    write("\u0000b", [r"\xc0\x80b"])
                    write("a\u0000b", [r"a\xc0\x80b"])

                # Multi-line messages. Avoid identical consecutive lines, as
                # they may activate "chatty" filtering and break the tests.
                write("\nx", [""])
                write("\na\n", ["x", "a"])
                write("\n", [""])
                write("b\n", ["b"])
                write("c\n\n", ["c", ""])
                write("d\ne", ["d"])
                write("xx", [])
                write("f\n\ng", ["exxf", ""])
                write("\n", ["g"])

                with self.unbuffered(stream):
                    write("\nx", ["", "x"])
                    write("\na\n", ["", "a"])
                    write("\n", [""])
                    write("b\n", ["b"])
                    write("c\n\n", ["c", ""])
                    write("d\ne", ["d", "e"])
                    write("xx", ["xx"])
                    write("f\n\ng", ["f", "", "g"])
                    write("\n", [""])

                # "\r\n" should be translated into "\n".
                write("hello\r\n", ["hello"])
                write("hello\r\nworld\r\n", ["hello", "world"])
                write("\r\n", [""])

                # Non-standard line separators should be preserved.
                write("before form feed\x0cafter form feed\n",
                      ["before form feed\x0cafter form feed"])
                write("before line separator\u2028after line separator\n",
                      ["before line separator\u2028after line separator"])

                # String subclasses are accepted, but they should be converted
                # to a standard str without calling any of their methods.
                class CustomStr(str):
                    def splitlines(self, *args, **kwargs):
                        raise AssertionError()

                    def __len__(self):
                        raise AssertionError()

                    def __str__(self):
                        raise AssertionError()

                write(CustomStr("custom\n"), ["custom"], write_len=7)

                # Non-string classes are not accepted.
                for obj in [b"", b"hello", None, 42]:
                    with self.subTest(obj=obj):
                        with self.assertRaisesRegex(
                            TypeError,
                            fr"write\(\) argument must be str, not "
                            fr"{type(obj).__name__}"
                        ):
                            stream.write(obj)

                # Manual flushing is supported.
                write("hello", [])
                stream.flush()
                self.assert_log(level, tag, "hello")
                write("hello", [])
                write("world", [])
                stream.flush()
                self.assert_log(level, tag, "helloworld")

                # Long lines are split into blocks of 1000 characters
                # (MAX_CHARS_PER_WRITE in _android_support.py), but
                # TextIOWrapper should then join them back together as much as
                # possible without exceeding 4000 UTF-8 bytes
                # (MAX_BYTES_PER_WRITE).
                #
                # ASCII (1 byte per character)
                write(("foobar" * 700) + "\n",
                      [("foobar" * 666) + "foob",  # 4000 bytes
                       "ar" + ("foobar" * 33)])  # 200 bytes

                # "Full-width" digits 0-9 (3 bytes per character)
                s = "\uff10\uff11\uff12\uff13\uff14\uff15\uff16\uff17\uff18\uff19"
                write((s * 150) + "\n",
                      [s * 100,  # 3000 bytes
                       s * 50])  # 1500 bytes

                s = "0123456789"
                write(s * 200, [])
                write(s * 150, [])
                write(s * 51, [s * 350])  # 3500 bytes
                write("\n", [s * 51])  # 510 bytes

    def test_bytes(self):
        for stream_name, level in [("stdout", "I"), ("stderr", "W")]:
            with self.subTest(stream=stream_name):
                stream = getattr(sys, stream_name).buffer
                tag = f"python.{stream_name}"
                self.assertEqual(f"<BinaryLogStream '{tag}'>", repr(stream))
                self.assertTrue(stream.writable())
                self.assertFalse(stream.readable())

                def write(b, lines=None, *, write_len=None):
                    if write_len is None:
                        write_len = len(b)
                    self.assertEqual(write_len, stream.write(b))
                    if lines is None:
                        lines = [b.decode()]
                    self.assert_logs(level, tag, lines)

                # Single-line messages,
                write(b"", [])

                write(b"a")
                write(b"Hello")
                write(b"Hello world")
                write(b" ")
                write(b"  ")

                # Non-ASCII text
                write(b"ol\xc3\xa9")  # Spanish
                write(b"\xe4\xb8\xad\xe6\x96\x87")  # Chinese

                # Non-BMP emoji
                write(b"\xf0\x9f\x98\x80")

                # Null bytes are logged using "modified UTF-8".
                write(b"\x00", [r"\xc0\x80"])
                write(b"a\x00", [r"a\xc0\x80"])
                write(b"\x00b", [r"\xc0\x80b"])
                write(b"a\x00b", [r"a\xc0\x80b"])

                # Invalid UTF-8
                write(b"\xff", [r"\xff"])
                write(b"a\xff", [r"a\xff"])
                write(b"\xffb", [r"\xffb"])
                write(b"a\xffb", [r"a\xffb"])

                # Log entries containing newlines are shown differently by
                # `logcat -v tag`, `logcat -v long`, and Android Studio. We
                # currently use `logcat -v tag`, which shows each line as if it
                # was a separate log entry, but strips a single trailing
                # newline.
                #
                # On newer versions of Android, all three of the above tools (or
                # maybe Logcat itself) will also strip any number of leading
                # newlines.
                write(b"\nx", ["", "x"] if api_level < 30 else ["x"])
                write(b"\na\n", ["", "a"] if api_level < 30 else ["a"])
                write(b"\n", [""])
                write(b"b\n", ["b"])
                write(b"c\n\n", ["c", ""])
                write(b"d\ne", ["d", "e"])
                write(b"xx", ["xx"])
                write(b"f\n\ng", ["f", "", "g"])
                write(b"\n", [""])

                # "\r\n" should be translated into "\n".
                write(b"hello\r\n", ["hello"])
                write(b"hello\r\nworld\r\n", ["hello", "world"])
                write(b"\r\n", [""])

                # Other bytes-like objects are accepted.
                write(bytearray(b"bytearray"))

                mv = memoryview(b"memoryview")
                write(mv, ["memoryview"])  # Continuous
                write(mv[::2], ["mmrve"])  # Discontinuous

                write(
                    # Android only supports little-endian architectures, so the
                    # bytes representation is as follows:
                    array("H", [
                        0,      # 00 00
                        1,      # 01 00
                        65534,  # FE FF
                        65535,  # FF FF
                    ]),

                    # After encoding null bytes with modified UTF-8, the only
                    # valid UTF-8 sequence is \x01. All other bytes are handled
                    # by backslashreplace.
                    ["\\xc0\\x80\\xc0\\x80"
                     "\x01\\xc0\\x80"
                     "\\xfe\\xff"
                     "\\xff\\xff"],
                    write_len=8,
                )

                # Non-bytes-like classes are not accepted.
                for obj in ["", "hello", None, 42]:
                    with self.subTest(obj=obj):
                        with self.assertRaisesRegex(
                            TypeError,
                            fr"write\(\) argument must be bytes-like, not "
                            fr"{type(obj).__name__}"
                        ):
                            stream.write(obj)
