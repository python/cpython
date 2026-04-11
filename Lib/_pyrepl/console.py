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

from __future__ import annotations

import os
import _colorize

from abc import ABC, abstractmethod
import ast
import code
import linecache
from dataclasses import dataclass
import re
import sys
from typing import TYPE_CHECKING

from .render import RenderedScreen
from .trace import trace

if TYPE_CHECKING:
    from typing import Callable, IO

    from .types import CursorXY


@dataclass
class Event:
    evt: str
    data: str
    raw: bytes = b""


@dataclass
class Console(ABC):
    posxy: CursorXY = (0, 0)
    height: int = 25
    width: int = 80
    _redraw_debug_palette: tuple[str, ...] = (
        "\x1b[41m",
        "\x1b[42m",
        "\x1b[43m",
        "\x1b[44m",
        "\x1b[45m",
        "\x1b[46m",
    )

    def __init__(
        self,
        f_in: IO[bytes] | int = 0,
        f_out: IO[bytes] | int = 1,
        term: str = "",
        encoding: str = "",
    ):
        self.encoding = encoding or sys.getdefaultencoding()

        if isinstance(f_in, int):
            self.input_fd = f_in
        else:
            self.input_fd = f_in.fileno()

        if isinstance(f_out, int):
            self.output_fd = f_out
        else:
            self.output_fd = f_out.fileno()

        self.posxy = (0, 0)
        self.height = 25
        self.width = 80
        self._rendered_screen = RenderedScreen.empty()
        self._redraw_visual_cycle = 0

    @property
    def screen(self) -> list[str]:
        return list(self._rendered_screen.screen_lines)

    def sync_rendered_screen(
        self,
        rendered_screen: RenderedScreen,
        posxy: CursorXY | None = None,
    ) -> None:
        if posxy is None:
            posxy = rendered_screen.cursor
        self.posxy = posxy
        self._rendered_screen = rendered_screen
        trace(
            "console.sync_rendered_screen lines={lines} cursor={cursor}",
            lines=len(rendered_screen.composed_lines),
            cursor=posxy,
        )

    def invalidate_render_state(self) -> None:
        self._rendered_screen = RenderedScreen.empty()
        trace("console.invalidate_render_state")

    def begin_redraw_visualization(self) -> str | None:
        if "PYREPL_VISUALIZE_REDRAWS" not in os.environ:
            return None

        palette = self._redraw_debug_palette
        cycle = self._redraw_visual_cycle
        style = palette[cycle % len(palette)]
        self._redraw_visual_cycle = cycle + 1
        trace(
            "console.begin_redraw_visualization cycle={cycle} style={style!r}",
            cycle=cycle,
            style=style,
        )
        return style

    @abstractmethod
    def refresh(self, rendered_screen: RenderedScreen) -> None: ...

    @abstractmethod
    def prepare(self) -> None: ...

    @abstractmethod
    def restore(self) -> None: ...

    @abstractmethod
    def move_cursor(self, x: int, y: int) -> None: ...

    @abstractmethod
    def set_cursor_vis(self, visible: bool) -> None: ...

    @abstractmethod
    def getheightwidth(self) -> tuple[int, int]:
        """Return (height, width) where height and width are the height
        and width of the terminal window in characters."""
        ...

    @abstractmethod
    def get_event(self, block: bool = True) -> Event | None:
        """Return an Event instance.  Returns None if |block| is false
        and there is no event pending, otherwise waits for the
        completion of an event."""
        ...

    @abstractmethod
    def push_char(self, char: int | bytes) -> None:
        """
        Push a character to the console event queue.
        """
        ...

    @abstractmethod
    def beep(self) -> None: ...

    @abstractmethod
    def clear(self) -> None:
        """Wipe the screen"""
        ...

    @abstractmethod
    def finish(self) -> None:
        """Move the cursor to the end of the display and otherwise get
        ready for end.  XXX could be merged with restore?  Hmm."""
        ...

    @abstractmethod
    def flushoutput(self) -> None:
        """Flush all output to the screen (assuming there's some
        buffering going on somewhere)."""
        ...

    @abstractmethod
    def forgetinput(self) -> None:
        """Forget all pending, but not yet processed input."""
        ...

    @abstractmethod
    def getpending(self) -> Event:
        """Return the characters that have been typed but not yet
        processed."""
        ...

    @abstractmethod
    def wait(self, timeout: float | None) -> bool:
        """Wait for an event. The return value is True if an event is
        available, False if the timeout has been reached. If timeout is
        None, wait forever. The timeout is in milliseconds."""
        ...

    @property
    def input_hook(self) -> Callable[[], int] | None:
        """Returns the current input hook."""
        ...

    @abstractmethod
    def repaint(self) -> None: ...


class InteractiveColoredConsole(code.InteractiveConsole):
    STATEMENT_FAILED = object()

    def __init__(
        self,
        locals: dict[str, object] | None = None,
        filename: str = "<console>",
        *,
        local_exit: bool = False,
    ) -> None:
        super().__init__(locals=locals, filename=filename, local_exit=local_exit)
        self.can_colorize = _colorize.can_colorize()

    def showsyntaxerror(self, filename=None, **kwargs):
        super().showsyntaxerror(filename=filename, **kwargs)

    def _excepthook(self, typ, value, tb):
        import traceback
        lines = traceback.format_exception(
                typ, value, tb,
                colorize=self.can_colorize,
                limit=traceback.BUILTIN_EXCEPTION_LIMIT)
        self.write(''.join(lines))

    def runcode(self, code):
        try:
            exec(code, self.locals)
        except SystemExit:
            raise
        except BaseException:
            self.showtraceback()
            return self.STATEMENT_FAILED
        return None

    def runsource(self, source, filename="<input>", symbol="single"):
        try:
            tree = self.compile.compiler(
                source,
                filename,
                "exec",
                ast.PyCF_ONLY_AST,
                incomplete_input=False,
            )
        except SyntaxError as e:
            # If it looks like pip install was entered (a common beginner
            # mistake), provide a hint to use the system command prompt.
            if re.match(r"^\s*(pip3?|py(thon3?)? -m pip) install.*", source):
                e.add_note(
                    "The Python package manager (pip) can only be used"
                    " outside of the Python REPL.\n"
                    "Try the 'pip' command in a separate terminal or"
                    " command prompt."
                )
            self.showsyntaxerror(filename, source=source)
            return False
        except (OverflowError, ValueError):
            self.showsyntaxerror(filename, source=source)
            return False
        if tree.body:
            *_, last_stmt = tree.body
        for stmt in tree.body:
            wrapper = ast.Interactive if stmt is last_stmt else ast.Module
            the_symbol = symbol if stmt is last_stmt else "exec"
            item = wrapper([stmt])
            try:
                code = self.compile.compiler(item, filename, the_symbol)
                linecache._register_code(code, source, filename)
            except SyntaxError as e:
                if e.args[0] == "'await' outside function":
                    python = os.path.basename(sys.executable)
                    e.add_note(
                        f"Try the asyncio REPL ({python} -m asyncio) to use"
                        f" top-level 'await' and run background asyncio tasks."
                    )
                self.showsyntaxerror(filename, source=source)
                return False
            except (OverflowError, ValueError):
                self.showsyntaxerror(filename, source=source)
                return False

            if code is None:
                return True

            result = self.runcode(code)
            if result is self.STATEMENT_FAILED:
                break
        return False
