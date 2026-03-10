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

from dataclasses import dataclass

from .console import Event
from .terminfo import TermInfo
from .trace import trace
from .base_eventqueue import BaseEventQueue
from termios import tcgetattr, VERASE
import os


# Mapping of human-readable key names to their terminal-specific codes
TERMINAL_KEYNAMES = {
    "delete": "kdch1",
    "down": "kcud1",
    "end": "kend",
    "enter": "kent",
    "home": "khome",
    "insert": "kich1",
    "left": "kcub1",
    "page down": "knp",
    "page up": "kpp",
    "right": "kcuf1",
    "up": "kcuu1",
}


# Function keys F1-F20 mapping
TERMINAL_KEYNAMES.update(("f%d" % i, "kf%d" % i) for i in range(1, 21))

# Known CTRL-arrow keycodes
CTRL_ARROW_KEYCODES= {
    # for xterm, gnome-terminal, xfce terminal, etc.
    b'\033[1;5D': 'ctrl left',
    b'\033[1;5C': 'ctrl right',
    # for rxvt
    b'\033Od': 'ctrl left',
    b'\033Oc': 'ctrl right',
}

KITTY_MOD_SHIFT = 0x01
KITTY_MOD_ALT = 0x02
KITTY_MOD_CTRL = 0x04
KITTY_MOD_SUPER = 0x08
KITTY_MOD_HYPER = 0x10
KITTY_MOD_META = 0x20
KITTY_MOD_CAPS_LOCK = 0x40
KITTY_MOD_NUM_LOCK = 0x80

KITTY_EVENT_PRESS = 1
KITTY_EVENT_REPEAT = 2
KITTY_EVENT_RELEASE = 3

KITTY_CSI_FINAL_BYTES = frozenset(b"~uABCDEFHPQS")
KITTY_FUNCTIONAL_TILDE_KEYS = {
    2: "insert",
    3: "delete",
    5: "page up",
    6: "page down",
    7: "home",
    8: "end",
    11: "f1",
    12: "f2",
    13: "f3",
    14: "f4",
    15: "f5",
    17: "f6",
    18: "f7",
    19: "f8",
    20: "f9",
    21: "f10",
    23: "f11",
    24: "f12",
    57427: "begin",
}
KITTY_FUNCTIONAL_LETTER_KEYS = {
    "A": "up",
    "B": "down",
    "C": "right",
    "D": "left",
    "E": "begin",
    "F": "end",
    "H": "home",
    "P": "f1",
    "Q": "f2",
    "S": "f4",
}
KITTY_FUNCTIONAL_U_KEYS = {
    9: "\t",
    13: "\r",
    27: "\033",
    127: "backspace",
    57358: "caps_lock",
    57359: "scroll_lock",
    57360: "num_lock",
}
KITTY_FUNCTIONAL_U_KEYS.update((57363 + i, f"f{i}") for i in range(13, 36))
KITTY_IGNORED_FUNCTIONAL_U_KEYS = frozenset(range(57399, 57455))
KITTY_CTRL_KEY_OVERRIDES = {
    ord(" "): 0,
    ord("2"): 0,
    ord("@"): 0,
    ord("3"): 27,
    ord("["): 27,
    ord("4"): 28,
    ord("\\"): 28,
    ord("5"): 29,
    ord("]"): 29,
    ord("6"): 30,
    ord("^"): 30,
    ord("7"): 31,
    ord("-"): 31,
    ord("_"): 31,
    ord("/"): 31,
    ord("8"): 127,
    ord("?"): 127,
}


@dataclass(slots=True)
class _KittyKeyEvent:
    key_code: int
    shifted_key: int | None = None
    base_layout_key: int | None = None
    text: str = ""
    modifiers: int = 0
    event_type: int = KITTY_EVENT_PRESS
    key_name: str | None = None

def get_terminal_keycodes(ti: TermInfo) -> dict[bytes, str]:
    """
    Generates a dictionary mapping terminal keycodes to human-readable names.
    """
    keycodes = {}
    for key, terminal_code in TERMINAL_KEYNAMES.items():
        keycode = ti.get(terminal_code)
        trace('key {key} tiname {terminal_code} keycode {keycode!r}', **locals())
        if keycode:
            keycodes[keycode] = key
    keycodes.update(CTRL_ARROW_KEYCODES)
    return keycodes


