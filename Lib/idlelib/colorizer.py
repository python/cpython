import builtins
import keyword
import re
import time
import token as T
import tokenize
from collections import deque
from io import StringIO
from tokenize import TokenInfo as TI
from typing import Iterable, Iterator, Match, NamedTuple, Self

from idlelib.config import idleConf
from idlelib.delegator import Delegator

DEBUG = False


ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")
ZERO_WIDTH_BRACKET = re.compile(r"\x01.*?\x02")
ZERO_WIDTH_TRANS = str.maketrans({"\x01": "", "\x02": ""})
IDENTIFIERS_AFTER = {"def", "class"}
KEYWORD_CONSTANTS = {"True", "False", "None"}
BUILTINS = {str(name) for name in dir(builtins) if not name.startswith('_')}


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


keyword_first_sets_match = {"False", "None", "True", "await", "lambda", "not"}
keyword_first_sets_case = {"False", "None", "True"}


def is_soft_keyword_used(*tokens: TI | None) -> bool:
    """Returns True if the current token is a keyword in this context.

    For the `*tokens` to match anything, they have to be a three-tuple of
    (previous, current, next).
    """
    #trace("is_soft_keyword_used{t}", t=tokens)
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
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(T.DEDENT) | TI(string=":"),
            TI(string="type"),
            TI(T.NAME, string=s)
        ):
            return not keyword.iskeyword(s)
        case (
            None | TI(T.NEWLINE) | TI(T.INDENT) | TI(T.DEDENT) | TI(string=":"),
            TI(string="match" | "case" | "type"),
            None | TI(T.ENDMARKER) | TI(T.NEWLINE)
        ):
            return True
        case _:
            return False


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
            start = last_emitted.span.end + 1

        span = Span(start, end)
        yield ColorSpan(span, "STRING")


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
                yield ColorSpan(span, "STRING")
            case T.COMMENT:
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "COMMENT")
            case T.NEWLINE:
                span = Span.from_token(token, line_lengths)
                yield ColorSpan(span, "SYNC")
            case T.OP:
                if token.string in "([{":
                    bracket_level += 1
                elif token.string in ")]}":
                    bracket_level -= 1
                # span = Span.from_token(token, line_lengths)
                # yield ColorSpan(span, "op")
            case T.NAME:
                if is_def_name:
                    is_def_name = False
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "DEFINITION")
                elif keyword.iskeyword(token.string):
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "KEYWORD")
                    if token.string in IDENTIFIERS_AFTER:
                        is_def_name = True
                elif (
                    keyword.issoftkeyword(token.string)
                    and bracket_level == 0
                    and is_soft_keyword_used(prev_token, token, next_token)
                ):
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "KEYWORD")
                elif (
                    token.string in BUILTINS
                    and not (prev_token and prev_token.exact_type == T.DOT)
                ):
                    span = Span.from_token(token, line_lengths)
                    yield ColorSpan(span, "BUILTIN")


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
    except (SyntaxError, tokenize.TokenError) as e:
        recovered = False
        if isinstance(e, tokenize.TokenError):
            for recovered_color in recover_unterminated_string(
                e, line_lengths, last_emitted, buffer
            ):
                yield recovered_color
                recovered = True

        # fall back to trying each line seperetly
        if not recovered:
            lines = buffer.split('\n')
            current_offset = 0
            for i, line in enumerate(lines):
                if not line.strip():
                    current_offset += len(line) + 1
                    continue
                try:
                    line_sio = StringIO(line + '\n')
                    line_gen = tokenize.generate_tokens(line_sio.readline)
                    line_line_lengths = [0, len(line) + 1]

                    for color in gen_colors_from_token_stream(line_gen, line_line_lengths):
                        adjusted_span = Span(
                            color.span.start + current_offset,
                            color.span.end + current_offset
                        )
                        yield ColorSpan(adjusted_span, color.tag)
                except Exception:
                    pass
                current_offset += len(line) + 1




def color_config(text):
    """Set color options of Text widget.

    If ColorDelegator is used, this should be called first.
    """
    # Called from htest, TextFrame, Editor, and Turtledemo.
    # Not automatic because ColorDelegator does not know 'text'.
    theme = idleConf.CurrentTheme()
    normal_colors = idleConf.GetHighlight(theme, 'normal')
    cursor_color = idleConf.GetHighlight(theme, 'cursor')['foreground']
    select_colors = idleConf.GetHighlight(theme, 'hilite')
    text.config(
        foreground=normal_colors['foreground'],
        background=normal_colors['background'],
        insertbackground=cursor_color,
        selectforeground=select_colors['foreground'],
        selectbackground=select_colors['background'],
        inactiveselectbackground=select_colors['background'],  # new in 8.5
        )


