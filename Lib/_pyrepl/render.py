from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass, field
from typing import Literal, Protocol, Self

from .utils import ANSI_ESCAPE_SEQUENCE, THEME, StyleRef, str_width
from .types import CursorXY

type RenderStyle = StyleRef | str | None
type LineUpdateKind = Literal[
    "insert_char",
    "replace_char",
    "replace_span",
    "delete_then_insert",
    "rewrite_suffix",
]


class _ThemeSyntax(Protocol):
    """Protocol for theme objects that map tag names to SGR escape strings."""
    def __getitem__(self, key: str, /) -> str: ...


@dataclass(frozen=True, slots=True)
class RenderCell:
    """One terminal cell: a character, its column width, and SGR style.

    A screen row like ``>>> def`` is a sequence of cells::

        >  >  >     d  e  f
       ╰─╯╰─╯╰─╯╰─╯╰─╯╰─╯╰─╯
    """

    text: str
    width: int
    style: StyleRef = field(default_factory=StyleRef)
    controls: tuple[str, ...] = ()

    @property
    def terminal_text(self) -> str:
        return render_cells((self,))


def _theme_style(theme: _ThemeSyntax, tag: str) -> str:
    return theme[tag]


def _style_escape(style: StyleRef) -> str:
    if style.sgr:
        return style.sgr
    if style.tag is None:
        return ""
    return _theme_style(THEME(), style.tag)


def _update_terminal_state(state: str, escape: str) -> str:
    if escape in {"\x1b[0m", "\x1b[m"}:
        return ""
    return state + escape


def _cells_from_rendered_text(text: str) -> tuple[RenderCell, ...]:
    if not text:
        return ()

    cells: list[RenderCell] = []
    pending_controls: list[str] = []
    active_sgr = ""
    index = 0

    def append_plain_text(segment: str) -> None:
        nonlocal pending_controls
        if not segment:
            return
        if pending_controls:
            cells.append(RenderCell("", 0, controls=tuple(pending_controls)))
            pending_controls = []
        for char in segment:
            cells.append(
                RenderCell(
                    char,
                    str_width(char),
                    style=StyleRef.from_sgr(active_sgr),
                )
            )

    for match in ANSI_ESCAPE_SEQUENCE.finditer(text):
        append_plain_text(text[index : match.start()])
        escape = match.group(0)
        if escape.endswith("m"):
            active_sgr = _update_terminal_state(active_sgr, escape)
        else:
            pending_controls.append(escape)
        index = match.end()

    append_plain_text(text[index:])
    if pending_controls:
        cells.append(RenderCell("", 0, controls=tuple(pending_controls)))

    return tuple(cells)


@dataclass(frozen=True, slots=True)
class RenderLine:
    """One physical screen row as a tuple of :class:`RenderCell` objects.

    ``text`` is the pre-rendered terminal string (characters + SGR escapes);
    ``width`` is the total visible column count.
    """

    cells: tuple[RenderCell, ...]
    text: str
    width: int

    @classmethod
    def from_cells(cls, cells: Iterable[RenderCell]) -> Self:
        cell_tuple = tuple(cells)
        return cls(
            cells=cell_tuple,
            text=render_cells(cell_tuple),
            width=sum(cell.width for cell in cell_tuple),
        )

    @classmethod
    def from_parts(
        cls,
        parts: Sequence[str],
        widths: Sequence[int],
        styles: Sequence[RenderStyle] | None = None,
    ) -> Self:
        if styles is None:
            return cls.from_cells(
                RenderCell(text, width)
                for text, width in zip(parts, widths)
            )

        cells: list[RenderCell] = []
        for text, width, style in zip(parts, widths, styles):
            if isinstance(style, StyleRef):
                cells.append(RenderCell(text, width, style=style))
            elif style is None:
                cells.append(RenderCell(text, width))
            else:
                cells.append(RenderCell(text, width, style=StyleRef.from_tag(style)))
        return cls.from_cells(cells)

    @classmethod
    def from_rendered_text(cls, text: str) -> Self:
        return cls.from_cells(_cells_from_rendered_text(text))


