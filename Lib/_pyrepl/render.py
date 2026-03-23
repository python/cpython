from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass

from .utils import ANSI_ESCAPE_SEQUENCE, str_width, wlen


@dataclass(frozen=True, slots=True)
class RenderCell:
    text: str
    width: int
    prefix: str = ""
    suffix: str = ""

    @classmethod
    def from_rendered_text(
        cls,
        text: str,
        width: int | None = None,
    ) -> RenderCell:
        prefix_end = 0
        while match := ANSI_ESCAPE_SEQUENCE.match(text, prefix_end):
            prefix_end = match.end()

        suffix_start = len(text)
        chain_start = len(text)
        chain_end = -1
        for match in ANSI_ESCAPE_SEQUENCE.finditer(text, prefix_end):
            if match.start() == chain_end:
                chain_end = match.end()
            else:
                chain_start = match.start()
                chain_end = match.end()
        if chain_end == len(text):
            suffix_start = chain_start

        visible_text = text[prefix_end:suffix_start]
        if width is None:
            width = wlen(visible_text)
        return cls(
            text=visible_text,
            width=width,
            prefix=text[:prefix_end],
            suffix=text[suffix_start:],
        )

    @property
    def terminal_text(self) -> str:
        return f"{self.prefix}{self.text}{self.suffix}"


@dataclass(frozen=True, slots=True)
class RenderLine:
    cells: tuple[RenderCell, ...]
    text: str
    width: int

    @classmethod
    def from_cells(cls, cells: Iterable[RenderCell]) -> RenderLine:
        normalized_cells: list[RenderCell] = []
        for cell in cells:
            if cell.suffix:
                normalized_cells.append(
                    RenderCell(cell.text, cell.width, prefix=cell.prefix)
                )
                normalized_cells.append(RenderCell("", 0, prefix=cell.suffix))
            else:
                normalized_cells.append(cell)
        cell_tuple = tuple(normalized_cells)
        return cls(
            cells=cell_tuple,
            text="".join(cell.terminal_text for cell in cell_tuple),
            width=sum(cell.width for cell in cell_tuple),
        )

    @classmethod
    def from_parts(
        cls,
        parts: Sequence[str],
        widths: Sequence[int],
    ) -> RenderLine:
        return cls.from_cells(
            RenderCell.from_rendered_text(text, width)
            for text, width in zip(parts, widths)
        )

    @classmethod
    def from_rendered_text(cls, text: str) -> RenderLine:
        if not text:
            return cls(cells=(), text="", width=0)

        cells: list[RenderCell] = []
        pending_escape = ""
        index = 0
        for match in ANSI_ESCAPE_SEQUENCE.finditer(text):
            pending_escape = cls._append_plain_text(
                cells, text[index : match.start()], pending_escape
            )
            pending_escape += match.group(0)
            index = match.end()

        pending_escape = cls._append_plain_text(cells, text[index:], pending_escape)

        if pending_escape:
            if cells:
                last = cells[-1]
                cells[-1] = RenderCell(
                    text=last.text,
                    width=last.width,
                    prefix=last.prefix,
                    suffix=last.suffix + pending_escape,
                )
            else:
                cells.append(RenderCell("", 0, prefix=pending_escape))

        return cls.from_cells(cells)

    @staticmethod
    def _append_plain_text(
        cells: list[RenderCell],
        text: str,
        pending_escape: str,
    ) -> str:
        for char in text:
            cells.append(RenderCell(char, str_width(char), prefix=pending_escape))
            pending_escape = ""
        return pending_escape


@dataclass(frozen=True, slots=True)
class ScreenOverlay:
    y: int
    lines: tuple[RenderLine, ...]


@dataclass(frozen=True, slots=True)
class RenderedScreen:
    lines: tuple[RenderLine, ...]
    cursor: tuple[int, int]
    overlays: tuple[ScreenOverlay, ...] = ()

    @classmethod
    def empty(cls) -> RenderedScreen:
        return cls((), (0, 0), ())

    @classmethod
    def from_screen_lines(
        cls,
        screen: Sequence[str],
        cursor: tuple[int, int],
    ) -> RenderedScreen:
        return cls(
            tuple(RenderLine.from_rendered_text(line) for line in screen),
            cursor,
            (),
        )

    def with_overlay(
        self,
        y: int,
        lines: Iterable[RenderLine],
    ) -> RenderedScreen:
        return RenderedScreen(
            self.lines,
            self.cursor,
            self.overlays + (ScreenOverlay(y, tuple(lines)),),
        )

    @property
    def composed_lines(self) -> tuple[RenderLine, ...]:
        if not self.overlays:
            return self.lines

        lines = list(self.lines)
        for overlay in self.overlays:
            target_len = overlay.y + len(overlay.lines)
            if len(lines) < target_len:
                lines.extend([EMPTY_RENDER_LINE] * (target_len - len(lines)))
            for index, line in enumerate(overlay.lines):
                lines[overlay.y + index] = line
        return tuple(lines)

    @property
    def screen_lines(self) -> tuple[str, ...]:
        return tuple(line.text for line in self.composed_lines)