class ColorDelegator(Delegator):
    """Delegator for syntax highlighting (text coloring).

    Instance variables:
        delegate: Delegator below this one in the stack, meaning the
                one this one delegates to.

        Used to track state:
        after_id: Identifier for scheduled after event, which is a
                timer for colorizing the text.
        allow_colorizing: Boolean toggle for applying colorizing.
        colorizing: Boolean flag when colorizing is in process.
        stop_colorizing: Boolean flag to end an active colorizing
                process.
    """

    def __init__(self):
        Delegator.__init__(self)
        self.init_state()
        self.LoadTagDefs()

    def init_state(self):
        "Initialize variables that track colorizing state."
        self.after_id = None
        self.allow_colorizing = True
        self.stop_colorizing = False
        self.colorizing = False

    def setdelegate(self, delegate):
        """Set the delegate for this instance.

        A delegate is an instance of a Delegator class and each
        delegate points to the next delegator in the stack.  This
        allows multiple delegators to be chained together for a
        widget.  The bottom delegate for a colorizer is a Text
        widget.

        If there is a delegate, also start the colorizing process.
        """
        if self.delegate is not None:
            self.unbind("<<toggle-auto-coloring>>")
        Delegator.setdelegate(self, delegate)
        if delegate is not None:
            self.config_colors()
            self.bind("<<toggle-auto-coloring>>", self.toggle_colorize_event)
            self.notify_range("1.0", "end")
        else:
            # No delegate - stop any colorizing.
            self.stop_colorizing = True
            self.allow_colorizing = False

    def config_colors(self):
        "Configure text widget tags with colors from tagdefs."
        for tag, cnf in self.tagdefs.items():
            self.tag_configure(tag, **cnf)
        self.tag_raise('sel')

    def LoadTagDefs(self):
        "Create dictionary of tag names to text colors."
        theme = idleConf.CurrentTheme()
        self.tagdefs = {
            "COMMENT": idleConf.GetHighlight(theme, "comment"),
            "KEYWORD": idleConf.GetHighlight(theme, "keyword"),
            "BUILTIN": idleConf.GetHighlight(theme, "builtin"),
            "STRING": idleConf.GetHighlight(theme, "string"),
            "DEFINITION": idleConf.GetHighlight(theme, "definition"),
            "SYNC": {'background': None, 'foreground': None},
            "TODO": {'background': None, 'foreground': None},
            "ERROR": idleConf.GetHighlight(theme, "error"),
            # "hit" is used by ReplaceDialog to mark matches. It shouldn't be changed by Colorizer, but
            # that currently isn't technically possible. This should be moved elsewhere in the future
            # when fixing the "hit" tag's visibility, or when the replace dialog is replaced with a
            # non-modal alternative.
            "hit": idleConf.GetHighlight(theme, "hit"),
            }
        if DEBUG: print('tagdefs', self.tagdefs)

    def insert(self, index, chars, tags=None):
        "Insert chars into widget at index and mark for colorizing."
        index = self.index(index)
        self.delegate.insert(index, chars, tags)
        self.notify_range(index, index + "+%dc" % len(chars))

    def delete(self, index1, index2=None):
        "Delete chars between indexes and mark for colorizing."
        index1 = self.index(index1)
        self.delegate.delete(index1, index2)
        self.notify_range(index1)

    def notify_range(self, index1, index2=None):
        "Mark text changes for processing and restart colorizing, if active."
        self.tag_add("TODO", index1, index2)
        if self.after_id:
            if DEBUG: print("colorizing already scheduled")
            return
        if self.colorizing:
            self.stop_colorizing = True
            if DEBUG: print("stop colorizing")
        if self.allow_colorizing:
            if DEBUG: print("schedule colorizing")
            self.after_id = self.after(1, self.recolorize)
        return

    def close(self):
        if self.after_id:
            after_id = self.after_id
            self.after_id = None
            if DEBUG: print("cancel scheduled recolorizer")
            self.after_cancel(after_id)
        self.allow_colorizing = False
        self.stop_colorizing = True

    def toggle_colorize_event(self, event=None):
        """Toggle colorizing on and off.

        When toggling off, if colorizing is scheduled or is in
        process, it will be cancelled and/or stopped.

        When toggling on, colorizing will be scheduled.
        """
        if self.after_id:
            after_id = self.after_id
            self.after_id = None
            if DEBUG: print("cancel scheduled recolorizer")
            self.after_cancel(after_id)
        if self.allow_colorizing and self.colorizing:
            if DEBUG: print("stop colorizing")
            self.stop_colorizing = True
        self.allow_colorizing = not self.allow_colorizing
        if self.allow_colorizing and not self.colorizing:
            self.after_id = self.after(1, self.recolorize)
        if DEBUG:
            print("auto colorizing turned",
                  "on" if self.allow_colorizing else "off")
        return "break"

    def recolorize(self):
        """Timer event (every 1ms) to colorize text.

        Colorizing is only attempted when the text widget exists,
        when colorizing is toggled on, and when the colorizing
        process is not already running.

        After colorizing is complete, some cleanup is done to
        make sure that all the text has been colorized.
        """
        self.after_id = None
        if not self.delegate:
            if DEBUG: print("no delegate")
            return
        if not self.allow_colorizing:
            if DEBUG: print("auto colorizing is off")
            return
        if self.colorizing:
            if DEBUG: print("already colorizing")
            return
        try:
            self.stop_colorizing = False
            self.colorizing = True
            if DEBUG: print("colorizing...")
            t0 = time.perf_counter()
            self.recolorize_main()
            t1 = time.perf_counter()
            if DEBUG: print("%.3f seconds" % (t1-t0))
        finally:
            self.colorizing = False
        if self.allow_colorizing and self.tag_nextrange("TODO", "1.0"):
            if DEBUG: print("reschedule colorizing")
            self.after_id = self.after(1, self.recolorize)

    def recolorize_main(self):
        "Evaluate text and apply colorizing tags."
        next = "1.0"
        while todo_tag_range := self.tag_nextrange("TODO", next):
            self.tag_remove("SYNC", todo_tag_range[0], todo_tag_range[1])
            sync_tag_range = self.tag_prevrange("SYNC", todo_tag_range[0])
            head = sync_tag_range[1] if sync_tag_range else "1.0"

            chars = ""
            next = head
            lines_to_get = 1
            ok = False
            while not ok:
                mark = next
                next = self.index(mark + "+%d lines linestart" %
                                         lines_to_get)
                lines_to_get = min(lines_to_get * 2, 100)
                ok = "SYNC" in self.tag_names(next + "-1c")
                line = self.get(mark, next)
                ##print head, "get", mark, next, "->", repr(line)
                if not line:
                    return
                for tag in self.tagdefs:
                    self.tag_remove(tag, mark, next)
                chars += line
                self._add_tags_in_section(chars, head)
                if "SYNC" in self.tag_names(next + "-1c"):
                    head = next
                    chars = ""
                else:
                    ok = False
                if not ok:
                    # We're in an inconsistent state, and the call to
                    # update may tell us to stop.  It may also change
                    # the correct value for "next" (since this is a
                    # line.col string, not a true mark).  So leave a
                    # crumb telling the next invocation to resume here
                    # in case update tells us to leave.
                    self.tag_add("TODO", next)
                self.update_idletasks()
                if self.stop_colorizing:
                    if DEBUG: print("colorizing stopped")
                    return

    def _add_tag(self, start, end, head, tag):
        """Add a tag to a given range in the text widget.

        This is a utility function, receiving the range as `start` and
        `end` positions, each of which is a number of characters
        relative to the given `head` index in the text widget.

        The tag to add is determined by `matched_group_name`, which is
        the name of a regular expression "named group" as matched by
        by the relevant highlighting regexps.
        """
        self.tag_add(tag,
                     f"{head}+{start:d}c",
                     f"{head}+{end:d}c")

    def _add_tags_in_section(self, chars, head):
        """Parse and add highlighting tags to a given part of the text.

        `chars` is a string with the text to parse and to which
        highlighting is to be applied.

            `head` is the index in the text widget where the text is found.
        """
        for color_span in gen_colors(chars):
            start_pos = color_span.span.start
            end_pos = color_span.span.end + 1
            tag = color_span.tag
            self._add_tag(start_pos, end_pos, head, tag)

    def removecolors(self):
        "Remove all colorizing tags."
        for tag in self.tagdefs:
            self.tag_remove(tag, "1.0", "end")


def _color_delegator(parent):  # htest #
    from tkinter import Toplevel, Text
    from idlelib.idle_test.test_colorizer import source
    from idlelib.percolator import Percolator

    top = Toplevel(parent)
    top.title("Test ColorDelegator")
    x, y = map(int, parent.geometry().split('+')[1:])
    top.geometry("700x550+%d+%d" % (x + 20, y + 175))

    text = Text(top, background="white")
    text.pack(expand=1, fill="both")
    text.insert("insert", source)
    text.focus_set()

    color_config(text)
    p = Percolator(text)
    d = ColorDelegator()
    p.insertfilter(d)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_colorizer', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_color_delegator)
