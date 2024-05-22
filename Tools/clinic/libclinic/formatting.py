"""A collection of string formatting helpers."""

import functools
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


def _add_prefix_and_suffix(text: str, *, prefix: str = "", suffix: str = "") -> str:
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


def _strip_leading_and_trailing_blank_lines(text: str) -> str:
    lines = text.rstrip().split("\n")
    while lines:
        line = lines[0]
        if line.strip():
            break
        del lines[0]
    return "\n".join(lines)


@functools.lru_cache()
def normalize_snippet(text: str, *, indent: int = 0) -> str:
    """
    Reformats 'text':
        * removes leading and trailing blank lines
        * ensures that it does not end with a newline
        * dedents so the first nonwhite character on any line is at column "indent"
    """
    text = _strip_leading_and_trailing_blank_lines(text)
    text = textwrap.dedent(text)
    if indent:
        text = textwrap.indent(text, " " * indent)
    return text


def format_escape(text: str) -> str:
    # double up curly-braces, this string will be used
    # as part of a format_map() template later
    text = text.replace("{", "{{")
    text = text.replace("}", "}}")
    return text


def wrap_declarations(text: str, length: int = 78) -> str:
    """
    A simple-minded text wrapper for C function declarations.

    It views a declaration line as looking like this:
        xxxxxxxx(xxxxxxxxx,xxxxxxxxx)
    If called with length=30, it would wrap that line into
        xxxxxxxx(xxxxxxxxx,
                 xxxxxxxxx)
    (If the declaration has zero or one parameters, this
    function won't wrap it.)

    If this doesn't work properly, it's probably better to
    start from scratch with a more sophisticated algorithm,
    rather than try and improve/debug this dumb little function.
    """
    lines = []
    for line in text.split("\n"):
        prefix, _, after_l_paren = line.partition("(")
        if not after_l_paren:
            lines.append(line)
            continue
        in_paren, _, after_r_paren = after_l_paren.partition(")")
        if not _:
            lines.append(line)
            continue
        if "," not in in_paren:
            lines.append(line)
            continue
        parameters = [x.strip() + ", " for x in in_paren.split(",")]
        prefix += "("
        if len(prefix) < length:
            spaces = " " * len(prefix)
        else:
            spaces = " " * 4

        while parameters:
            line = prefix
            first = True
            while parameters:
                if not first and (len(line) + len(parameters[0]) > length):
                    break
                line += parameters.pop(0)
                first = False
            if not parameters:
                line = line.rstrip(", ") + ")" + after_r_paren
            lines.append(line.rstrip())
            prefix = spaces
    return "\n".join(lines)
