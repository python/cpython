import functools
import re
import unicodedata
import _colorize

from _colorize import ColorSpan
from collections.abc import Iterator
from dataclasses import dataclass
from typing import NamedTuple, Self

from .types import CharBuffer, CharWidths


ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})


def THEME(**kwargs):
    # Not cached: the user can modify the theme inside the interactive session.
    return _colorize.get_theme(**kwargs).syntax


class StyledChar(NamedTuple):
    text: str
    width: int
    tag: str | None = None


def _ascii_control_repr(c: str) -> str | None:
    code = ord(c)
    if code < 32:
        return "^" + chr(code + 64)
    if code == 127:
        return "^?"
    return None


@functools.cache
def str_width(c: str) -> int:
    if ord(c) < 128:
        return 1
    # gh-139246 for zero-width joiner and combining characters
    if unicodedata.combining(c):
        return 0
    category = unicodedata.category(c)
    if category == "Cf" and c != "\u00ad":
        return 0
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


def iter_display_chars(
    buffer: str,
    colors: list[ColorSpan] | None = None,
    start_index: int = 0,
) -> Iterator[StyledChar]:
    """Yield visible display characters with widths and semantic color tags.

    Note: ``colors`` is consumed in place as spans are processed -- callers
    that split a buffer across multiple calls rely on this mutation to track
    which spans have already been handled.
    """

    if not buffer:
        return

    color_idx = 0
    if colors:
        while color_idx < len(colors) and colors[color_idx].span.end < start_index:
            color_idx += 1

    active_tag = None
    if colors and color_idx < len(colors) and colors[color_idx].span.start < start_index:
        active_tag = colors[color_idx].tag

    for i, c in enumerate(buffer, start_index):
        if colors and color_idx < len(colors) and colors[color_idx].span.start == i:
            active_tag = colors[color_idx].tag

        if control := _ascii_control_repr(c):
            text = control
            width = len(control)
        elif ord(c) < 128:
            text = c
            width = 1
        elif unicodedata.category(c).startswith("C"):
            text = r"\u%04x" % ord(c)
            width = len(text)
        else:
            text = c
            width = str_width(c)

        yield StyledChar(text, width, active_tag)

        if colors and color_idx < len(colors) and colors[color_idx].span.end == i:
            color_idx += 1
            active_tag = None
            # Check if the next span starts at the same position
            if color_idx < len(colors) and colors[color_idx].span.start == i:
                active_tag = colors[color_idx].tag

    # Remove consumed spans so callers see the mutation
    if color_idx > 0 and colors:
        del colors[:color_idx]


def disp_str(
    buffer: str,
    colors: list[ColorSpan] | None = None,
    start_index: int = 0,
    force_color: bool = False,
) -> tuple[CharBuffer, CharWidths]:
    r"""Decompose the input buffer into a printable variant with applied colors.

    Returns a tuple of two lists:
    - the first list is the input buffer, character by character, with color
      escape codes added (while those codes contain multiple ASCII characters,
      each code is considered atomic *and is attached for the corresponding
      visible character*);
    - the second list is the visible width of each character in the input
      buffer.

    Note on colors:
    - The `colors` list, if provided, is partially consumed within. We're using
      a list and not a generator since we need to hold onto the current
      unfinished span between calls to disp_str in case of multiline strings.
    - The `colors` list is computed from the start of the input block. `buffer`
      is only a subset of that input block, a single line within. This is why
      we need `start_index` to inform us which position is the start of `buffer`
      actually within user input. This allows us to match color spans correctly.

    Examples:
    >>> utils.disp_str("a = 9")
    (['a', ' ', '=', ' ', '9'], [1, 1, 1, 1, 1])

    >>> line = "while 1:"
    >>> colors = list(utils.gen_colors(line))
    >>> utils.disp_str(line, colors=colors)
    (['\x1b[1;34mw', 'h', 'i', 'l', 'e\x1b[0m', ' ', '1', ':'], [1, 1, 1, 1, 1, 1, 1, 1])

    """
    styled_chars = list(iter_display_chars(buffer, colors, start_index))
    chars: CharBuffer = []
    char_widths: CharWidths = []
    theme = THEME(force_color=force_color)

    for index, styled_char in enumerate(styled_chars):
        previous_tag = styled_chars[index - 1].tag if index else None
        next_tag = styled_chars[index + 1].tag if index + 1 < len(styled_chars) else None
        prefix = theme[styled_char.tag] if styled_char.tag and styled_char.tag != previous_tag else ""
        suffix = theme.reset if styled_char.tag and styled_char.tag != next_tag else ""
        chars.append(prefix + styled_char.text + suffix)
        char_widths.append(styled_char.width)

    return chars, char_widths


@dataclass(frozen=True, slots=True)
class StyleRef:
    tag: str | None = None  # From THEME().syntax, e.g. "keyword", "builtin"
    sgr: str = ""

    @classmethod
    def from_tag(cls, tag: str, sgr: str = "") -> Self:
        return cls(tag=tag, sgr=sgr)

    @classmethod
    def from_sgr(cls, sgr: str) -> Self:
        if not sgr:
            return cls()
        return cls(sgr=sgr)

    @property
    def is_plain(self) -> bool:
        return self.tag is None and not self.sgr
