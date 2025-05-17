import itertools
import os
import sys
import unittest
from functools import partial
from test.support import os_helper, force_not_colorized_test_class

from unittest import TestCase
from unittest.mock import MagicMock, call, patch, ANY

from .support import handle_all_events, code_to_events

try:
    from _pyrepl.console import Event
    from _pyrepl.unix_console import UnixConsole
except ImportError:
    pass


def unix_console(events, **kwargs):
    console = UnixConsole()
    console.get_event = MagicMock(side_effect=events)

    height = kwargs.get("height", 25)
    width = kwargs.get("width", 80)
    console.getheightwidth = MagicMock(side_effect=lambda: (height, width))
    console.wait = MagicMock()

    console.prepare()
    for key, val in kwargs.items():
        setattr(console, key, val)
    return console


handle_events_unix_console = partial(
    handle_all_events,
    prepare_console=unix_console,
)
handle_events_narrow_unix_console = partial(
    handle_all_events,
    prepare_console=partial(unix_console, width=5),
)
handle_events_short_unix_console = partial(
    handle_all_events,
    prepare_console=partial(unix_console, height=1),
)
handle_events_unix_console_height_3 = partial(
    handle_all_events, prepare_console=partial(unix_console, height=3)
)


TERM_CAPABILITIES = {
    "bel": b"\x07",
    "civis": b"\x1b[?25l",
    "clear": b"\x1b[H\x1b[2J",
    "cnorm": b"\x1b[?12l\x1b[?25h",
    "cub": b"\x1b[%p1%dD",
    "cub1": b"\x08",
    "cud": b"\x1b[%p1%dB",
    "cud1": b"\n",
    "cuf": b"\x1b[%p1%dC",
    "cuf1": b"\x1b[C",
    "cup": b"\x1b[%i%p1%d;%p2%dH",
    "cuu": b"\x1b[%p1%dA",
    "cuu1": b"\x1b[A",
    "dch1": b"\x1b[P",
    "dch": b"\x1b[%p1%dP",
    "el": b"\x1b[K",
    "hpa": b"\x1b[%i%p1%dG",
    "ich": b"\x1b[%p1%d@",
    "ich1": None,
    "ind": b"\n",
    "pad": None,
    "ri": b"\x1bM",
    "rmkx": b"\x1b[?1l\x1b>",
    "smkx": b"\x1b[?1h\x1b=",
}


