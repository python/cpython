#   Copyright 2000-2010 Michael Hudson-Doyle <micahel@gmail.com>
#                       Antonio Cuni
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

from __future__ import annotations

import sys
import _colorize

from contextlib import contextmanager
from dataclasses import dataclass, field, fields, replace
from typing import Self

from . import commands, console, input
from .content import (
    ContentFragment,
    ContentLine,
    SourceLine,
    build_body_fragments,
    process_prompt as build_prompt_content,
)
from .layout import LayoutMap, LayoutResult, LayoutRow, WrappedRow, layout_content_lines
from .render import RenderCell, RenderLine, RenderedScreen, ScreenOverlay
from .utils import ANSI_ESCAPE_SEQUENCE, THEME, StyleRef, wlen, gen_colors
from .trace import trace


# types
Command = commands.Command
from .types import (
    Callback,
    CommandName,
    CursorXY,
    Dimensions,
    EventData,
    KeySpec,
    Keymap,
    ScreenInfoRow,
    SimpleContextManager,
)

type CommandClass = type[Command]
type CommandInput = tuple[CommandName | CommandClass, EventData]
type PromptCellCacheKey = tuple[str, bool]


# syntax classes
SYNTAX_WHITESPACE, SYNTAX_WORD, SYNTAX_SYMBOL = range(3)


def make_default_syntax_table() -> dict[str, int]:
    # XXX perhaps should use some unicodedata here?
    st: dict[str, int] = {}
    for c in map(chr, range(256)):
        st[c] = SYNTAX_SYMBOL
    for c in [a for a in map(chr, range(256)) if a.isalnum()]:
        st[c] = SYNTAX_WORD
    st["\n"] = st[" "] = SYNTAX_WHITESPACE
    return st


def make_default_commands() -> dict[CommandName, CommandClass]:
    result: dict[CommandName, CommandClass] = {}
    for v in vars(commands).values():
        if isinstance(v, type) and issubclass(v, Command) and v.__name__[0].islower():
            result[v.__name__] = v
            result[v.__name__.replace("_", "-")] = v
    return result


default_keymap: Keymap = tuple(
    [
        (r"\C-a", "beginning-of-line"),
        (r"\C-b", "left"),
        (r"\C-c", "interrupt"),
        (r"\C-d", "delete"),
        (r"\C-e", "end-of-line"),
        (r"\C-f", "right"),
        (r"\C-g", "cancel"),
        (r"\C-h", "backspace"),
        (r"\C-j", "accept"),
        (r"\<return>", "accept"),
        (r"\C-k", "kill-line"),
        (r"\C-l", "clear-screen"),
        (r"\C-m", "accept"),
        (r"\C-t", "transpose-characters"),
        (r"\C-u", "unix-line-discard"),
        (r"\C-w", "unix-word-rubout"),
        (r"\C-x\C-u", "upcase-region"),
        (r"\C-y", "yank"),
        *(() if sys.platform == "win32" else ((r"\C-z", "suspend"), )),
        (r"\M-b", "backward-word"),
        (r"\M-c", "capitalize-word"),
        (r"\M-d", "kill-word"),
        (r"\M-f", "forward-word"),
        (r"\M-l", "downcase-word"),
        (r"\M-t", "transpose-words"),
        (r"\M-u", "upcase-word"),
        (r"\M-y", "yank-pop"),
        (r"\M--", "digit-arg"),
        (r"\M-0", "digit-arg"),
        (r"\M-1", "digit-arg"),
        (r"\M-2", "digit-arg"),
        (r"\M-3", "digit-arg"),
        (r"\M-4", "digit-arg"),
        (r"\M-5", "digit-arg"),
        (r"\M-6", "digit-arg"),
        (r"\M-7", "digit-arg"),
        (r"\M-8", "digit-arg"),
        (r"\M-9", "digit-arg"),
        (r"\M-\n", "accept"),
        ("\\\\", "self-insert"),
        (r"\x1b[200~", "perform-bracketed-paste"),
        (r"\x03", "ctrl-c"),
    ]
    + [(c, "self-insert") for c in map(chr, range(32, 127)) if c != "\\"]
    + [(c, "self-insert") for c in map(chr, range(128, 256)) if c.isalpha()]
    + [
        (r"\<up>", "up"),
        (r"\<down>", "down"),
        (r"\<left>", "left"),
        (r"\C-\<left>", "backward-word"),
        (r"\<right>", "right"),
        (r"\C-\<right>", "forward-word"),
        (r"\<delete>", "delete"),
        (r"\x1b[3~", "delete"),
        (r"\<backspace>", "backspace"),
        (r"\M-\<backspace>", "backward-kill-word"),
        (r"\<end>", "end-of-line"),  # was 'end'
        (r"\<home>", "beginning-of-line"),  # was 'home'
        (r"\<f1>", "help"),
        (r"\<f2>", "show-history"),
        (r"\<f3>", "paste-mode"),
        (r"\EOF", "end"),  # the entries in the terminfo database for xterms
        (r"\EOH", "home"),  # seem to be wrong.  this is a less than ideal
        # workaround
    ]
)


