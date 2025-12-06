import itertools
import functools
import rlcompleter
from textwrap import dedent
from unittest import TestCase
from unittest.mock import MagicMock
from test.support import force_colorized_test_class, force_not_colorized_test_class

from .support import handle_all_events, handle_events_narrow_console
from .support import ScreenEqualMixin, code_to_events
from .support import prepare_reader, prepare_console, prepare_vi_reader
from _pyrepl.console import Event
from _pyrepl.reader import Reader
from _pyrepl.types import VI_MODE_INSERT, VI_MODE_NORMAL
from _colorize import default_theme

overrides = {"reset": "z", "soft_keyword": "K"}
colors = {overrides.get(k, k[0].lower()): v for k, v in default_theme.syntax.items()}


@force_not_colorized_test_class
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
                # even if its followed by whitespace
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

@force_colorized_test_class
class TestReaderInColor(ScreenEqualMixin, TestCase):
    def test_syntax_highlighting_basic(self):
        code = dedent(
            """\
            import re, sys
            def funct(case: str = sys.platform) -> None:
                match = re.search(
                    "(me)",
                    '''
                    Come on
                      Come on now
                        You know that it's time to emerge
                    ''',
                )
                match case:
                    case "emscripten": print("on the web")
                    case "ios" | "android":
                        print("on the phone")
                    case _: print('arms around', match.group(1))
            type type = type[type]
            """
        )
        expected = dedent(
            """\
            {k}import{z} re{o},{z} sys
            {a}{k}def{z} {d}funct{z}{o}({z}case{o}:{z} {b}str{z} {o}={z} sys{o}.{z}platform{o}){z} {o}->{z} {k}None{z}{o}:{z}
                match {o}={z} re{o}.{z}search{o}({z}
                    {s}"(me)"{z}{o},{z}
                    {s}'''{z}
            {s}        Come on{z}
            {s}          Come on now{z}
            {s}            You know that it's time to emerge{z}
            {s}        '''{z}{o},{z}
                {o}){z}
                {K}match{z} case{o}:{z}
                    {K}case{z} {s}"emscripten"{z}{o}:{z} {b}print{z}{o}({z}{s}"on the web"{z}{o}){z}
                    {K}case{z} {s}"ios"{z} {o}|{z} {s}"android"{z}{o}:{z}
                        {b}print{z}{o}({z}{s}"on the phone"{z}{o}){z}
                    {K}case{z} {K}_{z}{o}:{z} {b}print{z}{o}({z}{s}'arms around'{z}{o},{z} match{o}.{z}group{o}({z}{n}1{z}{o}){z}{o}){z}
            {K}type{z} {b}type{z} {o}={z} {b}type{z}{o}[{z}{b}type{z}{o}]{z}
            """
        )
        expected_sync = expected.format(a="", **colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected_sync)
        self.assertEqual(reader.pos, 419)
        self.assertEqual(reader.cxy, (0, 16))

        async_msg = "{k}async{z} ".format(**colors)
        expected_async = expected.format(a=async_msg, **colors)
        more_events = itertools.chain(
            code_to_events(code),
            [Event(evt="key", data="up", raw=bytearray(b"\x1bOA"))] * 15,
            code_to_events("async "),
        )
        reader, _ = handle_all_events(more_events)
        self.assert_screen_equal(reader, expected_async)
        self.assertEqual(reader.pos, 21)
        self.assertEqual(reader.cxy, (6, 1))

    def test_syntax_highlighting_incomplete_string_first_line(self):
        code = dedent(
            """\
            def unfinished_function(arg: str = "still typing
            """
        )
        expected = dedent(
            """\
            {k}def{z} {d}unfinished_function{z}{o}({z}arg{o}:{z} {b}str{z} {o}={z} {s}"still typing{z}
            """
        ).format(**colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected)

    def test_syntax_highlighting_incomplete_string_another_line(self):
        code = dedent(
            """\
            def unfinished_function(
                arg: str = "still typing
            """
        )
        expected = dedent(
            """\
            {k}def{z} {d}unfinished_function{z}{o}({z}
                arg{o}:{z} {b}str{z} {o}={z} {s}"still typing{z}
            """
        ).format(**colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected)

    def test_syntax_highlighting_incomplete_multiline_string(self):
        code = dedent(
            """\
            def unfinished_function():
                '''Still writing
                the docstring
            """
        )
        expected = dedent(
            """\
            {k}def{z} {d}unfinished_function{z}{o}({z}{o}){z}{o}:{z}
                {s}'''Still writing{z}
            {s}    the docstring{z}
            """
        ).format(**colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected)

    def test_syntax_highlighting_incomplete_fstring(self):
        code = dedent(
            """\
            def unfinished_function():
                var = f"Single-quote but {
                1
                +
                1
                } multi-line!
            """
        )
        expected = dedent(
            """\
            {k}def{z} {d}unfinished_function{z}{o}({z}{o}){z}{o}:{z}
                var {o}={z} {s}f"{z}{s}Single-quote but {z}{o}{OB}{z}
                {n}1{z}
                {o}+{z}
                {n}1{z}
                {o}{CB}{z}{s} multi-line!{z}
            """
        ).format(OB="{", CB="}", **colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected)

    def test_syntax_highlighting_indentation_error(self):
        code = dedent(
            """\
            def unfinished_function():
                var = 1
               oops
            """
        )
        expected = dedent(
            """\
            {k}def{z} {d}unfinished_function{z}{o}({z}{o}){z}{o}:{z}
                var {o}={z} {n}1{z}
               oops
            """
        ).format(**colors)
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.assert_screen_equal(reader, expected)

    def test_syntax_highlighting_literal_brace_in_fstring_or_tstring(self):
        code = dedent(
            """\
            f"{{"
            f"}}"
            f"a{{b"
            f"a}}b"
            f"a{{b}}c"
            t"a{{b}}c"
            f"{{{0}}}"
            f"{ {0} }"
            """
        )
        expected = dedent(
            """\
            {s}f"{z}{s}<<{z}{s}"{z}
            {s}f"{z}{s}>>{z}{s}"{z}
            {s}f"{z}{s}a<<{z}{s}b{z}{s}"{z}
            {s}f"{z}{s}a>>{z}{s}b{z}{s}"{z}
            {s}f"{z}{s}a<<{z}{s}b>>{z}{s}c{z}{s}"{z}
            {s}t"{z}{s}a<<{z}{s}b>>{z}{s}c{z}{s}"{z}
            {s}f"{z}{s}<<{z}{o}<{z}{n}0{z}{o}>{z}{s}>>{z}{s}"{z}
            {s}f"{z}{o}<{z} {o}<{z}{n}0{z}{o}>{z} {o}>{z}{s}"{z}
            """
        ).format(**colors).replace("<", "{").replace(">", "}")
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, code, clean=True)
        self.maxDiff=None
        self.assert_screen_equal(reader, expected)

    def test_vi_escape_switches_to_normal_mode(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="h", raw=bytearray(b"h")),
            ],
        )
        reader, _ = handle_all_events(
            events,
            prepare_reader=prepare_vi_reader,
        )
        self.assertEqual(reader.get_unicode(), "hello")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)
        self.assertEqual(reader.pos, len("hello") - 2)  # After 'h' left movement

    def test_control_characters(self):
        code = 'flag = "🏳️‍🌈"'
        events = code_to_events(code)
        reader, _ = handle_all_events(events)
        self.assert_screen_equal(reader, 'flag = "🏳️\\u200d🌈"', clean=True)
        self.assert_screen_equal(reader, 'flag {o}={z} {s}"🏳️\\u200d🌈"{z}'.format(**colors))


