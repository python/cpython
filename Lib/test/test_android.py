import io
import platform
import queue
import re
import subprocess
import sys
import unittest
from _android_support import TextLogStream
from array import array
from contextlib import ExitStack, contextmanager
from threading import Thread
from test.support import LOOPBACK_TIMEOUT
from time import time
from unittest.mock import patch


if sys.platform != "android":
    raise unittest.SkipTest("Android-specific")

api_level = platform.android_ver().api_level

# (name, level, fileno)
STREAM_INFO = [("stdout", "I", 1), ("stderr", "W", 2)]


# Test redirection of stdout and stderr to the Android log.
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

        self.logcat_thread = Thread(target=logcat_thread)
        self.logcat_thread.start()

        try:
            from ctypes import CDLL, c_char_p, c_int
            android_log_write = getattr(CDLL("liblog.so"), "__android_log_write")
            android_log_write.argtypes = (c_int, c_char_p, c_char_p)
            ANDROID_LOG_INFO = 4

            # Separate tests using a marker line with a different tag.
            tag, message = "python.test", f"{self.id()} {time()}"
            android_log_write(
                ANDROID_LOG_INFO, tag.encode("UTF-8"), message.encode("UTF-8"))
            self.assert_log("I", tag, message, skip=True)
        except:
            # If setUp throws an exception, tearDown is not automatically
            # called. Avoid leaving a dangling thread which would keep the
            # Python process alive indefinitely.
            self.tearDown()
            raise

    def assert_logs(self, level, tag, expected, **kwargs):
        for line in expected:
            self.assert_log(level, tag, line, **kwargs)

    def assert_log(self, level, tag, expected, *, skip=False):
        deadline = time() + LOOPBACK_TIMEOUT
        while True:
            try:
                line = self.logcat_queue.get(timeout=(deadline - time()))
            except queue.Empty:
                raise self.failureException(
                    f"line not found: {expected!r}"
                ) from None
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
        self.logcat_thread.join(LOOPBACK_TIMEOUT)

        # Avoid an irrelevant warning about threading._dangling.
        self.logcat_thread = None

    @contextmanager
    def reconfigure(self, stream, **settings):
        original_settings = {key: getattr(stream, key, None) for key in settings.keys()}
        stream.reconfigure(**settings)
        try:
            yield
        finally:
            stream.reconfigure(**original_settings)

    def stream_context(self, stream_name, level):
        stack = ExitStack()
        stack.enter_context(self.subTest(stream_name))

        # In --verbose3 mode, sys.stdout and sys.stderr are captured, so we can't
        # test them directly. Detect this mode and use some temporary streams with
        # the same properties.
        stream = getattr(sys, stream_name)
        native_stream = getattr(sys, f"__{stream_name}__")
        if isinstance(stream, io.StringIO):
            # https://developer.android.com/ndk/reference/group/logging
            prio = {"I": 4, "W": 5}[level]
            stack.enter_context(
                patch(
                    f"sys.{stream_name}",
                    stream := TextLogStream(
                        prio, f"python.{stream_name}", native_stream,
                    ),
                )
            )

        # The tests assume the stream is initially buffered.
        stack.enter_context(self.reconfigure(stream, write_through=False))

        return stack

    def test_str(self):
        for stream_name, level, fileno in STREAM_INFO:
            with self.stream_context(stream_name, level):
                stream = getattr(sys, stream_name)
                tag = f"python.{stream_name}"
                self.assertEqual(f"<TextLogStream '{tag}'>", repr(stream))

                self.assertIs(stream.writable(), True)
                self.assertIs(stream.readable(), False)
                self.assertEqual(stream.fileno(), fileno)
                self.assertEqual("UTF-8", stream.encoding)
                self.assertEqual("backslashreplace", stream.errors)
                self.assertIs(stream.line_buffering, True)
                self.assertIs(stream.write_through, False)

                def write(s, lines=None, *, write_len=None):
                    if write_len is None:
                        write_len = len(s)
                    self.assertEqual(write_len, stream.write(s))
                    if lines is None:
                        lines = [s]
                    self.assert_logs(level, tag, lines)

                # Single-line messages,
                with self.reconfigure(stream, write_through=True):
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

                # Since this is a line-based logging system, line buffering
                # cannot be turned off, i.e. a newline always causes a flush.
                stream.reconfigure(line_buffering=False)
                self.assertIs(stream.line_buffering, True)

                # However, buffering can be turned off completely if you want a
                # flush after every write.
                with self.reconfigure(stream, write_through=True):
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
                write(("foobar" * 700) + "\n",  # 4200 bytes in
                      [("foobar" * 666) + "foob",  # 4000 bytes out
                       "ar" + ("foobar" * 33)])  # 200 bytes out

                # "Full-width" digits 0-9 (3 bytes per character)
                s = "\uff10\uff11\uff12\uff13\uff14\uff15\uff16\uff17\uff18\uff19"
                write((s * 150) + "\n",  # 4500 bytes in
                      [s * 100,  # 3000 bytes out
                       s * 50])  # 1500 bytes out

                s = "0123456789"
                write(s * 200, [])  # 2000 bytes in
                write(s * 150, [])  # 1500 bytes in
                write(s * 51, [s * 350])  # 510 bytes in, 3500 bytes out
                write("\n", [s * 51])  # 0 bytes in, 510 bytes out

    def test_bytes(self):
        for stream_name, level, fileno in STREAM_INFO:
            with self.stream_context(stream_name, level):
                stream = getattr(sys, stream_name).buffer
                tag = f"python.{stream_name}"
                self.assertEqual(f"<BinaryLogStream '{tag}'>", repr(stream))
                self.assertIs(stream.writable(), True)
                self.assertIs(stream.readable(), False)
                self.assertEqual(stream.fileno(), fileno)

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