@unittest.skipIf(sys.platform == "win32", "No Unix event queue on Windows")
@patch("_pyrepl.curses.tigetstr", lambda s: TERM_CAPABILITIES.get(s))
@patch(
    "_pyrepl.curses.tparm",
    lambda s, *args: s + b":" + b",".join(str(i).encode() for i in args),
)
@patch("_pyrepl.curses.setupterm", lambda a, b: None)
@patch(
    "termios.tcgetattr",
    lambda _: [
        27394,
        3,
        19200,
        536872399,
        38400,
        38400,
        [
            b"\x04",
            b"\xff",
            b"\xff",
            b"\x7f",
            b"\x17",
            b"\x15",
            b"\x12",
            b"\x00",
            b"\x03",
            b"\x1c",
            b"\x1a",
            b"\x19",
            b"\x11",
            b"\x13",
            b"\x16",
            b"\x0f",
            b"\x01",
            b"\x00",
            b"\x14",
            b"\x00",
        ],
    ],
)
@patch("termios.tcsetattr", lambda a, b, c: None)
@patch("os.write")
@force_not_colorized_test_class
class TestConsole(TestCase):
    def test_simple_addition(self, _os_write):
        code = "12+34"
        events = code_to_events(code)
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, b"2")
        _os_write.assert_any_call(ANY, b"+")
        _os_write.assert_any_call(ANY, b"3")
        _os_write.assert_any_call(ANY, b"4")
        con.restore()

    def test_wrap(self, _os_write):
        code = "12+34"
        events = code_to_events(code)
        _, con = handle_events_narrow_unix_console(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, b"2")
        _os_write.assert_any_call(ANY, b"+")
        _os_write.assert_any_call(ANY, b"3")
        _os_write.assert_any_call(ANY, b"\\")
        _os_write.assert_any_call(ANY, b"\n")
        _os_write.assert_any_call(ANY, b"4")
        con.restore()

    def test_cursor_left(self, _os_write):
        code = "1"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
        )
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cub"] + b":1")
        con.restore()

    def test_cursor_left_right(self, _os_write):
        code = "1"
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            ],
        )
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cub"] + b":1")
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cuf"] + b":1")
        con.restore()

    def test_cursor_up(self, _os_write):
        code = "1\n2+3"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="up", raw=bytearray(b"\x1bOA"))],
        )
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cuu"] + b":1")
        con.restore()

    def test_cursor_up_down(self, _os_write):
        code = "1\n2+3"
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cuu"] + b":1")
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cud"] + b":1")
        con.restore()

    def test_cursor_back_write(self, _os_write):
        events = itertools.chain(
            code_to_events("1"),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
            code_to_events("2"),
        )
        _, con = handle_events_unix_console(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["cub"] + b":1")
        _os_write.assert_any_call(ANY, b"2")
        con.restore()

    def test_multiline_function_move_up_short_terminal(self, _os_write):
        # fmt: off
        code = (
            "def f():\n"
            "  foo"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="scroll", data=None),
            ],
        )
        _, con = handle_events_short_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["ri"] + b":")
        con.restore()

    def test_multiline_function_move_up_down_short_terminal(self, _os_write):
        # fmt: off
        code = (
            "def f():\n"
            "  foo"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="scroll", data=None),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="scroll", data=None),
            ],
        )
        _, con = handle_events_short_unix_console(events)
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["ri"] + b":")
        _os_write.assert_any_call(ANY, TERM_CAPABILITIES["ind"] + b":")
        con.restore()

    def test_resize_bigger_on_multiline_function(self, _os_write):
        # fmt: off
        code = (
            "def f():\n"
            "  foo"
        )
        # fmt: on

        events = itertools.chain(code_to_events(code))
        reader, console = handle_events_short_unix_console(events)

        console.height = 2
        console.getheightwidth = MagicMock(lambda _: (2, 80))

        def same_reader(_):
            return reader

        def same_console(events):
            console.get_event = MagicMock(side_effect=events)
            return console

        _, con = handle_all_events(
            [Event(evt="resize", data=None)],
            prepare_reader=same_reader,
            prepare_console=same_console,
        )
        _os_write.assert_has_calls(
            [
                call(ANY, TERM_CAPABILITIES["ri"] + b":"),
                call(ANY, TERM_CAPABILITIES["cup"] + b":0,0"),
                call(ANY, b"def f():"),
            ]
        )
        console.restore()
        con.restore()

    def test_resize_smaller_on_multiline_function(self, _os_write):
        # fmt: off
        code = (
            "def f():\n"
            "  foo"
        )
        # fmt: on

        events = itertools.chain(code_to_events(code))
        reader, console = handle_events_unix_console_height_3(events)

        console.height = 1
        console.getheightwidth = MagicMock(lambda _: (1, 80))

        def same_reader(_):
            return reader

        def same_console(events):
            console.get_event = MagicMock(side_effect=events)
            return console

        _, con = handle_all_events(
            [Event(evt="resize", data=None)],
            prepare_reader=same_reader,
            prepare_console=same_console,
        )
        _os_write.assert_has_calls(
            [
                call(ANY, TERM_CAPABILITIES["ind"] + b":"),
                call(ANY, TERM_CAPABILITIES["cup"] + b":0,0"),
                call(ANY, b"  foo"),
            ]
        )
        console.restore()
        con.restore()

    def test_getheightwidth_with_invalid_environ(self, _os_write):
        # gh-128636
        console = UnixConsole()
        with os_helper.EnvironmentVarGuard() as env:
            env["LINES"] = ""
            self.assertIsInstance(console.getheightwidth(), tuple)
            env["COLUMNS"] = ""
            self.assertIsInstance(console.getheightwidth(), tuple)
            os.environ = []
            self.assertIsInstance(console.getheightwidth(), tuple)
