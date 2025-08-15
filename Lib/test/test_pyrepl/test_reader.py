import itertools
import functools
import rlcompleter
from unittest import TestCase
from unittest.mock import MagicMock

from .support import handle_all_events, handle_events_narrow_console
from .support import ScreenEqualMixin, code_to_events
from .support import prepare_reader, prepare_console
from _pyrepl.console import Event
from _pyrepl.reader import Reader


class TestReader(ScreenEqualMixin, TestCase):
    def test_calc_screen_wrap_simple(self):
        events = code_to_events(10 * "a")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equal(reader, f"{9*"a"}\\\na")

    def test_calc_screen_wrap_wide_characters(self):
        events = code_to_events(8 * "a" + "樂")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equal(reader, f"{8*"a"}\\\n樂")

    def test_calc_screen_wrap_three_lines(self):
        events = code_to_events(20 * "a")
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equal(reader, f"{9*"a"}\\\n{9*"a"}\\\naa")

    def test_calc_screen_prompt_handling(self):
        def prepare_reader_keep_prompts(*args, **kwargs):
            reader = prepare_reader(*args, **kwargs)
            del reader.get_prompt
            reader.ps1 = ">>> "
            reader.ps2 = ">>> "
            reader.ps3 = "... "
            reader.ps4 = ""
            reader.can_colorize = False
            reader.paste_mode = False
            return reader

        events = code_to_events("if some_condition:\nsome_function()")
        reader, _ = handle_events_narrow_console(
            events,
            prepare_reader=prepare_reader_keep_prompts,
        )
        # fmt: off
        self.assert_screen_equal(
            reader,
            (
            ">>> if so\\\n"
            "me_condit\\\n"
            "ion:\n"
            "...     s\\\n"
            "ome_funct\\\n"
            "ion()"
            )
        )
        # fmt: on

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
        self.assert_screen_equal(
            reader,
            (
                "def f():\n"
               f"  {7*"a"}\\\n"
                "a\n"
               f"  {3*"樂"}\\\n"
                "樂樂"
            ),
            clean=True,
        )
        # fmt: on

    def test_calc_screen_backspace(self):
        events = itertools.chain(
            code_to_events("aaa"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, "aa")

    def test_calc_screen_wrap_removes_after_backspace(self):
        events = itertools.chain(
            code_to_events(10 * "a"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equal(reader, 9 * "a")

    def test_calc_screen_backspace_in_second_line_after_wrap(self):
        events = itertools.chain(
            code_to_events(11 * "a"),
            [
                Event(evt="key", data="backspace", raw=bytearray(b"\x7f")),
            ],
        )
        reader, _ = handle_events_narrow_console(events)
        self.assert_screen_equal(reader, f"{9*"a"}\\\na")

    def test_setpos_for_xy_simple(self):
        events = code_to_events("11+11")
        reader, _ = handle_all_events(events)
        reader.setpos_from_xy(0, 0)
        self.assertEqual(reader.pos, 0)

    def test_control_characters(self):
        code = 'flag = "🏳️‍🌈"'
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, 'flag = "🏳️\\u200d🌈"', clean=True)

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
        self.assert_screen_equal(reader, "")

    def test_newline_within_block_trailing_whitespace(self):
        # fmt: off
        code = (
            "def foo():\n"
                 "a = 1\n"
        )
        # fmt: on

        events = itertools.chain(
            code_to_events(code),
            [
                # go to the end of the first line
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="up", raw=bytearray(b"\x1bOA")),
                Event(evt="key", data="\x05", raw=bytearray(b"\x1bO5")),
                # new lines in-block shouldn't terminate the block
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                # end of line 2
                Event(evt="key", data="down", raw=bytearray(b"\x1bOB")),
                Event(evt="key", data="\x05", raw=bytearray(b"\x1bO5")),
                # a double new line in-block should terminate the block
                # even if its followed by whitespace
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
                Event(evt="key", data="\n", raw=bytearray(b"\n")),
            ],
        )

        no_paste_reader = functools.partial(prepare_reader, paste_mode=False)
        reader, _ = handle_all_events(events, prepare_reader=no_paste_reader)

        expected = (
            "def foo():\n"
            "    \n"
            "    \n"
            "    a = 1\n"
            "    \n"
            "    "  # HistoricalReader will trim trailing whitespace
        )
        self.assert_screen_equal(reader, expected, clean=True)
        self.assertTrue(reader.finished)

    def test_input_hook_is_called_if_set(self):
        input_hook = MagicMock()

        def _prepare_console(events):
            console = MagicMock()
            console.get_event.side_effect = events
            console.height = 100
            console.width = 80
            console.input_hook = input_hook
            return console

        events = code_to_events("a")
        reader, _ = handle_all_events(events, prepare_console=_prepare_console)

        self.assertEqual(len(input_hook.mock_calls), 4)

    def test_keyboard_interrupt_clears_screen(self):
        namespace = {"itertools": itertools}
        code = "import itertools\nitertools."
        events = itertools.chain(
            code_to_events(code),
            [
                # Two tabs for completion
                Event(evt="key", data="\t", raw=bytearray(b"\t")),
                Event(evt="key", data="\t", raw=bytearray(b"\t")),
                Event(evt="key", data="\x03", raw=bytearray(b"\x03")),  # Ctrl-C
            ],
        )
        console = prepare_console(events)
        reader = prepare_reader(
            console,
            readline_completer=rlcompleter.Completer(namespace).complete,
        )
        try:
            # we're not using handle_all_events() here to be able to
            # follow the KeyboardInterrupt sequence of events. Normally this
            # happens in simple_interact.run_multiline_interactive_console.
            while True:
                reader.handle1()
        except KeyboardInterrupt:
            # at this point the completions are still visible
            self.assertTrue(len(reader.screen) > 2)
            reader.refresh()
            # after the refresh, they are gone
            self.assertEqual(len(reader.screen), 2)
            self.assert_screen_equal(reader, code, clean=True)
        else:
            self.fail("KeyboardInterrupt not raised.")

    def test_prompt_length(self):
        # Handles simple ASCII prompt
        ps1 = ">>> "
        prompt, l = Reader.process_prompt(ps1)
        self.assertEqual(prompt, ps1)
        self.assertEqual(l, 4)

        # Handles ANSI escape sequences
        ps1 = "\033[0;32m>>> \033[0m"
        prompt, l = Reader.process_prompt(ps1)
        self.assertEqual(prompt, "\033[0;32m>>> \033[0m")
        self.assertEqual(l, 4)

        # Handles ANSI escape sequences bracketed in \001 .. \002
        ps1 = "\001\033[0;32m\002>>> \001\033[0m\002"
        prompt, l = Reader.process_prompt(ps1)
        self.assertEqual(prompt, "\033[0;32m>>> \033[0m")
        self.assertEqual(l, 4)

        # Handles wide characters in prompt
        ps1 = "樂>> "
        prompt, l = Reader.process_prompt(ps1)
        self.assertEqual(prompt, ps1)
        self.assertEqual(l, 5)

        # Handles wide characters AND ANSI sequences together
        ps1 = "\001\033[0;32m\002樂>\001\033[0m\002> "
        prompt, l = Reader.process_prompt(ps1)
        self.assertEqual(prompt, "\033[0;32m樂>\033[0m> ")
        self.assertEqual(l, 5)

    def test_completions_updated_on_key_press(self):
        namespace = {"itertools": itertools}
        code = "itertools."
        events = itertools.chain(
            code_to_events(code),
            [
                # Two tabs for completion
                Event(evt="key", data="\t", raw=bytearray(b"\t")),
                Event(evt="key", data="\t", raw=bytearray(b"\t")),
            ],
            code_to_events("a"),
        )

        completing_reader = functools.partial(
            prepare_reader,
            readline_completer=rlcompleter.Completer(namespace).complete,
        )
        reader, _ = handle_all_events(events, prepare_reader=completing_reader)

        actual = reader.screen
        self.assertEqual(len(actual), 2)
        self.assertEqual(actual[0], f"{code}a")
        self.assertEqual(actual[1].rstrip(), "itertools.accumulate(")

    def test_key_press_on_tab_press_once(self):
        namespace = {"itertools": itertools}
        code = "itertools."
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="\t", raw=bytearray(b"\t")),
            ],
            code_to_events("a"),
        )

        completing_reader = functools.partial(
            prepare_reader,
            readline_completer=rlcompleter.Completer(namespace).complete,
        )
        reader, _ = handle_all_events(events, prepare_reader=completing_reader)

        self.assert_screen_equal(reader, f"{code}a")

    def test_pos2xy_with_no_columns(self):
        console = prepare_console([])
        reader = prepare_reader(console)
        # Simulate a resize to 0 columns
        reader.screeninfo = []
        self.assertEqual(reader.pos2xy(), (0, 0))

    def test_setpos_from_xy_for_non_printing_char(self):
        code = "# non \u200c printing character"
        events = code_to_events(code)

        reader, _ = handle_all_events(events)
        reader.setpos_from_xy(8, 0)
        self.assertEqual(reader.pos, 7)
