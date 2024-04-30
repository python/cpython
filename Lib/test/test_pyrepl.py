import curses
import itertools
import os
import rlcompleter
import sys
from code import InteractiveConsole
from contextlib import suppress
from functools import partial
from io import BytesIO
from unittest import TestCase
from unittest.mock import MagicMock, call, patch

import _pyrepl.unix_eventqueue as unix_eventqueue
from _pyrepl.console import Console, Event
from _pyrepl.readline import ReadlineAlikeReader, ReadlineConfig
from _pyrepl.simple_interact import _strip_final_indent
from _pyrepl.unix_eventqueue import EventQueue


def more_lines(unicodetext, namespace=None):
    if namespace is None:
        namespace = {}
    src = _strip_final_indent(unicodetext)
    console = InteractiveConsole(namespace, filename="<stdin>")
    try:
        code = console.compile(src, "<stdin>", "single")
    except (OverflowError, SyntaxError, ValueError):
        return False
    else:
        return code is None


def multiline_input(reader, namespace=None):
    saved = reader.more_lines
    try:
        reader.more_lines = partial(more_lines, namespace=namespace)
        reader.ps1 = reader.ps2 = ">>>"
        reader.ps3 = reader.ps4 = "..."
        return reader.readline()
    finally:
        reader.more_lines = saved
        reader.paste_mode = False


def code_to_events(code):
    for c in code:
        yield Event(evt='key', data=c, raw=bytearray(c.encode('utf-8')))


class FakeConsole(Console):
    def __init__(self, events, encoding="utf-8"):
        self.events = iter(events)
        self.encoding = encoding
        self.screen = []
        self.height = 100
        self.width = 80

    def get_event(self, block: bool = True) -> Event:
        return next(self.events)

    def getpending(self) -> Event:
        return self.get_event(block=False)

    def getheightwidth(self) -> tuple[int, int]:
        return self.height, self.width

    def refresh(self, screen: list[str], xy: tuple[int, int]) -> None:
        pass

    def prepare(self) -> None:
        pass

    def restore(self) -> None:
        pass

    def move_cursor(self, x: int, y: int) -> None:
        pass

    def set_cursor_vis(self, visible: bool) -> None:
        pass

    def push_char(self, char: str) -> None:
        pass

    def beep(self) -> None:
        pass

    def clear(self) -> None:
        pass

    def finish(self) -> None:
        pass

    def flushoutput(self) -> None:
        pass

    def forgetinput(self) -> None:
        pass

    def wait(self) -> None:
        pass