@dataclass(frozen=True, slots=True)
class ScreenOverlay:
    """An overlay that replaces or inserts lines at a screen position.

    If *insert* is True, lines are spliced in (shifting content down);
    if False (default), lines replace existing content at *y*.

    Overlays are used to display tab completion menus and status messages.
    For example, a tab-completion menu inserted below the input::

        >>> os.path.j           ← line 0 (base content)
                    join        ← ScreenOverlay(y=1, insert=True)
                    junction    ←   (pushes remaining lines down)
        ...                     ← line 1 (shifted down by 2)
    """
    y: int
    lines: tuple[RenderLine, ...]
    insert: bool = False


@dataclass(frozen=True, slots=True)
class RenderedScreen:
    """The complete screen state: content lines, cursor, and overlays.

    ``lines`` holds the base content; ``composed_lines`` is the final
    result after overlays (completion menus, messages) are applied::

        lines:                     composed_lines:
        ┌──────────────────┐       ┌──────────────────┐
        │>>> os.path.j     │       │>>> os.path.j     │
        │...               │  ──►  │            join  │ ← overlay
        └──────────────────┘       │...               │
                                   └──────────────────┘
    """

    lines: tuple[RenderLine, ...]
    cursor: CursorXY
    overlays: tuple[ScreenOverlay, ...] = ()
    composed_lines: tuple[RenderLine, ...] = field(init=False, default=())

    def __post_init__(self) -> None:
        object.__setattr__(self, "composed_lines", self._compose())

    def _compose(self) -> tuple[RenderLine, ...]:
        """Apply overlays in tuple order; inserts shift subsequent positions."""
        if not self.overlays:
            return self.lines

        lines = list(self.lines)
        y_offset = 0
        for overlay in self.overlays:
            adjusted_y = overlay.y + y_offset
            assert adjusted_y >= 0, (
                f"Overlay y={overlay.y} with offset={y_offset} is negative; "
                "overlays must be sorted by ascending y"
            )
            if overlay.insert:
                # Splice overlay lines in, pushing existing content down.
                lines[adjusted_y:adjusted_y] = overlay.lines
                y_offset += len(overlay.lines)
            else:
                # Replace existing lines at the overlay position.
                target_len = adjusted_y + len(overlay.lines)
                if len(lines) < target_len:
                    lines.extend([EMPTY_RENDER_LINE] * (target_len - len(lines)))
                for index, line in enumerate(overlay.lines):
                    lines[adjusted_y + index] = line
        return tuple(lines)

    @classmethod
    def empty(cls) -> Self:
        return cls((), (0, 0), ())

    @classmethod
    def from_screen_lines(
        cls,
        screen: Sequence[str],
        cursor: CursorXY,
    ) -> Self:
        return cls(
            tuple(RenderLine.from_rendered_text(line) for line in screen),
            cursor,
            (),
        )

    def with_overlay(
        self,
        y: int,
        lines: Iterable[RenderLine],
    ) -> Self:
        return type(self)(
            self.lines,
            self.cursor,
            self.overlays + (ScreenOverlay(y, tuple(lines)),),
        )

    @property
    def screen_lines(self) -> tuple[str, ...]:
        return tuple(line.text for line in self.composed_lines)