@dataclass(frozen=True, slots=True)
class LineDiff:
    start_cell: int
    start_x: int
    old_cells: tuple[RenderCell, ...]
    new_cells: tuple[RenderCell, ...]
    old_width: int
    new_width: int

    @property
    def old_text(self) -> str:
        return "".join(cell.terminal_text for cell in self.old_cells)

    @property
    def new_text(self) -> str:
        return "".join(cell.terminal_text for cell in self.new_cells)

    @property
    def old_changed_width(self) -> int:
        return sum(cell.width for cell in self.old_cells)

    @property
    def new_changed_width(self) -> int:
        return sum(cell.width for cell in self.new_cells)


EMPTY_RENDER_LINE = RenderLine(cells=(), text="", width=0)


@dataclass(frozen=True, slots=True)
class LineUpdate:
    kind: str
    y: int
    start_cell: int
    start_x: int
    cells: tuple[RenderCell, ...]
    char_width: int = 0
    clear_eol: bool = False
    reset_to_margin: bool = False

    @property
    def text(self) -> str:
        return "".join(cell.terminal_text for cell in self.cells)


def _update_terminal_state(state: str, text: str) -> str:
    for match in ANSI_ESCAPE_SEQUENCE.finditer(text):
        escape = match.group(0)
        if escape in {"\x1b[0m", "\x1b[m"}:
            state = ""
        else:
            state += escape
    return state


def _text_requires_cursor_resync(text: str) -> bool:
    return any(
        match.group(0)[-1] != "m"
        for match in ANSI_ESCAPE_SEQUENCE.finditer(text)
    )


def requires_cursor_resync(cells: Sequence[RenderCell]) -> bool:
    return any(
        _text_requires_cursor_resync(cell.prefix)
        or _text_requires_cursor_resync(cell.suffix)
        for cell in cells
    )


def active_prefix_before(line: RenderLine, stop_cell: int) -> str:
    state = ""
    for cell in line.cells[:stop_cell]:
        state = _update_terminal_state(state, cell.prefix)
        state = _update_terminal_state(state, cell.suffix)
    return state


def with_active_prefix(
    line: RenderLine,
    start_cell: int,
    cells: Sequence[RenderCell],
) -> tuple[RenderCell, ...]:
    prefix = active_prefix_before(line, start_cell)
    if not prefix or not cells:
        return tuple(cells)

    first = cells[0]
    replayed = RenderCell(
        text=first.text,
        width=first.width,
        prefix=prefix + first.prefix,
        suffix=first.suffix,
    )
    return (replayed, *cells[1:])


def render_cells(
    cells: Sequence[RenderCell],
    visual_style: str | None = None,
) -> str:
    if visual_style is None:
        return "".join(cell.terminal_text for cell in cells)

    rendered: list[str] = []
    for cell in cells:
        if cell.prefix:
            rendered.append(cell.prefix)
        if cell.text:
            rendered.append(visual_style)
            rendered.append(cell.text)
            rendered.append("\x1b[0m")
        if cell.suffix:
            rendered.append(cell.suffix)
    return "".join(rendered)


def diff_render_lines(old: RenderLine, new: RenderLine) -> LineDiff | None:
    if old == new:
        return None

    prefix = 0
    start_x = 0
    max_prefix = min(len(old.cells), len(new.cells))
    while prefix < max_prefix and old.cells[prefix] == new.cells[prefix]:
        start_x += old.cells[prefix].width
        prefix += 1

    old_suffix = len(old.cells)
    new_suffix = len(new.cells)
    while (
        old_suffix > prefix
        and new_suffix > prefix
        and old.cells[old_suffix - 1] == new.cells[new_suffix - 1]
    ):
        old_suffix -= 1
        new_suffix -= 1

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
