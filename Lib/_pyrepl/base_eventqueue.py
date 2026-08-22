#   Copyright 2000-2008 Michael Hudson-Doyle <micahel@gmail.com>
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

"""
OS-independent base for an event and VT sequence scanner

See unix_eventqueue and windows_eventqueue for subclasses.
"""

import os
from collections import deque

from . import keymap
from .console import Event
from .trace import trace

ESC_TIMEOUT_DEFAULT = 0.1

class BaseEventQueue:
    def __init__(self, encoding: str, keymap_dict: dict[bytes, str],
                 esc_timeout: float | None = None) -> None:
        self.compiled_keymap = keymap.compile_keymap(keymap_dict)
        self.keymap = self.compiled_keymap
        trace("keymap {k!r}", k=self.keymap)
        self.encoding = encoding
        self.events: deque[Event] = deque()
        self.buf = bytearray()
        default = float(os.environ.get("PYREPL_ESC_TIMEOUT", ESC_TIMEOUT_DEFAULT))
        self.esc_timeout = esc_timeout if esc_timeout is not None else default

    def get(self) -> Event | None:
        """
        Retrieves the next event from the queue.
        """
        if self.events:
            return self.events.popleft()
        else:
            return None

    def empty(self) -> bool:
        """
        Checks if the queue is empty.
        """
        return not self.events

    def pending(self) -> bool:
        """
        True when we have received the start of an escape/key sequence but
        are still waiting for further bytes to disambiguate it.

        This is the case right after a byte (such as ESC) that our keymap
        knows only as a prefix of a longer sequence has been pushed, but
        before the rest of that sequence arrives.  Consoles use this to
        decide whether the next read should block indefinitely or only for
        the escape timeout.
        """
        return self.keymap is not self.compiled_keymap

    def flush(self) -> None:
        """
        Finalize any pending, incomplete input as discrete events.
        """
        if self.keymap is self.compiled_keymap:
            # Nothing pending: either idle, or a complete key was already
            # emitted by ``push``.
            return
        buf = self.buf.take_bytes() # type: ignore[attr-defined]
        self.keymap = self.compiled_keymap
        if buf and buf[0] == 27:  # escape
            self.insert(Event('key', '\033', b'\033'))
            for _c in buf[1:]:
                self.push(_c)
        else:
            # Defensive: a pending prefix that does not start with ESC
            # (e.g. a partial multi-byte sequence). Emit it as text.
            data = bytes(buf).decode(self.encoding, 'replace')
            self.insert(Event('key', data, buf))

    def insert(self, event: Event) -> None:
        """
        Inserts an event into the queue.
        """
        trace('added event {event}', event=event)
        self.events.append(event)

    def push(self, char: int | bytes) -> None:
        """
        Processes a character by updating the buffer and handling special key mappings.
        """
        assert isinstance(char, (int, bytes))
        ord_char = char if isinstance(char, int) else ord(char)
        char = ord_char.to_bytes()
        self.buf.append(ord_char)

        if char in self.keymap:
            if self.keymap is self.compiled_keymap:
                # sanity check, buffer is empty when a special key comes
                assert len(self.buf) == 1
            k = self.keymap[char]
            trace('found map {k!r}', k=k)
            if isinstance(k, dict):
                self.keymap = k
            else:
                self.insert(Event('key', k, self.buf.take_bytes()))  # type: ignore[attr-defined]
                self.keymap = self.compiled_keymap

        elif self.buf and self.buf[0] == 27:  # escape
            # escape sequence not recognized by our keymap: propagate it
            # outside so that i can be recognized as an M-... key (see also
            # the docstring in keymap.py
            trace('unrecognized escape sequence, propagating...')
            self.keymap = self.compiled_keymap
            self.insert(Event('key', '\033', b'\033'))
            for _c in self.buf.take_bytes()[1:]:  # type: ignore[attr-defined]
                self.push(_c)

        else:
            try:
                decoded = self.buf.decode(self.encoding)
            except UnicodeError:
                return
            else:
                self.insert(Event('key', decoded, self.buf.take_bytes()))  # type: ignore[attr-defined]
            self.keymap = self.compiled_keymap
