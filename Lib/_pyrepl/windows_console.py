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
import sys

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from _pyrepl.console import Event, Console
from .trace import trace
import ctypes
from ctypes.wintypes import _COORD, WORD, SMALL_RECT, BOOL, HANDLE, CHAR, DWORD, WCHAR, SHORT
from ctypes import Structure, POINTER, Union
from ctypes import windll

if False:
    from typing import IO

class CONSOLE_SCREEN_BUFFER_INFO(Structure):
    _fields_ = [
        ('dwSize', _COORD),
        ('dwCursorPosition', _COORD),
        ('wAttributes', WORD),
        ('srWindow', SMALL_RECT),
        ('dwMaximumWindowSize', _COORD),
    ]

class CONSOLE_CURSOR_INFO(Structure):
    _fields_ = [
        ('dwSize', DWORD),
        ('bVisible', BOOL),
    ]

class CHAR_INFO(Structure):
    _fields_ = [
        ('UnicodeChar', WCHAR),
        ('Attributes', WORD),
    ]

STD_INPUT_HANDLE = -10
STD_OUTPUT_HANDLE = -11
GetStdHandle = windll.kernel32.GetStdHandle
GetStdHandle.argtypes = [ctypes.wintypes.DWORD]
GetStdHandle.restype = ctypes.wintypes.HANDLE

GetConsoleScreenBufferInfo = windll.kernel32.GetConsoleScreenBufferInfo
GetConsoleScreenBufferInfo.use_last_error = True
GetConsoleScreenBufferInfo.argtypes = [HANDLE, ctypes.POINTER(CONSOLE_SCREEN_BUFFER_INFO)]
GetConsoleScreenBufferInfo.restype = BOOL

SetConsoleCursorInfo = windll.kernel32.SetConsoleCursorInfo
SetConsoleCursorInfo.use_last_error = True
SetConsoleCursorInfo.argtypes = [HANDLE, POINTER(CONSOLE_CURSOR_INFO)]
SetConsoleCursorInfo.restype = BOOL

GetConsoleCursorInfo = windll.kernel32.GetConsoleCursorInfo
GetConsoleCursorInfo.use_last_error = True
GetConsoleCursorInfo.argtypes = [HANDLE, POINTER(CONSOLE_CURSOR_INFO)]
GetConsoleCursorInfo.restype = BOOL

SetConsoleCursorPosition = windll.kernel32.SetConsoleCursorPosition
SetConsoleCursorPosition.argtypes = [HANDLE, _COORD]
SetConsoleCursorPosition.restype = BOOL
SetConsoleCursorPosition.use_last_error = True

FillConsoleOutputCharacter = windll.kernel32.FillConsoleOutputCharacterW
FillConsoleOutputCharacter.use_last_error = True
FillConsoleOutputCharacter.argtypes = [HANDLE, CHAR, DWORD, _COORD, POINTER(DWORD)]
FillConsoleOutputCharacter.restype = BOOL

ScrollConsoleScreenBuffer = windll.kernel32.ScrollConsoleScreenBufferW
ScrollConsoleScreenBuffer.use_last_error = True
ScrollConsoleScreenBuffer.argtypes = [HANDLE, POINTER(SMALL_RECT), POINTER(SMALL_RECT), _COORD, POINTER(CHAR_INFO)]
ScrollConsoleScreenBuffer.restype = BOOL

SetConsoleMode = windll.kernel32.SetConsoleMode
SetConsoleMode.use_last_error = True
SetConsoleMode.argtypes = [HANDLE, DWORD]
SetConsoleMode.restype = BOOL

class Char(Union):
    _fields_ = [
        ("UnicodeChar",WCHAR),
        ("Char", CHAR), 
    ]

class KeyEvent(ctypes.Structure):
    _fields_ = [
        ("bKeyDown", BOOL),
        ("wRepeatCount", WORD),
        ("wVirtualKeyCode", WORD),
        ("wVirtualScanCode", WORD),
        ("uChar", Char),
        ("dwControlKeyState", DWORD),
    ]