@force_not_colorized_test_class
class TestViMode(TestCase):
    def _run_vi(self, events, prepare_reader_hook=prepare_vi_reader):
        return handle_all_events(events, prepare_reader=prepare_reader_hook)

    def test_insert_typing_and_ctrl_a_e(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x01", raw=bytearray(b"\x01")),  # Ctrl-A
                Event(evt="key", data="X", raw=bytearray(b"X")),
                Event(evt="key", data="\x05", raw=bytearray(b"\x05")),  # Ctrl-E
                Event(evt="key", data="!", raw=bytearray(b"!")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "Xhello!")
        self.assertTrue(reader.vi_mode == VI_MODE_INSERT)

    def test_escape_switches_to_normal_mode_and_is_idempotent(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="h", raw=bytearray(b"h")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)
        self.assertEqual(reader.pos, len("hello") - 2)  # After 'h' left movement

    def test_normal_mode_motion_and_edit_commands(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # go to bol
                Event(evt="key", data="l", raw=bytearray(b"l")),        # right
                Event(evt="key", data="x", raw=bytearray(b"x")),        # delete
                Event(evt="key", data="a", raw=bytearray(b"a")),        # append
                Event(evt="key", data="!", raw=bytearray(b"!")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hl!lo")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)

    def test_open_below_and_above(self):
        events = itertools.chain(
            code_to_events("first"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="o", raw=bytearray(b"o")),
            ],
            code_to_events("second"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="O", raw=bytearray(b"O")),
            ],
            code_to_events("zero"),
            [Event(evt="key", data="\x1b", raw=bytearray(b"\x1b"))],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "first\nzero\nsecond")

    def test_mode_resets_to_insert_on_prepare(self):
        events = itertools.chain(
            code_to_events("text"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
            ],
        )
        reader, console = self._run_vi(events)
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)
        reader.prepare()
        self.assertTrue(reader.vi_mode == VI_MODE_INSERT)
        console.prepare.assert_called()  # ensure console prepare called again

    def test_translator_stack_preserves_mode(self):
        events_insert_path = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x12", raw=bytearray(b"\x12")),  # Ctrl-R
                Event(evt="key", data="h", raw=bytearray(b"h")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
            ],
        )
        reader, _ = self._run_vi(events_insert_path)
        self.assertTrue(reader.vi_mode == VI_MODE_INSERT)

        events_normal_path = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="\x12", raw=bytearray(b"\x12")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
            ],
        )
        reader, _ = self._run_vi(events_normal_path)
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)

    def test_insert_bol_and_append_eol(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to normal
                Event(evt="key", data="I", raw=bytearray(b"I")),        # Insert at BOL
                Event(evt="key", data="[", raw=bytearray(b"[")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # Back to normal
                Event(evt="key", data="A", raw=bytearray(b"A")),        # Append at EOL
                Event(evt="key", data="]", raw=bytearray(b"]")),
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "[hello]")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)

    def test_insert_mode_from_normal(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to normal
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to beginning
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Move right
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Move right again
                Event(evt="key", data="i", raw=bytearray(b"i")),        # Insert mode
                Event(evt="key", data="X", raw=bytearray(b"X")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "heXllo")
        self.assertTrue(reader.vi_mode == VI_MODE_INSERT)

    def test_hjkl_motions(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to normal
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to start of line
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Right (h->e)
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Right (e->l)
                Event(evt="key", data="h", raw=bytearray(b"h")),        # Left (l->e)
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 'e'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hllo")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)

    def test_dollar_end_of_line(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning
                Event(evt="key", data="$", raw=bytearray(b"$")),        # End (on last char)
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 'o'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hell")

    def test_word_motions(self):
        events = itertools.chain(
            code_to_events("one two"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning
                Event(evt="key", data="w", raw=bytearray(b"w")),        # Forward word
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete first char of 'two'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertIn("one", reader.get_unicode())
        self.assertNotEqual(reader.get_unicode(), "one two")  # Something was deleted

    def test_repeat_counts(self):
        events = itertools.chain(
            code_to_events("abcdefghij"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning
                Event(evt="key", data="3", raw=bytearray(b"3")),        # Count 3
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Move right 3 times
                Event(evt="key", data="2", raw=bytearray(b"2")),        # Count 2
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 2 chars (d, e)
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "abcfghij")
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)

    def test_multiline_navigation(self):
        # Test j/k navigation across multiple lines
        code = "first\nsecond\nthird"
        events = itertools.chain(
            code_to_events(code),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # Up to "second"
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning of line
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 's'
                Event(evt="key", data="j", raw=bytearray(b"j")),        # Down to "third"
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 't'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "first\necond\nhird")

    def test_arrow_keys_in_normal_mode(self):
        events = itertools.chain(
            code_to_events("test"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="left", raw=bytearray(b"\x1b[D")),  # Left arrow
                Event(evt="key", data="left", raw=bytearray(b"\x1b[D")),  # Left arrow
                Event(evt="key", data="x", raw=bytearray(b"x")),        # Delete 'e'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "tst")

    def test_escape_in_normal_mode_is_noop(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to normal
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC again (no-op)
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC again (no-op)
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)
        self.assertEqual(reader.get_unicode(), "hello")

    def test_backspace_in_normal_mode(self):
        events = itertools.chain(
            code_to_events("abcd"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="\x7f", raw=bytearray(b"\x7f")),  # Backspace
                Event(evt="key", data="\x7f", raw=bytearray(b"\x7f")),  # Backspace again
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertTrue(reader.vi_mode == VI_MODE_NORMAL)
        self.assertIsNotNone(reader.get_unicode())

    def test_end_of_word_motion(self):
        events = itertools.chain(
            code_to_events("hello world test"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Beginning
                Event(evt="key", data="e", raw=bytearray(b"e")),        # End of "hello"
            ],
        )
        reader, _ = self._run_vi(events)
        # Should be on 'o' of "hello" (last char of word)
        self.assertEqual(reader.pos, 4)
        self.assertEqual(reader.buffer[reader.pos], 'o')

        # Test multiple 'e' commands
        events2 = itertools.chain(
            code_to_events("one two three"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),
                Event(evt="key", data="e", raw=bytearray(b"e")),  # End of "one"
                Event(evt="key", data="e", raw=bytearray(b"e")),  # End of "two"
            ],
        )
        reader2, _ = self._run_vi(events2)
        # Should be on 'o' of "two"
        self.assertEqual(reader2.buffer[reader2.pos], 'o')

    def test_backward_word_motion(self):
        # Test from end of buffer
        events = itertools.chain(
            code_to_events("one two"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC at end
                Event(evt="key", data="b", raw=bytearray(b"b")),        # Back to start of "two"
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 4)  # At 't' of "two"
        self.assertEqual(reader.buffer[reader.pos], 't')

        # Test multiple backwards
        events2 = itertools.chain(
            code_to_events("one two three"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="b", raw=bytearray(b"b")),        # Back to "three"
                Event(evt="key", data="b", raw=bytearray(b"b")),        # Back to "two"
                Event(evt="key", data="b", raw=bytearray(b"b")),        # Back to "one"
            ],
        )
        reader2, _ = self._run_vi(events2)
        # Should be at beginning of "one"
        self.assertEqual(reader2.pos, 0)
        self.assertEqual(reader2.buffer[reader2.pos], 'o')

    def test_first_non_whitespace_character(self):
        events = itertools.chain(
            code_to_events("   hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="^", raw=bytearray(b"^")),        # First non-ws
            ],
        )
        reader, _ = self._run_vi(events)
        # Should be at 'h' of "hello", skipping the 3 spaces
        self.assertEqual(reader.pos, 3)
        self.assertEqual(reader.buffer[reader.pos], 'h')

        # Test with tabs and spaces
        events2 = itertools.chain(
            code_to_events("\t  text"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),       # Go to BOL first
                Event(evt="key", data="^", raw=bytearray(b"^")),       # Then to first non-ws
            ],
        )
        reader2, _ = self._run_vi(events2)
        self.assertEqual(reader2.buffer[reader2.pos], 't')

    def test_word_motion_edge_cases(self):
        # Test with underscore - in vi mode, underscore IS a word character
        events = itertools.chain(
            code_to_events("hello_world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),
                Event(evt="key", data="w", raw=bytearray(b"w")),  # Forward word
            ],
        )
        reader, _ = self._run_vi(events)
        # In vi mode, underscore is part of word, so 'w' goes past end of "hello_world"
        # which clamps to end of buffer (pos 10, on 'd')
        self.assertEqual(reader.pos, 10)

        # Test 'e' at end of buffer stays in bounds
        events2 = itertools.chain(
            code_to_events("end"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="e", raw=bytearray(b"e")),  # Already at end of word
                Event(evt="key", data="e", raw=bytearray(b"e")),  # Should stay in bounds
            ],
        )
        reader2, _ = self._run_vi(events2)
        # Should not go past end of buffer
        self.assertLessEqual(reader2.pos, len(reader2.buffer) - 1)

        # Test 'b' at beginning doesn't crash
        events3 = itertools.chain(
            code_to_events("start"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),
                Event(evt="key", data="b", raw=bytearray(b"b")),  # Should stay at 0
            ],
        )
        reader3, _ = self._run_vi(events3)
        self.assertEqual(reader3.pos, 0)

    def test_repeat_count_with_word_motions(self):
        events = itertools.chain(
            code_to_events("one two three four"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),
                Event(evt="key", data="2", raw=bytearray(b"2")),  # Count 2
                Event(evt="key", data="w", raw=bytearray(b"w")),  # Forward 2 words
            ],
        )
        reader, _ = self._run_vi(events)
        # Should be at start of "three" (2 words forward from "one")
        self.assertEqual(reader.buffer[reader.pos], 't')  # 't' of "three"

        # Test with 'e'
        events2 = itertools.chain(
            code_to_events("alpha beta gamma"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),
                Event(evt="key", data="0", raw=bytearray(b"0")),
                Event(evt="key", data="2", raw=bytearray(b"2")),
                Event(evt="key", data="e", raw=bytearray(b"e")),  # End of 2nd word
            ],
        )
        reader2, _ = self._run_vi(events2)
        # Should be at end of "beta"
        self.assertEqual(reader2.buffer[reader2.pos], 'a')  # Last 'a' of "beta"

    def test_vi_word_boundaries(self):
        """Test vi word motions match vim behavior.

        Vi has three character classes:
        1. Word chars: alphanumeric + underscore
        2. Punctuation: non-word, non-whitespace (forms separate words)
        3. Whitespace: delimiters
        """
        # Test cases: (text, start_key_sequence, expected_pos, description)
        test_cases = [
            # Underscore is part of word in vi, unlike emacs mode
            ("function_name", "0w", 12, "underscore is word char, w clamps to end"),
            ("hello_world test", "0w", 12, "underscore word to end"),

            # Punctuation is a separate word
            ("foo.bar", "0w", 3, "w stops at dot (punctuation)"),
            ("foo.bar", "0ww", 4, "second w goes to bar"),
            ("foo..bar", "0w", 3, "w stops at first dot"),
            ("foo..bar", "0ww", 5, "second w skips dot sequence to bar"),
            ("get_value(x)", "0w", 9, "underscore word stops at ("),
            ("get_value(x)", "0ww", 10, "second w goes to x"),

            # Basic word motion
            ("hello world", "0w", 6, "basic word jump"),
            ("one  two", "0w", 5, "double space handled"),
            ("abc def ghi", "0ww", 8, "two w's"),

            # End of word (e) - lands ON last char
            ("function_name", "0e", 12, "e lands on last char of underscore word"),
            ("foo bar", "0e", 2, "e lands on last char of foo"),
            ("foo.bar", "0e", 2, "e lands on last o of foo"),
            ("foo.bar", "0ee", 3, "second e lands on dot"),
            ("foo.bar", "0eee", 6, "third e lands on last r of bar"),

            # Backward word (b) - cursor at end after ESC
            ("foo bar", "$b", 4, "b from end lands on bar"),
            ("foo bar", "$bb", 0, "two b's lands on foo"),
            ("foo.bar", "$b", 4, "b from end lands on bar"),
            ("foo.bar", "$bb", 3, "second b lands on dot"),
            ("foo.bar", "$bbb", 0, "third b lands on foo"),
            ("get_value(x)", "$b", 10, "b from end lands on x"),
            ("get_value(x)", "$bb", 9, "second b lands on ("),
            ("get_value(x)", "$bbb", 0, "third b lands on get_value"),

            # W (WORD motion by whitespace-delimited words)
            ("foo.bar baz", "0W", 8, "W skips punctuation to baz"),
            ("one,two three", "0W", 8, "W skips comma to three"),
            ("hello   world", "0W", 8, "W handles multiple spaces"),
            ("get_value(x)", "0W", 11, "W clamps to end (no whitespace)"),

            # Backward W (B)
            ("foo.bar baz", "$B", 8, "B from end lands on baz"),
            ("foo.bar baz", "$BB", 0, "second B lands on foo.bar"),
            ("one,two three", "$B", 8, "B from end lands on three"),
            ("one,two three", "$BB", 0, "second B lands on one,two"),
            ("hello   world", "$B", 8, "B from end lands on world"),
            ("hello   world", "$BB", 0, "second B lands on hello"),

            # Edge cases
            ("   spaces", "0w", 3, "w from BOL skips leading spaces"),
            ("trailing   ", "0w", 10, "w clamps at end after trailing spaces"),
            ("a", "0w", 0, "w on single char stays in bounds"),
            ("", "0w", 0, "w on empty buffer stays at 0"),
            ("a b c", "0www", 4, "multiple w's work correctly"),
        ]

        for text, keys, expected_pos, desc in test_cases:
            with self.subTest(text=text, keys=keys, desc=desc):
                key_events = []
                for k in keys:
                    key_events.append(Event(evt="key", data=k, raw=bytearray(k.encode())))
                events = itertools.chain(
                    code_to_events(text),
                    [Event(evt="key", data="\x1b", raw=bytearray(b"\x1b"))],  # ESC
                    key_events,
                )
                reader, _ = self._run_vi(events)
                self.assertEqual(reader.pos, expected_pos,
                    f"Expected pos {expected_pos} but got {reader.pos} for '{text}' with keys '{keys}'")

    def test_find_char_forward(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to BOL
                Event(evt="key", data="f", raw=bytearray(b"f")),        # Find
                Event(evt="key", data="o", raw=bytearray(b"o")),        # Target char
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 4)
        self.assertEqual(reader.buffer[reader.pos], 'o')

    def test_find_char_backward(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="F", raw=bytearray(b"F")),        # Find back
                Event(evt="key", data="l", raw=bytearray(b"l")),        # Target char
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.buffer[reader.pos], 'l')

    def test_till_char_forward(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to BOL
                Event(evt="key", data="t", raw=bytearray(b"t")),        # Till
                Event(evt="key", data="o", raw=bytearray(b"o")),        # Target char
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 3)
        self.assertEqual(reader.buffer[reader.pos], 'l')

    def test_semicolon_repeat_find(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to BOL
                Event(evt="key", data="f", raw=bytearray(b"f")),        # Find
                Event(evt="key", data="o", raw=bytearray(b"o")),        # First 'o'
                Event(evt="key", data=";", raw=bytearray(b";")),        # Repeat - second 'o'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 7)
        self.assertEqual(reader.buffer[reader.pos], 'o')

    def test_comma_repeat_find_opposite(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to BOL
                Event(evt="key", data="f", raw=bytearray(b"f")),        # Find forward
                Event(evt="key", data="l", raw=bytearray(b"l")),        # First 'l' at pos 2
                Event(evt="key", data=";", raw=bytearray(b";")),        # Second 'l' at pos 3
                Event(evt="key", data=",", raw=bytearray(b",")),        # Reverse - back to pos 2
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 2)
        self.assertEqual(reader.buffer[reader.pos], 'l')

    def test_find_char_escape_cancels(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to normal
                Event(evt="key", data="0", raw=bytearray(b"0")),        # Go to BOL
                Event(evt="key", data="f", raw=bytearray(b"f")),        # Start find
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC to cancel
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.pos, 0)

    def test_change_word(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="c", raw=bytearray(b"c")),        # change
                Event(evt="key", data="w", raw=bytearray(b"w")),        # word
            ],
            code_to_events("hi"),  # replacement text
        )
        reader, _ = self._run_vi(events)
        self.assertEqual("".join(reader.buffer), "hi world")

    def test_replace_char(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="l", raw=bytearray(b"l")),        # move right to 'e'
                Event(evt="key", data="l", raw=bytearray(b"l")),        # move right to first 'l'
                Event(evt="key", data="r", raw=bytearray(b"r")),        # replace
                Event(evt="key", data="X", raw=bytearray(b"X")),        # replacement char
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual("".join(reader.buffer), "heXlo")
        self.assertEqual(reader.pos, 2)  # cursor stays on replaced char
        self.assertEqual(reader.vi_mode, VI_MODE_NORMAL)  # stays in normal mode

    def test_undo_after_insert(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "")
        self.assertEqual(reader.vi_mode, VI_MODE_NORMAL)

    def test_undo_after_delete_word(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="d", raw=bytearray(b"d")),        # delete
                Event(evt="key", data="w", raw=bytearray(b"w")),        # word
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello world")

    def test_undo_after_x_delete(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="x", raw=bytearray(b"x")),        # delete char
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello")

    def test_undo_stack_cleared_on_prepare(self):
        events = itertools.chain(
            code_to_events("hello"),
            [Event(evt="key", data="\x1b", raw=bytearray(b"\x1b"))],
        )
        reader, console = self._run_vi(events)
        self.assertGreater(len(reader.undo_stack), 0)
        reader.prepare()
        self.assertEqual(len(reader.undo_stack), 0)

    def test_multiple_undo(self):
        events = itertools.chain(
            code_to_events("a"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="a", raw=bytearray(b"a")),        # append
            ],
            code_to_events("b"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo 'b'
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo 'a'
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "")

    def test_undo_after_replace(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="r", raw=bytearray(b"r")),        # replace
                Event(evt="key", data="X", raw=bytearray(b"X")),        # replacement
                Event(evt="key", data="u", raw=bytearray(b"u")),        # undo
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello")

    def test_D_delete_to_eol(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="w", raw=bytearray(b"w")),        # forward word
                Event(evt="key", data="D", raw=bytearray(b"D")),        # delete to EOL
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello ")

    def test_C_change_to_eol(self):
        events = itertools.chain(
            code_to_events("hello world"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="w", raw=bytearray(b"w")),        # forward word
                Event(evt="key", data="C", raw=bytearray(b"C")),        # change to EOL
            ],
            code_to_events("there"),
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "hello there")
        self.assertEqual(reader.vi_mode, VI_MODE_INSERT)

    def test_s_substitute_char(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="s", raw=bytearray(b"s")),        # substitute
            ],
            code_to_events("j"),
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "jello")
        self.assertEqual(reader.vi_mode, VI_MODE_INSERT)

    def test_X_delete_char_before(self):
        events = itertools.chain(
            code_to_events("hello"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="X", raw=bytearray(b"X")),        # delete before
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "helo")
        self.assertEqual(reader.vi_mode, VI_MODE_NORMAL)

    def test_dd_deletes_current_line(self):
        events = itertools.chain(
            code_to_events("first"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("second"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("third"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # up to second
                Event(evt="key", data="d", raw=bytearray(b"d")),        # dd
                Event(evt="key", data="d", raw=bytearray(b"d")),
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "first\n\nthird")

    def test_j_k_navigation_between_lines(self):
        events = itertools.chain(
            code_to_events("short"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("longer line"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # up
                Event(evt="key", data="$", raw=bytearray(b"$")),        # end of line
                Event(evt="key", data="j", raw=bytearray(b"j")),        # down
            ],
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "short\nlonger line")
        # Cursor should be somewhere on second line
        self.assertGreater(reader.pos, 6)  # past the newline

    def test_dollar_stops_at_line_end(self):
        events = itertools.chain(
            code_to_events("first"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("second"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # up to first
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL
                Event(evt="key", data="$", raw=bytearray(b"$")),        # EOL
            ],
        )
        reader, _ = self._run_vi(events)
        # $ should stop at end of "first", not go to newline or beyond
        self.assertEqual(reader.pos, 4)  # 'first'[4] = 't'

    def test_zero_goes_to_line_start(self):
        events = itertools.chain(
            code_to_events("first"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("second"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="0", raw=bytearray(b"0")),        # BOL of second
            ],
        )
        reader, _ = self._run_vi(events)
        # 0 should go to start of "second" line (position 6)
        self.assertEqual(reader.pos, 6)

    def test_o_opens_line_below(self):
        events = itertools.chain(
            code_to_events("first"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("third"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # up to first
                Event(evt="key", data="o", raw=bytearray(b"o")),        # open below
            ],
            code_to_events("second"),
        )
        reader, _ = self._run_vi(events)
        self.assertEqual(reader.get_unicode(), "first\nsecond\nthird")
        self.assertEqual(reader.vi_mode, VI_MODE_INSERT)

    def test_w_motion_crosses_lines(self):
        events = itertools.chain(
            code_to_events("end"),
            [Event(evt="key", data="\n", raw=bytearray(b"\n"))],
            code_to_events("start"),
            [
                Event(evt="key", data="\x1b", raw=bytearray(b"\x1b")),  # ESC
                Event(evt="key", data="k", raw=bytearray(b"k")),        # up
                Event(evt="key", data="$", raw=bytearray(b"$")),        # end of "end"
                Event(evt="key", data="w", raw=bytearray(b"w")),        # word forward
            ],
        )
        reader, _ = self._run_vi(events)
        # w from end of first line should go to start of "start"
        self.assertEqual(reader.pos, 4)  # position of 's' in "start"


@force_not_colorized_test_class
class TestHistoricalReaderBindings(TestCase):
    def test_meta_bindings_present_only_in_emacs_mode(self):
        console = prepare_console(iter(()))
        reader = prepare_reader(console)
        emacs_keymap = dict(reader.collect_keymap())
        self.assertIn(r"\M-r", emacs_keymap)
        self.assertIn(r"\x1b[6~", emacs_keymap)

        reader.use_vi_mode = True
        reader.enter_insert_mode()
        vi_keymap = dict(reader.collect_keymap())
        self.assertNotIn(r"\M-r", vi_keymap)
        self.assertNotIn(r"\x1b[6~", vi_keymap)
        self.assertIn(r"\C-r", vi_keymap)
