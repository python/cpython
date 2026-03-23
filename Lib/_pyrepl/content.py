from __future__ import annotations

from dataclasses import dataclass

from .utils import ColorSpan, StyleRef, THEME, iter_display_chars, unbracket, wlen


@dataclass(frozen=True, slots=True)
class ContentFragment:
    text: str
    width: int
    style: StyleRef = StyleRef()


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
    theme = THEME()
    return tuple(
        ContentFragment(
            styled_char.text,
            styled_char.width,
            StyleRef.from_tag(styled_char.tag, theme[styled_char.tag])
            if styled_char.tag else
            StyleRef(),
        )
        for styled_char in iter_display_chars(buffer, colors, start_index)
    )
