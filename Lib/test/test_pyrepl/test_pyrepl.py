import io
import itertools
import os
import rlcompleter
import select
import subprocess
import sys
from unittest import TestCase, skipUnless
from unittest.mock import patch
from test.support import force_not_colorized
from test.support import SHORT_TIMEOUT

from .support import (
    FakeConsole,
    handle_all_events,
    handle_events_narrow_console,
    more_lines,
    multiline_input,
    code_to_events,
)
from _pyrepl.console import Event
from _pyrepl.readline import ReadlineAlikeReader, ReadlineConfig
from _pyrepl.readline import multiline_input as readline_multiline_input

try:
    import pty
except ImportError:
    pty = None

class TestCursorPosition(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        config = ReadlineConfig(readline_completer=None)
        reader = ReadlineAlikeReader(console=console, config=config)
        return reader

    def test_up_arrow_simple(self):
        # fmt: off
        code = (
            "def f():\n"
            "  ...\n"
        )
        # fmt: on
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            ],
        )

        reader, console = handle_all_events(events)
        self.assertEqual(reader.cxy, (0, 1))
        console.move_cursor.assert_called_once_with(0, 1)

    def test_down_arrow_end_of_input(self):
        # fmt: off
        code = (
            "def f():\n"
            "  ...\n"
        )
        # fmt: on
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )

        reader, console = handle_all_events(events)
        self.assertEqual(reader.cxy, (0, 2))
        console.move_cursor.assert_called_once_with(0, 2)

    def test_left_arrow_simple(self):
        events = itertools.chain(
            code_to_events("11+11"),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            ],
        )

        reader, console = handle_all_events(events)
        self.assertEqual(reader.cxy, (4, 0))
        console.move_cursor.assert_called_once_with(4, 0)

    def test_right_arrow_end_of_line(self):
        events = itertools.chain(
            code_to_events("11+11"),
            [
                Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            ],
        )

        reader, console = handle_all_events(events)
        self.assertEqual(reader.cxy, (5, 0))
        console.move_cursor.assert_called_once_with(5, 0)

    def test_cursor_position_simple_character(self):
        events = itertools.chain(code_to_events("k"))

        reader, _ = handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 1 for simple character
        self.assertEqual(reader.cxy, (1, 0))

    def test_cursor_position_double_width_character(self):
        events = itertools.chain(code_to_events("樂"))

        reader, _ = handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 2 for wide character
        self.assertEqual(reader.cxy, (2, 0))

    def test_cursor_position_double_width_character_move_left(self):
        events = itertools.chain(
            code_to_events("樂"),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            ],
        )

        reader, _ = handle_all_events(events)
        self.assertEqual(reader.pos, 0)
        self.assertEqual(reader.cxy, (0, 0))

    def test_cursor_position_double_width_character_move_left_right(self):
        events = itertools.chain(
            code_to_events("樂"),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            ],
        )

        reader, _ = handle_all_events(events)
        self.assertEqual(reader.pos, 1)

        # 2 for wide character
        self.assertEqual(reader.cxy, (2, 0))

    def test_cursor_position_double_width_characters_move_up(self):
        for_loop = "for _ in _:"

        # fmt: off
        code = (
           f"{for_loop}\n"
            "  ' 可口可乐; 可口可樂'"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            ],
        )

        reader, _ = handle_all_events(events)

        # cursor at end of first line
        self.assertEqual(reader.pos, len(for_loop))
        self.assertEqual(reader.cxy, (len(for_loop), 0))

    def test_cursor_position_double_width_characters_move_up_down(self):
        for_loop = "for _ in _:"

        # fmt: off
        code = (
           f"{for_loop}\n"
            "  ' 可口可乐; 可口可樂'"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )

        reader, _ = handle_all_events(events)

        # cursor here (showing 2nd line only):
        # <  ' 可口可乐; 可口可樂'>
        #              ^
        self.assertEqual(reader.pos, 19)
        self.assertEqual(reader.cxy, (10, 1))

    def test_cursor_position_multiple_double_width_characters_move_left(self):
        events = itertools.chain(
            code_to_events("' 可口可乐; 可口可樂'"),
            [
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            ],
        )

        reader, _ = handle_all_events(events)
        self.assertEqual(reader.pos, 10)

        # 1 for quote, 1 for space, 2 per wide character,
        # 1 for semicolon, 1 for space, 2 per wide character
        self.assertEqual(reader.cxy, (16, 0))

    def test_cursor_position_move_up_to_eol(self):
        first_line = "for _ in _:"
        second_line = "  hello"

        # fmt: off
        code = (
            f"{first_line}\n"
            f"{second_line}\n"
             "  h\n"
             "  hel"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            ],
        )

        reader, _ = handle_all_events(events)

        # Cursor should be at end of line 1, even though line 2 is shorter
        # for _ in _:
        #   hello
        #   h
        #   hel
        self.assertEqual(
            reader.pos, len(first_line) + len(second_line) + 1
        )  # +1 for newline
        self.assertEqual(reader.cxy, (len(second_line), 1))

    def test_cursor_position_move_down_to_eol(self):
        last_line = "  hel"

        # fmt: off
        code = (
            "for _ in _:\n"
            "  hello\n"
            "  h\n"
           f"{last_line}"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )

        reader, _ = handle_all_events(events)

        # Cursor should be at end of line 3, even though line 2 is shorter
        # for _ in _:
        #   hello
        #   h
        #   hel
        self.assertEqual(reader.pos, len(code))
        self.assertEqual(reader.cxy, (len(last_line), 3))

    def test_cursor_position_multiple_mixed_lines_move_up(self):
        # fmt: off
        code = (
            "def foo():\n"
            "  x = '可口可乐; 可口可樂'\n"
            "  y = 'abckdfjskldfjslkdjf'"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            13 * [Event(evt="key", data="left", raw=bytearray(b"\x1bOD"))],
            [Event(evt="key", data="up", raw=bytearray(b"\x1bOA"))],
        )

        reader, _ = handle_all_events(events)

        # By moving left, we're before the s:
        # y = 'abckdfjskldfjslkdjf'
        #             ^
        # And we should move before the semi-colon despite the different offset
        # x = '可口可乐; 可口可樂'
        #            ^
        self.assertEqual(reader.pos, 22)
        self.assertEqual(reader.cxy, (15, 1))

    def test_cursor_position_after_wrap_and_move_up(self):
        # fmt: off
        code = (
            "def foo():\n"
            "  hello"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            ],
        )
        reader, _ = handle_events_narrow_console(events)

        # The code looks like this:
        # def foo()\
        # :
        #   hello
        # After moving up we should be after the colon in line 2
        self.assertEqual(reader.pos, 10)
        self.assertEqual(reader.cxy, (1, 1))