@dataclass(frozen=True, slots=True)
class RefreshInvalidation:
    """Which parts of the screen need to be recomputed on the next refresh."""

    cursor_only: bool = False
    buffer_from_pos: int | None = None
    prompt: bool = False
    layout: bool = False
    theme: bool = False
    message: bool = False
    overlay: bool = False
    full: bool = False

    @classmethod
    def empty(cls) -> Self:
        return cls()

    @property
    def needs_screen_refresh(self) -> bool:
        return any(
            (
                self.buffer_from_pos is not None,
                self.prompt,
                self.layout,
                self.theme,
                self.message,
                self.overlay,
                self.full,
            )
        )

    @property
    def is_cursor_only(self) -> bool:
        return self.cursor_only and not self.needs_screen_refresh

    @property
    def buffer_rebuild_from_pos(self) -> int | None:
        if self.full or self.prompt or self.layout or self.theme:
            return 0
        return self.buffer_from_pos

    def with_cursor(self) -> Self:
        if self.needs_screen_refresh:
            return self
        return replace(self, cursor_only=True)

    def with_buffer(self, from_pos: int) -> Self:
        current = from_pos
        if self.buffer_from_pos is not None:
            current = min(current, self.buffer_from_pos)
        return replace(self, cursor_only=False, buffer_from_pos=current)

    def with_prompt(self) -> Self:
        return replace(self, cursor_only=False, prompt=True)

    def with_layout(self) -> Self:
        return replace(self, cursor_only=False, layout=True)

    def with_theme(self) -> Self:
        return replace(self, cursor_only=False, theme=True)

    def with_message(self) -> Self:
        return replace(self, cursor_only=False, message=True)

    def with_overlay(self) -> Self:
        return replace(self, cursor_only=False, overlay=True)

    def with_full(self) -> Self:
        return replace(self, cursor_only=False, full=True)


