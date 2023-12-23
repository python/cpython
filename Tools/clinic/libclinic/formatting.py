"""A collection of string formatting helpers."""

import textwrap
from typing import Final


SIG_END_MARKER: Final = "--"


def docstring_for_c_string(docstring: str) -> str:
    lines = []
    # Turn docstring into a properly quoted C string.
    for line in docstring.split("\n"):
        lines.append('"')
        lines.append(_quoted_for_c_string(line))
        lines.append('\\n"\n')

    if lines[-2] == SIG_END_MARKER:
        # If we only have a signature, add the blank line that the
        # __text_signature__ getter expects to be there.
        lines.append('"\\n"')
    else:
        lines.pop()
        lines.append('"')
    return "".join(lines)


def _quoted_for_c_string(text: str) -> str:
    """Helper for docstring_for_c_string()."""
    for old, new in (
        ("\\", "\\\\"),  # must be first!
        ('"', '\\"'),
        ("'", "\\'"),
    ):
        text = text.replace(old, new)
    return text


def c_repr(text: str) -> str:
    return '"' + text + '"'


def wrapped_c_string_literal(
    text: str,
    *,
    width: int = 72,
    suffix: str = "",
    initial_indent: int = 0,
    subsequent_indent: int = 4
) -> str:
    wrapped = textwrap.wrap(
        text,
        width=width,
        replace_whitespace=False,
        drop_whitespace=False,
        break_on_hyphens=False,
    )
    separator = c_repr(suffix + "\n" + subsequent_indent * " ")
    return initial_indent * " " + c_repr(separator.join(wrapped))


def _add_prefix_and_suffix(
    text: str,
    prefix: str = "",
    suffix: str = ""
) -> str:
    """Return 'text' with 'prefix' prepended and 'suffix' appended to all lines.

    If the last line is empty, it remains unchanged.
    If text is blank, return text unchanged.

    (textwrap.indent only adds to non-blank lines.)
    """
    *split, last = text.split("\n")
    lines = [prefix + line + suffix + "\n" for line in split]
    if last:
        lines.append(prefix + last + suffix)
    return "".join(lines)


def indent_all_lines(text: str, prefix: str) -> str:
    return _add_prefix_and_suffix(text, prefix=prefix)


def suffix_all_lines(text: str, suffix: str) -> str:
    return _add_prefix_and_suffix(text, suffix=suffix)


def pprint_words(items: list[str]) -> str:
    if len(items) <= 2:
        return " and ".join(items)
    return ", ".join(items[:-1]) + " and " + items[-1]
