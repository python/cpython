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

from collections import deque

from . import keymap
from .console import Event
from .trace import trace

class BaseEventQueue:
    _ESCAPE_TIMEOUT_MS = 50

    def __init__(self, encoding: str, keymap_dict: dict[bytes, str]) -> None:
        self.compiled_keymap = keymap.compile_keymap(keymap_dict)
        self.keymap = self.compiled_keymap
        trace("keymap {k!r}", k=self.keymap)
        self.encoding = encoding
        self.events: deque[Event] = deque()
        self.buf = bytearray()
        self._pending_escape_deadline: float | None = None

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

    def flush_buf(self) -> bytearray:
        """
        Flushes the buffer and returns its contents.
        """
        old = self.buf
        self.buf = bytearray()
        return old

    def insert(self, event: Event) -> None:
        """
        Inserts an event into the queue.
        """
        trace('added event {event}', event=event)
        self.events.append(event)

    def has_pending_escape_sequence(self) -> bool:
        """
        Check if there's a potential escape sequence waiting for more input.

        Returns True if we have exactly one byte (ESC) in the buffer and
        we're in the middle of keymap navigation, indicating we're waiting
        to see if more bytes will arrive to complete an escape sequence.
        """
        return (
            len(self.buf) == 1
            and self.buf[0] == 27  # ESC byte
            and self.keymap is not self.compiled_keymap
        )

    def should_emit_standalone_escape(self, current_time_ms: float) -> bool:
        """
        Check if a pending ESC should be emitted as a standalone escape key.
        """
        if not self.has_pending_escape_sequence():
            return False

        if self._pending_escape_deadline is None:
            self._pending_escape_deadline = current_time_ms + self._ESCAPE_TIMEOUT_MS
            return False

        return current_time_ms >= self._pending_escape_deadline

    def emit_standalone_escape(self) -> None:
        """
        Emit the buffered ESC byte as a standalone escape key event.
        """
        self.keymap = self.compiled_keymap
        # Standalone ESC event
        self.insert(Event('key', '\033', b'\033'))

        # Just in case there are remaining bytes in the buffer
        remaining = self.flush_buf()[1:]
        for byte in remaining:
            self.push(byte)

        self._pending_escape_deadline = None

    def push(self, char: int | bytes) -> None:
        """
        Processes a character by updating the buffer and handling special key mappings.
        """
        assert isinstance(char, (int, bytes))
        ord_char = char if isinstance(char, int) else ord(char)
        char = ord_char.to_bytes()
        self.buf.append(ord_char)

        if self._pending_escape_deadline is not None:
            self._pending_escape_deadline = None

        if char in self.keymap:
            if self.keymap is self.compiled_keymap:
                # sanity check, buffer is empty when a special key comes
                assert len(self.buf) == 1
            k = self.keymap[char]
            trace('found map {k!r}', k=k)
            if isinstance(k, dict):
                self.keymap = k
            else:
                self.insert(Event('key', k, bytes(self.flush_buf())))
                self.keymap = self.compiled_keymap

        elif self.buf and self.buf[0] == 27:  # escape
            # escape sequence not recognized by our keymap: propagate it
            # outside so that i can be recognized as an M-... key (see also
            # the docstring in keymap.py
            trace('unrecognized escape sequence, propagating...')
            self.keymap = self.compiled_keymap
            self.insert(Event('key', '\033', b'\033'))
            for _c in self.flush_buf()[1:]:
                self.push(_c)

        else:
            try:
                decoded = bytes(self.buf).decode(self.encoding)
            except UnicodeError:
                return
            else:
                self.insert(Event('key', decoded, bytes(self.flush_buf())))
            self.keymap = self.compiled_keymap
