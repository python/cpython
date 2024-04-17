import io
import sys
from ctypes import CDLL, c_char_p, c_int


# The maximum length of a log message in bytes, including the level marker and
# tag, is defined as LOGGER_ENTRY_MAX_PAYLOAD in
# platform/system/logging/liblog/include/log/log.h. As of API level 30, messages
# longer than this will be be truncated by logcat. This limit has already been
# reduced at least once in the history of Android (from 4076 to 4068 between API
# level 23 and 26), so leave some headroom.
MAX_BYTES_PER_WRITE = 4000

# UTF-8 uses a maximum of 4 bytes per character, so limiting text writes to this
# size ensures that TextIOWrapper can always avoid exceeding MAX_BYTES_PER_WRITE.
# However, if the actual number of bytes per character is smaller than that,
# then TextIOWrapper may still join multiple consecutive text writes into binary
# writes containing a larger number of characters.
MAX_CHARS_PER_WRITE = MAX_BYTES_PER_WRITE // 4


# When embedded in an app on current versions of Android, there's no easy way to
# monitor the C-level stdout and stderr. The testbed comes with a .c file to
# redirect them to the system log using a pipe, but that wouldn't be convenient
# or appropriate for all apps. So we redirect at the Python level instead.
def init_streams():
    if sys.executable:
        return  # Not embedded in an app.

    # Despite its name, this function is part of the public API
    # (https://developer.android.com/ndk/reference/group/logging).
    # Use `getattr` to avoid private name mangling.
    global android_log_write
    android_log_write = getattr(CDLL("liblog.so"), "__android_log_write")
    android_log_write.argtypes = (c_int, c_char_p, c_char_p)

    # These log levels match those used by Java's System.out and System.err.
    ANDROID_LOG_INFO = 4
    ANDROID_LOG_WARN = 5

    sys.stdout = TextLogStream(
        ANDROID_LOG_INFO, "python.stdout", errors=sys.stdout.errors)
    sys.stderr = TextLogStream(
        ANDROID_LOG_WARN, "python.stderr", errors=sys.stderr.errors)


class TextLogStream(io.TextIOWrapper):
    def __init__(self, level, tag, **kwargs):
        kwargs.setdefault("encoding", "UTF-8")
        kwargs.setdefault("line_buffering", True)
        super().__init__(BinaryLogStream(level, tag), **kwargs)
        self._CHUNK_SIZE = MAX_BYTES_PER_WRITE

    def __repr__(self):
        return f"<TextLogStream {self.buffer.tag!r}>"

    def write(self, s):
        if not isinstance(s, str):
            raise TypeError(
                f"write() argument must be str, not {type(s).__name__}")

        # We want to emit one log message per line wherever possible, so split
        # the string before sending it to the superclass. Note that
        # "".splitlines() == [], so nothing will be logged for an empty string.
        for line, line_keepends in zip(
            s.splitlines(), s.splitlines(keepends=True)
        ):
            # Simplify the later stages by translating all newlines into "\n".
            if line != line_keepends:
                line += "\n"
            while line:
                super().write(line[:MAX_CHARS_PER_WRITE])
                line = line[MAX_CHARS_PER_WRITE:]

        return len(s)


class BinaryLogStream(io.RawIOBase):
    def __init__(self, level, tag):
        self.level = level
        self.tag = tag

    def __repr__(self):
        return f"<BinaryLogStream {self.tag!r}>"

    def writable(self):
        return True

    def write(self, b):
        if hasattr(b, "__buffer__"):
            b = bytes(b)
        else:
            raise TypeError(
                f"write() argument must be bytes-like, not {type(b).__name__}")

        # Writing an empty string to the stream should have no effect.
        if b:
            android_log_write(self.level, self.tag.encode("UTF-8"), b)
        return len(b)
