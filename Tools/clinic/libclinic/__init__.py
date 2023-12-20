from typing import Final

from .formatting import (
    c_repr,
    docstring_for_c_string,
    indent_all_lines,
    pprint_words,
    suffix_all_lines,
    wrapped_c_string_literal,
    SIG_END_MARKER,
)


CLINIC_PREFIX: Final[str] = "__clinic_"
CLINIC_PREFIXED_ARGS: Final[frozenset[str]] = frozenset({
    "_keywords",
    "_parser",
    "args",
    "argsbuf",
    "fastargs",
    "kwargs",
    "kwnames",
    "nargs",
    "noptargs",
    "return_value",
})


__all__ = [
    # Formatting helpers
    "c_repr",
    "docstring_for_c_string",
    "indent_all_lines",
    "pprint_words",
    "suffix_all_lines",
    "wrapped_c_string_literal",
    "SIG_END_MARKER",
]