class TestPyReplAutoindent(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        config = ReadlineConfig(readline_completer=None)
        reader = ReadlineAlikeReader(console=console, config=config)
        return reader

    def test_auto_indent_default(self):
        # fmt: off
        input_code = (
            "def f():\n"
                "pass\n\n"
        )

        output_code = (
            "def f():\n"
            "    pass\n"
            "    "
        )
        # fmt: on

    def test_auto_indent_continuation(self):
        # auto indenting according to previous user indentation
        # fmt: off
        events = itertools.chain(
            code_to_events("def f():\n"),
            # add backspace to delete default auto-indent
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
            code_to_events(
                "  pass\n"
                  "pass\n\n"
            ),
        )

        output_code = (
            "def f():\n"
            "  pass\n"
            "  pass\n"
            "  "
        )
        # fmt: on

        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)

    def test_auto_indent_prev_block(self):
        # auto indenting according to indentation in different block
        # fmt: off
        events = itertools.chain(
            code_to_events("def f():\n"),
            # add backspace to delete default auto-indent
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
            code_to_events(
                "  pass\n"
                "pass\n\n"
            ),
            code_to_events(
                "def g():\n"
                  "pass\n\n"
            ),
        )

        output_code = (
            "def g():\n"
            "  pass\n"
            "  "
        )
        # fmt: on

        reader = self.prepare_reader(events)
        output1 = multiline_input(reader)
        output2 = multiline_input(reader)
        self.assertEqual(output2, output_code)

    def test_auto_indent_multiline(self):
        # fmt: off
        events = itertools.chain(
            code_to_events(
                "def f():\n"
                    "pass"
            ),
            [
                # go to the end of the first line
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="\x05", raw=bytearray(b"\x1bO5")),
                # new line should be autoindented
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
            code_to_events(
                "pass"
            ),
            [
                # go to end of last line
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="key", data="\x05", raw=bytearray(b"\x1bO5")),
                # double newline to terminate the block
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
        )

        output_code = (
            "def f():\n"
            "    pass\n"
            "    pass\n"
            "    "
        )
        # fmt: on

        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)

    def test_auto_indent_with_comment(self):
        # fmt: off
        events = code_to_events(
            "def f():  # foo\n"
                "pass\n\n"
        )

        output_code = (
            "def f():  # foo\n"
            "    pass\n"
            "    "
        )
        # fmt: on

        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)

    def test_auto_indent_ignore_comments(self):
        # fmt: off
        events = code_to_events(
            "pass  #:\n"
        )

        output_code = (
            "pass  #:"
        )
        # fmt: on

        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)


