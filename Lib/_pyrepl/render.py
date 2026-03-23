from __future__ import annotations

from collections.abc import Iterable, Sequence
from dataclasses import dataclass

from .utils import ANSI_ESCAPE_SEQUENCE, str_width


@dataclass(frozen=True, slots=True)
class RenderCell:
    text: str
    width: int
    has_escape: bool = False


@dataclass(frozen=True, slots=True)
class RenderLine:
    cells: tuple[RenderCell, ...]
    text: str
    width: int

    @classmethod
    def from_cells(cls, cells: Iterable[RenderCell]) -> RenderLine:
        cell_tuple = tuple(cells)
        return cls(
            cells=cell_tuple,
            text="".join(cell.text for cell in cell_tuple),
            width=sum(cell.width for cell in cell_tuple),
        )

    @classmethod
    def from_parts(
        cls,
        parts: Sequence[str],
        widths: Sequence[int],
    ) -> RenderLine:
        return cls.from_cells(
            RenderCell(text, width, "\x1b" in text)
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
                    text=last.text + pending_escape,
                    width=last.width,
                    has_escape=True,
                )
            else:
                cells.append(RenderCell(pending_escape, 0, True))

        return cls.from_cells(cells)

    @staticmethod
    def _append_plain_text(
        cells: list[RenderCell],
        text: str,
        pending_escape: str,
    ) -> str:
        for char in text:
            rendered = pending_escape + char
            cells.append(RenderCell(rendered, str_width(char), bool(pending_escape)))
            pending_escape = ""
        return pending_escape


@dataclass(frozen=True, slots=True)
class RenderedScreen:
    lines: tuple[RenderLine, ...]
    cursor: tuple[int, int]

    @classmethod
    def empty(cls) -> RenderedScreen:
        return cls((), (0, 0))

    @classmethod
    def from_screen_lines(
        cls,
        screen: Sequence[str],
        cursor: tuple[int, int],
    ) -> RenderedScreen:
        return cls(
            tuple(RenderLine.from_rendered_text(line) for line in screen),
            cursor,
        )

    @property
    def screen_lines(self) -> tuple[str, ...]:
        return tuple(line.text for line in self.lines)


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
        return "".join(cell.text for cell in self.old_cells)

    @property
    def new_text(self) -> str:
        return "".join(cell.text for cell in self.new_cells)

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
    text: str
    char_width: int = 0
    clear_eol: bool = False
    reset_to_margin: bool = False


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

    return LineDiff(
        start_cell=prefix,
        start_x=start_x,
        old_cells=old.cells[prefix:old_suffix],
        new_cells=new.cells[prefix:new_suffix],
        old_width=old.width,
        new_width=new.width,
    )
