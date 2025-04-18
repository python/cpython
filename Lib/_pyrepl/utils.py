import re
import unicodedata
import functools

from .types import CharBuffer, CharWidths
from .trace import trace

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})


@functools.cache
def str_width(c: str) -> int:
    if ord(c) < 128:
        return 1
    w = unicodedata.east_asian_width(c)
    if w in ("N", "Na", "H", "A"):
        return 1
    return 2


def wlen(s: str) -> int:
    if len(s) == 1 and s != "\x1a":
        return str_width(s)
    length = sum(str_width(i) for i in s)
    # remove lengths of any escape sequences
    sequence = ANSI_ESCAPE_SEQUENCE.findall(s)
    ctrl_z_cnt = s.count("\x1a")
    return length - sum(len(i) for i in sequence) + ctrl_z_cnt


def unbracket(s: str, including_content: bool = False) -> str:
    r"""Return `s` with \001 and \002 characters removed.

    If `including_content` is True, content between \001 and \002 is also
    stripped.
    """
    if including_content:
        return ZERO_WIDTH_BRACKET.sub("", s)
    return s.translate(ZERO_WIDTH_TRANS)


def disp_str(buffer: str) -> tuple[CharBuffer, CharWidths]:
    r"""Decompose the input buffer into a printable variant.

    Returns a tuple of two lists:
    - the first list is the input buffer, character by character;
    - the second list is the visible width of each character in the input
      buffer.

    Examples:
    >>> utils.disp_str("a = 9")
    (['a', ' ', '=', ' ', '9'], [1, 1, 1, 1, 1])
    """
    chars: CharBuffer = []
    char_widths: CharWidths = []

    if not buffer:
        return chars, char_widths

    for c in buffer:
        if c == "\x1a":  # CTRL-Z on Windows
            chars.append(c)
            char_widths.append(2)
        elif ord(c) < 128:
            chars.append(c)
            char_widths.append(1)
        elif unicodedata.category(c).startswith("C"):
            c = r"\u%04x" % ord(c)
            chars.append(c)
            char_widths.append(len(c))
        else:
            chars.append(c)
            char_widths.append(str_width(c))
    trace("disp_str({buffer}) = {s}, {b}", buffer=repr(buffer), s=chars, b=char_widths)
    return chars, char_widths