class TestPyReplOutput(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        config = ReadlineConfig(readline_completer=None)
        reader = ReadlineAlikeReader(console=console, config=config)
        return reader

    def test_basic(self):
        reader = self.prepare_reader(code_to_events("1+1\n"))

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_multiline_edit(self):
        events = itertools.chain(
            code_to_events("def f():\n...\n\n"),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
                Event(evt="key", data="backspace", raw=bytearray(b"\x08")),
                Event(evt="key", data="g", raw=bytearray(b"g")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="key", data="backspace", raw=bytearray(b"\x08")),
                Event(evt="key", data="delete", raw=bytearray(b"\x7F")),
                Event(evt="key", data="right", raw=bytearray(b"g")),
                Event(evt="key", data="backspace", raw=bytearray(b"\x08")),
                Event(evt="key", data="p", raw=bytearray(b"p")),
                Event(evt="key", data="a", raw=bytearray(b"a")),
                Event(evt="key", data="s", raw=bytearray(b"s")),
                Event(evt="key", data="s", raw=bytearray(b"s")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
        )
        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "def f():\n    ...\n    ")
        output = multiline_input(reader)
        self.assertEqual(output, "def g():\n    pass\n    ")

    def test_history_navigation_with_up_arrow(self):
        events = itertools.chain(
            code_to_events("1+1\n2+2\n"),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
        )

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
        events = itertools.chain(
            code_to_events("1+1\n2+2\n"),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
        )

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_history_search(self):
        events = itertools.chain(
            code_to_events("1+1\n2+2\n3+3\n"),
            [
                Event(evt="key", data="\x12", raw=bytearray(b"\x12")),
                Event(evt="key", data="1", raw=bytearray(b"1")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
        )

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")
        output = multiline_input(reader)
        self.assertEqual(output, "2+2")
        output = multiline_input(reader)
        self.assertEqual(output, "3+3")
        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_control_character(self):
        events = code_to_events("c\x1d\n")
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, "c\x1d")


class TestPyReplCompleter(TestCase):
    def prepare_reader(self, events, namespace):
        console = FakeConsole(events)
        config = ReadlineConfig()
        config.readline_completer = rlcompleter.Completer(namespace).complete
        reader = ReadlineAlikeReader(console=console, config=config)
        return reader

    @patch("rlcompleter._readline_available", False)
    def test_simple_completion(self):
        events = code_to_events("os.getpid\t\n")

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.getpid()")

    def test_completion_with_many_options(self):
        # Test with something that initially displays many options
        # and then complete from one of them. The first time tab is
        # pressed, the options are displayed (which corresponds to
        # when the repl shows [ not unique ]) and the second completes
        # from one of them.
        events = code_to_events("os.\t\tO_AP\t\n")

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.O_APPEND")

    def test_empty_namespace_completion(self):
        events = code_to_events("os.geten\t\n")
        namespace = {}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.geten")

    def test_global_namespace_completion(self):
        events = code_to_events("py\t\n")
        namespace = {"python": None}
        reader = self.prepare_reader(events, namespace)
        output = multiline_input(reader, namespace)
        self.assertEqual(output, "python")

    def test_updown_arrow_with_completion_menu(self):
        """Up arrow in the middle of unfinished tab completion when the menu is displayed
        should work and trigger going back in history. Down arrow should subsequently
        get us back to the incomplete command."""
        code = "import os\nos.\t\t"
        namespace = {"os": os}

        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            ],
            code_to_events("\n"),
        )
        reader = self.prepare_reader(events, namespace=namespace)
        output = multiline_input(reader, namespace)
        # This is the first line, nothing to see here
        self.assertEqual(output, "import os")
        # This is the second line. We pressed up and down arrows
        # so we should end up where we were when we initiated tab completion.
        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.")

    @patch("_pyrepl.readline._ReadlineWrapper.get_reader")
    @patch("sys.stderr", new_callable=io.StringIO)
    def test_completion_with_warnings(self, mock_stderr, mock_get_reader):
        class Dummy:
            @property
            def test_func(self):
                import warnings

                warnings.warn("warnings\n")
                return None

        dummy = Dummy()
        events = code_to_events("dummy.test_func.\t\n\n")
        namespace = {"dummy": dummy}
        reader = self.prepare_reader(events, namespace)
        mock_get_reader.return_value = reader
        output = readline_multiline_input(more_lines, ">>>", "...")
        self.assertEqual(output, "dummy.test_func.__")
        self.assertEqual(mock_stderr.getvalue(), "")