@dataclass(slots=True)
class Reader:
    """The Reader class implements the bare bones of a command reader,
    handling such details as editing and cursor motion.  What it does
    not support are such things as completion or history support -
    these are implemented elsewhere.

    Instance variables of note include:

      * buffer:
        A per-character list containing all the characters that have been
        entered. Does not include color information.
      * console:
        Hopefully encapsulates the OS dependent stuff.
      * pos:
        A 0-based index into 'buffer' for where the insertion point
        is.
      * layout:
        A mapping between buffer positions and rendered rows/columns.
        It is the internal source of truth for cursor placement.
      * cxy, lxy:
        the position of the insertion point in screen ...
      * syntax_table:
        Dictionary mapping characters to 'syntax class'; read the
        emacs docs to see what this means :-)
      * commands:
        Dictionary mapping command names to command classes.
      * arg:
        The emacs-style prefix argument.  It will be None if no such
        argument has been provided.
      * kill_ring:
        The emacs-style kill-ring; manipulated with yank & yank-pop
      * ps1, ps2, ps3, ps4:
        prompts.  ps1 is the prompt for a one-line input; for a
        multiline input it looks like:
            ps2> first line of input goes here
            ps3> second and further
            ps3> lines get ps3
            ...
            ps4> and the last one gets ps4
        As with the usual top-level, you can set these to instances if
        you like; str() will be called on them (once) at the beginning
        of each command.  Don't put really long or newline containing
        strings here, please!
        This is just the default policy; you can change it freely by
        overriding get_prompt() (and indeed some standard subclasses
        do).
      * finished:
        handle1 will set this to a true value if a command signals
        that we're done.
    """

    console: console.Console

    ## state
    buffer: list[str] = field(default_factory=list)
    pos: int = 0
    ps1: str = "->> "
    ps2: str = "/>> "
    ps3: str = "|.. "
    ps4: str = R"\__ "
    kill_ring: list[list[str]] = field(default_factory=list)
    msg: str = ""
    arg: int | None = None
    finished: bool = False
    paste_mode: bool = False
    commands: dict[CommandName, CommandClass] = field(default_factory=make_default_commands)
    last_command: CommandClass | None = None
    syntax_table: dict[str, int] = field(default_factory=make_default_syntax_table)
    keymap: Keymap = ()
    input_trans: input.KeymapTranslator = field(init=False)
    input_trans_stack: list[input.KeymapTranslator] = field(default_factory=list)
    rendered_screen: RenderedScreen = field(init=False)
    layout: LayoutMap = field(init=False)
    cxy: CursorXY = field(init=False)
    lxy: CursorXY = field(init=False)
    scheduled_commands: list[CommandName] = field(default_factory=list)
    can_colorize: bool = False
    threading_hook: Callback | None = None
    invalidation: RefreshInvalidation = field(init=False)

    ## cached metadata to speed up screen refreshes
    @dataclass
    class RefreshCache:
        """Previously computed render/layout data for incremental refresh."""

        render_lines: list[RenderLine] = field(default_factory=list)
        layout_rows: list[LayoutRow] = field(default_factory=list)
        line_end_offsets: list[int] = field(default_factory=list)
        pos: int = 0
        dimensions: Dimensions = (0, 0)

        def update_cache(self,
                         reader: Reader,
                         render_lines: list[RenderLine],
                         layout_rows: list[LayoutRow],
                         line_end_offsets: list[int],
            ) -> None:
            self.render_lines = render_lines.copy()
            self.layout_rows = layout_rows.copy()
            self.line_end_offsets = line_end_offsets.copy()
            self.pos = reader.pos
            self.dimensions = reader.console.width, reader.console.height

        def valid(self, reader: Reader) -> bool:
            dimensions = reader.console.width, reader.console.height
            dimensions_changed = dimensions != self.dimensions
            return not dimensions_changed

        def get_cached_location(
            self,
            reader: Reader,
            buffer_from_pos: int | None = None,
            *,
            reuse_full: bool = False,
        ) -> tuple[int, int]:
            """Return (buffer_offset, num_reusable_lines) for incremental refresh.

            Three paths:
            - reuse_full (overlay/message-only): reuse all cached lines.
            - buffer_from_pos=None (full rebuild): rewind to common cursor pos.
            - explicit buffer_from_pos: reuse lines before that position.
            """
            if reuse_full:
                if self.line_end_offsets:
                    last_offset = self.line_end_offsets[-1]
                    if last_offset >= len(reader.buffer):
                        return last_offset, len(self.line_end_offsets)
                return 0, 0
            if buffer_from_pos is None:
                buffer_from_pos = min(reader.pos, self.pos)
            num_common_lines = len(self.line_end_offsets)
            while num_common_lines > 0:
                candidate = self.line_end_offsets[num_common_lines - 1]
                if buffer_from_pos > candidate:
                    break
                num_common_lines -= 1
            # Prompt-only leading rows consume no buffer content. Reusing them
            # in isolation causes the next incremental rebuild to emit them a
            # second time.
            while (
                num_common_lines > 0
                and self.layout_rows[num_common_lines - 1].buffer_advance == 0
            ):
                num_common_lines -= 1
            offset = self.line_end_offsets[num_common_lines - 1] if num_common_lines else 0
            return offset, num_common_lines

    last_refresh_cache: RefreshCache = field(default_factory=RefreshCache)

    def __post_init__(self) -> None:
        # Enable the use of `insert` without a `prepare` call - necessary to
        # facilitate the tab completion hack implemented for
        # <https://bugs.python.org/issue25660>.
        self.keymap = self.collect_keymap()
        self.input_trans = input.KeymapTranslator(
            self.keymap, invalid_cls="invalid-key", character_cls="self-insert"
        )
        self.layout = LayoutMap.empty()
        self.cxy = self.pos2xy()
        self.lxy = (self.pos, 0)
        self.rendered_screen = RenderedScreen.empty()
        self.can_colorize = _colorize.can_colorize()
        self.invalidation = RefreshInvalidation.empty()

        self.last_refresh_cache.layout_rows = list(self.layout.rows)
        self.last_refresh_cache.pos = self.pos

        self.last_refresh_cache.dimensions = (0, 0)

    @property
    def screen(self) -> list[str]:
        return list(self.rendered_screen.screen_lines)

    @property
    def screeninfo(self) -> list[ScreenInfoRow]:
        return self.layout.screeninfo

    def collect_keymap(self) -> Keymap:
        return default_keymap

    def calc_screen(self) -> RenderedScreen:
        """Translate the editable buffer into a base rendered screen."""
        num_common_lines = 0
        offset = 0
        if self.last_refresh_cache.valid(self):
            if (
                self.invalidation.buffer_from_pos is None
                and not (
                    self.invalidation.full
                    or self.invalidation.prompt
                    or self.invalidation.layout
                    or self.invalidation.theme
                )
                and (self.invalidation.message or self.invalidation.overlay)
            ):
                # Fast path: only overlays or messages changed.
                offset, num_common_lines = self.last_refresh_cache.get_cached_location(
                    self,
                    reuse_full=True,
                )
                assert not self.last_refresh_cache.line_end_offsets or (
                    self.last_refresh_cache.line_end_offsets[-1] >= len(self.buffer)
                ), "Buffer modified without invalidate_buffer() call"
            else:
                offset, num_common_lines = self.last_refresh_cache.get_cached_location(
                    self,
                    self._buffer_refresh_from_pos(),
                )

        base_render_lines = self.last_refresh_cache.render_lines[:num_common_lines]
        layout_rows = self.last_refresh_cache.layout_rows[:num_common_lines]
        last_refresh_line_end_offsets = self.last_refresh_cache.line_end_offsets[:num_common_lines]

        source_lines = self._build_source_lines(offset, num_common_lines)
        content_lines = self._build_content_lines(
            source_lines,
            prompt_from_cache=bool(offset and self.buffer[offset - 1] != "\n"),
        )
        layout_result = self._layout_content(content_lines, offset)
        base_render_lines.extend(self._render_wrapped_rows(layout_result.wrapped_rows))
        layout_rows.extend(layout_result.layout_map.rows)
        last_refresh_line_end_offsets.extend(layout_result.line_end_offsets)

        self.layout = LayoutMap(tuple(layout_rows))
        self.cxy = self.pos2xy()
        if not source_lines:
            # reuse_full path: _build_source_lines didn't run,
            # so lxy wasn't updated. Derive it from the buffer.
            self.lxy = self._compute_lxy()
        self.last_refresh_cache.update_cache(
            self,
            base_render_lines,
            layout_rows,
            last_refresh_line_end_offsets,
        )
        return RenderedScreen(tuple(base_render_lines), self.cxy)

    def _buffer_refresh_from_pos(self) -> int:
        """Return buffer position from which to rebuild content.

        Returns 0 (full rebuild) when no incremental position is known.
        """
        buffer_from_pos = self.invalidation.buffer_rebuild_from_pos
        if buffer_from_pos is not None:
            return buffer_from_pos
        return 0

    def _compute_lxy(self) -> CursorXY:
        """Derive logical cursor (col, lineno) from the buffer and pos."""
        text = "".join(self.buffer[:self.pos])
        lineno = text.count("\n")
        if lineno:
            col = self.pos - text.rindex("\n") - 1
        else:
            col = self.pos
        return col, lineno

    def _build_source_lines(
        self,
        offset: int,
        first_lineno: int,
    ) -> tuple[SourceLine, ...]:
        if offset == len(self.buffer) and (offset > 0 or first_lineno > 0):
            return ()

        pos = self.pos - offset
        lines = "".join(self.buffer[offset:]).split("\n")
        cursor_found = False
        lines_beyond_cursor = 0
        source_lines: list[SourceLine] = []
        current_offset = offset

        for line_index, line in enumerate(lines):
            lineno = first_lineno + line_index
            has_newline = line_index < len(lines) - 1
            line_len = len(line)
            cursor_index: int | None = None
            if 0 <= pos <= line_len:
                cursor_index = pos
                self.lxy = pos, lineno
                cursor_found = True
            elif cursor_found:
                lines_beyond_cursor += 1
                if lines_beyond_cursor > self.console.height:
                    break

            source_lines.append(
                SourceLine(
                    lineno=lineno,
                    text=line,
                    start_offset=current_offset,
                    has_newline=has_newline,
                    cursor_index=cursor_index,
                )
            )
            pos -= line_len + 1
            current_offset += line_len + (1 if has_newline else 0)

        return tuple(source_lines)

    def _build_content_lines(
        self,
        source_lines: tuple[SourceLine, ...],
        *,
        prompt_from_cache: bool,
    ) -> tuple[ContentLine, ...]:
        if self.can_colorize:
            colors = list(gen_colors(self.get_unicode()))
        else:
            colors = None
        trace("colors = {colors}", colors=colors)

        content_lines: list[ContentLine] = []
        for source_line in source_lines:
            if prompt_from_cache:
                prompt_from_cache = False
                prompt = ""
            else:
                prompt = self.get_prompt(source_line.lineno, source_line.cursor_on_line)
            content_lines.append(
                ContentLine(
                    source=source_line,
                    prompt=build_prompt_content(prompt),
                    body=build_body_fragments(
                        source_line.text,
                        colors,
                        source_line.start_offset,
                    ),
                )
            )
        return tuple(content_lines)

    def _layout_content(
        self,
        content_lines: tuple[ContentLine, ...],
        offset: int,
    ) -> LayoutResult:
        return layout_content_lines(content_lines, self.console.width, offset)

    def _render_wrapped_rows(
        self,
        wrapped_rows: tuple[WrappedRow, ...],
    ) -> list[RenderLine]:
        return [
            self._render_line(
                row.prompt_text,
                row.fragments,
                row.suffix,
            )
            for row in wrapped_rows
        ]

    def _render_message_lines(self) -> tuple[RenderLine, ...]:
        if not self.msg:
            return ()
        width = self.console.width
        render_lines: list[RenderLine] = []
        for message_line in self.msg.split("\n"):
            # If self.msg is larger than console width, make it fit.
            # TODO: try to split between words?
            if not message_line:
                render_lines.append(RenderLine.from_rendered_text(""))
                continue
            for offset in range(0, len(message_line), width):
                render_lines.append(
                    RenderLine.from_rendered_text(message_line[offset : offset + width])
                )
        return tuple(render_lines)

    def get_screen_overlays(self) -> tuple[ScreenOverlay, ...]:
        return ()

    def compose_rendered_screen(self, base_screen: RenderedScreen) -> RenderedScreen:
        overlays = list(self.get_screen_overlays())
        message_lines = self._render_message_lines()
        if message_lines:
            overlays.append(ScreenOverlay(len(base_screen.lines), message_lines))
        if not overlays:
            return base_screen
        return RenderedScreen(base_screen.lines, base_screen.cursor, tuple(overlays))

    _prompt_cell_cache: dict[PromptCellCacheKey, tuple[RenderCell, ...]] = field(
        init=False, default_factory=dict, repr=False
    )

    def _render_line(
        self,
        prefix: str,
        fragments: tuple[ContentFragment, ...],
        suffix: str = "",
    ) -> RenderLine:
        cells: list[RenderCell] = []
        if prefix:
            cache_key = (prefix, self.can_colorize)
            cached = self._prompt_cell_cache.get(cache_key)
            if cached is None:
                prompt_cells = RenderLine.from_rendered_text(prefix).cells
                if self.can_colorize and prompt_cells and not ANSI_ESCAPE_SEQUENCE.search(prefix):
                    prompt_style = StyleRef.from_tag("prompt", THEME()["prompt"])
                    prompt_cells = tuple(
                        RenderCell(
                            cell.text,
                            cell.width,
                            style=prompt_style if cell.text else cell.style,
                            controls=cell.controls,
                        )
                        for cell in prompt_cells
                    )
                self._prompt_cell_cache[cache_key] = prompt_cells
                cached = prompt_cells
            cells.extend(cached)
        cells.extend(
            RenderCell(fragment.text, fragment.width, style=fragment.style)
            for fragment in fragments
        )
        if suffix:
            cells.extend(RenderLine.from_rendered_text(suffix).cells)
        return RenderLine.from_cells(cells)

    @staticmethod
    def process_prompt(prompt: str) -> tuple[str, int]:
        r"""Return a tuple with the prompt string and its visible length.

        The prompt string has the zero-width brackets recognized by shells
        (\x01 and \x02) removed.  The length ignores anything between those
        brackets as well as any ANSI escape sequences.
        """
        prompt_content = build_prompt_content(prompt)
        return prompt_content.text, prompt_content.width

    def bow(self, p: int | None = None) -> int:
        """Return the 0-based index of the word break preceding p most
        immediately.

        p defaults to self.pos; word boundaries are determined using
        self.syntax_table."""
        if p is None:
            p = self.pos
        st = self.syntax_table
        b = self.buffer
        p -= 1
        while p >= 0 and st.get(b[p], SYNTAX_WORD) != SYNTAX_WORD:
            p -= 1
        while p >= 0 and st.get(b[p], SYNTAX_WORD) == SYNTAX_WORD:
            p -= 1
        return p + 1

    def eow(self, p: int | None = None) -> int:
        """Return the 0-based index of the word break following p most
        immediately.

        p defaults to self.pos; word boundaries are determined using
        self.syntax_table."""
        if p is None:
            p = self.pos
        st = self.syntax_table
        b = self.buffer
        while p < len(b) and st.get(b[p], SYNTAX_WORD) != SYNTAX_WORD:
            p += 1
        while p < len(b) and st.get(b[p], SYNTAX_WORD) == SYNTAX_WORD:
            p += 1
        return p

    def bol(self, p: int | None = None) -> int:
        """Return the 0-based index of the line break preceding p most
        immediately.

        p defaults to self.pos."""
        if p is None:
            p = self.pos
        b = self.buffer
        p -= 1
        while p >= 0 and b[p] != "\n":
            p -= 1
        return p + 1

    def eol(self, p: int | None = None) -> int:
        """Return the 0-based index of the line break following p most
        immediately.

        p defaults to self.pos."""
        if p is None:
            p = self.pos
        b = self.buffer
        while p < len(b) and b[p] != "\n":
            p += 1
        return p

    def max_column(self, y: int) -> int:
        """Return the last x-offset for line y"""
        return self.layout.max_column(y)

    def max_row(self) -> int:
        return self.layout.max_row()

    def get_arg(self, default: int = 1) -> int:
        """Return any prefix argument that the user has supplied,
        returning 'default' if there is None.  Defaults to 1.
        """
        if self.arg is None:
            return default
        return self.arg

    def get_prompt(self, lineno: int, cursor_on_line: bool) -> str:
        """Return what should be in the left-hand margin for line
        'lineno'."""
        if self.arg is not None and cursor_on_line:
            prompt = f"(arg: {self.arg}) "
        elif self.paste_mode:
            prompt = "(paste) "
        elif "\n" in self.buffer:
            if lineno == 0:
                prompt = self.ps2
            elif self.ps4 and lineno == self.buffer.count("\n"):
                prompt = self.ps4
            else:
                prompt = self.ps3
        else:
            prompt = self.ps1
        return prompt

    def push_input_trans(self, itrans: input.KeymapTranslator) -> None:
        self.input_trans_stack.append(self.input_trans)
        self.input_trans = itrans

    def pop_input_trans(self) -> None:
        self.input_trans = self.input_trans_stack.pop()

    def setpos_from_xy(self, x: int, y: int) -> None:
        """Set pos according to coordinates x, y"""
        self.pos = self.layout.xy_to_pos(x, y)

    def pos2xy(self) -> CursorXY:
        """Return the x, y coordinates of position 'pos'."""
        assert 0 <= self.pos <= len(self.buffer)
        return self.layout.pos_to_xy(self.pos)

    def insert(self, text: str | list[str]) -> None:
        """Insert 'text' at the insertion point."""
        start = self.pos
        self.buffer[self.pos : self.pos] = list(text)
        self.pos += len(text)
        self.invalidate_buffer(start)

    def invalidate_cursor(self) -> None:
        self.invalidation = self.invalidation.with_cursor()

    def invalidate_buffer(self, from_pos: int) -> None:
        self.invalidation = self.invalidation.with_buffer(from_pos)

    def invalidate_prompt(self) -> None:
        self._prompt_cell_cache.clear()
        self.invalidation = self.invalidation.with_prompt()

    def invalidate_layout(self) -> None:
        self.invalidation = self.invalidation.with_layout()

    def invalidate_theme(self) -> None:
        self._prompt_cell_cache.clear()
        self.invalidation = self.invalidation.with_theme()

    def invalidate_message(self) -> None:
        self.invalidation = self.invalidation.with_message()

    def invalidate_overlay(self) -> None:
        self.invalidation = self.invalidation.with_overlay()

    def invalidate_full(self) -> None:
        self.invalidation = self.invalidation.with_full()

    def clear_invalidation(self) -> None:
        self.invalidation = RefreshInvalidation.empty()

    def update_cursor(self) -> None:
        """Move the cursor to reflect changes in self.pos"""
        self.cxy = self.pos2xy()
        trace("update_cursor({pos}) = {cxy}", pos=self.pos, cxy=self.cxy)
        self.console.move_cursor(*self.cxy)

    def after_command(self, cmd: Command) -> None:
        """This function is called to allow post command cleanup."""
        if getattr(cmd, "kills_digit_arg", True):
            if self.arg is not None:
                self.invalidate_prompt()
            self.arg = None

    def prepare(self) -> None:
        """Get ready to run.  Call restore when finished.  You must not
        write to the console in between the calls to prepare and
        restore."""
        try:
            self.console.prepare()
            self.arg = None
            self.finished = False
            del self.buffer[:]
            self.pos = 0
            self.layout = LayoutMap.empty()
            self.cxy = self.pos2xy()
            self.lxy = (self.pos, 0)
            self.rendered_screen = RenderedScreen.empty()
            self.invalidate_full()
            self.last_command = None
            base_screen = self.calc_screen()
            self.rendered_screen = self.compose_rendered_screen(base_screen)
            self.invalidation = RefreshInvalidation.empty()
        except BaseException:
            self.restore()
            raise

        while self.scheduled_commands:
            cmd = self.scheduled_commands.pop()
            self.do_cmd((cmd, []))

    def last_command_is(self, cls: CommandClass) -> bool:
        if not self.last_command:
            return False
        return issubclass(cls, self.last_command)

    def restore(self) -> None:
        """Clean up after a run."""
        self.console.restore()

    @contextmanager
    def suspend(self) -> SimpleContextManager:
        """A context manager to delegate to another reader."""
        prev_state = {f.name: getattr(self, f.name) for f in fields(self)}
        try:
            self.restore()
            yield
        finally:
            for arg in ("msg", "ps1", "ps2", "ps3", "ps4", "paste_mode"):
                setattr(self, arg, prev_state[arg])
            self.prepare()

    @contextmanager
    def suspend_colorization(self) -> SimpleContextManager:
        try:
            old_can_colorize = self.can_colorize
            self.can_colorize = False
            yield
        finally:
            self.can_colorize = old_can_colorize

    def finish(self) -> None:
        """Called when a command signals that we're finished."""
        pass

    def error(self, msg: str = "none") -> None:
        self.msg = "! " + msg + " "
        self.invalidate_message()
        self.console.beep()

    def update_screen(self) -> None:
        if self.invalidation.is_cursor_only:
            self.update_cursor()
            self.clear_invalidation()
        elif self.invalidation.needs_screen_refresh:
            self.refresh()

    def refresh(self) -> None:
        """Recalculate and refresh the screen."""
        self.console.height, self.console.width = self.console.getheightwidth()
        # this call sets up self.cxy, so call it first.
        base_screen = self.calc_screen()
        rendered_screen = self.compose_rendered_screen(base_screen)
        self.rendered_screen = rendered_screen
        trace(
            "reader.refresh cursor={cursor} lines={lines} "
            "dims=({width},{height}) invalidation={invalidation}",
            cursor=self.cxy,
            lines=len(rendered_screen.composed_lines),
            width=self.console.width,
            height=self.console.height,
            invalidation=self.invalidation,
        )
        self.console.refresh(rendered_screen)
        self.clear_invalidation()

    def do_cmd(self, cmd: CommandInput) -> None:
        """`cmd` is a tuple of "event_name" and "event", which in the current
        implementation is always just the "buffer" which happens to be a list
        of single-character strings."""

        trace("received command {cmd}", cmd=cmd)
        if isinstance(cmd[0], str):
            command_type = self.commands.get(cmd[0], commands.invalid_command)
        elif isinstance(cmd[0], type):
            command_type = cmd[0]
        else:
            return  # nothing to do

        command = command_type(self, *cmd)  # type: ignore[arg-type]
        command.do()

        self.after_command(command)
        if (
            not self.invalidation.needs_screen_refresh
            and not self.invalidation.is_cursor_only
        ):
            self.invalidate_cursor()
        self.update_screen()

        if command_type is not commands.digit_arg:
            self.last_command = command_type

        self.finished = bool(command.finish)
        if self.finished:
            self.console.finish()
            self.finish()

    def run_hooks(self) -> None:
        threading_hook = self.threading_hook
        if threading_hook is None and 'threading' in sys.modules:
            from ._threading_handler import install_threading_hook
            install_threading_hook(self)
        if threading_hook is not None:
            try:
                threading_hook()
            except Exception:
                pass

        input_hook = self.console.input_hook
        if input_hook:
            try:
                input_hook()
            except Exception:
                pass

    def handle1(self, block: bool = True) -> bool:
        """Handle a single event.  Wait as long as it takes if block
        is true (the default), otherwise return False if no event is
        pending."""

        if self.msg:
            self.msg = ""
            self.invalidate_message()

        while True:
            # We use the same timeout as in readline.c: 100ms
            self.run_hooks()
            self.console.wait(100)
            event = self.console.get_event(block=False)
            if not event:
                if block:
                    continue
                return False

            translate = True

            if event.evt == "key":
                self.input_trans.push(event)
            elif event.evt == "scroll":
                self.invalidate_full()
                self.refresh()
                return True
            elif event.evt == "resize":
                self.invalidate_full()
                self.refresh()
                return True
            else:
                translate = False

            if translate:
                cmd = self.input_trans.get()
            else:
                cmd = [event.evt, event.data]

            if cmd is None:
                if block:
                    continue
                return False

            self.do_cmd(cmd)
            return True

    def push_char(self, char: int | bytes) -> None:
        self.console.push_char(char)
        self.handle1(block=False)

    def readline(self, startup_hook: Callback | None = None) -> str:
        """Read a line.  The implementation of this method also shows
        how to drive Reader if you want more control over the event
        loop."""
        self.prepare()
        try:
            if startup_hook is not None:
                startup_hook()
            self.refresh()
            while not self.finished:
                self.handle1()
            return self.get_unicode()

        finally:
            self.restore()

    def bind(self, spec: KeySpec, command: CommandName) -> None:
        self.keymap = self.keymap + ((spec, command),)
        self.input_trans = input.KeymapTranslator(
            self.keymap, invalid_cls="invalid-key", character_cls="self-insert"
        )

    def get_unicode(self) -> str:
        """Return the current buffer as a unicode string."""
        return "".join(self.buffer)
