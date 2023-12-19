from __future__ import annotations

import dataclasses as dc
import enum
import hashlib
import re
from typing import (
    Callable,
    Final,
    Literal,
    NamedTuple,
    NoReturn,
    overload,
)


# Clinic instance
clinic = None


@dc.dataclass
class ClinicError(Exception):
    message: str
    _: dc.KW_ONLY
    lineno: int | None = None
    filename: str | None = None

    def __post_init__(self) -> None:
        super().__init__(self.message)

    def report(self, *, warn_only: bool = False) -> str:
        msg = "Warning" if warn_only else "Error"
        if self.filename is not None:
            msg += f" in file {self.filename!r}"
        if self.lineno is not None:
            msg += f" on line {self.lineno}"
        msg += ":\n"
        msg += f"{self.message}\n"
        return msg

@overload
def warn_or_fail(
    *args: object,
    fail: Literal[True],
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn: ...

@overload
def warn_or_fail(
    *args: object,
    fail: Literal[False] = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None: ...


def warn_or_fail(
    *args: object,
    fail: bool = False,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    joined = " ".join([str(a) for a in args])
    if clinic:
        if filename is None:
            filename = clinic.filename
        if getattr(clinic, 'block_parser', None) and (line_number is None):
            line_number = clinic.block_parser.line_number
    error = ClinicError(joined, filename=filename, lineno=line_number)
    if fail:
        raise error
    else:
        print(error.report(warn_only=True))


def warn(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> None:
    return warn_or_fail(*args, filename=filename, line_number=line_number, fail=False)


def fail(
    *args: object,
    filename: str | None = None,
    line_number: int | None = None,
) -> NoReturn:
    warn_or_fail(*args, filename=filename, line_number=line_number, fail=True)


VersionTuple = tuple[int, int]
TemplateDict = dict[str, str]


def compute_checksum(
        input: str | None,
        length: int | None = None
) -> str:
    input = input or ''
    s = hashlib.sha1(input.encode('utf-8')).hexdigest()
    if length:
        s = s[:length]
    return s


class Sentinels(enum.Enum):
    unspecified = "unspecified"
    unknown = "unknown"

    def __repr__(self) -> str:
        return f"<{self.value.capitalize()}>"


unspecified: Final = Sentinels.unspecified
unknown: Final = Sentinels.unknown


def c_repr(s: str) -> str:
    return '"' + s + '"'


Appender = Callable[[str], None]
Outputter = Callable[[], str]

class _TextAccumulator(NamedTuple):
    text: list[str]
    append: Appender
    output: Outputter

def _text_accumulator() -> _TextAccumulator:
    text: list[str] = []
    def output() -> str:
        s = ''.join(text)
        text.clear()
        return s
    return _TextAccumulator(text, text.append, output)


class TextAccumulator(NamedTuple):
    append: Appender
    output: Outputter

def text_accumulator() -> TextAccumulator:
    """
    Creates a simple text accumulator / joiner.

    Returns a pair of callables:
        append, output
    "append" appends a string to the accumulator.
    "output" returns the contents of the accumulator
       joined together (''.join(accumulator)) and
       empties the accumulator.
    """
    text, append, output = _text_accumulator()
    return TextAccumulator(append, output)


def create_regex(
        before: str,
        after: str,
        word: bool = True,
        whole_line: bool = True
) -> re.Pattern[str]:
    """Create an re object for matching marker lines."""
    group_re = r"\w+" if word else ".+"
    pattern = r'{}({}){}'
    if whole_line:
        pattern = '^' + pattern + '$'
    pattern = pattern.format(re.escape(before), group_re, re.escape(after))
    return re.compile(pattern)


is_legal_c_identifier = re.compile('^[A-Za-z_][A-Za-z0-9_]*$').match

def is_legal_py_identifier(s: str) -> bool:
    return all(is_legal_c_identifier(field) for field in s.split('.'))


# identifiers that are okay in Python but aren't a good idea in C.
# so if they're used Argument Clinic will add "_value" to the end
# of the name in C.
c_keywords = set("""
asm auto break case char const continue default do double
else enum extern float for goto if inline int long
register return short signed sizeof static struct switch
typedef typeof union unsigned void volatile while
""".strip().split())

def ensure_legal_c_identifier(s: str) -> str:
    # for now, just complain if what we're given isn't legal
    if not is_legal_c_identifier(s):
        fail("Illegal C identifier:", s)
    # but if we picked a C keyword, pick something else
    if s in c_keywords:
        return s + "_value"
    return s
