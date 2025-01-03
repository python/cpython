"""
Windows event and VT sequence scanner, similar to `unix_eventqueue.py`
"""

from collections import deque

from . import keymap
from .console import Event
from .trace import trace
import os

# Reference: https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#input-sequences
VT_MAP: dict[bytes, str] = {
    b'\x1b[A': 'up',
    b'\x1b[B': 'down',
    b'\x1b[C': 'right',
    b'\x1b[D': 'left',
    b'\x1b[1;5D': 'ctrl left',
    b'\x1b[1;5C': 'ctrl right',

    b'\x1b[H': 'home',
    b'\x1b[F': 'end',

    b'\x7f': 'backspace',
    b'\x1b[2~': 'insert',
    b'\x1b[3~': 'delete',
    b'\x1b[5~': 'page up',
    b'\x1b[6~': 'page down',

    b'\x1bOP':    'f1',
    b'\x1bOQ':    'f2',
    b'\x1bOR':    'f3',
    b'\x1bOS':    'f4',
    b'\x1b[15~':  'f5',
    b'\x1b[17~]': 'f6',
    b'\x1b[18~]': 'f7',
    b'\x1b[19~]': 'f8',
    b'\x1b[20~]': 'f9',
    b'\x1b[21~]': 'f10',
    b'\x1b[23~]': 'f11',
    b'\x1b[24~]': 'f12',
}

class EventQueue:
    def __init__(self, encoding: str) -> None:
        self.compiled_keymap = keymap.compile_keymap(VT_MAP)
        self.keymap = self.compiled_keymap
        trace("keymap {k!r}", k=self.keymap)
        self.encoding = encoding
        self.events: deque[Event] = deque()
        self.buf = bytearray()

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

    def push(self, char: int | bytes) -> None:
        """
        Processes a character by updating the buffer and handling special key mappings.
        """
        ord_char = char if isinstance(char, int) else ord(char)
        char = bytes(bytearray((ord_char,)))
        self.buf.append(ord_char)
        if char in self.keymap:
            if self.keymap is self.compiled_keymap:
                #sanity check, buffer is empty when a special key comes
                assert len(self.buf) == 1
            k = self.keymap[char]
            trace('found map {k!r}', k=k)
            if isinstance(k, dict):
                self.keymap = k
            else:
                self.insert(Event('key', k, self.flush_buf()))
                self.keymap = self.compiled_keymap

        elif self.buf and self.buf[0] == 27:  # escape
            # escape sequence not recognized by our keymap: propagate it
            # outside so that i can be recognized as an M-... key (see also
            # the docstring in keymap.py
            trace('unrecognized escape sequence, propagating...')
            self.keymap = self.compiled_keymap
            self.insert(Event('key', '\033', bytearray(b'\033')))
            for _c in self.flush_buf()[1:]:
                self.push(_c)

        else:
            try:
                decoded = bytes(self.buf).decode(self.encoding)
            except UnicodeError:
                return
            else:
                self.insert(Event('key', decoded, self.flush_buf()))
            self.keymap = self.compiled_keymap
