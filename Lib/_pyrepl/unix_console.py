#   Copyright 2000-2010 Michael Hudson-Doyle <micahel@gmail.com>
#                       Antonio Cuni
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

from __future__ import annotations

import errno
import os
import re
import select
import signal
import struct
import termios
import time
import types
import platform
from collections.abc import Callable
from dataclasses import dataclass
from fcntl import ioctl
from typing import TYPE_CHECKING, cast, overload

from . import terminfo
from .console import Console, Event
from .fancy_termios import tcgetattr, tcsetattr, TermState
from .render import (
    EMPTY_RENDER_LINE,
    LineUpdate,
    RenderLine,
    RenderedScreen,
    requires_cursor_resync,
    diff_render_lines,
    render_cells,
)
from .trace import trace, trace_text
from .unix_eventqueue import EventQueue

# declare posix optional to allow None assignment on other platforms
posix: types.ModuleType | None
try:
    import posix
except ImportError:
    posix = None

# types
if TYPE_CHECKING:
    from typing import AbstractSet, IO, Literal

type _MoveFunc = Callable[[int, int], None]
type _PendingWrite = tuple[str | bytes, bool]


class InvalidTerminal(RuntimeError):
    def __init__(self, message: str) -> None:
        super().__init__(errno.EIO, message)


_error = (termios.error, InvalidTerminal)
_error_codes_to_ignore = frozenset([errno.EIO, errno.ENXIO, errno.EPERM])

SIGWINCH_EVENT = "repaint"

FIONREAD = getattr(termios, "FIONREAD", None)
TIOCGWINSZ = getattr(termios, "TIOCGWINSZ", None)

# ------------ start of baudrate definitions ------------

# Add (possibly) missing baudrates (check termios man page) to termios


def add_baudrate_if_supported(dictionary: dict[int, int], rate: int) -> None:
    baudrate_name = "B%d" % rate
    if hasattr(termios, baudrate_name):
        dictionary[getattr(termios, baudrate_name)] = rate


# Check the termios man page (Line speed) to know where these
# values come from.
potential_baudrates = [
    0,
    110,
    115200,
    1200,
    134,
    150,
    1800,
    19200,
    200,
    230400,
    2400,
    300,
    38400,
    460800,
    4800,
    50,
    57600,
    600,
    75,
    9600,
]

ratedict: dict[int, int] = {}
for rate in potential_baudrates:
    add_baudrate_if_supported(ratedict, rate)

# Clean up variables to avoid unintended usage
del rate, add_baudrate_if_supported

# ------------ end of baudrate definitions ------------

delayprog = re.compile(b"\\$<([0-9]+)((?:/|\\*){0,2})>")

try:
    poll: type[select.poll] = select.poll
except AttributeError:
    # this is exactly the minimum necessary to support what we
    # do with poll objects
    class MinimalPoll:
        def __init__(self):
            pass

        def register(self, fd, flag):
            self.fd = fd

        # note: The 'timeout' argument is received as *milliseconds*
        def poll(self, timeout: float | None = None) -> list[int]:
            if timeout is None:
                r, w, e = select.select([self.fd], [], [])
            else:
                r, w, e = select.select([self.fd], [], [], timeout / 1000)
            return r

    poll = MinimalPoll  # type: ignore[assignment]


@dataclass(frozen=True, slots=True)
class UnixRefreshPlan:
    """Instructions for updating the terminal after a screen change.

    After the user types ``e`` to complete ``name``::

        Before: >>> def greet(nam|):
                                 ▲
                        LineUpdate here: insert_char "e"

         After: >>> def greet(name|):
                                  ▲

    Only the changed cells are sent to the terminal; unchanged rows
    are skipped entirely.
    """

    grow_lines: int
    """Number of blank lines to append at the bottom to accommodate new content."""
    use_tall_mode: bool
    """Use absolute cursor addressing via ``cup`` instead of relative moves.
    Activated when content exceeds one screen height."""
    offset: int
    """Vertical scroll offset: the buffer row displayed at the top of the terminal window."""
    reverse_scroll: int
    """Number of lines to scroll backwards (content moves down)."""
    forward_scroll: int
    """Number of lines to scroll forwards (content moves up)."""
    line_updates: tuple[LineUpdate, ...]
    cleared_lines: tuple[int, ...]
    """Row indices to erase (old content with no replacement)."""
    rendered_screen: RenderedScreen
    cursor: tuple[int, int]


