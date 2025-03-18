import re
import unicodedata
import functools

from idlelib import colorizer
from typing import cast, Iterator, Literal, Match, NamedTuple, Pattern, Self
from _colorize import ANSIColors

from .types import CharBuffer, CharWidths
from .trace import trace

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})
COLORIZE_RE: Pattern[str] = colorizer.prog
IDENTIFIER_RE: Pattern[str] = colorizer.idprog
IDENTIFIERS_AFTER = {"def", "class"}
COLORIZE_GROUP_NAME_MAP: dict[str, str] = colorizer.prog_group_name_to_tag

type ColorTag = (
    Literal["KEYWORD"]
    | Literal["BUILTIN"]
    | Literal["COMMENT"]
    | Literal["STRING"]
    | Literal["DEFINITION"]
    | Literal["SYNC"]
)


class Span(NamedTuple):
    """Span indexing that's inclusive on both ends."""

    start: int
    end: int

    @classmethod
    def from_re(cls, m: Match[str], group: int | str) -> Self:
        re_span = m.span(group)
        return cls(re_span[0], re_span[1] - 1)


class ColorSpan(NamedTuple):
    span: Span
    tag: ColorTag


TAG_TO_ANSI: dict[ColorTag, str] = {
    "KEYWORD": ANSIColors.BOLD_BLUE,
    "BUILTIN": ANSIColors.CYAN,
    "COMMENT": ANSIColors.RED,
    "STRING": ANSIColors.GREEN,
    "DEFINITION": ANSIColors.BOLD_WHITE,
    "SYNC": ANSIColors.RESET,
}


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


def gen_colors(buffer: str) -> Iterator[ColorSpan]:
    """Returns a list of index spans to color using the given color tag.

    The input `buffer` should be a valid start of a Python code block, i.e.
    it cannot be a block starting in the middle of a multiline string.
    """
    for match in COLORIZE_RE.finditer(buffer):
        yield from gen_color_spans(match)


def gen_color_spans(re_match: Match[str]) -> Iterator[ColorSpan]:
    """Generate non-empty color spans."""
    for tag, data in re_match.groupdict().items():
        if not data:
            continue
        span = Span.from_re(re_match, tag)
        tag = COLORIZE_GROUP_NAME_MAP.get(tag, tag)
        yield ColorSpan(span, cast(ColorTag, tag))
        if data in IDENTIFIERS_AFTER:
            if name_match := IDENTIFIER_RE.match(re_match.string, span.end + 1):
                span = Span.from_re(name_match, 1)
                yield ColorSpan(span, "DEFINITION")


def disp_str(
    buffer: str, colors: list[ColorSpan] | None = None, start_index: int = 0
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

    pre_color = ""
    post_color = ""
    if colors and colors[0].span.start < start_index:
        # looks like we're continuing a previous color (e.g. a multiline str)
        pre_color = TAG_TO_ANSI[colors[0].tag]

    for i, c in enumerate(buffer, start_index):
        if colors and colors[0].span.start == i:  # new color starts now
            pre_color = TAG_TO_ANSI[colors[0].tag]

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
            post_color = TAG_TO_ANSI["SYNC"]
            colors.pop(0)

        chars[-1] = pre_color + chars[-1] + post_color
        pre_color = ""
        post_color = ""

    if colors and colors[0].span.start < i and colors[0].span.end > i:
        # even though the current color should be continued, reset it for now.
        # the next call to `disp_str()` will revive it.
        chars[-1] += TAG_TO_ANSI["SYNC"]

    trace("disp_str({buffer}) = {s}, {b}", buffer=repr(buffer), s=chars, b=char_widths)
    return chars, char_widths
