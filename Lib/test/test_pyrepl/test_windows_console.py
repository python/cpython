import itertools
import sys
import unittest
from _pyrepl.console import Event, Console
from _pyrepl.windows_console import (
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    ERASE_IN_LINE,
)
from functools import partial
from typing import Iterable
from unittest import TestCase, main
from unittest.mock import MagicMock, call, patch, ANY

from .support import handle_all_events, code_to_events

try:
    from _pyrepl.console import Event
    from _pyrepl.windows_console import WindowsConsole
except ImportError:
    pass


@patch("os.write")
@unittest.skipIf(sys.platform != "win32", "No Unix event queue on Windows")
class WindowsConsoleTests(TestCase):
    def console(self, events, **kwargs) -> Console:
        console = WindowsConsole()
        console.get_event = MagicMock(side_effect=events)
        console._scroll = MagicMock()
        console._hide_cursor = MagicMock()
        console._show_cursor = MagicMock()
        console._getscrollbacksize = MagicMock(42)

        height = kwargs.get("height", 25)
        width = kwargs.get("width", 80)
        console.getheightwidth = MagicMock(side_effect=lambda: (height, width))

        console.prepare()
        for key, val in kwargs.items():
            setattr(console, key, val)
        return console

    def handle_events(self, events: Iterable[Event], **kwargs):
        return handle_all_events(events, partial(self.console, **kwargs))

    def handle_events_narrow(self, events):
        return self.handle_events(events, width=5)

    def handle_events_short(self, events):
        return self.handle_events(events, height=1)

    def handle_events_height_3(self, events):
        return self.handle_events(events, height=3)

    def test_simple_addition(self, _os_write):
        code = "12+34"
        events = code_to_events(code)
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, b"2")
        _os_write.assert_any_call(ANY, b"+")
        _os_write.assert_any_call(ANY, b"3")
        _os_write.assert_any_call(ANY, b"4")
        con.restore()

    def test_wrap(self, _os_write):
        code = "12+34"
        events = code_to_events(code)
        _, con = self.handle_events_narrow(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, b"2")
        _os_write.assert_any_call(ANY, b"+")
        _os_write.assert_any_call(ANY, b"3")
        _os_write.assert_any_call(ANY, b"\\")
        _os_write.assert_any_call(ANY, b"\n")
        _os_write.assert_any_call(ANY, b"4")
        con.restore()

    def test_resize_wider(self, _os_write):
        code = "1234567890"
        events = code_to_events(code)
        reader, console = self.handle_events_narrow(events)

        console.height = 20
        console.width = 80
        console.getheightwidth = MagicMock(lambda _: (20, 80))

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

        _os_write.assert_any_call(ANY, self.move_right(2))
        _os_write.assert_any_call(ANY, self.move_up(2))
        _os_write.assert_any_call(ANY, b"567890")

        con.restore()

    def test_resize_narrower(self, _os_write):
        code = "1234567890"
        events = code_to_events(code)
        reader, console = self.handle_events(events)

        console.height = 20
        console.width = 4
        console.getheightwidth = MagicMock(lambda _: (20, 4))

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

        _os_write.assert_any_call(ANY, b"456\\")
        _os_write.assert_any_call(ANY, b"789\\")

        con.restore()

    def test_cursor_left(self, _os_write):
        code = "1"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
        )
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, self.move_left())
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
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, self.move_left())
        _os_write.assert_any_call(ANY, self.move_right())
        con.restore()

    def test_cursor_up(self, _os_write):
        code = "1\n2+3"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="up", raw=bytearray(b"\x1bOA"))],
        )
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, self.move_up())
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
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, self.move_up())
        _os_write.assert_any_call(ANY, self.move_down())
        con.restore()

    def test_cursor_back_write(self, _os_write):
        events = itertools.chain(
            code_to_events("1"),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
            code_to_events("2"),
        )
        _, con = self.handle_events(events)
        _os_write.assert_any_call(ANY, b"1")
        _os_write.assert_any_call(ANY, self.move_left())
        _os_write.assert_any_call(ANY, b"21")
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
        _, con = self.handle_events_short(events)
        _os_write.assert_any_call(ANY, self.move_left(5))
        _os_write.assert_any_call(ANY, self.move_up())
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
        _, con = self.handle_events_short(events)
        _os_write.assert_any_call(ANY, self.move_left(8))
        _os_write.assert_any_call(ANY, self.erase_in_line())
        con.restore()

    def test_resize_bigger_on_multiline_function(self, _os_write):
        # fmt: off
        code = (
            "def f():\n"
            "  foo"
        )
        # fmt: on

        events = itertools.chain(code_to_events(code))
        reader, console = self.handle_events_short(events)

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
                call(ANY, self.move_left(5)),
                call(ANY, self.move_up()),
                call(ANY, b"def f():"),
                call(ANY, self.move_left(3)),
                call(ANY, self.move_down()),
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
        reader, console = self.handle_events_height_3(events)

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
                call(ANY, self.move_left(5)),
                call(ANY, self.move_up()),
                call(ANY, self.erase_in_line()),
                call(ANY, b"  foo"),
            ]
        )
        console.restore()
        con.restore()

    def move_up(self, lines=1):
        return MOVE_UP.format(lines).encode("utf8")

    def move_down(self, lines=1):
        return MOVE_DOWN.format(lines).encode("utf8")

    def move_left(self, cols=1):
        return MOVE_LEFT.format(cols).encode("utf8")

    def move_right(self, cols=1):
        return MOVE_RIGHT.format(cols).encode("utf8")

    def erase_in_line(self):
        return ERASE_IN_LINE.encode("utf8")


if __name__ == "__main__":
    unittest.main()
