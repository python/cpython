import sys
import unittest

if sys.platform != "win32":
    raise unittest.SkipTest("test only relevant on win32")


import itertools
from functools import partial
from test.support import force_not_colorized_test_class
from typing import Iterable
from unittest import TestCase
from unittest.mock import MagicMock, call

from .support import handle_all_events, code_to_events
from .support import prepare_reader as default_prepare_reader

try:
    from _pyrepl.console import Event, Console
    from _pyrepl.windows_console import (
        WindowsConsole,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        ERASE_IN_LINE,
    )
    import _pyrepl.windows_console as wc
except ImportError:
    pass


@force_not_colorized_test_class
class WindowsConsoleTests(TestCase):
    def console(self, events, **kwargs) -> Console:
        console = WindowsConsole()
        console.get_event = MagicMock(side_effect=events)
        console.getpending = MagicMock(return_value=Event("key", ""))
        console.wait = MagicMock()
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

    def handle_events(
        self,
        events: Iterable[Event],
        prepare_console=None,
        prepare_reader=None,
        **kwargs,
    ):
        prepare_console = prepare_console or partial(self.console, **kwargs)
        prepare_reader = prepare_reader or default_prepare_reader
        return handle_all_events(events, prepare_console, prepare_reader)

    def handle_events_narrow(self, events):
        return self.handle_events(events, width=5)

    def handle_events_short(self, events, **kwargs):
        return self.handle_events(events, height=1, **kwargs)

    def handle_events_height_3(self, events):
        return self.handle_events(events, height=3)

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
        reader, con = self.handle_events_narrow(events)
        self.assertEqual(reader.cxy, (2, 3))
        con.restore()


