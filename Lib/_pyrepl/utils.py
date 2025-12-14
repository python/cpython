from __future__ import annotations
import functools
import re
import unicodedata
import _colorize

from collections import deque
from typing import Iterable, Iterator

from .types import CharBuffer, CharWidths

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})

# Re-export from _colorize for backward compatibility
gen_colors = _colorize._gen_colors
ColorSpan = _colorize._ColorSpan
Span = _colorize._Span


def THEME(**kwargs):
    # Not cached: the user can modify the theme inside the interactive session.
    return _colorize.get_theme(**kwargs).syntax


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
    chars: CharBuffer = []
    char_widths: CharWidths = []

    if not buffer:
        return chars, char_widths

    while colors and colors[0].span.end < start_index:
        # move past irrelevant spans
        colors.pop(0)

    theme = THEME(force_color=force_color)
    pre_color = ""
    post_color = ""
    if colors and colors[0].span.start < start_index:
        # looks like we're continuing a previous color (e.g. a multiline str)
        pre_color = theme[colors[0].tag]

    for i, c in enumerate(buffer, start_index):
        if colors and colors[0].span.start == i:  # new color starts now
            pre_color = theme[colors[0].tag]

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

        if colors and colors[0].span.end == i:  # current color ends now
            post_color = theme.reset
            colors.pop(0)

        chars[-1] = pre_color + chars[-1] + post_color
        pre_color = ""
        post_color = ""

    if colors and colors[0].span.start < i and colors[0].span.end > i:
        # even though the current color should be continued, reset it for now.
        # the next call to `disp_str()` will revive it.
        chars[-1] += theme.reset

    return chars, char_widths


def prev_next_window[T](
    iterable: Iterable[T]
) -> Iterator[tuple[T | None, ...]]:
    """Generates three-tuples of (previous, current, next) items.

    On the first iteration previous is None. On the last iteration next
    is None. In case of exception next is None and the exception is re-raised
    on a subsequent next() call.

    Inspired by `sliding_window` from `itertools` recipes.
    """

    iterator = iter(iterable)
    window = deque((None, next(iterator)), maxlen=3)
    try:
        for x in iterator:
            window.append(x)
            yield tuple(window)
    except Exception:
        raise
    finally:
        window.append(None)
        yield tuple(window)
