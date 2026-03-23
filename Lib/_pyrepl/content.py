from __future__ import annotations

from dataclasses import dataclass

from .utils import ColorSpan, disp_str, unbracket, wlen


@dataclass(frozen=True, slots=True)
class ContentFragment:
    text: str
    width: int


@dataclass(frozen=True, slots=True)
class PromptContent:
    leading_lines: tuple[ContentFragment, ...]
    text: str
    width: int


@dataclass(frozen=True, slots=True)
class SourceLine:
    lineno: int
    text: str
    start_offset: int
    has_newline: bool
    cursor_index: int | None = None

    @property
    def cursor_on_line(self) -> bool:
        return self.cursor_index is not None


@dataclass(frozen=True, slots=True)
class ContentLine:
    source: SourceLine
    prompt: PromptContent
    body: tuple[ContentFragment, ...]


def process_prompt(prompt: str) -> PromptContent:
    r"""Return prompt content with width measured without zero-width markup."""

    prompt_text = unbracket(prompt, including_content=False)
    visible_prompt = unbracket(prompt, including_content=True)
    leading_lines: list[ContentFragment] = []

    while "\n" in prompt_text:
        leading_text, _, prompt_text = prompt_text.partition("\n")
        visible_leading, _, visible_prompt = visible_prompt.partition("\n")
        leading_lines.append(ContentFragment(leading_text, wlen(visible_leading)))

    return PromptContent(tuple(leading_lines), prompt_text, wlen(visible_prompt))


def build_body_fragments(
    buffer: str,
    colors: list[ColorSpan] | None,
    start_index: int,
) -> tuple[ContentFragment, ...]:
    chars, char_widths = disp_str(buffer, colors, start_index)
    return tuple(
        ContentFragment(text, width)
        for text, width in zip(chars, char_widths)
    )
