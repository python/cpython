"""
Generate enumeration for XSAVE state components (XCR0 control register).
"""

from __future__ import annotations

__all__ = ["generate_xsave_features_enum"]

from functools import partial
from io import StringIO
from typing import TYPE_CHECKING
from . import _util as util

if TYPE_CHECKING:
    from typing import Final

    type Feature = str
    type Bit = int

XSAVE_FEATURES: Final[dict[Feature, Bit]] = {
    "SSE": 1,
    "AVX": 2,
    "AVX512_OPMASK": 5,
    "AVX512_ZMM_HI256": 6,
    "AVX512_HI16_ZMM": 7,
}


def get_member_name(feature: Feature) -> str:
    return f"Py_XSAVE_MASK_XCR0_{feature}"


NAMESIZE: Final[int] = util.next_block(
    max(map(len, map(get_member_name, XSAVE_FEATURES)))
)


def generate_xsave_features_enum(enum_name: str) -> str:
    # The enumeration is rendered as follows:
    #
    #   <INDENT><MEMBER_NAME> <TAB>= 0x<MASK>, <TAB>// bit = BIT
    #   ^       ^             ^    ^   ^       ^    ^
    #
    # where ^ indicates a column that is a multiple of 4, <MASK> has
    # exactly 8 characters and <BIT> has at most 2 characters.

    output = StringIO()
    write = partial(print, file=output)
    indent = " " * 4

    write(f"typedef enum {enum_name} {{")
    for feature_name, bit in XSAVE_FEATURES.items():
        if not 0 <= bit < 32:
            raise ValueError(f"invalid bit value for {feature_name!r}")
        key = get_member_name(feature_name)
        member_def = util.make_enum_member(key, bit, NAMESIZE)
        write(indent, member_def, sep="")
    write(f"}} {enum_name};")
    return output.getvalue().rstrip("\n")
