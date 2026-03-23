from __future__ import annotations

from dataclasses import dataclass

from .content import ContentFragment, ContentLine


@dataclass(frozen=True, slots=True)
class LayoutRow:
    prompt_width: int
    char_widths: tuple[int, ...]
    suffix_width: int = 0
    buffer_advance: int = 0

    @property
    def width(self) -> int:
        return self.prompt_width + sum(self.char_widths) + self.suffix_width

    @property
    def screeninfo(self) -> tuple[int, list[int]]:
        widths = list(self.char_widths)
        if self.suffix_width:
            widths.append(self.suffix_width)
        return self.prompt_width, widths


@dataclass(frozen=True, slots=True)
class LayoutMap:
    rows: tuple[LayoutRow, ...]

    @classmethod
    def empty(cls) -> LayoutMap:
        return cls((LayoutRow(0, ()),))

    @property
    def screeninfo(self) -> list[tuple[int, list[int]]]:
        return [row.screeninfo for row in self.rows]

    def max_column(self, y: int) -> int:
        return self.rows[y].width

    def max_row(self) -> int:
        return len(self.rows) - 1

    def pos_to_xy(self, pos: int) -> tuple[int, int]:
        if not self.rows:
            return 0, 0

        remaining = pos
        for y, row in enumerate(self.rows):
            if remaining <= len(row.char_widths):
                x = row.prompt_width
                for width in row.char_widths[:remaining]:
                    x += width
                return x, y
            remaining -= row.buffer_advance
        last_row = self.rows[-1]
        return last_row.width - last_row.suffix_width, len(self.rows) - 1

    def xy_to_pos(self, x: int, y: int) -> int:
        pos = 0
        for row in self.rows[:y]:
            pos += row.buffer_advance

        row = self.rows[y]
        cur_x = row.prompt_width
        for width in row.char_widths:
            if cur_x >= x:
                break
            if width == 0:
                pos += 1
                continue
            cur_x += width
            pos += 1
        return pos


@dataclass(frozen=True, slots=True)
class WrappedRow:
    prompt_text: str = ""
    prompt_width: int = 0
    fragments: tuple[ContentFragment, ...] = ()
    layout_widths: tuple[int, ...] = ()
    suffix: str = ""
    suffix_width: int = 0
    buffer_advance: int = 0
    line_end_offset: int = 0


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
    offset = start_offset
    wrapped_rows: list[WrappedRow] = []
    layout_rows: list[LayoutRow] = []
    line_end_offsets: list[int] = []

    for line in lines:
        for leading in line.prompt.leading_lines:
            line_end_offsets.append(offset)
            wrapped_rows.append(
                WrappedRow(
                    fragments=(leading,),
                    line_end_offset=offset,
                )
            )
            layout_rows.append(LayoutRow(0, (), buffer_advance=0))

        prompt_text = line.prompt.text
        prompt_width = line.prompt.width
        remaining = list(line.body)
        remaining_widths = [fragment.width for fragment in remaining]

        if not remaining_widths or (sum(remaining_widths) + prompt_width) // width == 0:
            offset += len(remaining) + (1 if line.source.has_newline else 0)
            line_end_offsets.append(offset)
            wrapped_rows.append(
                WrappedRow(
                    prompt_text=prompt_text,
                    prompt_width=prompt_width,
                    fragments=tuple(remaining),
                    layout_widths=tuple(remaining_widths),
                    buffer_advance=len(remaining) + (1 if line.source.has_newline else 0),
                    line_end_offset=offset,
                )
            )
            layout_rows.append(
                LayoutRow(
                    prompt_width,
                    tuple(remaining_widths),
                    buffer_advance=len(remaining) + (1 if line.source.has_newline else 0),
                )
            )
            continue

        current_prompt = prompt_text
        current_prompt_width = prompt_width
        while True:
            index_to_wrap_before = 0
            column = 0
            for char_width in remaining_widths:
                if column + char_width + current_prompt_width >= width:
                    break
                index_to_wrap_before += 1
                column += char_width

            at_line_end = len(remaining) <= index_to_wrap_before
            if at_line_end:
                offset += index_to_wrap_before + (1 if line.source.has_newline else 0)
                suffix = ""
                suffix_width = 0
                buffer_advance = index_to_wrap_before + (1 if line.source.has_newline else 0)
            else:
                offset += index_to_wrap_before
                suffix = "\\"
                suffix_width = 1
                buffer_advance = index_to_wrap_before

            row_fragments = tuple(remaining[:index_to_wrap_before])
            row_widths = tuple(remaining_widths[:index_to_wrap_before])
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
                    line_end_offset=offset,
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

            remaining = remaining[index_to_wrap_before:]
            remaining_widths = remaining_widths[index_to_wrap_before:]
            current_prompt = ""
            current_prompt_width = 0
            if at_line_end:
                break

    return LayoutResult(
        tuple(wrapped_rows),
        LayoutMap(tuple(layout_rows)),
        tuple(line_end_offsets),
    )