class TestPasteEvent(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        config = ReadlineConfig(readline_completer=None)
        reader = ReadlineAlikeReader(console=console, config=config)
        return reader

    def test_paste(self):
        # fmt: off
        code = (
            "def a():\n"
            "  for x in range(10):\n"
            "    if x%2:\n"
            "      print(x)\n"
            "    else:\n"
            "      pass\n"
        )
        # fmt: on

        events = itertools.chain(
            [
                Event(evt="key", data="f3", raw=bytearray(b"\x1bOR")),
            ],
            code_to_events(code),
            [
                Event(evt="key", data="f3", raw=bytearray(b"\x1bOR")),
            ],
            code_to_events("\n"),
        )
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, code)

    def test_paste_mid_newlines(self):
        # fmt: off
        code = (
            "def f():\n"
            "  x = y\n"
            "  \n"
            "  y = z\n"
        )
        # fmt: on

        events = itertools.chain(
            [
                Event(evt="key", data="f3", raw=bytearray(b"\x1bOR")),
            ],
            code_to_events(code),
            [
                Event(evt="key", data="f3", raw=bytearray(b"\x1bOR")),
            ],
            code_to_events("\n"),
        )
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, code)

    def test_paste_mid_newlines_not_in_paste_mode(self):
        # fmt: off
        code = (
            "def f():\n"
                "x = y\n"
                "\n"
                "y = z\n\n"
        )

        expected = (
            "def f():\n"
            "    x = y\n"
            "    "
        )
        # fmt: on

        events = code_to_events(code)
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, expected)

    def test_paste_not_in_paste_mode(self):
        # fmt: off
        input_code = (
            "def a():\n"
                "for x in range(10):\n"
                    "if x%2:\n"
                        "print(x)\n"
                    "else:\n"
                        "pass\n\n"
        )

        output_code = (
            "def a():\n"
            "    for x in range(10):\n"
            "        if x%2:\n"
            "            print(x)\n"
            "            else:"
        )
        # fmt: on

        events = code_to_events(input_code)
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)

    def test_bracketed_paste(self):
        """Test that bracketed paste using \x1b[200~ and \x1b[201~ works."""
        # fmt: off
        input_code = (
            "def a():\n"
            "  for x in range(10):\n"
            "\n"
            "    if x%2:\n"
            "      print(x)\n"
            "\n"
            "    else:\n"
            "      pass\n"
        )

        output_code = (
            "def a():\n"
            "  for x in range(10):\n"
            "\n"
            "    if x%2:\n"
            "      print(x)\n"
            "\n"
            "    else:\n"
            "      pass\n"
        )
        # fmt: on

        paste_start = "\x1b[200~"
        paste_end = "\x1b[201~"

        events = itertools.chain(
            code_to_events(paste_start),
            code_to_events(input_code),
            code_to_events(paste_end),
            code_to_events("\n"),
        )
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, output_code)

    def test_bracketed_paste_single_line(self):
        input_code = "oneline"

        paste_start = "\x1b[200~"
        paste_end = "\x1b[201~"

        events = itertools.chain(
            code_to_events(paste_start),
            code_to_events(input_code),
            code_to_events(paste_end),
            code_to_events("\n"),
        )
        reader = self.prepare_reader(events)
        output = multiline_input(reader)
        self.assertEqual(output, input_code)