class ConsoleEvent(ctypes.Union):
    _fields_ = [
        ("KeyEvent", KeyEvent),
#        ("MouseEvent", ),
#        ("WindowsBufferSizeEvent", ),
#        ("MenuEvent", )
#        ("FocusEvent", )
    ]


KEY_EVENT = 0x01
FOCUS_EVENT = 0x10
MENU_EVENT = 0x08
MOUSE_EVENT = 0x02
WINDOW_BUFFER_SIZE_EVENT = 0x04

class INPUT_RECORD(Structure):
    _fields_ = [
        ("EventType", WORD),
        ("Event", ConsoleEvent)
    ]

ReadConsoleInput = windll.kernel32.ReadConsoleInputW
ReadConsoleInput.use_last__error = True
ReadConsoleInput.argtypes = [HANDLE, POINTER(INPUT_RECORD), DWORD, POINTER(DWORD)]
ReadConsoleInput.restype = BOOL



OutHandle = GetStdHandle(STD_OUTPUT_HANDLE)
InHandle = GetStdHandle(STD_INPUT_HANDLE)

VK_MAP: dict[int, str] = {
    0x23: "end", # VK_END
    0x24: "home", # VK_HOME
    0x25: "left", # VK_LEFT
    0x26: "up", # VK_UP
    0x27: "right", # VK_RIGHT
    0x28: "down", # VK_DOWN
    0x2E: "delete", # VK_DELETE
    0x70: "f1", # VK_F1
    0x71: "f2", # VK_F2
    0x72: "f3", # VK_F3
    0x73: "f4", # VK_F4
    0x74: "f5", # VK_F5
    0x75: "f6", # VK_F6
    0x76: "f7", # VK_F7
    0x77: "f8", # VK_F8
    0x78: "f9", # VK_F9
    0x79: "f10", # VK_F10
    0x7A: "f11", # VK_F11
    0x7B: "f12", # VK_F12
    0x7C: "f13", # VK_F13
    0x7D: "f14", # VK_F14
    0x7E: "f15", # VK_F15
    0x7F: "f16", # VK_F16
    0x79: "f17", # VK_F17
    0x80: "f18", # VK_F18
    0x81: "f19", # VK_F19
    0x82: "f20", # VK_F20
}

ENABLE_PROCESSED_OUTPUT = 0x01
ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x04

class _error(Exception):
    pass

def wlen(s: str) -> int:
    return len(s)

