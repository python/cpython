import itertools
from unittest import TestCase

from .support import handle_all_events, handle_events_narrow_console, code_to_events
from _pyrepl.console import Event


class TestReader(TestCase):
    def assert_screen_equals(self, reader, expected):
        actual = reader.calc_screen()
        expected = expected.split("\n")
        self.assertListEqual(actual, expected)

    def test_calc_screen_wrap_simple(self):
        events = code_to_events(10 * "a")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equals(reader, f"{9*"a"}\\\na")

    def test_calc_screen_wrap_wide_characters(self):
        events = code_to_events(8 * "a" + "樂")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equals(reader, f"{8*"a"}\\\n樂")

    def test_calc_screen_wrap_three_lines(self):
        events = code_to_events(20 * "a")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equals(reader, f"{9*"a"}\\\n{9*"a"}\\\naa")

    def test_calc_screen_wrap_three_lines_mixed_character(self):
        # fmt: off
        code = (
            "def f():\n"
           f"  {8*"a"}\n"
           f"  {5*"樂"}"
        )
        # fmt: on

        events = code_to_events(code)
        reader, _ = handle_events_narrow_console(events)

        # fmt: off
        self.assert_screen_equals(reader, (
            "def f():\n"
           f"  {7*"a"}\\\n"
            "a\n"
           f"  {3*"樂"}\\\n"
            "樂樂"
        ))
        # fmt: on

    def test_calc_screen_backspace(self):
        events = itertools.chain(
            code_to_events("aaa"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_all_events(events)
        self.assert_screen_equals(reader, "aa")

    def test_calc_screen_wrap_removes_after_backspace(self):
        events = itertools.chain(
            code_to_events(10 * "a"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equals(reader, 9 * "a")

    def test_calc_screen_backspace_in_second_line_after_wrap(self):
        events = itertools.chain(
            code_to_events(11 * "a"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equals(reader, f"{9*"a"}\\\na")

    def test_setpos_for_xy_simple(self):
        events = code_to_events("11+11")
        reader, _ = handle_all_events(events)
        reader.setpos_from_xy(0, 0)
        self.assertEqual(reader.pos, 0)

    def test_setpos_from_xy_multiple_lines(self):
        # fmt: off
        code = (
            "def foo():\n"
            "  return 1"
        )
        # fmt: on

        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        reader.setpos_from_xy(2, 1)
        self.assertEqual(reader.pos, 13)

    def test_setpos_from_xy_after_wrap(self):
        # fmt: off
        code = (
            "def foo():\n"
            "  hello"
        )
        # fmt: on

        events = code_to_events(code)
        reader, _ = handle_events_narrow_console(events)
        reader.setpos_from_xy(2, 2)
        self.assertEqual(reader.pos, 13)

    def test_setpos_fromxy_in_wrapped_line(self):
        # fmt: off
        code = (
            "def foo():\n"
            "  hello"
        )
        # fmt: on

        events = code_to_events(code)
        reader, _ = handle_events_narrow_console(events)
        reader.setpos_from_xy(0, 1)
        self.assertEqual(reader.pos, 9)

    def test_up_arrow_after_ctrl_r(self):
        events = iter(
            [
                Event(evt="key", data="\x12", raw=bytearray(b"\x12")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            ]
        )

        reader, _ = handle_all_events(events)
        self.assert_screen_equals(reader, "")