@skipUnless(pty, "requires pty")
class TestMain(TestCase):
    @force_not_colorized
    def test_exposed_globals_in_repl(self):
        pre = "['__annotations__', '__builtins__'"
        post = "'__loader__', '__name__', '__package__', '__spec__']"
        output, exit_code = self.run_repl(["sorted(dir())", "exit"])
        if "can't use pyrepl" in output:
            self.skipTest("pyrepl not available")
        self.assertEqual(exit_code, 0)

        # if `__main__` is not a file (impossible with pyrepl)
        case1 = f"{pre}, '__doc__', {post}" in output

        # if `__main__` is an uncached .py file (no .pyc)
        case2 = f"{pre}, '__doc__', '__file__', {post}" in output

        # if `__main__` is a cached .pyc file and the .py source exists
        case3 = f"{pre}, '__cached__', '__doc__', '__file__', {post}" in output

        # if `__main__` is a cached .pyc file but there's no .py source file
        case4 = f"{pre}, '__cached__', '__doc__', {post}" in output

        self.assertTrue(case1 or case2 or case3 or case4, output)

    def test_dumb_terminal_exits_cleanly(self):
        env = os.environ.copy()
        env.update({"TERM": "dumb"})
        output, exit_code = self.run_repl("exit()\n", env=env)
        self.assertEqual(exit_code, 0)
        self.assertIn("warning: can\'t use pyrepl", output)
        self.assertNotIn("Exception", output)
        self.assertNotIn("Traceback", output)

    @force_not_colorized
    def test_python_basic_repl(self):
        env = os.environ.copy()
        commands = ("from test.support import initialized_with_pyrepl\n"
                    "initialized_with_pyrepl()\n"
                    "exit()\n")

        env.pop("PYTHON_BASIC_REPL", None)
        output, exit_code = self.run_repl(commands, env=env)
        if "can\'t use pyrepl" in output:
            self.skipTest("pyrepl not available")
        self.assertEqual(exit_code, 0)
        self.assertIn("True", output)
        self.assertNotIn("False", output)
        self.assertNotIn("Exception", output)
        self.assertNotIn("Traceback", output)

        env["PYTHON_BASIC_REPL"] = "1"
        output, exit_code = self.run_repl(commands, env=env)
        self.assertEqual(exit_code, 0)
        self.assertIn("False", output)
        self.assertNotIn("True", output)
        self.assertNotIn("Exception", output)
        self.assertNotIn("Traceback", output)

    def run_repl(self, repl_input: str | list[str], env: dict | None = None) -> tuple[str, int]:
        master_fd, slave_fd = pty.openpty()
        process = subprocess.Popen(
            [sys.executable, "-i", "-u"],
            stdin=slave_fd,
            stdout=slave_fd,
            stderr=slave_fd,
            text=True,
            close_fds=True,
            env=env if env else os.environ,
       )
        if isinstance(repl_input, list):
            repl_input = "\n".join(repl_input) + "\n"
        os.write(master_fd, repl_input.encode("utf-8"))

        output = []
        while select.select([master_fd], [], [], 0.5)[0]:
            data = os.read(master_fd, 1024).decode("utf-8")
            if not data:
                break
            output.append(data)

        os.close(master_fd)
        os.close(slave_fd)
        try:
            exit_code = process.wait(timeout=SHORT_TIMEOUT)
        except subprocess.TimeoutExpired:
            process.kill()
            exit_code = process.wait()
        return "\n".join(output), exit_code