class UnixConsole(Console):
    __buffer: list[_PendingWrite]
    __gone_tall: bool
    __move: _MoveFunc
    __offset: int

    def __init__(
        self,
        f_in: IO[bytes] | int = 0,
        f_out: IO[bytes] | int = 1,
        term: str = "",
        encoding: str = "",
    ):
        """
        Initialize the UnixConsole.

        Parameters:
        - f_in (int or file-like object): Input file descriptor or object.
        - f_out (int or file-like object): Output file descriptor or object.
        - term (str): Terminal name.
        - encoding (str): Encoding to use for I/O operations.
        """
        super().__init__(f_in, f_out, term, encoding)

        self.pollob = poll()
        self.pollob.register(self.input_fd, select.POLLIN)
        self.terminfo = terminfo.TermInfo(term or None)
        self.term = term
        self.is_apple_terminal = (
            platform.system() == "Darwin"
            and os.getenv("TERM_PROGRAM") == "Apple_Terminal"
        )

        try:
            self.__input_fd_set(tcgetattr(self.input_fd), ignore=frozenset())
        except _error as e:
            raise RuntimeError(f"termios failure ({e.args[1]})")

        @overload
        def _my_getstr(
            cap: str, optional: Literal[False] = False
        ) -> bytes: ...

        @overload
        def _my_getstr(cap: str, optional: bool) -> bytes | None: ...

        def _my_getstr(cap: str, optional: bool = False) -> bytes | None:
            r = self.terminfo.get(cap)
            if not optional and r is None:
                raise InvalidTerminal(
                    f"terminal doesn't have the required {cap} capability"
                )
            return r

        self._bel = _my_getstr("bel")
        self._civis = _my_getstr("civis", optional=True)
        self._clear = _my_getstr("clear")
        self._cnorm = _my_getstr("cnorm", optional=True)
        self._cub = _my_getstr("cub", optional=True)
        self._cub1 = _my_getstr("cub1", optional=True)
        self._cud = _my_getstr("cud", optional=True)
        self._cud1 = _my_getstr("cud1", optional=True)
        self._cuf = _my_getstr("cuf", optional=True)
        self._cuf1 = _my_getstr("cuf1", optional=True)
        self._cup = _my_getstr("cup")
        self._cuu = _my_getstr("cuu", optional=True)
        self._cuu1 = _my_getstr("cuu1", optional=True)
        self._dch1 = _my_getstr("dch1", optional=True)
        self._dch = _my_getstr("dch", optional=True)
        self._el = _my_getstr("el")
        self._hpa = _my_getstr("hpa", optional=True)
        self._ich = _my_getstr("ich", optional=True)
        self._ich1 = _my_getstr("ich1", optional=True)
        self._ind = _my_getstr("ind", optional=True)
        self._pad = _my_getstr("pad", optional=True)
        self._ri = _my_getstr("ri", optional=True)
        self._rmkx = _my_getstr("rmkx", optional=True)
        self._smkx = _my_getstr("smkx", optional=True)

        self.__setup_movement()

        self.event_queue = EventQueue(
            self.input_fd, self.encoding, self.terminfo
        )
        self.cursor_visible = True

        signal.signal(signal.SIGCONT, self._sigcont_handler)

    def _sigcont_handler(self, signum, frame):
        self.restore()
        self.prepare()

    def __read(self, n: int) -> bytes:
        return os.read(self.input_fd, n)

    def change_encoding(self, encoding: str) -> None:
        """
        Change the encoding used for I/O operations.

        Parameters:
        - encoding (str): New encoding to use.
        """
        self.encoding = encoding

    def refresh(self, rendered_screen: RenderedScreen) -> None:
        """
        Refresh the console screen.

        Parameters:
        - rendered_screen: Structured rendered screen contents and cursor.
        """
        c_xy = rendered_screen.cursor
        trace(
            "unix.refresh start cursor={cursor} lines={lines} prev_lines={prev_lines} "
            "offset={offset} posxy={posxy}",
            cursor=c_xy,
            lines=len(rendered_screen.composed_lines),
            prev_lines=len(self._rendered_screen.composed_lines),
            offset=self.__offset,
            posxy=self.posxy,
        )
        plan = self.__plan_refresh(rendered_screen, c_xy)
        self.__apply_refresh_plan(plan)

    def __plan_refresh(
        self,
        rendered_screen: RenderedScreen,
        c_xy: tuple[int, int],
    ) -> UnixRefreshPlan:
        cx, cy = c_xy
        height = self.height
        old_offset = offset = self.__offset
        prev_composed = self._rendered_screen.composed_lines
        previous_lines = list(prev_composed)
        next_lines = list(rendered_screen.composed_lines)
        line_count = len(next_lines)

        grow_lines = 0
        if not self.__gone_tall:
            grow_lines = max(
                min(line_count, height) - len(prev_composed),
                0,
            )
            previous_lines.extend([EMPTY_RENDER_LINE] * grow_lines)
        elif len(previous_lines) < line_count:
            previous_lines.extend([EMPTY_RENDER_LINE] * (line_count - len(previous_lines)))

        use_tall_mode = self.__gone_tall or line_count > height

        # we make sure the cursor is on the screen, and that we're
        # using all of the screen if we can
        if cy < offset:
            offset = cy
        elif cy >= offset + height:
            offset = cy - height + 1
        elif offset > 0 and line_count < offset + height:
            offset = max(line_count - height, 0)
            next_lines.append(EMPTY_RENDER_LINE)

        oldscr = previous_lines[old_offset : old_offset + height]
        newscr = next_lines[offset : offset + height]

        reverse_scroll = 0
        forward_scroll = 0
        if old_offset > offset and self._ri:
            reverse_scroll = old_offset - offset
            for _ in range(reverse_scroll):
                if oldscr:
                    oldscr.pop(-1)
                oldscr.insert(0, EMPTY_RENDER_LINE)
        elif old_offset < offset and self._ind:
            forward_scroll = offset - old_offset
            for _ in range(forward_scroll):
                if oldscr:
                    oldscr.pop(0)
                oldscr.append(EMPTY_RENDER_LINE)

        line_updates: list[LineUpdate] = []
        px, _ = self.posxy
        for y, oldline, newline in zip(range(offset, offset + height), oldscr, newscr):
            update = self.__plan_changed_line(y, oldline, newline, px)
            if update is not None:
                line_updates.append(update)

        cleared_lines = tuple(range(offset + len(newscr), offset + len(oldscr)))
        console_rendered_screen = RenderedScreen(tuple(next_lines), c_xy)
        trace(
            "unix.refresh plan grow={grow} tall={tall} offset={offset} "
            "reverse_scroll={reverse_scroll} forward_scroll={forward_scroll} "
            "updates={updates} clears={clears}",
            grow=grow_lines,
            tall=use_tall_mode,
            offset=offset,
            reverse_scroll=reverse_scroll,
            forward_scroll=forward_scroll,
            updates=len(line_updates),
            clears=len(cleared_lines),
        )
        return UnixRefreshPlan(
            grow_lines=grow_lines,
            use_tall_mode=use_tall_mode,
            offset=offset,
            reverse_scroll=reverse_scroll,
            forward_scroll=forward_scroll,
            line_updates=tuple(line_updates),
            cleared_lines=cleared_lines,
            rendered_screen=console_rendered_screen,
            cursor=(cx, cy),
        )

    def __apply_refresh_plan(self, plan: UnixRefreshPlan) -> None:
        cx, cy = plan.cursor
        trace(
            "unix.refresh apply cursor={cursor} updates={updates} clears={clears}",
            cursor=plan.cursor,
            updates=len(plan.line_updates),
            clears=len(plan.cleared_lines),
        )
        visual_style = self.begin_redraw_visualization()
        screen_line_count = len(self._rendered_screen.composed_lines)

        for _ in range(plan.grow_lines):
            self.__hide_cursor()
            if screen_line_count:
                self.__move(0, screen_line_count - 1)
                self.__write("\n")
            self.posxy = 0, screen_line_count
            screen_line_count += 1

        if plan.use_tall_mode and not self.__gone_tall:
            self.__gone_tall = True
            self.__move = self.__move_tall

        old_offset = self.__offset
        if plan.reverse_scroll:
            self.__hide_cursor()
            self.__write_code(self._cup, 0, 0)
            self.posxy = 0, old_offset
            for _ in range(plan.reverse_scroll):
                self.__write_code(self._ri)
        elif plan.forward_scroll:
            self.__hide_cursor()
            self.__write_code(self._cup, self.height - 1, 0)
            self.posxy = 0, old_offset + self.height - 1
            for _ in range(plan.forward_scroll):
                self.__write_code(self._ind)

        self.__offset = plan.offset

        for update in plan.line_updates:
            self.__apply_line_update(update, visual_style)

        for y in plan.cleared_lines:
            self.__hide_cursor()
            self.__move(0, y)
            self.posxy = 0, y
            self.__write_code(self._el)

        self.__show_cursor()
        self.move_cursor(cx, cy)
        self.flushoutput()
        self.sync_rendered_screen(plan.rendered_screen, self.posxy)

    def move_cursor(self, x: int, y: int) -> None:
        """
        Move the cursor to the specified position on the screen.

        Parameters:
        - x (int): X coordinate.
        - y (int): Y coordinate.
        """
        if y < self.__offset or y >= self.__offset + self.height:
            trace(
                "unix.move_cursor offscreen x={x} y={y} offset={offset} height={height}",
                x=x,
                y=y,
                offset=self.__offset,
                height=self.height,
            )
            self.event_queue.insert(Event("scroll", ""))
        else:
            trace("unix.move_cursor x={x} y={y}", x=x, y=y)
            self.__move(x, y)
            self.posxy = x, y
            self.flushoutput()

    def prepare(self) -> None:
        """
        Prepare the console for input/output operations.
        """
        trace("unix.prepare")
        self.__buffer = []

        self.__svtermstate = tcgetattr(self.input_fd)
        raw = self.__svtermstate.copy()
        raw.iflag &= ~(termios.INPCK | termios.ISTRIP | termios.IXON)
        raw.oflag &= ~(termios.OPOST)
        raw.cflag &= ~(termios.CSIZE | termios.PARENB)
        raw.cflag |= termios.CS8
        raw.iflag |= termios.BRKINT
        raw.lflag &= ~(termios.ICANON | termios.ECHO | termios.IEXTEN)
        raw.lflag |= termios.ISIG
        raw.cc[termios.VMIN] = b"\x01"
        raw.cc[termios.VTIME] = b"\x00"
        self.__input_fd_set(raw)

        # Apple Terminal will re-wrap lines for us unless we preempt the
        # damage.
        if self.is_apple_terminal:
            os.write(self.output_fd, b"\033[?7l")

        self.height, self.width = self.getheightwidth()

        self.posxy = 0, 0
        self.__gone_tall = False
        self.__move = self.__move_short
        self.__offset = 0
        self.sync_rendered_screen(RenderedScreen.empty(), self.posxy)

        self.__maybe_write_code(self._smkx)

        try:
            self.old_sigwinch = signal.signal(signal.SIGWINCH, self.__sigwinch)
        except ValueError:
            pass

        self.__enable_bracketed_paste()

    def restore(self) -> None:
        """
        Restore the console to the default state
        """
        trace("unix.restore")
        self.__disable_bracketed_paste()
        self.__maybe_write_code(self._rmkx)
        self.flushoutput()
        self.__input_fd_set(self.__svtermstate)

        if self.is_apple_terminal:
            os.write(self.output_fd, b"\033[?7h")

        if hasattr(self, "old_sigwinch"):
            try:
                signal.signal(signal.SIGWINCH, self.old_sigwinch)
            except ValueError as e:
                import threading
                if threading.current_thread() is threading.main_thread():
                    raise e
            del self.old_sigwinch

    def push_char(self, char: int | bytes) -> None:
        """
        Push a character to the console event queue.
        """
        trace("push char {char!r}", char=char)
        self.event_queue.push(char)

    def get_event(self, block: bool = True) -> Event | None:
        """
        Get an event from the console event queue.

        Parameters:
        - block (bool): Whether to block until an event is available.

        Returns:
        - Event: Event object from the event queue.
        """
        if not block and not self.wait(timeout=0):
            return None

        while self.event_queue.empty():
            while True:
                try:
                    self.push_char(self.__read(1))
                except OSError as err:
                    if err.errno == errno.EINTR:
                        if not self.event_queue.empty():
                            return self.event_queue.get()
                        else:
                            continue
                    elif err.errno == errno.EIO:
                        raise SystemExit(errno.EIO)
                    else:
                        raise
                else:
                    break
        return self.event_queue.get()

    def wait(self, timeout: float | None = None) -> bool:
        """
        Wait for events on the console.
        """
        return (
            not self.event_queue.empty()
            or bool(self.pollob.poll(timeout))
        )

    def set_cursor_vis(self, visible: bool) -> None:
        """
        Set the visibility of the cursor.

        Parameters:
        - visible (bool): Visibility flag.
        """
        if visible:
            self.__show_cursor()
        else:
            self.__hide_cursor()

    if TIOCGWINSZ:

        def getheightwidth(self):
            """
            Get the height and width of the console.

            Returns:
            - tuple: Height and width of the console.
            """
            try:
                return int(os.environ["LINES"]), int(os.environ["COLUMNS"])
            except (KeyError, TypeError, ValueError):
                try:
                    size = ioctl(self.input_fd, TIOCGWINSZ, b"\000" * 8)
                except OSError:
                    return 25, 80
                height, width = struct.unpack("hhhh", size)[0:2]
                if not height:
                    return 25, 80
                return height, width

    else:

        def getheightwidth(self):
            """
            Get the height and width of the console.

            Returns:
            - tuple: Height and width of the console.
            """
            try:
                return int(os.environ["LINES"]), int(os.environ["COLUMNS"])
            except (KeyError, TypeError, ValueError):
                return 25, 80

    def forgetinput(self):
        """
        Discard any pending input on the console.
        """
        termios.tcflush(self.input_fd, termios.TCIFLUSH)

    def flushoutput(self):
        """
        Flush the output buffer.
        """
        for text, iscode in self.__buffer:
            if iscode:
                self.__tputs(text)
            else:
                os.write(self.output_fd, text.encode(self.encoding, "replace"))
        del self.__buffer[:]

    def finish(self):
        """
        Finish console operations and flush the output buffer.
        """
        rendered_lines = self._rendered_screen.composed_lines
        y = len(rendered_lines) - 1
        while y >= 0 and not rendered_lines[y].text:
            y -= 1
        self.__move(0, min(y, self.height + self.__offset - 1))
        self.__write("\n\r")
        self.flushoutput()

    def beep(self):
        """
        Emit a beep sound.
        """
        self.__maybe_write_code(self._bel)
        self.flushoutput()

    if FIONREAD:

        def getpending(self):
            """
            Get pending events from the console event queue.

            Returns:
            - Event: Pending event from the event queue.
            """
            e = Event("key", "", b"")

            while not self.event_queue.empty():
                e2 = self.event_queue.get()
                e.data += e2.data
                e.raw += e2.raw

            amount = struct.unpack("i", ioctl(self.input_fd, FIONREAD, b"\0\0\0\0"))[0]
            trace("getpending({a})", a=amount)
            raw = self.__read(amount)
            data = str(raw, self.encoding, "replace")
            e.data += data
            e.raw += raw
            return e

    else:

        def getpending(self):
            """
            Get pending events from the console event queue.

            Returns:
            - Event: Pending event from the event queue.
            """
            e = Event("key", "", b"")

            while not self.event_queue.empty():
                e2 = self.event_queue.get()
                e.data += e2.data
                e.raw += e2.raw

            amount = 10000
            raw = self.__read(amount)
            data = str(raw, self.encoding, "replace")
            e.data += data
            e.raw += raw
            return e

    def clear(self):
        """
        Clear the console screen.
        """
        trace("unix.clear")
        self.__write_code(self._clear)
        self.__gone_tall = True
        self.__move = self.__move_tall
        self.posxy = 0, 0
        self.sync_rendered_screen(RenderedScreen.empty(), self.posxy)

    @property
    def input_hook(self):
        # avoid inline imports here so the repl doesn't get flooded
        # with import logging from -X importtime=2
        if posix is not None and posix._is_inputhook_installed():
            return posix._inputhook

    def __enable_bracketed_paste(self) -> None:
        os.write(self.output_fd, b"\x1b[?2004h")

    def __disable_bracketed_paste(self) -> None:
        os.write(self.output_fd, b"\x1b[?2004l")

    def __setup_movement(self):
        """
        Set up the movement functions based on the terminal capabilities.
        """
        if 0 and self._hpa:  # hpa don't work in windows telnet :-(
            self.__move_x = self.__move_x_hpa
        elif self._cub and self._cuf:
            self.__move_x = self.__move_x_cub_cuf
        elif self._cub1 and self._cuf1:
            self.__move_x = self.__move_x_cub1_cuf1
        else:
            raise RuntimeError("insufficient terminal (horizontal)")

        if self._cuu and self._cud:
            self.__move_y = self.__move_y_cuu_cud
        elif self._cuu1 and self._cud1:
            self.__move_y = self.__move_y_cuu1_cud1
        else:
            raise RuntimeError("insufficient terminal (vertical)")

        if self._dch1:
            self.dch1 = self._dch1
        elif self._dch:
            self.dch1 = terminfo.tparm(self._dch, 1)
        else:
            self.dch1 = None

        if self._ich1:
            self.ich1 = self._ich1
        elif self._ich:
            self.ich1 = terminfo.tparm(self._ich, 1)
        else:
            self.ich1 = None

        self.__move = self.__move_short

    @staticmethod
    def __cell_index_from_x(line: RenderLine, x_coord: int) -> int:
        width = 0
        index = 0
        while index < len(line.cells) and width < x_coord:
            width += line.cells[index].width
            index += 1
        return index

    def __plan_changed_line(
        self,
        y: int,
        oldline: RenderLine,
        newline: RenderLine,
        px_coord: int,
    ) -> LineUpdate | None:
        # NOTE: The shared replace_char / replace_span / rewrite_suffix logic
        # is duplicated in WindowsConsole.__plan_changed_line. Keep changes to
        # these common cases synchronised between the two files. Yes, this is
        # duplicated on purpose; the two backends agree just enough to make a
        # shared helper a trap. Unix-only cases (insert_char, delete_then_insert)
        # rely on terminal capabilities (ich1/dch1) that are unavailable on
        # Windows.
        diff = diff_render_lines(oldline, newline)
        if diff is None:
            return None

        start_cell = diff.start_cell
        start_x = diff.start_x

        if (
            self.ich1
            and not diff.old_cells
            and (visible_new_cells := tuple(
                cell for cell in diff.new_cells if cell.width
            ))
            and len(visible_new_cells) == 1
            and all(cell.width == 0 for cell in diff.new_cells[1:])
            and oldline.cells[start_cell:] == newline.cells[start_cell + 1 :]
        ):
            px_cell = self.__cell_index_from_x(oldline, px_coord)
            if (
                y == self.posxy[1]
                and start_x > self.posxy[0]
                and oldline.cells[px_cell:start_cell]
                == newline.cells[px_cell + 1 : start_cell + 1]
            ):
                start_cell = px_cell
                start_x = px_coord
            planned_cells = diff.new_cells
            changed_cell = visible_new_cells[0]
            return LineUpdate(
                kind="insert_char",
                y=y,
                start_cell=start_cell,
                start_x=start_x,
                cells=planned_cells,
                char_width=changed_cell.width,
                reset_to_margin=requires_cursor_resync(planned_cells),
            )

        if (
            len(diff.old_cells) == 1
            and len(diff.new_cells) == 1
            and diff.old_cells[0].width == diff.new_cells[0].width
        ):
            planned_cells = diff.new_cells
            changed_cell = planned_cells[0]
            return LineUpdate(
                kind="replace_char",
                y=y,
                start_cell=start_cell,
                start_x=start_x,
                cells=planned_cells,
                char_width=changed_cell.width,
                reset_to_margin=requires_cursor_resync(planned_cells),
            )

        if diff.old_changed_width == diff.new_changed_width:
            planned_cells = diff.new_cells
            return LineUpdate(
                kind="replace_span",
                y=y,
                start_cell=start_cell,
                start_x=start_x,
                cells=planned_cells,
                char_width=diff.new_changed_width,
                reset_to_margin=requires_cursor_resync(planned_cells),
            )

        if (
            self.dch1
            and self.ich1
            and newline.width == self.width
            and start_x < newline.width - 2
            and newline.cells[start_cell + 1 : -1] == oldline.cells[start_cell:-2]
        ):
            planned_cells = (newline.cells[start_cell],)
            changed_cell = planned_cells[0]
            return LineUpdate(
                kind="delete_then_insert",
                y=y,
                start_cell=start_cell,
                start_x=start_x,
                cells=planned_cells,
                char_width=changed_cell.width,
                reset_to_margin=requires_cursor_resync(planned_cells),
            )

        suffix_cells = newline.cells[start_cell:]
        return LineUpdate(
            kind="rewrite_suffix",
            y=y,
            start_cell=start_cell,
            start_x=start_x,
            cells=suffix_cells,
            char_width=sum(cell.width for cell in suffix_cells),
            clear_eol=oldline.width > newline.width,
            reset_to_margin=requires_cursor_resync(suffix_cells),
        )

    def __apply_line_update(
        self,
        update: LineUpdate,
        visual_style: str | None = None,
    ) -> None:
        text = render_cells(update.cells, visual_style) if visual_style else update.text
        trace(
            "unix.refresh update kind={kind} y={y} x={x} text={text} "
            "clear_eol={clear_eol} reset_to_margin={reset}",
            kind=update.kind,
            y=update.y,
            x=update.start_x,
            text=trace_text(text),
            clear_eol=update.clear_eol,
            reset=update.reset_to_margin,
        )
        if update.kind == "insert_char":
            self.__move(update.start_x, update.y)
            self.__write_code(self.ich1)
            self.__write(text)
            self.posxy = update.start_x + update.char_width, update.y
        elif update.kind in {"replace_char", "replace_span"}:
            self.__move(update.start_x, update.y)
            self.__write(text)
            self.posxy = update.start_x + update.char_width, update.y
        elif update.kind == "delete_then_insert":
            self.__hide_cursor()
            self.__move(self.width - 2, update.y)
            self.posxy = self.width - 2, update.y
            self.__write_code(self.dch1)
            self.__move(update.start_x, update.y)
            self.__write_code(self.ich1)
            self.__write(text)
            self.posxy = update.start_x + update.char_width, update.y
        else:
            self.__hide_cursor()
            self.__move(update.start_x, update.y)
            if update.clear_eol:
                self.__write_code(self._el)
            self.__write(text)
            self.posxy = update.start_x + update.char_width, update.y

        if update.reset_to_margin:
            # Non-SGR terminal controls can affect the cursor position.
            self.move_cursor(0, update.y)

    def __write(self, text):
        self.__buffer.append((text, False))

    def __write_code(self, fmt, *args):
        self.__buffer.append((terminfo.tparm(fmt, *args), True))

    def __maybe_write_code(self, fmt, *args):
        if fmt:
            self.__write_code(fmt, *args)

    def __move_y_cuu1_cud1(self, y):
        assert self._cud1 is not None
        assert self._cuu1 is not None
        dy = y - self.posxy[1]
        if dy > 0:
            self.__write_code(dy * self._cud1)
        elif dy < 0:
            self.__write_code((-dy) * self._cuu1)

    def __move_y_cuu_cud(self, y):
        dy = y - self.posxy[1]
        if dy > 0:
            self.__write_code(self._cud, dy)
        elif dy < 0:
            self.__write_code(self._cuu, -dy)

    def __move_x_hpa(self, x: int) -> None:
        if x != self.posxy[0]:
            self.__write_code(self._hpa, x)

    def __move_x_cub1_cuf1(self, x: int) -> None:
        assert self._cuf1 is not None
        assert self._cub1 is not None
        dx = x - self.posxy[0]
        if dx > 0:
            self.__write_code(self._cuf1 * dx)
        elif dx < 0:
            self.__write_code(self._cub1 * (-dx))

    def __move_x_cub_cuf(self, x: int) -> None:
        dx = x - self.posxy[0]
        if dx > 0:
            self.__write_code(self._cuf, dx)
        elif dx < 0:
            self.__write_code(self._cub, -dx)

    def __move_short(self, x, y):
        self.__move_x(x)
        self.__move_y(y)

    def __move_tall(self, x, y):
        assert 0 <= y - self.__offset < self.height, y - self.__offset
        self.__write_code(self._cup, y - self.__offset, x)

    def __sigwinch(self, signum, frame):
        self.event_queue.insert(Event("resize", ""))

    def __hide_cursor(self):
        if self.cursor_visible:
            self.__maybe_write_code(self._civis)
            self.cursor_visible = False

    def __show_cursor(self):
        if not self.cursor_visible:
            self.__maybe_write_code(self._cnorm)
            self.cursor_visible = True

    def repaint(self):
        composed = self._rendered_screen.composed_lines
        trace(
            "unix.repaint gone_tall={gone_tall} screen_lines={lines} offset={offset}",
            gone_tall=self.__gone_tall,
            lines=len(composed),
            offset=self.__offset,
        )
        if not self.__gone_tall:
            self.posxy = 0, self.posxy[1]
            self.__write("\r")
            ns = len(composed) * ["\000" * self.width]
        else:
            self.posxy = 0, self.__offset
            self.__move(0, self.__offset)
            ns = self.height * ["\000" * self.width]
        self.sync_rendered_screen(
            RenderedScreen.from_screen_lines(ns, self.posxy),
            self.posxy,
        )

    def __tputs(self, fmt, prog=delayprog):
        """A Python implementation of the curses tputs function; the
        curses one can't really be wrapped in a sane manner.

        I have the strong suspicion that this is complexity that
        will never do anyone any good."""
        # using .get() means that things will blow up
        # only if the bps is actually needed (which I'm
        # betting is pretty unlikely)
        bps = ratedict.get(self.__svtermstate.ospeed)
        while True:
            m = prog.search(fmt)
            if not m:
                os.write(self.output_fd, fmt)
                break
            x, y = m.span()
            os.write(self.output_fd, fmt[:x])
            fmt = fmt[y:]
            delay = int(m.group(1))
            if b"*" in m.group(2):
                delay *= self.height
            if self._pad and bps is not None:
                nchars = (bps * delay) / 1000
                os.write(self.output_fd, self._pad * nchars)
            else:
                time.sleep(float(delay) / 1000.0)

    def __input_fd_set(
        self,
        state: TermState,
        ignore: AbstractSet[int] = _error_codes_to_ignore,
    ) -> bool:
        try:
            tcsetattr(self.input_fd, termios.TCSADRAIN, state)
        except termios.error as te:
            if te.args[0] not in ignore:
                raise
            return False
        else:
            return True
