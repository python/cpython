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

from abc import ABC, abstractmethod
from dataclasses import dataclass, field


@dataclass
class Event:
    evt: str
    data: str
    raw: bytes = b""


@dataclass
class Console(ABC):
    screen: list[str] = field(default_factory=list)
    height: int = 25
    width: int = 80

    @abstractmethod
    def refresh(self, screen: list[str], xy: tuple[int, int]) -> None: ...

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
    def wait(self) -> None:
        """Wait for an event."""
        ...

    @abstractmethod
    def repaint(self) -> None:
        ...