class EventQueue(BaseEventQueue):
    def __init__(self, fd: int, encoding: str, ti: TermInfo) -> None:
        keycodes = get_terminal_keycodes(ti)
        if os.isatty(fd):
            backspace = tcgetattr(fd)[6][VERASE]
            keycodes[backspace] = "backspace"
        BaseEventQueue.__init__(self, encoding, keycodes)
        self._escape_buffer = bytearray()

    def push(self, char: int | bytes) -> None:
        assert isinstance(char, (int, bytes))
        ord_char = char if isinstance(char, int) else ord(char)

        if self.keymap is not self.compiled_keymap:
            super().push(ord_char)
            return

        if self._escape_buffer:
            self._escape_buffer.append(ord_char)
            self._process_escape_buffer()
            return

        if ord_char == 0x1b:
            self._escape_buffer.append(ord_char)
            return

        super().push(ord_char)

    def _process_escape_buffer(self) -> None:
        if len(self._escape_buffer) < 2:
            return

        if self._escape_buffer[1] != ord("["):
            self._flush_escape_buffer()
            return

        final = self._escape_buffer[-1]
        if len(self._escape_buffer) > 2 and 0x40 <= final <= 0x7E:
            seq = bytes(self._escape_buffer)
            self._escape_buffer.clear()
            if final in KITTY_CSI_FINAL_BYTES and self._handle_kitty_sequence(seq):
                return
            self._push_bytes(seq)

    def _flush_escape_buffer(self) -> None:
        if self._escape_buffer:
            self._push_bytes(bytes(self._escape_buffer))
            self._escape_buffer.clear()

    def _push_bytes(self, data: bytes) -> None:
        for byte in data:
            super().push(byte)

    def _handle_kitty_sequence(self, seq: bytes) -> bool:
        params = seq[2:-1].decode("ascii", "strict")
        final = chr(seq[-1])

        try:
            if final == "u":
                event = self._parse_kitty_u(params)
            elif final == "~":
                event = self._parse_kitty_tilde(params)
            else:
                event = self._parse_kitty_letter(params, final)
        except ValueError:
            return False

        if event is None:
            return False
        return self._emit_kitty_event(event, seq)

    def _parse_kitty_u(self, params: str) -> _KittyKeyEvent | None:
        fields = params.split(";")
        if not fields or not fields[0]:
            raise ValueError

        key_fields = fields[0].split(":")
        key_code = int(key_fields[0])
        shifted_key = int(key_fields[1]) if len(key_fields) > 1 and key_fields[1] else None
        base_layout_key = int(key_fields[2]) if len(key_fields) > 2 and key_fields[2] else None

        modifiers = 0
        event_type = KITTY_EVENT_PRESS
        if len(fields) > 1 and fields[1]:
            modifier_fields = fields[1].split(":")
            modifiers = self._decode_kitty_modifiers(modifier_fields[0])
            if len(modifier_fields) > 1 and modifier_fields[1]:
                event_type = int(modifier_fields[1])

        text = ""
        if len(fields) > 2 and fields[2]:
            text = "".join(chr(int(cp)) for cp in fields[2].split(":") if cp)

        key_name = KITTY_FUNCTIONAL_U_KEYS.get(key_code)
        if key_name is None and key_code in KITTY_IGNORED_FUNCTIONAL_U_KEYS:
            key_name = ""

        return _KittyKeyEvent(
            key_code=key_code,
            shifted_key=shifted_key,
            base_layout_key=base_layout_key,
            text=text,
            modifiers=modifiers,
            event_type=event_type,
            key_name=key_name,
        )

    def _parse_kitty_tilde(self, params: str) -> _KittyKeyEvent | None:
        fields = params.split(";")
        if not fields or not fields[0]:
            raise ValueError
        key_code = int(fields[0])
        key_name = KITTY_FUNCTIONAL_TILDE_KEYS.get(key_code)
        if key_name is None:
            return None
        modifiers = self._decode_kitty_modifiers(fields[1]) if len(fields) > 1 and fields[1] else 0
        return _KittyKeyEvent(key_code=key_code, modifiers=modifiers, key_name=key_name)

    def _parse_kitty_letter(self, params: str, final: str) -> _KittyKeyEvent | None:
        key_name = KITTY_FUNCTIONAL_LETTER_KEYS.get(final)
        if key_name is None:
            return None
        modifiers = 0
        if params:
            fields = params.split(";")
            if fields[0] not in ("", "1"):
                return None
            if len(fields) > 1 and fields[1]:
                modifiers = self._decode_kitty_modifiers(fields[1])
        return _KittyKeyEvent(key_code=1, modifiers=modifiers, key_name=key_name)

    def _decode_kitty_modifiers(self, value: str) -> int:
        modifier_value = int(value)
        return max(modifier_value - 1, 0)

    def _emit_kitty_event(self, event: _KittyKeyEvent, raw: bytes) -> bool:
        if event.event_type == KITTY_EVENT_RELEASE:
            return True

        keys = self._translate_kitty_event(event)
        for key in keys:
            self.insert(Event("key", key, raw))
        return True

    def _translate_kitty_event(self, event: _KittyKeyEvent) -> list[str]:
        if event.key_name == "":
            return []

        modifiers = event.modifiers
        key_name = event.key_name

        if key_name in {"caps_lock", "scroll_lock", "num_lock"}:
            return []

        if key_name is not None:
            if modifiers & KITTY_MOD_ALT:
                return self._prefix_alt([key_name])
            ctrl_key = self._maybe_ctrl_special_key(key_name, modifiers)
            return [ctrl_key] if ctrl_key is not None else [key_name]

        text = event.text
        if not text:
            text = self._kitty_key_text(event)
        if not text:
            return []

        if modifiers & KITTY_MOD_CTRL:
            text = self._apply_ctrl_to_text(text, event)
        if modifiers & KITTY_MOD_ALT:
            return self._prefix_alt(list(text))
        return list(text)

    def _maybe_ctrl_special_key(self, key_name: str, modifiers: int) -> str | None:
        if not modifiers & KITTY_MOD_CTRL:
            return None
        if key_name in {"left", "right"}:
            return f"ctrl {key_name}"
        return None

    def _kitty_key_text(self, event: _KittyKeyEvent) -> str:
        codepoint = event.base_layout_key or event.key_code
        if event.modifiers & KITTY_MOD_SHIFT and event.shifted_key is not None:
            codepoint = event.shifted_key
        try:
            return chr(codepoint)
        except ValueError:
            return ""

    def _apply_ctrl_to_text(self, text: str, event: _KittyKeyEvent) -> str:
        codepoint = event.base_layout_key or event.key_code
        if codepoint in KITTY_CTRL_KEY_OVERRIDES:
            return chr(KITTY_CTRL_KEY_OVERRIDES[codepoint])

        if not text:
            return ""

        char = text[0]
        if "a" <= char <= "z" or "A" <= char <= "Z":
            return chr(ord(char.upper()) & 0x1F)
        return char

    def _prefix_alt(self, keys: list[str]) -> list[str]:
        return ["\033", *keys]
