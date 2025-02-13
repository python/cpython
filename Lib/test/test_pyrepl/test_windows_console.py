import sys
import unittest

if sys.platform != "win32":
    raise unittest.SkipTest("test only relevant on win32")


import itertools
from functools import partial
from typing import Iterable
from unittest import TestCase
from unittest.mock import MagicMock, call

from .support import handle_all_events, code_to_events

try:
    from _pyrepl.console import Event, Console
    from _pyrepl.windows_console import (
        WindowsConsole,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        ERASE_IN_LINE,
        INPUT_RECORD,
        ConsoleEvent,
        KeyEvent,
        Char,
        KEY_EVENT
    )
except ImportError:
    pass


def make_input_record(character: str, repeat_count: int = 1, virtual_keycode: int = 0):
    assert len(character) == 1

    rec = INPUT_RECORD()
    rec.EventType = KEY_EVENT
    rec.Event = ConsoleEvent()
    rec.Event.KeyEvent = KeyEvent()

    rec.Event.KeyEvent.bKeyDown = True
    rec.Event.KeyEvent.wRepeatCount = repeat_count
    rec.Event.KeyEvent.wVirtualKeyCode = virtual_keycode  # Only used for special keys (see VK_MAP in windows_console.py)
    rec.Event.KeyEvent.wVirtualScanCode = 0  # Not used by WindowsConsole
    rec.Event.KeyEvent.uChar = Char()
    rec.Event.KeyEvent.uChar.UnicodeChar = character
    rec.Event.KeyEvent.dwControlKeyState = False
    return rec


class WindowsConsoleTests(TestCase):
    def console(self, events, mock_input_record=False, **kwargs) -> Console:
        console = WindowsConsole()
        if mock_input_record:
            # Mock the lower level _read_input method instead of get_event
            console._read_input = MagicMock(side_effect=events)
        else:
            console.get_event = MagicMock(side_effect=events)
        console._scroll = MagicMock()
        console._hide_cursor = MagicMock()
        console._show_cursor = MagicMock()
        console._getscrollbacksize = MagicMock(42)
        console.out = MagicMock()

        height = kwargs.get("height", 25)
        width = kwargs.get("width", 80)
        console.getheightwidth = MagicMock(side_effect=lambda: (height, width))

        console.prepare()
        for key, val in kwargs.items():
            setattr(console, key, val)
        return console

    def handle_events(self, events: Iterable[Event], **kwargs):
        return handle_all_events(events, partial(self.console, **kwargs))

    def handle_input_records(self, input_records: Iterable[INPUT_RECORD], **kwargs):
        return handle_all_events(input_records, partial(self.console, mock_input_record=True, **kwargs))

    def handle_events_narrow(self, events):
        return self.handle_events(events, width=5)

    def handle_events_short(self, events):
        return self.handle_events(events, height=1)

    def handle_events_height_3(self, events):
        return self.handle_events(events, height=3)

    def test_key_records_no_repeat(self):
        input_records = [make_input_record(c) for c in "12+34"]
        _, con = self.handle_input_records(input_records)
        expected_calls = [call(c.encode()) for c in "12+34"]
        con.out.write.assert_has_calls(expected_calls)
        con.restore()

    def test_key_records_with_repeat(self):
        input_records = [make_input_record(c, 2) for c in "12+34"]
        input_records.append(make_input_record("5", 3))
        _, con = self.handle_input_records(input_records)
        expected_calls = [call(c.encode()) for c in "1122++3344555"]
        con.out.write.assert_has_calls(expected_calls)
        con.restore()

    def test_simple_addition(self):
        code = "12+34"
        events = code_to_events(code)
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(b"1")
        con.out.write.assert_any_call(b"2")
        con.out.write.assert_any_call(b"+")
        con.out.write.assert_any_call(b"3")
        con.out.write.assert_any_call(b"4")
        con.restore()

    def test_wrap(self):
        code = "12+34"
        events = code_to_events(code)
        _, con = self.handle_events_narrow(events)
        con.out.write.assert_any_call(b"1")
        con.out.write.assert_any_call(b"2")
        con.out.write.assert_any_call(b"+")
        con.out.write.assert_any_call(b"3")
        con.out.write.assert_any_call(b"\\")
        con.out.write.assert_any_call(b"\n")
        con.out.write.assert_any_call(b"4")
        con.restore()

    def test_resize_wider(self):
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

        con.out.write.assert_any_call(self.move_right(2))
        con.out.write.assert_any_call(self.move_up(2))
        con.out.write.assert_any_call(b"567890")

        con.restore()

    def test_resize_narrower(self):
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

        con.out.write.assert_any_call(b"456\\")
        con.out.write.assert_any_call(b"789\\")

        con.restore()

    def test_cursor_left(self):
        code = "1"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
        )
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(self.move_left())
        con.restore()

    def test_cursor_left_right(self):
        code = "1"
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            ],
        )
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(self.move_left())
        con.out.write.assert_any_call(self.move_right())
        con.restore()

    def test_cursor_up(self):
        code = "1\n2+3"
        events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="up", raw=bytearray(b"\x1bOA"))],
        )
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(self.move_up())
        con.restore()

    def test_cursor_up_down(self):
        code = "1\n2+3"
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(self.move_up())
        con.out.write.assert_any_call(self.move_down())
        con.restore()

    def test_cursor_back_write(self):
        events = itertools.chain(
            code_to_events("1"),
            [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
            code_to_events("2"),
        )
        _, con = self.handle_events(events)
        con.out.write.assert_any_call(b"1")
        con.out.write.assert_any_call(self.move_left())
        con.out.write.assert_any_call(b"21")
        con.restore()

    def test_multiline_function_move_up_short_terminal(self):
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
        con.out.write.assert_any_call(self.move_left(5))
        con.out.write.assert_any_call(self.move_up())
        con.restore()

    def test_multiline_function_move_up_down_short_terminal(self):
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
        con.out.write.assert_any_call(self.move_left(8))
        con.out.write.assert_any_call(self.erase_in_line())
        con.restore()

    def test_resize_bigger_on_multiline_function(self):
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
        con.out.write.assert_has_calls(
            [
                call(self.move_left(5)),
                call(self.move_up()),
                call(b"def f():"),
                call(self.move_left(3)),
                call(self.move_down()),
            ]
        )
        console.restore()
        con.restore()

    def test_resize_smaller_on_multiline_function(self):
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
        con.out.write.assert_has_calls(
            [
                call(self.move_left(5)),
                call(self.move_up()),
                call(self.erase_in_line()),
                call(b"  foo"),
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

    def test_multiline_ctrl_z(self):
        # see gh-126332
        code = "abcdefghi"

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data='\x1a', raw=bytearray(b'\x1a')),
                Event(evt="key", data='\x1a', raw=bytearray(b'\x1a')),
            ],
        )
        reader, _ = self.handle_events_narrow(events)
        self.assertEqual(reader.cxy, (2, 3))


if __name__ == "__main__":
    unittest.main()
