#   Copyright 2000-2004 Michael Hudson-Doyle <micahel@gmail.com>
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

import curses
import termios


class InvalidTerminal(RuntimeError):
    pass


class TermState:
    def __init__(self, tuples):
        (
            self.iflag,
            self.oflag,
            self.cflag,
            self.lflag,
            self.ispeed,
            self.ospeed,
            self.cc,
        ) = tuples

    def as_list(self):
        return [
            self.iflag,
            self.oflag,
            self.cflag,
            self.lflag,
            self.ispeed,
            self.ospeed,
            self.cc,
        ]

    def copy(self):
        return self.__class__(self.as_list())


def tcgetattr(fd):
    return TermState(termios.tcgetattr(fd))


def tcsetattr(fd, when, attrs):
    termios.tcsetattr(fd, when, attrs.as_list())


class TermCapability:
    """Base class for all terminal capabilities we need"""

    def __init__(self, buffer):
        self._buffer = buffer
        self._args = []

        self._bytes = curses.tigetstr(self.name)
        if self._bytes is None and not self.optional:
            raise InvalidTerminal(
                f"terminal doesn't have the required {self.name!r} capability"
            )

        self.supported = self._bytes is not None

    def __call__(self, *args):
        if self.supported:
            self._args = args
            self._buffer.push(self)

    def text(self):
        return curses.tparm(self._bytes, *self._args)


class Bell(TermCapability):
    """Audible signal (bell)"""

    name = "bel"
    optional = False


class CursorInvisible(TermCapability):
    """Make cursor invisible"""

    name = "civis"
    optional = True


class ClearScreen(TermCapability):
    """Clear screen and home cursor"""

    name = "clear"
    optional = False


class CursorNormal(TermCapability):
    """Make cursor appear normal (undo civis/cvvis)"""

    name = "cnorm"
    optional = True


class ParmLeftCursor(TermCapability):
    """Move #1 characters to the left"""

    name = "cub"
    optional = True


class CursorLeft(TermCapability):
    """Move left one space"""

    name = "cub1"
    optional = True

    def text(self):
        assert len(self._args) == 1  # cursor_down needs to have been called with dx
        return curses.tparm(self._args[0] * self._bytes)


class ParmDownCursor(TermCapability):
    """Down #1 lines"""

    name = "cud"
    optional = True


class CursorDown(TermCapability):
    """Down one line"""

    name = "cud1"
    optional = True

    def text(self):
        assert len(self._args) == 1  # cursor_down needs to have been called with dy
        return curses.tparm(self._args[0] * self._bytes)


class ParmRightCursor(TermCapability):
    """Move #1 characters to the right"""

    name = "cuf"
    optional = True


class CursorRight(TermCapability):
    """Non-destructive space (move right one space)"""

    name = "cuf1"
    optional = True

    def text(self):
        assert len(self._args) == 1  # cursor_down needs to have been called with dx
        return curses.tparm(self._args[0] * self._bytes)


class CursorAddress(TermCapability):
    """Move to row #1 columns #2"""

    name = "cup"
    optional = False


class ParmUpCursor(TermCapability):
    """Up #1 lines"""

    name = "cuu"
    optional = True


class CursorUp(TermCapability):
    """Up 1 line"""

    name = "cuu1"
    optional = True

    def text(self):
        assert len(self._args) == 1  # cursor_down needs to have been called with dy
        return curses.tparm(self._args[0] * self._bytes)


class ParmDeleteCharacter(TermCapability):
    """Delete #1 characters"""

    name = "dch"
    optional = True


class DeleteCharacter(TermCapability):
    """Delete character"""

    name = "dch1"
    optional = True


class ClearEol(TermCapability):
    """Clear to end of line"""

    name = "el"
    optional = False


class ColumnAddress(TermCapability):
    """Horizontal position #1, absolute"""

    name = "hpa"
    optional = True


class ParmInsertCharacter(TermCapability):
    """Insert #1 characters"""

    name = "ich"
    optional = True


class InsertCharacter(TermCapability):
    """Insert character"""

    name = "ich1"
    optional = True


class ScrollForward(TermCapability):
    """Scroll text up"""

    name = "ind"
    optional = True


class PadChar(TermCapability):
    """Padding char (instead of null)"""

    name = "pad"
    optional = True

    def text(self, nchars):
        return self._bytes * nchars


class ScrollReverse(TermCapability):
    """Scroll text down"""

    name = "ri"
    optional = True


class KeypadLocal(TermCapability):
    """Leave 'keyboard_transmit' mode"""

    name = "rmkx"
    optional = True


class KeypadXmit(TermCapability):
    """Enter 'keyboard_transmit' mode"""

    name = "smkx"
    optional = True