class TestPyReplDriver(TestCase):
    def prepare_reader(self, events):
        console = MagicMock()
        console.get_event.side_effect = events
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        return reader, console

    def test_up_arrow(self):
        code = (
            'def f():\n'
            '  ...\n'
        )
        events = itertools.chain(code_to_events(code), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader, console = self.prepare_reader(events)

        with suppress(StopIteration):
            _ = multiline_input(reader)

        console.move_cursor.assert_called_with(1, 3)

    def test_down_arrow(self):
        code = (
            'def f():\n'
            '  ...\n'
        )
        events = itertools.chain(code_to_events(code), [
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader, console = self.prepare_reader(events)

        with suppress(StopIteration):
            _ = multiline_input(reader)

        console.move_cursor.assert_called_with(1, 5)

    def test_left_arrow(self):
        events = itertools.chain(code_to_events('11+11'), [
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader, console = self.prepare_reader(events)

        _ = multiline_input(reader)

        console.move_cursor.assert_has_calls(
            [
                call(3, 1),
            ]
        )

    def test_right_arrow(self):
        events = itertools.chain(code_to_events('11+11'), [
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader, console = self.prepare_reader(events)

        _ = multiline_input(reader)

        console.move_cursor.assert_has_calls(
            [
                call(4, 1),
            ]
        )


class TestCursorPosition(TestCase):
    def prepare_reader(self, events):
        console = MagicMock()
        console.get_event.side_effect = events
        console.height = 100
        console.width = 80

        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        reader.more_lines = partial(more_lines, namespace=None)
        reader.paste_mode = True  # Avoid extra indents

        def get_prompt(lineno, cursor_on_line) -> str:
            return ""

        reader.get_prompt = get_prompt  # Remove prompt for easier calculations of (x, y)
        return reader, console

    def handle_all_events(self, events):
        reader, _ = self.prepare_reader(events)
        try:
            while True:
                reader.handle1()
        except StopIteration:
            pass
        return reader

    def test_cursor_position_simple_character(self):
        events = itertools.chain(code_to_events("k"))

        reader = self.handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 1 for simple character
        self.assertEqual(reader.pos2xy(reader.pos), (1, 0))

    def test_cursor_position_double_width_character(self):
        events = itertools.chain(code_to_events("樂"))

        reader = self.handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 2 for wide character
        self.assertEqual(reader.pos2xy(reader.pos), (2, 0))

    def test_cursor_position_double_width_character_move_left(self):
        events = itertools.chain(code_to_events("樂"), [
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
        ])

        reader = self.handle_all_events(events)
        self.assertEqual(reader.pos, 0)
        self.assertEqual(reader.pos2xy(reader.pos), (0, 0))

    def test_cursor_position_double_width_character_move_left_right(self):
        events = itertools.chain(code_to_events("樂"), [
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
        ])

        reader = self.handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 2 for wide character
        self.assertEqual(reader.pos2xy(reader.pos), (2, 0))

    def test_cursor_position_double_width_characters_move_up(self):
        for_loop = "for _ in _:"
        events = itertools.chain(code_to_events(f"{for_loop}\n  ' 可口可乐; 可口可樂'"), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
        ])

        reader = self.handle_all_events(events)

        # cursor at end of first line
        self.assertEqual(reader.pos, len(for_loop))
        self.assertEqual(reader.pos2xy(reader.pos), (len(for_loop), 0))

    def test_cursor_position_double_width_characters_move_up_down(self):
        for_loop = "for _ in _:"
        events = itertools.chain(code_to_events(f"{for_loop}\n  ' 可口可乐; 可口可樂'"), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
        ])

        reader = self.handle_all_events(events)

        # cursor here (showing 2nd line only):
        # <  ' 可口可乐; 可口可樂'>
        #              ^
        self.assertEqual(reader.pos, 22)
        self.assertEqual(reader.pos2xy(reader.pos), (14, 1))

    def test_cursor_position_multiple_double_width_characters_move_left(self):
        events = itertools.chain(code_to_events("' 可口可乐; 可口可樂'"), [
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
        ])

        reader = self.handle_all_events(events)
        self.assertEqual(reader.pos, 10)

        # 1 for quote, 1 for space, 2 per wide character,
        # 1 for semicolon, 1 for space, 2 per wide character
        self.assertEqual(reader.pos2xy(reader.pos), (16, 0))

    def test_cursor_position_move_up_to_eol(self):
        for_loop = "for _ in _"
        code = f"{for_loop}:\n  hello\n  h\n  hel"
        events = itertools.chain(code_to_events(code), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
        ])

        reader = self.handle_all_events(events)

        # Cursor should be at end of line 1, even though line 2 is shorter
        # for _ in _:
        #   hello
        #   h
        #   hel
        self.assertEqual(reader.pos, len(for_loop))
        self.assertEqual(reader.pos2xy(reader.pos), (len(for_loop), 0))

    def test_cursor_position_move_down_to_eol(self):
        last_line = "  hel"
        code = f"for _ in _:\n  hello\n  h\n{last_line}"
        events = itertools.chain(code_to_events(code), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
        ])

        reader = self.handle_all_events(events)

        # Cursor should be at end of line 3, even though line 2 is shorter
        # for _ in _:
        #   hello
        #   h
        #   hel
        self.assertEqual(reader.pos, len(code))
        self.assertEqual(reader.pos2xy(reader.pos), (len(last_line), 3))


class TestPyReplOutput(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        reader.config.readline_completer = None
        return reader

    def test_basic(self):
        reader = self.prepare_reader(code_to_events('1+1\n'))

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_multiline_edit(self):
        events = itertools.chain(code_to_events('def f():\n  ...\n\n'), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            Event(evt="key", data="g", raw=bytearray(b"g")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])
        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "def f():\n  ...\n  ")
        output = multiline_input(reader)
        self.assertEqual(output, "def g():\n  ...\n  ")

    def test_history_navigation_with_up_arrow(self):
        events = itertools.chain(code_to_events('1+1\n2+2\n'), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")
        output = multiline_input(reader)
        self.assertEqual(output, "2+2")
        output = multiline_input(reader)
        self.assertEqual(output, "2+2")
        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_history_navigation_with_down_arrow(self):
        events = itertools.chain(code_to_events('1+1\n2+2\n'), [
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
        ])

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_history_search(self):
        events = itertools.chain(code_to_events('1+1\n2+2\n3+3\n'), [
            Event(evt="key", data="\x12", raw=bytearray(b"\x12")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ])

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")
        output = multiline_input(reader)
        self.assertEqual(output, "2+2")
        output = multiline_input(reader)
        self.assertEqual(output, "3+3")
        output = multiline_input(reader)
        self.assertEqual(output, "1+1")


class TestPyReplCompleter(TestCase):
    def prepare_reader(self, events, namespace):
        console = FakeConsole(events)
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        reader.config.readline_completer = rlcompleter.Completer(namespace).complete
        return reader

    def test_simple_completion(self):
        events = code_to_events('os.geten\t\n')

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.getenv")

    def test_completion_with_many_options(self):
        events = code_to_events('os.\t\tO_AS\t\n')

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.O_ASYNC")

    def test_empty_namespace_completion(self):
        events = code_to_events('os.geten\t\n')
        namespace = {}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.geten")

    def test_global_namespace_completion(self):
        events = code_to_events('py\t\n')
        namespace = {"python": None}
        reader = self.prepare_reader(events, namespace)
        output = multiline_input(reader, namespace)
        self.assertEqual(output, "python")


class TestUnivEventQueue(TestCase):
    def setUp(self) -> None:
        curses.setupterm()
        return super().setUp()

    def test_get(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        event = Event("key", "a", b"a")
        eq.insert(event)
        self.assertEqual(eq.get(), event)

    def test_empty(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        self.assertTrue(eq.empty())
        eq.insert(Event("key", "a", b"a"))
        self.assertFalse(eq.empty())

    def test_flush_buf(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.buf.extend(b"test")
        self.assertEqual(eq.flush_buf(), b"test")
        self.assertEqual(eq.buf, bytearray())

    def test_insert(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        event = Event("key", "a", b"a")
        eq.insert(event)
        self.assertEqual(eq.events[0], event)

    @patch("_pyrepl.unix_eventqueue.keymap")
    def test_push_with_key_in_keymap(self, mock_keymap):
        mock_keymap.compile_keymap.return_value = {"a": "b"}
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {b"a": "b"}
        eq.push("a")
        self.assertTrue(mock_keymap.compile_keymap.called)
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "b")

    @patch("_pyrepl.unix_eventqueue.keymap")
    def test_push_without_key_in_keymap(self, mock_keymap):
        mock_keymap.compile_keymap.return_value = {"a": "b"}
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {b"c": "d"}
        eq.push("a")
        self.assertTrue(mock_keymap.compile_keymap.called)
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "a")

    @patch("_pyrepl.unix_eventqueue.keymap")
    def test_push_with_keymap_in_keymap(self, mock_keymap):
        mock_keymap.compile_keymap.return_value = {"a": "b"}
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {b"a": {b"b": "c"}}
        eq.push("a")
        self.assertTrue(mock_keymap.compile_keymap.called)
        self.assertTrue(eq.empty())
        eq.push("b")
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "c")
        eq.push("d")
        self.assertEqual(eq.events[1].evt, "key")
        self.assertEqual(eq.events[1].data, "d")

    @patch("_pyrepl.unix_eventqueue.keymap")
    def test_push_with_keymap_in_keymap_and_escape(self, mock_keymap):
        mock_keymap.compile_keymap.return_value = {"a": "b"}
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {b"a": {b"b": "c"}}
        eq.push("a")
        self.assertTrue(mock_keymap.compile_keymap.called)
        self.assertTrue(eq.empty())
        eq.flush_buf()
        eq.push("\033")
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "\033")
        eq.push("b")
        self.assertEqual(eq.events[1].evt, "key")
        self.assertEqual(eq.events[1].data, "b")

    def test_push_special_key(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {}
        eq.push("\x1b")
        eq.push("[")
        eq.push("A")
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "\x1b")

    def test_push_unrecognized_escape_sequence(self):
        eq = EventQueue(sys.stdout.fileno(), "utf-8")
        eq.keymap = {}
        eq.push("\x1b")
        eq.push("[")
        eq.push("Z")
        self.assertEqual(len(eq.events), 3)
        self.assertEqual(eq.events[0].evt, "key")
        self.assertEqual(eq.events[0].data, "\x1b")
        self.assertEqual(eq.events[1].evt, "key")
        self.assertEqual(eq.events[1].data, "[")
        self.assertEqual(eq.events[2].evt, "key")
        self.assertEqual(eq.events[2].data, "Z")


class TestPasteEvent(TestCase):
    def setUp(self) -> None:
        curses.setupterm()
        return super().setUp()

    def prepare_reader(self, events):
        console = FakeConsole(events)
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        reader.config.readline_completer = None
        return reader

    def test_paste(self):
        code = (
            'def a():\n'
            '  for x in range(10):\n'
            '    if x%2:\n'
            '      print(x)\n'
            '    else:\n'
            '      pass\n\n'
        )

        events = itertools.chain([
            Event(evt='key', data='f3', raw=bytearray(b'\x1bOR')),
        ], code_to_events(code))
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, code[:-1])

    def test_paste_not_in_paste_mode(self):
        input_code = (
            'def a():\n'
            '  for x in range(10):\n'
            '    if x%2:\n'
            '      print(x)\n'
            '    else:\n'
            '      pass\n\n'
        )

        output_code = (
            'def a():\n'
            '  for x in range(10):\n'
            '      if x%2:\n'
            '            print(x)\n'
            '                else:'
        )

        events = code_to_events(input_code)
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)


if __name__ == "__main__":
    unittest.main()
