"""A collection of string formatting helpers."""

import functools
import textwrap
from typing import Final

from libclinic import ClinicError


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


# Use octals, because \x... in C has arbitrary number of hexadecimal digits.
_c_repr = [chr(i) if 32 <= i < 127 else fr'\{i:03o}' for i in range(256)]
_c_repr[ord('"')] = r'\"'
_c_repr[ord('\\')] = r'\\'
_c_repr[ord('\a')] = r'\a'
_c_repr[ord('\b')] = r'\b'
_c_repr[ord('\f')] = r'\f'
_c_repr[ord('\n')] = r'\n'
_c_repr[ord('\r')] = r'\r'
_c_repr[ord('\t')] = r'\t'
_c_repr[ord('\v')] = r'\v'

def _break_trigraphs(s: str) -> str:
    # Prevent trigraphs from being interpreted inside string literals.
    if '??' in s:
        s = s.replace('??', r'?\?')
        s = s.replace(r'\??', r'\?\?')
    # Also Argument Clinic does not like comment-like sequences
    # in string literals.
    s = s.replace(r'/*', r'/\*')
    s = s.replace(r'*/', r'*\/')
    return s

def c_bytes_repr(data: bytes) -> str:
    r = ''.join(_c_repr[i] for i in data)
    r = _break_trigraphs(r)
    return '"' + r + '"'

def c_str_repr(text: str) -> str:
    r = ''.join(_c_repr[i] if i < 0x80
                else fr'\u{i:04x}' if i < 0x10000
                else fr'\U{i:08x}'
                for i in map(ord, text))
    r = _break_trigraphs(r)
    return '"' + r + '"'

def c_unichar_repr(char: str) -> str:
    if char == "'":
        return r"'\''"
    if char == '"':
        return """'"'"""
    if char == '\0':
        return '0'
    i = ord(char)
    if i < 0x80:
        r = _c_repr[i]
        if not r.startswith((r'\0', r'\1')):
            return "'" + r + "'"
    return f'0x{i:02x}'


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
    separator = '"' + suffix + "\n" + subsequent_indent * " " + '"'
    return initial_indent * " " + '"' + separator.join(wrapped) + '"'


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


def linear_format(text: str, **kwargs: str) -> str:
    """
    Perform str.format-like substitution, except:
      * The strings substituted must be on lines by
        themselves.  (This line is the "source line".)
      * If the substitution text is empty, the source line
        is removed in the output.
      * If the field is not recognized, the original line
        is passed unmodified through to the output.
      * If the substitution text is not empty:
          * Each line of the substituted text is indented
            by the indent of the source line.
          * A newline will be added to the end.
    """
    lines = []
    for line in text.split("\n"):
        indent, curly, trailing = line.partition("{")
        if not curly:
            lines.extend([line, "\n"])
            continue

        name, curly, trailing = trailing.partition("}")
        if not curly or name not in kwargs:
            lines.extend([line, "\n"])
            continue

        if trailing:
            raise ClinicError(
                f"Text found after '{{{name}}}' block marker! "
                "It must be on a line by itself."
            )
        if indent.strip():
            raise ClinicError(
                f"Non-whitespace characters found before '{{{name}}}' block marker! "
                "It must be on a line by itself."
            )

        value = kwargs[name]
        if not value:
            continue

        stripped = [line.rstrip() for line in value.split("\n")]
        value = textwrap.indent("\n".join(stripped), indent)
        lines.extend([value, "\n"])

    return "".join(lines[:-1])
