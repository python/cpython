import io
import sys


def init_streams(log_write, stdout_level, stderr_level):
    # Redirect stdout and stderr to the Apple system log. This method is
    # invoked by init_apple_streams() (initconfig.c) if config->use_system_logger
    # is enabled.
    sys.stdout = SystemLog(log_write, stdout_level, errors=sys.stderr.errors)
    sys.stderr = SystemLog(log_write, stderr_level, errors=sys.stderr.errors)


class SystemLog(io.TextIOWrapper):
    def __init__(self, log_write, level, **kwargs):
        kwargs.setdefault("encoding", "UTF-8")
        kwargs.setdefault("line_buffering", True)
        super().__init__(LogStream(log_write, level), **kwargs)

    def __repr__(self):
        return f"<SystemLog (level {self.buffer.level})>"

    def write(self, s):
        if not isinstance(s, str):
            raise TypeError(
                f"write() argument must be str, not {type(s).__name__}")

        # In case `s` is a str subclass that writes itself to stdout or stderr
        # when we call its methods, convert it to an actual str.
        s = str.__str__(s)

        # We want to emit one log message per line, so split
        # the string before sending it to the superclass.
        for line in s.splitlines(keepends=True):
            super().write(line)

        return len(s)


class LogStream(io.RawIOBase):
    # Marker appended to any log message that does not end with a newline,
    # so a cooperating log reader can re-join it with the following message
    # instead of rendering it as a spurious standalone line. Placing the
    # marker after the data also prevents the system log from stripping any
    # trailing whitespace. Uses ASCII Unit Separator (0x1f).
    PARTIAL_LINE_MARKER = b"\x1f"

    def __init__(self, log_write, level):
        self.log_write = log_write
        self.level = level

    def __repr__(self):
        return f"<LogStream (level {self.level!r})>"

    def writable(self):
        return True

    def write(self, b):
        if type(b) is not bytes:
            try:
                b = bytes(memoryview(b))
            except TypeError:
                raise TypeError(
                    f"write() argument must be bytes-like, not {type(b).__name__}"
                ) from None

        # Writing an empty string to the stream should have no effect.
        if b:
            # Encode null bytes using "modified UTF-8" to avoid truncating the
            # message.
            data = b.replace(b"\x00", b"\xc0\x80")

            # Append a marker to partial lines (see PARTIAL_LINE_MARKER).
            if not b.endswith(b"\n"):
                data += self.PARTIAL_LINE_MARKER
            self.log_write(self.level, data)

        # Modifications of the changed data should not affect the return value, as
        # the caller may be expecting it to match the length of the input.
        return len(b)