class WindowsConsoleGetEventTests(TestCase):
    # Virtual-Key Codes: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    VK_BACK = 0x08
    VK_RETURN = 0x0D
    VK_LEFT = 0x25
    VK_7 = 0x37
    VK_M = 0x4D
    # Used for miscellaneous characters; it can vary by keyboard.
    # For the US standard keyboard, the '" key.
    # For the German keyboard, the Ä key.
    VK_OEM_7 = 0xDE

    # State of control keys: https://learn.microsoft.com/en-us/windows/console/key-event-record-str
    RIGHT_ALT_PRESSED = 0x0001
    RIGHT_CTRL_PRESSED = 0x0004
    LEFT_ALT_PRESSED = 0x0002
    LEFT_CTRL_PRESSED = 0x0008
    ENHANCED_KEY = 0x0100
    SHIFT_PRESSED = 0x0010


    def get_event(self, input_records, **kwargs) -> Console:
        self.console = WindowsConsole(encoding='utf-8')
        self.mock = MagicMock(side_effect=input_records)
        self.console._read_input = self.mock
        self.console._WindowsConsole__vt_support = kwargs.get("vt_support",
                                                              False)
        self.console.wait = MagicMock(return_value=True)
        event = self.console.get_event(block=False)
        return event

    def get_input_record(self, unicode_char, vcode=0, control=0):
        return wc.INPUT_RECORD(
            wc.KEY_EVENT,
            wc.ConsoleEvent(KeyEvent=
                wc.KeyEvent(
                    bKeyDown=True,
                    wRepeatCount=1,
                    wVirtualKeyCode=vcode,
                    wVirtualScanCode=0, # not used
                    uChar=wc.Char(unicode_char),
                    dwControlKeyState=control
                    )))

    def test_EmptyBuffer(self):
        self.assertEqual(self.get_event([None]), None)
        self.assertEqual(self.mock.call_count, 1)

    def test_WINDOW_BUFFER_SIZE_EVENT(self):
        ir = wc.INPUT_RECORD(
            wc.WINDOW_BUFFER_SIZE_EVENT,
            wc.ConsoleEvent(WindowsBufferSizeEvent=
                wc.WindowsBufferSizeEvent(
                    wc._COORD(0, 0))))
        self.assertEqual(self.get_event([ir]), Event("resize", ""))
        self.assertEqual(self.mock.call_count, 1)

    def test_KEY_EVENT_up_ignored(self):
        ir = wc.INPUT_RECORD(
            wc.KEY_EVENT,
            wc.ConsoleEvent(KeyEvent=
                wc.KeyEvent(bKeyDown=False)))
        self.assertEqual(self.get_event([ir]), None)
        self.assertEqual(self.mock.call_count, 1)

    def test_unhandled_events(self):
        for event in (wc.FOCUS_EVENT, wc.MENU_EVENT, wc.MOUSE_EVENT):
            ir = wc.INPUT_RECORD(
                event,
                # fake data, nothing is read except bKeyDown
                wc.ConsoleEvent(KeyEvent=
                    wc.KeyEvent(bKeyDown=False)))
            self.assertEqual(self.get_event([ir]), None)
            self.assertEqual(self.mock.call_count, 1)

    def test_enter(self):
        ir = self.get_input_record("\r", self.VK_RETURN)
        self.assertEqual(self.get_event([ir]), Event("key", "\n"))
        self.assertEqual(self.mock.call_count, 1)

    def test_backspace(self):
        ir = self.get_input_record("\x08", self.VK_BACK)
        self.assertEqual(
            self.get_event([ir]), Event("key", "backspace"))
        self.assertEqual(self.mock.call_count, 1)

    def test_m(self):
        ir = self.get_input_record("m", self.VK_M)
        self.assertEqual(self.get_event([ir]), Event("key", "m"))
        self.assertEqual(self.mock.call_count, 1)

    def test_M(self):
        ir = self.get_input_record("M", self.VK_M, self.SHIFT_PRESSED)
        self.assertEqual(self.get_event([ir]), Event("key", "M"))
        self.assertEqual(self.mock.call_count, 1)

    def test_left(self):
        # VK_LEFT is sent as ENHANCED_KEY
        ir = self.get_input_record("\x00", self.VK_LEFT, self.ENHANCED_KEY)
        self.assertEqual(self.get_event([ir]), Event("key", "left"))
        self.assertEqual(self.mock.call_count, 1)

    def test_left_RIGHT_CTRL_PRESSED(self):
        ir = self.get_input_record(
            "\x00", self.VK_LEFT, self.RIGHT_CTRL_PRESSED | self.ENHANCED_KEY)
        self.assertEqual(
            self.get_event([ir]), Event("key", "ctrl left"))
        self.assertEqual(self.mock.call_count, 1)

    def test_left_LEFT_CTRL_PRESSED(self):
        ir = self.get_input_record(
            "\x00", self.VK_LEFT, self.LEFT_CTRL_PRESSED | self.ENHANCED_KEY)
        self.assertEqual(
            self.get_event([ir]), Event("key", "ctrl left"))
        self.assertEqual(self.mock.call_count, 1)

    def test_left_RIGHT_ALT_PRESSED(self):
        ir = self.get_input_record(
            "\x00", self.VK_LEFT, self.RIGHT_ALT_PRESSED | self.ENHANCED_KEY)
        self.assertEqual(self.get_event([ir]), Event(evt="key", data="\033"))
        self.assertEqual(
            self.console.get_event(), Event("key", "left"))
        # self.mock is not called again, since the second time we read from the
        # command queue
        self.assertEqual(self.mock.call_count, 1)

    def test_left_LEFT_ALT_PRESSED(self):
        ir = self.get_input_record(
            "\x00", self.VK_LEFT, self.LEFT_ALT_PRESSED | self.ENHANCED_KEY)
        self.assertEqual(self.get_event([ir]), Event(evt="key", data="\033"))
        self.assertEqual(
            self.console.get_event(), Event("key", "left"))
        self.assertEqual(self.mock.call_count, 1)

    def test_m_LEFT_ALT_PRESSED_and_LEFT_CTRL_PRESSED(self):
        # For the shift keys, Windows does not send anything when
        # ALT and CTRL are both pressed, so let's test with VK_M.
        # get_event() receives this input, but does not
        # generate an event.
        # This is for e.g. an English keyboard layout, for a
        # German layout this returns `µ`, see test_AltGr_m.
        ir = self.get_input_record(
            "\x00", self.VK_M, self.LEFT_ALT_PRESSED | self.LEFT_CTRL_PRESSED)
        self.assertEqual(self.get_event([ir]), None)
        self.assertEqual(self.mock.call_count, 1)

    def test_m_LEFT_ALT_PRESSED(self):
        ir = self.get_input_record(
            "m", vcode=self.VK_M, control=self.LEFT_ALT_PRESSED)
        self.assertEqual(self.get_event([ir]), Event(evt="key", data="\033"))
        self.assertEqual(self.console.get_event(), Event("key", "m"))
        self.assertEqual(self.mock.call_count, 1)

    def test_m_RIGHT_ALT_PRESSED(self):
        ir = self.get_input_record(
            "m", vcode=self.VK_M, control=self.RIGHT_ALT_PRESSED)
        self.assertEqual(self.get_event([ir]), Event(evt="key", data="\033"))
        self.assertEqual(self.console.get_event(), Event("key", "m"))
        self.assertEqual(self.mock.call_count, 1)

    def test_AltGr_7(self):
        # E.g. on a German keyboard layout, '{' is entered via
        # AltGr + 7, where AltGr is the right Alt key on the keyboard.
        # In this case, Windows automatically sets
        # RIGHT_ALT_PRESSED = 0x0001 + LEFT_CTRL_PRESSED = 0x0008
        # This can also be entered like
        # LeftAlt + LeftCtrl + 7 or
        # LeftAlt + RightCtrl + 7
        # See https://learn.microsoft.com/en-us/windows/console/key-event-record-str
        # https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-vkkeyscanw
        ir = self.get_input_record(
            "{", vcode=self.VK_7,
            control=self.RIGHT_ALT_PRESSED | self.LEFT_CTRL_PRESSED)
        self.assertEqual(self.get_event([ir]), Event("key", "{"))
        self.assertEqual(self.mock.call_count, 1)

    def test_AltGr_m(self):
        # E.g. on a German keyboard layout, this yields 'µ'
        # Let's use LEFT_ALT_PRESSED and RIGHT_CTRL_PRESSED this
        # time, to cover that, too. See above in test_AltGr_7.
        ir = self.get_input_record(
            "µ", vcode=self.VK_M, control=self.LEFT_ALT_PRESSED | self.RIGHT_CTRL_PRESSED)
        self.assertEqual(self.get_event([ir]), Event("key", "µ"))
        self.assertEqual(self.mock.call_count, 1)

    def test_umlaut_a_german(self):
        ir = self.get_input_record("ä", self.VK_OEM_7)
        self.assertEqual(self.get_event([ir]), Event("key", "ä"))
        self.assertEqual(self.mock.call_count, 1)

    # virtual terminal tests
    # Note: wVirtualKeyCode, wVirtualScanCode and dwControlKeyState
    # are always zero in this case.
    # "\r" and backspace are handled specially, everything else
    # is handled in "elif self.__vt_support:" in WindowsConsole.get_event().
    # Hence, only one regular key ("m") and a terminal sequence
    # are sufficient to test here, the real tests happen in test_eventqueue
    # and test_keymap.

    def test_enter_vt(self):
        ir = self.get_input_record("\r")
        self.assertEqual(self.get_event([ir], vt_support=True),
                         Event("key", "\n"))
        self.assertEqual(self.mock.call_count, 1)

    def test_backspace_vt(self):
        ir = self.get_input_record("\x7f")
        self.assertEqual(self.get_event([ir], vt_support=True),
                         Event("key", "backspace", b"\x7f"))
        self.assertEqual(self.mock.call_count, 1)

    def test_up_vt(self):
        irs = [self.get_input_record(x) for x in "\x1b[A"]
        self.assertEqual(self.get_event(irs, vt_support=True),
                         Event(evt='key', data='up', raw=bytearray(b'\x1b[A')))
        self.assertEqual(self.mock.call_count, 3)


if __name__ == "__main__":
    unittest.main()