class WindowsConsole(Console):
    def __init__(
        self,
        f_in: IO[bytes] | int = 0,
        f_out: IO[bytes] | int = 1,
        term: str = "",
        encoding: str = "",
    ):
        
        SetConsoleMode(OutHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        self.encoding = encoding or sys.getdefaultencoding()

        if isinstance(f_in, int):
            self.input_fd = f_in
        else:
            self.input_fd = f_in.fileno()

        if isinstance(f_out, int):
            self.output_fd = f_out
        else:
            self.output_fd = f_out.fileno()


    def refresh(self, screen, c_xy):
        """
        Refresh the console screen.

        Parameters:
        - screen (list): List of strings representing the screen contents.
        - c_xy (tuple): Cursor position (x, y) on the screen.
        """
        cx, cy = c_xy

        trace('!!Refresh {}', screen)
        while len(self.screen) < min(len(screen), self.height):
            trace("...")
            cur_x, cur_y = self.get_abs_position(0, len(self.screen) - 1)
            self.__hide_cursor()
            self.__move_relative(0, len(self.screen) - 1)
            self.__write("\n")
            self.__posxy = 0, len(self.screen)
            self.screen.append("")


        px, py = self.__posxy
        old_offset = offset = self.__offset
        height = self.height

        # we make sure the cursor is on the screen, and that we're
        # using all of the screen if we can
        if cy < offset:
            offset = cy
        elif cy >= offset + height:
            offset = cy - height + 1
            scroll_lines = offset - old_offset

            trace(f'adj offset {cy} {height} {offset} {old_offset}')
            # Scrolling the buffer as the current input is greater than the visible
            # portion of the window.  We need to scroll the visible portion and the
            # entire history
            info = CONSOLE_SCREEN_BUFFER_INFO()
            if not GetConsoleScreenBufferInfo(OutHandle, info):
                raise ctypes.WinError(ctypes.GetLastError())
            
            bottom = info.srWindow.Bottom

            trace("Scrolling {} {} {} {} {}", scroll_lines, info.srWindow.Bottom, self.height, len(screen) - self.height, self.__posxy)
            self.scroll(scroll_lines, bottom)
            self.__posxy = self.__posxy[0], self.__posxy[1] + scroll_lines
            self.__offset += scroll_lines
            
            for i in range(scroll_lines):
                self.screen.append("")
            
        elif offset > 0 and len(screen) < offset + height:
            trace("Adding extra line")
            offset = max(len(screen) - height, 0)
            #screen.append("")

        oldscr = self.screen[old_offset : old_offset + height]
        newscr = screen[offset : offset + height]
        trace('old screen {}', oldscr)
        trace('new screen {}', newscr)

        trace('new offset {} {}', offset, px)
        self.__offset = offset

        for (
            y,
            oldline,
            newline,
        ) in zip(range(offset, offset + height), oldscr, newscr):
            if oldline != newline:
                self.__write_changed_line(y, oldline, newline, px)

        y = len(newscr)
        while y < len(oldscr):
            self.__hide_cursor()
            self.__move_relative(0, y)
            self.__posxy = 0, y
            self.erase_to_end()
            y += 1

        self.__show_cursor()

        self.screen = screen
        trace("Done and moving")
        self.move_cursor(cx, cy)

    def scroll(self, top: int, bottom: int, left: int | None = None, right: int | None = None):
        scroll_rect = SMALL_RECT()
        scroll_rect.Top =  SHORT(top)
        scroll_rect.Bottom = SHORT(bottom)
        scroll_rect.Left = SHORT(0 if left is None else left)
        scroll_rect.Right = SHORT(self.getheightwidth()[1] - 1 if right is None else right)
        trace(f"Scrolling {scroll_rect.Top} {scroll_rect.Bottom}")
        destination_origin = _COORD()
        fill_info = CHAR_INFO()
        fill_info.UnicodeChar = ' '

        if not ScrollConsoleScreenBuffer(OutHandle, scroll_rect, None, destination_origin, fill_info):
            raise ctypes.WinError(ctypes.GetLastError())

    def __hide_cursor(self):
        info = CONSOLE_CURSOR_INFO()
        if not GetConsoleCursorInfo(OutHandle, info):
             raise ctypes.WinError(ctypes.GetLastError())

        info.bVisible = False
        if not SetConsoleCursorInfo(OutHandle, info):
             raise ctypes.WinError(ctypes.GetLastError())

    def __show_cursor(self):
        info = CONSOLE_CURSOR_INFO()
        if not GetConsoleCursorInfo(OutHandle, info):
             raise ctypes.WinError(ctypes.GetLastError())

        info.bVisible = True
        if not SetConsoleCursorInfo(OutHandle, info):
             raise ctypes.WinError(ctypes.GetLastError())

    def __write(self, text):
        os.write(self.output_fd, text.encode(self.encoding, "replace"))
        #self.__buffer.append((text, 0))

    def __move(self, x, y):
        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())
        x += info.dwCursorPosition.X
        y += info.dwCursorPosition.Y
        trace('..', x, y)
        self.move_cursor(x, y)

    @property
    def screen_xy(self):
        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())
        return info.dwCursorPosition.X, info.dwCursorPosition.Y

    def __write_changed_line(self, y: int, oldline: str, newline: str, px_coord):
        # this is frustrating; there's no reason to test (say)
        # self.dch1 inside the loop -- but alternative ways of
        # structuring this function are equally painful (I'm trying to
        # avoid writing code generators these days...)
        minlen = min(wlen(oldline), wlen(newline))
        x_pos = 0
        x_coord = 0

        px_pos = 0
        j = 0
        for c in oldline:
            if j >= px_coord: break
            j += wlen(c)
            px_pos += 1

        # reuse the oldline as much as possible, but stop as soon as we
        # encounter an ESCAPE, because it might be the start of an escape
        # sequene
        while x_coord < minlen and oldline[x_pos] == newline[x_pos] and newline[x_pos] != "\x1b":
            x_coord += wlen(newline[x_pos])
            x_pos += 1

        # if we need to insert a single character right after the first detected change
        if oldline[x_pos:] == newline[x_pos + 1 :]:
            trace('insert single')
            if (
                y == self.__posxy[1]
                and x_coord > self.__posxy[0]
                and oldline[px_pos:x_pos] == newline[px_pos + 1 : x_pos + 1]
            ):
                x_pos = px_pos
                x_coord = px_coord
            character_width = wlen(newline[x_pos])

            ins_x, ins_y = self.get_abs_position(x_coord + 1, y)
            ins_x -= 1
            scroll_rect = SMALL_RECT()
            scroll_rect.Top = scroll_rect.Bottom = SHORT(ins_y)
            scroll_rect.Left = ins_x
            scroll_rect.Right = self.getheightwidth()[1] - 1
            destination_origin = _COORD(X = scroll_rect.Left + 1, Y = scroll_rect.Top)
            fill_info = CHAR_INFO()
            fill_info.UnicodeChar = ' '

            if not ScrollConsoleScreenBuffer(OutHandle, scroll_rect, None, destination_origin, fill_info):
                raise ctypes.WinError(ctypes.GetLastError())

            self.__move_relative(x_coord, y)
            self.__write(newline[x_pos])
            self.__posxy = x_coord + character_width, y

        else:
            trace("Rewrite all {!r} {} {} {} {} {}", newline, x_coord, len(oldline), y, wlen(newline), self.__posxy)
            self.__hide_cursor()
            self.__move_relative(x_coord, y)
            if wlen(oldline) > wlen(newline):
                self.erase_to_end()
            #os.write(self.output_fd, newline[x_pos:].encode(self.encoding, "replace"))
            self.__write(newline[x_pos:])
            self.__posxy = wlen(newline), y


        if "\x1b" in newline:
            # ANSI escape characters are present, so we can't assume
            # anything about the position of the cursor.  Moving the cursor
            # to the left margin should work to get to a known position.
            _, cur_y = self.get_abs_position(0, y)
            self.__move_absolute(0, cur_y)
            self.__posxy = 0, y

    def erase_to_end(self):
        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())

        size = info.srWindow.Right - info.srWindow.Left + 1 - info.dwCursorPosition.X
        if not FillConsoleOutputCharacter(OutHandle, b' ',  size, info.dwCursorPosition, DWORD()):
            raise ctypes.WinError(ctypes.GetLastError())

    def prepare(self) -> None:
        self.screen = []
        self.height, self.width = self.getheightwidth()

        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())

        self.__posxy = 0, 0
        self.__gone_tall = 0
        self.__offset = 0

    def restore(self) -> None: ...

    def get_abs_position(self, x: int, y: int) -> tuple[int, int]:
        cur_x, cur_y = self.screen_xy
        dx = x - self.__posxy[0]
        dy = y - self.__posxy[1]
        cur_x += dx
        cur_y += dy
        return cur_x, cur_y

    def __move_relative(self, x, y):
        trace('move relative {} {} {} {}', x, y, self.__posxy, self.screen_xy)
        cur_x, cur_y = self.get_abs_position(x, y)
        trace('move is {} {}', cur_x, cur_y)
        self.__move_absolute(cur_x, cur_y)

    def __move_absolute(self, x, y):
        """Moves to an absolute location in the screen buffer"""
        if y < 0:
            trace(f"Negative offset: {self.__posxy} {self.screen_xy}")
        cord = _COORD()
        cord.X = x
        cord.Y = y
        if not SetConsoleCursorPosition(OutHandle, cord):
            raise ctypes.WinError(ctypes.GetLastError())

    def move_cursor(self, x: int, y: int) -> None:
        trace(f'move_cursor {x} {y}')

        if x < 0 or y < 0:
            raise ValueError(f"Bad cussor position {x}, {y}")

        self.__move_relative(x, y)
        self.__posxy = x, y

    def set_cursor_vis(self, visible: bool) -> None: 
        if visible:
            self.__show_cursor()
        else:
            self.__hide_cursor()

    def getheightwidth(self) -> tuple[int, int]:
        """Return (height, width) where height and width are the height
        and width of the terminal window in characters."""
        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())
        return (info.srWindow.Bottom - info.srWindow.Top + 1, 
                info.srWindow.Right - info.srWindow.Left + 1)
    
    def get_event(self, block: bool = True) -> Event | None:
        """Return an Event instance.  Returns None if |block| is false
        and there is no event pending, otherwise waits for the
        completion of an event."""
        while True:
            rec = INPUT_RECORD()
            read = DWORD()
            if not ReadConsoleInput(InHandle, rec, 1, read):
                raise ctypes.WinError(ctypes.GetLastError())
            
            if read.value == 0:
                if block:
                    continue
                return None
            
            if rec.EventType != KEY_EVENT or not rec.Event.KeyEvent.bKeyDown:
                # Only process keys and keydown events
                if block:
                    continue
                return None
            
            key = rec.Event.KeyEvent.uChar.UnicodeChar
            
            if rec.Event.KeyEvent.uChar.UnicodeChar == '\r':
                # Make enter make unix-like
                return Event(evt="key", data="\n", raw="\n")
            elif rec.Event.KeyEvent.wVirtualKeyCode == 8:
                # Turn backspace directly into the command
                return Event(evt="key", data="backspace", raw=rec.Event.KeyEvent.uChar.UnicodeChar)
            elif rec.Event.KeyEvent.wVirtualKeyCode == 27:
                # Turn escape directly into the command
                return Event(evt="key", data="escape", raw=rec.Event.KeyEvent.uChar.UnicodeChar)
            elif rec.Event.KeyEvent.uChar.UnicodeChar == '\x00':
                # Handle special keys like arrow keys and translate them into the appropriate command
                code = VK_MAP.get(rec.Event.KeyEvent.wVirtualKeyCode)
                if code:
                    return Event(evt="key", data=code, raw=rec.Event.KeyEvent.uChar.UnicodeChar) 
                if block:
                    continue

                return None

            return Event(evt="key", data=key, raw=rec.Event.KeyEvent.uChar.UnicodeChar)

    def push_char(self, char: int | bytes) -> None:
        """
        Push a character to the console event queue.
        """
        trace(f'put_char {char}')

    def beep(self) -> None: ...

    def clear(self) -> None:
        """Wipe the screen"""
        info = CONSOLE_SCREEN_BUFFER_INFO()
        if not GetConsoleScreenBufferInfo(OutHandle, info):
            raise ctypes.WinError(ctypes.GetLastError())
        size = info.dwSize.X * info.dwSize.Y
        if not FillConsoleOutputCharacter(OutHandle, b' ',  size, _COORD(), DWORD()):
            raise ctypes.WinError(ctypes.GetLastError())

    def finish(self) -> None:
        """Move the cursor to the end of the display and otherwise get
        ready for end.  XXX could be merged with restore?  Hmm."""
        y = len(self.screen) - 1
        while y >= 0 and not self.screen[y]:
            y -= 1
        self.__move_relative(0, min(y, self.height + self.__offset - 1))
        self.__write("\r\n")

    def flushoutput(self) -> None:
        """Flush all output to the screen (assuming there's some
        buffering going on somewhere).
        
        All output on Windows is unbuffered so this is a nop"""
        pass
    
    def forgetinput(self) -> None:
        """Forget all pending, but not yet processed input."""
        ...

    def getpending(self) -> Event:
        """Return the characters that have been typed but not yet
        processed."""
        return Event("key", "", b"")

    def wait(self) -> None:
        """Wait for an event."""
        ...

    def repaint(self) -> None:
        trace('repaint')
