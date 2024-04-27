#   Copyright 2000-2010 Michael Hudson-Doyle <micahel@gmail.com>
#                       Armin Rigo
#
#                        All Rights Reserved
#
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose is hereby granted without fee,
# provided that the above copyright notice appear in all copies and
# that both that copyright notice and this permission notice appear in
# supporting documentation.
#
# THE AUTHOR MICHAEL HUDSON DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"""This is an alternative to python_reader which tries to emulate
the CPython prompt as closely as possible, with the exception of
allowing multiline input and multiline history entries.
"""

import _sitebuiltins
import linecache
import sys
import code
import traceback

from .readline import _error, _get_reader, multiline_input


def check():  # returns False if there is a problem initializing the state
    try:
        _get_reader()
    except _error:
        return False
    return True


def _strip_final_indent(text):
    # kill spaces and tabs at the end, but only if they follow '\n'.
    # meant to remove the auto-indentation only (although it would of
    # course also remove explicitly-added indentation).
    short = text.rstrip(" \t")
    n = len(short)
    if n > 0 and text[n - 1] == "\n":
        return short
    return text


REPL_COMMANDS = {
    "exit": _sitebuiltins.Quitter('exit', ''),
    "quit": _sitebuiltins.Quitter('quit' ,''),
    "copyright": _sitebuiltins._Printer('copyright', sys.copyright),
    "help": _sitebuiltins._Helper(),
}

class InteractiveColoredConsole(code.InteractiveConsole):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.can_colorize = traceback._can_colorize()
    def showtraceback(self):
        super().showtraceback(colorize=self.can_colorize)


def run_multiline_interactive_console(mainmodule=None, future_flags=0):
    import code
    import __main__

    mainmodule = mainmodule or __main__
    console = InteractiveColoredConsole(mainmodule.__dict__, filename="<stdin>")
    if future_flags:
        console.compile.compiler.flags |= future_flags

    input_n = 0

    def more_lines(unicodetext):
        # ooh, look at the hack:
        src = _strip_final_indent(unicodetext)
        try:
            code = console.compile(src, "<stdin>", "single")
        except (OverflowError, SyntaxError, ValueError):
            return False
        else:
            return code is None

    while 1:
        try:
            try:
                sys.stdout.flush()
            except Exception:
                pass
            ps1 = getattr(sys, "ps1", ">>> ")
            ps2 = getattr(sys, "ps2", "... ")
            try:
                statement = multiline_input(more_lines, ps1, ps2, returns_unicode=True)
            except EOFError:
                break

            input_name = f"<python-input-{input_n}>"
            linecache._register_code(input_name, statement, "<stdin>")
            maybe_repl_command = REPL_COMMANDS.get(statement.strip())
            if maybe_repl_command is not None:
                maybe_repl_command()
                continue
            else:
                more = console.push(_strip_final_indent(statement), filename=input_name)
                assert not more
            input_n += 1
        except KeyboardInterrupt:
            console.write("\nKeyboardInterrupt\n")
            console.resetbuffer()
        except MemoryError:
            console.write("\nMemoryError\n")
            console.resetbuffer()
