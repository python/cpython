"""
Windows event and VT sequence scanner
"""

from .base_eventqueue import BaseEventQueue


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

    b'\x1bOP':   'f1',
    b'\x1bOQ':   'f2',
    b'\x1bOR':   'f3',
    b'\x1bOS':   'f4',
    b'\x1b[15~': 'f5',
    b'\x1b[17~': 'f6',
    b'\x1b[18~': 'f7',
    b'\x1b[19~': 'f8',
    b'\x1b[20~': 'f9',
    b'\x1b[21~': 'f10',
    b'\x1b[23~': 'f11',
    b'\x1b[24~': 'f12',
}

class EventQueue(BaseEventQueue):
    def __init__(self, encoding: str) -> None:
        BaseEventQueue.__init__(self, encoding, VT_MAP)
