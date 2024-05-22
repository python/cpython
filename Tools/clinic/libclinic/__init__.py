from typing import Final

from .errors import (
    ClinicError,
)
from .formatting import (
    SIG_END_MARKER,
    c_repr,
    docstring_for_c_string,
    format_escape,
    indent_all_lines,
    normalize_snippet,
    pprint_words,
    suffix_all_lines,
    wrap_declarations,
    wrapped_c_string_literal,
)


__all__ = [
    # Error handling
    "ClinicError",

    # Formatting helpers
    "SIG_END_MARKER",
    "c_repr",
    "docstring_for_c_string",
    "format_escape",
    "indent_all_lines",
    "normalize_snippet",
    "pprint_words",
    "suffix_all_lines",
    "wrap_declarations",
    "wrapped_c_string_literal",
]


CLINIC_PREFIX: Final = "__clinic_"
CLINIC_PREFIXED_ARGS: Final = frozenset(
    {
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
    }
)
