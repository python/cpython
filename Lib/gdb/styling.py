# Styling related hooks.
# Copyright (C) 2010-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Utilities for styling."""

import gdb

try:
    from pygments import formatters, highlight, lexers
    from pygments.filters import TokenMergeFilter
    from pygments.token import Comment, Error, Text

    _formatter = None

    def get_formatter():
        global _formatter
        if _formatter is None:
            _formatter = formatters.TerminalFormatter()
        return _formatter

    def colorize(filename, contents):
        # Don't want any errors.
        try:
            lexer = lexers.get_lexer_for_filename(filename, stripnl=False)
            formatter = get_formatter()
            return highlight(contents, lexer, formatter).encode(
                gdb.host_charset(), "backslashreplace"
            )
        except Exception:
            return None

    class HandleNasmComments(TokenMergeFilter):
        @staticmethod
        def fix_comments(lexer, stream):
            in_comment = False
            for ttype, value in stream:
                if ttype is Error and value == "#":
                    in_comment = True
                if in_comment:
                    if ttype is Text and value == "\n":
                        in_comment = False
                    else:
                        ttype = Comment.Single
                yield ttype, value

        def filter(self, lexer, stream):
            f = HandleNasmComments.fix_comments
            return super().filter(lexer, f(lexer, stream))

    _asm_lexers = {}

    def __get_asm_lexer(gdbarch):
        lexer_type = "asm"
        try:
            # For an i386 based architecture, in 'intel' mode, use the nasm
            # lexer.
            flavor = gdb.parameter("disassembly-flavor")
            if flavor == "intel" and gdbarch.name()[:4] == "i386":
                lexer_type = "nasm"
        except Exception:
            # If GDB is built without i386 support then attempting to fetch
            # the 'disassembly-flavor' parameter will throw an error, which we
            # ignore.
            pass

        global _asm_lexers
        if lexer_type not in _asm_lexers:
            _asm_lexers[lexer_type] = lexers.get_lexer_by_name(lexer_type)
            _asm_lexers[lexer_type].add_filter(HandleNasmComments())
            _asm_lexers[lexer_type].add_filter("raiseonerror")
        return _asm_lexers[lexer_type]

    def colorize_disasm(content, gdbarch):
        # Don't want any errors.
        try:
            lexer = __get_asm_lexer(gdbarch)
            formatter = get_formatter()
            return highlight(content, lexer, formatter).rstrip().encode()
        except Exception:
            return content

except ImportError:

    def colorize(filename, contents):
        return None

    def colorize_disasm(content, gdbarch):
        return None