class TestAndroidRateLimit(unittest.TestCase):
    def test_rate_limit(self):
        # https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/logging/liblog/include/log/log_read.h;l=39
        PER_MESSAGE_OVERHEAD = 28

        # https://developer.android.com/ndk/reference/group/logging
        ANDROID_LOG_DEBUG = 3

        # To avoid flooding the test script output, use a different tag rather
        # than stdout or stderr.
        tag = "python.rate_limit"
        stream = TextLogStream(ANDROID_LOG_DEBUG, tag)

        # Make a test message which consumes 1 KB of the logcat buffer.
        message = "Line {:03d} "
        message += "." * (
            1024 - PER_MESSAGE_OVERHEAD - len(tag) - len(message.format(0))
        ) + "\n"

        # To avoid depending on the performance of the test device, we mock the
        # passage of time.
        mock_now = time()

        def mock_time():
            # Avoid division by zero by simulating a small delay.
            mock_sleep(0.0001)
            return mock_now

        def mock_sleep(duration):
            nonlocal mock_now
            mock_now += duration

        # See _android_support.py. The default values of these parameters work
        # well across a wide range of devices, but we'll use smaller values to
        # ensure a quick and reliable test that doesn't flood the log too much.
        MAX_KB_PER_SECOND = 100
        BUCKET_KB = 10
        with (
            patch("_android_support.MAX_BYTES_PER_SECOND", MAX_KB_PER_SECOND * 1024),
            patch("_android_support.BUCKET_SIZE", BUCKET_KB * 1024),
            patch("_android_support.sleep", mock_sleep),
            patch("_android_support.time", mock_time),
        ):
            # Make sure the token bucket is full.
            stream.write("Initial message to reset _prev_write_time")
            mock_sleep(BUCKET_KB / MAX_KB_PER_SECOND)
            line_num = 0

            # Write BUCKET_KB messages, and return the rate at which they were
            # accepted in KB per second.
            def write_bucketful():
                nonlocal line_num
                start = mock_time()
                max_line_num = line_num + BUCKET_KB
                while line_num < max_line_num:
                    stream.write(message.format(line_num))
                    line_num += 1
                return BUCKET_KB / (mock_time() - start)

            # The first bucketful should be written with minimal delay. The
            # factor of 2 here is not arbitrary: it verifies that the system can
            # write fast enough to empty the bucket within two bucketfuls, which
            # the next part of the test depends on.
            self.assertGreater(write_bucketful(), MAX_KB_PER_SECOND * 2)

            # Write another bucketful to empty the token bucket completely.
            write_bucketful()

            # The next bucketful should be written at the rate limit.
            self.assertAlmostEqual(
                write_bucketful(), MAX_KB_PER_SECOND,
                delta=MAX_KB_PER_SECOND * 0.1
            )

            # Once the token bucket refills, we should go back to full speed.
            mock_sleep(BUCKET_KB / MAX_KB_PER_SECOND)
            self.assertGreater(write_bucketful(), MAX_KB_PER_SECOND * 2)