@dataclass(frozen=True, slots=True)
class LineDiff:
    """The changed region between an old and new version of one screen row.

    When the user types ``e`` so the row changes from
    ``>>> nam`` to ``>>> name``::

        >>> n a m       old
        >>> n a m e     new
                  ╰─╯
                  start_cell=7, new_cells=("m","e"), old_cells=("m",)
    """

    start_cell: int
    start_x: int
    old_cells: tuple[RenderCell, ...]
    new_cells: tuple[RenderCell, ...]
    old_width: int
    new_width: int

    @property
    def old_text(self) -> str:
        return render_cells(self.old_cells)

    @property
    def new_text(self) -> str:
        return render_cells(self.new_cells)

    @property
    def old_changed_width(self) -> int:
        return sum(cell.width for cell in self.old_cells)

    @property
    def new_changed_width(self) -> int:
        return sum(cell.width for cell in self.new_cells)


EMPTY_RENDER_LINE = RenderLine(cells=(), text="", width=0)


@dataclass(frozen=True, slots=True)
class LineUpdate:
    kind: LineUpdateKind
    y: int
    start_cell: int
    start_x: int
    """Screen x-coordinate where the update begins. Used for cursor positioning."""
    cells: tuple[RenderCell, ...]
    char_width: int = 0
    clear_eol: bool = False
    reset_to_margin: bool = False
    """If True, the console must resync the cursor position after writing
    (needed when cells contain non-SGR escape sequences that may move the cursor)."""
    text: str = field(init=False, default="")

    def __post_init__(self) -> None:
        object.__setattr__(self, "text", render_cells(self.cells))


def _controls_require_cursor_resync(controls: Sequence[str]) -> bool:
    # Anything beyond SGR means the cursor may no longer be where we left it.
    return any(not control.endswith("m") for control in controls)


def requires_cursor_resync(cells: Sequence[RenderCell]) -> bool:
    return any(_controls_require_cursor_resync(cell.controls) for cell in cells)


def render_cells(
    cells: Sequence[RenderCell],
    visual_style: str | None = None,
) -> str:
    """Render a sequence of cells into a terminal string with SGR escapes.

    Tracks the active SGR state to emit resets only when the style
    actually changes, minimizing output bytes.

    If *visual_style* is given (used by redraw visualization), it is appended
    to every cell's style.
    """
    rendered: list[str] = []
    active_escape = ""
    for cell in cells:
        if cell.controls:
            rendered.extend(cell.controls)
        if not cell.text:
            continue

        target_escape = _style_escape(cell.style)
        if visual_style is not None:
            target_escape += visual_style
        if target_escape != active_escape:
            if active_escape:
                rendered.append("\x1b[0m")
            if target_escape:
                rendered.append(target_escape)
            active_escape = target_escape
        rendered.append(cell.text)

    if active_escape:
        rendered.append("\x1b[0m")
    return "".join(rendered)


def diff_render_lines(old: RenderLine, new: RenderLine) -> LineDiff | None:
    if old == new:
        return None

    prefix = 0
    start_x = 0
    max_prefix = min(len(old.cells), len(new.cells))
    while prefix < max_prefix and old.cells[prefix] == new.cells[prefix]:
        # Stop at any cell with non-SGR controls, since those might affect
        # cursor position and must be re-emitted.
        if old.cells[prefix].controls:
            break
        start_x += old.cells[prefix].width
        prefix += 1

    old_suffix = len(old.cells)
    new_suffix = len(new.cells)
    while old_suffix > prefix and new_suffix > prefix:
        old_cell = old.cells[old_suffix - 1]
        new_cell = new.cells[new_suffix - 1]
        if old_cell.controls or new_cell.controls or old_cell != new_cell:
            break
        old_suffix -= 1
        new_suffix -= 1

    # Extend diff range to include trailing zero-width combining characters,
    # so we never render a combining char without its base character.
    while old_suffix < len(old.cells) and old.cells[old_suffix].width == 0:
        old_suffix += 1
    while new_suffix < len(new.cells) and new.cells[new_suffix].width == 0:
        new_suffix += 1

    return LineDiff(
        start_cell=prefix,
        start_x=start_x,
        old_cells=old.cells[prefix:old_suffix],
        new_cells=new.cells[prefix:new_suffix],
        old_width=old.width,
        new_width=new.width,
    )
