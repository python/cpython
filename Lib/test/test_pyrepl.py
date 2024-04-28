from unittest import TestCase
from unittest.mock import MagicMock
from unittest.mock import call
from code import InteractiveConsole
from contextlib import suppress
from functools import partial
import rlcompleter
import os

from _pyrepl.readline import ReadlineAlikeReader
from _pyrepl.readline import ReadlineConfig
from _pyrepl.simple_interact import _strip_final_indent
from _pyrepl.console import Event
from _pyrepl.console import Console


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
        return reader.readline(returns_unicode=True)
    finally:
        reader.more_lines = saved
        reader.paste_mode = False


class FakeConsole(Console):
    def __init__(self, events, encoding="utf-8"):
        self.events = iter(events)
        self.encoding = encoding
        self.screen = []
        self.height = 100
        self.width = 80

    def get_event(self, block=True):
        return next(self.events)


class TestPyReplDriver(TestCase):
    def prepare_reader(self, events):
        console = MagicMock()
        console.get_event.side_effect = events
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        return reader, console

    def test_up_arrow(self):
        events = [
            Event(evt="key", data="d", raw=bytearray(b"d")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data="(", raw=bytearray(b"(")),
            Event(evt="key", data=")", raw=bytearray(b")")),
            Event(evt="key", data=":", raw=bytearray(b":")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        reader, console = self.prepare_reader(events)

        with suppress(StopIteration):
            _ = multiline_input(reader)

        console.move_cursor.assert_called_with(1, 3)

    def test_down_arrow(self):
        events = [
            Event(evt="key", data="d", raw=bytearray(b"d")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data="(", raw=bytearray(b"(")),
            Event(evt="key", data=")", raw=bytearray(b")")),
            Event(evt="key", data=":", raw=bytearray(b":")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        reader, console = self.prepare_reader(events)

        with suppress(StopIteration):
            _ = multiline_input(reader)

        console.move_cursor.assert_called_with(1, 5)

    def test_left_arrow(self):
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="left", raw=bytearray(b"\x1bOD")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        reader, console = self.prepare_reader(events)

        _ = multiline_input(reader)

        console.move_cursor.assert_has_calls(
            [
                call(3, 1),
            ]
        )

    def test_right_arrow(self):
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="right", raw=bytearray(b"\x1bOC")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        reader, console = self.prepare_reader(events)

        _ = multiline_input(reader)

        console.move_cursor.assert_has_calls(
            [
                call(4, 1),
            ]
        )


class TestPyReplOutput(TestCase):
    def prepare_reader(self, events):
        console = FakeConsole(events)
        reader = ReadlineAlikeReader(console)
        reader.config = ReadlineConfig()
        reader.config.readline_completer = None
        return reader

    def test_basic(self):
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]
        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_multiline_edit(self):
        events = [
            Event(evt="key", data="d", raw=bytearray(b"d")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data="f", raw=bytearray(b"f")),
            Event(evt="key", data="(", raw=bytearray(b"(")),
            Event(evt="key", data=")", raw=bytearray(b")")),
            Event(evt="key", data=":", raw=bytearray(b":")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=" ", raw=bytearray(b" ")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
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
        ]
        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "def f():\n  ...\n  ")
        output = multiline_input(reader)
        self.assertEqual(output, "def g():\n  ...\n  ")

    def test_history_navigation_with_up_arrow(self):
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="2", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="2", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

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
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="2", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="2", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
            Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
        ]

        reader = self.prepare_reader(events)

        output = multiline_input(reader)
        self.assertEqual(output, "1+1")

    def test_history_search(self):
        events = [
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="2", raw=bytearray(b"2")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="2", raw=bytearray(b"2")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="3", raw=bytearray(b"3")),
            Event(evt="key", data="+", raw=bytearray(b"+")),
            Event(evt="key", data="3", raw=bytearray(b"3")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="\x12", raw=bytearray(b"\x12")),
            Event(evt="key", data="1", raw=bytearray(b"1")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

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
        events = [
            Event(evt="key", data="o", raw=bytearray(b"o")),
            Event(evt="key", data="s", raw=bytearray(b"s")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="g", raw=bytearray(b"g")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="t", raw=bytearray(b"t")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="n", raw=bytearray(b"n")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.getenv")

    def test_completion_with_many_options(self):
        events = [
            Event(evt="key", data="o", raw=bytearray(b"o")),
            Event(evt="key", data="s", raw=bytearray(b"s")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="O", raw=bytearray(b"O")),
            Event(evt="key", data="_", raw=bytearray(b"_")),
            Event(evt="key", data="A", raw=bytearray(b"A")),
            Event(evt="key", data="S", raw=bytearray(b"S")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        namespace = {"os": os}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.O_ASYNC")

    def test_empty_namespace_completion(self):
        events = [
            Event(evt="key", data="o", raw=bytearray(b"o")),
            Event(evt="key", data="s", raw=bytearray(b"s")),
            Event(evt="key", data=".", raw=bytearray(b".")),
            Event(evt="key", data="g", raw=bytearray(b"g")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="t", raw=bytearray(b"t")),
            Event(evt="key", data="e", raw=bytearray(b"e")),
            Event(evt="key", data="n", raw=bytearray(b"n")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        namespace = {}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "os.geten")

    def test_global_namespace_completion(self):
        events = [
            Event(evt="key", data="p", raw=bytearray(b"p")),
            Event(evt="key", data="y", raw=bytearray(b"y")),
            Event(evt="key", data="\t", raw=bytearray(b"\t")),
            Event(evt="key", data="\n", raw=bytearray(b"\n")),
        ]

        namespace = {"python": None}
        reader = self.prepare_reader(events, namespace)

        output = multiline_input(reader, namespace)
        self.assertEqual(output, "python")


if __name__ == "__main__":
    unittest.main()


if __name__ == "__main__":
    unittest.main()
