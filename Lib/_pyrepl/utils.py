from __future__ import annotations
import builtins
import functools
import keyword
import re
import token as T
import tokenize
import unicodedata
import _colorize

from collections import deque
from io import StringIO
from tokenize import TokenInfo as TI
from typing import Iterable, Iterator, Match, NamedTuple, Self

from .types import CharBuffer, CharWidths
from .trace import trace

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})
IDENTIFIERS_AFTER = {"def", "class"}
KEYWORD_CONSTANTS = {"True", "False", "None"}
BUILTINS = {str(name) for name in dir(builtins) if not name.startswith('_')}


def THEME(**kwargs):
    # Not cached: the user can modify the theme inside the interactive session.
    return _colorize.get_theme(**kwargs).syntax


class Span(NamedTuple):
    """Span indexing that's inclusive on both ends."""

    start: int
    end: int

    @classmethod
    def from_re(cls, m: Match[str], group: int | str) -> Self:
        re_span = m.span(group)
        return cls(re_span[0], re_span[1] - 1)

    @classmethod
    def from_token(cls, token: TI, line_len: list[int]) -> Self:
        end_offset = -1
        if (token.type in {T.FSTRING_MIDDLE, T.TSTRING_MIDDLE}
            and token.string.endswith(("{", "}"))):
            # gh-134158: a visible trailing brace comes from a double brace in input
            end_offset += 1

        return cls(
            line_len[token.start[0] - 1] + token.start[1],
            line_len[token.end[0] - 1] + token.end[1] + end_offset,
        )


class ColorSpan(NamedTuple):
    span: Span
    tag: str


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
    sio = StringIO(buffer)
    line_lengths = [0] + [len(line) for line in sio.readlines()]
    # make line_lengths cumulative
    for i in range(1, len(line_lengths)):
        line_lengths[i] += line_lengths[i-1]

    sio.seek(0)
    gen = tokenize.generate_tokens(sio.readline)
    last_emitted: ColorSpan | None = None
    try:
        for color in gen_colors_from_token_stream(gen, line_lengths):
            yield color
            last_emitted = color
    except SyntaxError:
        return
    except tokenize.TokenError as te:
        yield from recover_unterminated_string(
            te, line_lengths, last_emitted, buffer
        )


def recover_unterminated_string(
    exc: tokenize.TokenError,
    line_lengths: list[int],
    last_emitted: ColorSpan | None,
    buffer: str,
) -> Iterator[ColorSpan]:
    msg, loc = exc.args
    if loc is None:
        return

    line_no, column = loc

    if msg.startswith(
        (
            "unterminated string literal",
            "unterminated f-string literal",
            "unterminated t-string literal",
            "EOF in multi-line string",
            "unterminated triple-quoted f-string literal",
            "unterminated triple-quoted t-string literal",
        )
    ):
        start = line_lengths[line_no - 1] + column - 1
        end = line_lengths[-1] - 1

        # in case FSTRING_START was already emitted
        if last_emitted and start <= last_emitted.span.start:
            trace("before last emitted = {s}", s=start)
            start = last_emitted.span.end + 1

        span = Span(start, end)
        trace("yielding span {a} -> {b}", a=span.start, b=span.end)
        yield ColorSpan(span, "string")
    else:
        trace(
            "unhandled token error({buffer}) = {te}",
            buffer=repr(buffer),
            te=str(exc),
        )


def gen_colors_from_token_stream(
    token_generator: Iterator[TI],
    line_lengths: list[int],
) -> Iterator[ColorSpan]:
    token_window = prev_next_window(token_generator)

    is_def_name = False
    bracket_level = 0
    for prev_token, token, next_token in token_window:
        assert token is not None
        if token.start == token.end:
            continue

        match token.type:
            case (
                T.STRING
                | T.FSTRING_START | T.FSTRING_MIDDLE | T.FSTRING_END
                | T.TSTRING_START | T.TSTRING_MIDDLE | T.TSTRING_END
            ):
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "string")
            case T.COMMENT:
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "comment")
            case T.NUMBER:
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "number")
            case T.OP:
                if token.string in "([{":
                    bracket_level += 1
                elif token.string in ")]}":
                    bracket_level -= 1
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "op")
            case T.NAME:
                if is_def_name:
                    is_def_name = False
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "definition")
                elif keyword.iskeyword(token.string):
                    span_cls = "keyword"
                    if token.string in KEYWORD_CONSTANTS:
                        span_cls = "keyword_constant"
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, span_cls)
                    if token.string in IDENTIFIERS_AFTER:
                        is_def_name = True
                elif (
                    keyword.issoftkeyword(token.string)
                    and bracket_level == 0
                    and is_soft_keyword_used(prev_token, token, next_token)
                ):
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "soft_keyword")
                elif (
                    token.string in BUILTINS
                    and not (prev_token and prev_token.exact_type == T.DOT)
                ):
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "builtin")


keyword_first_sets_match = {"False", "None", "True", "await", "lambda", "not"}
keyword_first_sets_case = {"False", "None", "True"}


def is_soft_keyword_used(*tokens: TI | None) -> bool:
    """Returns True if the current token is a keyword in this context.

    For the `*tokens` to match anything, they have to be a three-tuple of
    (previous, current, next).
    """
    trace("is_soft_keyword_used{t}", t=tokens)
    match tokens:
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(string=":"),
            TI(string="match"),
            TI(T.NUMBER | T.STRING | T.FSTRING_START | T.TSTRING_START)
            | TI(T.OP, string="(" | "*" | "[" | "{" | "~" | "...")
        ):
            return True
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(string=":"),
            TI(string="match"),
            TI(T.NAME, string=s)
        ):
            if keyword.iskeyword(s):
                return s in keyword_first_sets_match
            return True
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(T.DEDENT) | TI(string=":"),
            TI(string="case"),
            TI(T.NUMBER | T.STRING | T.FSTRING_START | T.TSTRING_START)
            | TI(T.OP, string="(" | "*" | "-" | "[" | "{")
        ):
            return True
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(T.DEDENT) | TI(string=":"),
            TI(string="case"),
            TI(T.NAME, string=s)
        ):
            if keyword.iskeyword(s):
                return s in keyword_first_sets_case
            return True
        case (TI(string="case"), TI(string="_"), TI(string=":")):
            return True
        case _:
            return False


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
