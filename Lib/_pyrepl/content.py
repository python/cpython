from __future__ import annotations

from dataclasses import dataclass

from .utils import ColorSpan, StyleRef, THEME, iter_display_chars, unbracket, wlen


@dataclass(frozen=True, slots=True)
class ContentFragment:
    """A single display character with its visual width and style.

    The body of ``>>> def greet`` becomes one fragment per character::

        d  e  f     g  r  e  e  t
        ╰──┴──╯     ╰──┴──┴──┴──╯
        keyword       (unstyled)

    e.g. ``ContentFragment("d", 1, StyleRef(tag="keyword"))``.
    """

    text: str
    width: int
    style: StyleRef = StyleRef()


@dataclass(frozen=True, slots=True)
class PromptContent:
    """The prompt split into leading full-width lines and an inline portion.

    For the common ``">>> "`` prompt (no newlines)::

        >>> def greet(name):
        ╰─╯
        text=">>> ", width=4, leading_lines=()

    If ``sys.ps1`` contains newlines, e.g. ``"Python 3.13\\n>>> "``::

        Python 3.13              ← leading_lines[0]
        >>> def greet(name):
        ╰─╯
        text=">>> ", width=4
    """

    leading_lines: tuple[ContentFragment, ...]
    text: str
    width: int


@dataclass(frozen=True, slots=True)
class SourceLine:
    """One logical line from the editor buffer, before styling.

    Given this two-line input in the REPL::

        >>> def greet(name):
        ...     return name
                       ▲ cursor

    The buffer ``"def greet(name):\\n    return name"`` yields::

        SourceLine(lineno=0, text="def greet(name):",
                   start_offset=0, has_newline=True)
        SourceLine(lineno=1, text="    return name",
                   start_offset=17, cursor_index=14)
    """

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
    """A logical line paired with its prompt and styled body.

    For ``>>> def greet(name):``::

        >>> def greet(name):
        ╰─╯ ╰──────────────╯
        prompt       body: one ContentFragment per character
    """

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
    """Convert a line's text into styled content fragments."""
    # Two separate loops to avoid the THEME() call in the common uncolored path.
    if colors is None:
        return tuple(
            ContentFragment(
                styled_char.text,
                styled_char.width,
                StyleRef(),
            )
            for styled_char in iter_display_chars(buffer, colors, start_index)
        )

    theme = THEME()
    return tuple(
        ContentFragment(
            styled_char.text,
            styled_char.width,
            StyleRef.from_tag(styled_char.tag, theme[styled_char.tag])
            if styled_char.tag
            else StyleRef(),
        )
        for styled_char in iter_display_chars(buffer, colors, start_index)
    )
