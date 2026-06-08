"""Wrap content lines to the terminal width before rendering."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Self

from .content import ContentFragment, ContentLine
from .types import CursorXY, ScreenInfoRow


@dataclass(frozen=True, slots=True)
class LayoutRow:
    """Metadata for one physical screen row.

    For the row ``>>> def greet(name):``::

        >>> def greet(name):
        ╰─╯ ╰──────────────╯
         4    char_widths=(1,1,1,…)  ← 16 entries
              buffer_advance=17      ← includes the newline
    """

    prompt_width: int
    char_widths: tuple[int, ...]
    suffix_width: int = 0
    buffer_advance: int = 0

    @property
    def width(self) -> int:
        return self.prompt_width + sum(self.char_widths) + self.suffix_width

    @property
    def screeninfo(self) -> ScreenInfoRow:
        widths = list(self.char_widths)
        if self.suffix_width:
            widths.append(self.suffix_width)
        return self.prompt_width, widths


@dataclass(frozen=True, slots=True)
class LayoutMap:
    """Mapping between buffer positions and screen coordinates.

    Single source of truth for cursor placement.  Given::

        >>> def greet(name):     ← row 0, buffer_advance=17
        ...     return name      ← row 1, buffer_advance=15
                       ▲cursor

    ``pos_to_xy(31)`` → ``(18, 1)``:  prompt width 4 + 14 body chars.
    """
    rows: tuple[LayoutRow, ...]

    @classmethod
    def empty(cls) -> Self:
        return cls((LayoutRow(0, ()),))

    @property
    def screeninfo(self) -> list[ScreenInfoRow]:
        return [row.screeninfo for row in self.rows]

    def max_column(self, y: int) -> int:
        return self.rows[y].width

    def max_row(self) -> int:
        return len(self.rows) - 1

    def pos_to_xy(self, pos: int) -> CursorXY:
        if not self.rows:
            return 0, 0

        remaining = pos
        for y, row in enumerate(self.rows):
            if remaining <= len(row.char_widths):
                # Prompt-only leading rows are terminal scenery, not real
                # buffer positions. Treating them as real just manufactures
                # bugs.
                if remaining == 0 and not row.char_widths and row.buffer_advance == 0 and y < len(self.rows) - 1:
                    continue
                x = row.prompt_width
                for width in row.char_widths[:remaining]:
                    x += width
                return x, y
            remaining -= row.buffer_advance
        last_row = self.rows[-1]
        return last_row.width - last_row.suffix_width, len(self.rows) - 1

    def xy_to_pos(self, x: int, y: int) -> int:
        if not self.rows:
            return 0

        pos = 0
        for row in self.rows[:y]:
            pos += row.buffer_advance

        row = self.rows[y]
        cur_x = row.prompt_width
        char_widths = row.char_widths
        i = 0
        for i, width in enumerate(char_widths):
            if cur_x >= x:
                # Include trailing zero-width (combining) chars at this position
                for trailing_width in char_widths[i:]:
                    if trailing_width == 0:
                        pos += 1
                    else:
                        break
                return pos
            if width == 0:
                pos += 1
                continue
            cur_x += width
            pos += 1
        return pos


@dataclass(frozen=True, slots=True)
class WrappedRow:
    """One physical screen row after wrapping, ready for rendering.

    When a line overflows the terminal width, it splits into
    multiple rows with a ``\\`` continuation marker::

        >>> x = "a very long li\\   ← suffix="\\", suffix_width=1
        ne that wraps"              ← prompt_text="" (continuation)
    """
    prompt_text: str = ""
    prompt_width: int = 0
    fragments: tuple[ContentFragment, ...] = ()
    layout_widths: tuple[int, ...] = ()
    suffix: str = ""
    suffix_width: int = 0
    buffer_advance: int = 0


@dataclass(frozen=True, slots=True)
class LayoutResult:
    wrapped_rows: tuple[WrappedRow, ...]
    layout_map: LayoutMap
    line_end_offsets: tuple[int, ...]


def layout_content_lines(
    lines: tuple[ContentLine, ...],
    width: int,
    start_offset: int,
) -> LayoutResult:
    """Wrap content lines to fit *width* columns.

    A short line passes through as one ``WrappedRow``; a long line is
    split at the column boundary with ``\\`` markers::

        >>> short = 1           ← one WrappedRow
        >>> x = "a long stri\\  ← two WrappedRows, first has suffix="\\"
        ng"
    """
    if width <= 0:
        return LayoutResult((), LayoutMap(()), ())

    offset = start_offset
    wrapped_rows: list[WrappedRow] = []
    layout_rows: list[LayoutRow] = []
    line_end_offsets: list[int] = []

    for line in lines:
        newline_advance = int(line.source.has_newline)
        for leading in line.prompt.leading_lines:
            line_end_offsets.append(offset)
            wrapped_rows.append(
                WrappedRow(
                    fragments=(leading,),
                )
            )
            layout_rows.append(LayoutRow(0, (), buffer_advance=0))

        prompt_text = line.prompt.text
        prompt_width = line.prompt.width
        body = tuple(line.body)
        body_widths = tuple(fragment.width for fragment in body)

        # Fast path: line fits on one row.
        if not body_widths or (sum(body_widths) + prompt_width) < width:
            offset += len(body) + newline_advance
            line_end_offsets.append(offset)
            wrapped_rows.append(
                WrappedRow(
                    prompt_text=prompt_text,
                    prompt_width=prompt_width,
                    fragments=body,
                    layout_widths=body_widths,
                    buffer_advance=len(body) + newline_advance,
                )
            )
            layout_rows.append(
                LayoutRow(
                    prompt_width,
                    body_widths,
                    buffer_advance=len(body) + newline_advance,
                )
            )
            continue

        # Slow path: line needs wrapping.
        current_prompt = prompt_text
        current_prompt_width = prompt_width
        start = 0
        total = len(body)
        while True:
            # Find how many characters fit on this row.
            index_to_wrap_before = 0
            column = 0
            for char_width in body_widths[start:]:
                if column + char_width + current_prompt_width >= width:
                    break
                index_to_wrap_before += 1
                column += char_width

            if index_to_wrap_before == 0 and start < total:
                index_to_wrap_before = 1  # force progress

            at_line_end = (start + index_to_wrap_before) >= total
            if at_line_end:
                offset += index_to_wrap_before + newline_advance
                suffix = ""
                suffix_width = 0
                buffer_advance = index_to_wrap_before + newline_advance
            else:
                offset += index_to_wrap_before
                suffix = "\\"
                suffix_width = 1
                buffer_advance = index_to_wrap_before

            end = start + index_to_wrap_before
            row_fragments = body[start:end]
            row_widths = body_widths[start:end]
            line_end_offsets.append(offset)
            wrapped_rows.append(
                WrappedRow(
                    prompt_text=current_prompt,
                    prompt_width=current_prompt_width,
                    fragments=row_fragments,
                    layout_widths=row_widths,
                    suffix=suffix,
                    suffix_width=suffix_width,
                    buffer_advance=buffer_advance,
                )
            )
            layout_rows.append(
                LayoutRow(
                    current_prompt_width,
                    row_widths,
                    suffix_width=suffix_width,
                    buffer_advance=buffer_advance,
                )
            )

            start = end
            current_prompt = ""
            current_prompt_width = 0
            if at_line_end:
                break

    return LayoutResult(
        tuple(wrapped_rows),
        LayoutMap(tuple(layout_rows)),
        tuple(line_end_offsets),
    )
