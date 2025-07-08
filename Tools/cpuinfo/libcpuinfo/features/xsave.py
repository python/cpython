"""
Generate constants for XSAVE state components (XCR0 control register).

See https://en.wikipedia.org/wiki/Control_register#XCR0_and_XSS.

.. seealso:: :file:`Include/internal/pycore_cpuinfo_xsave_features.h`
"""

from __future__ import annotations

__all__ = ["make_xsave_features_constants"]

from typing import TYPE_CHECKING

import libcpuinfo.util as util
from libcpuinfo.util import DOXYGEN_STYLE

if TYPE_CHECKING:
    from typing import Final

    type Feature = str
    type BitIndex = int

XSAVE_FEATURES: Final[dict[Feature, BitIndex]] = {
    "SSE": 1,
    "AVX": 2,
    "AVX512_OPMASK": 5,
    "AVX512_ZMM_HI256": 6,
    "AVX512_HI16_ZMM": 7,
}


def get_constant_name(feature: Feature) -> str:
    return f"_Py_XSAVE_MASK_XCR0_{feature}"


_NAME_MAXSIZE: Final[int] = util.next_block(
    max(map(len, map(get_constant_name, XSAVE_FEATURES)))
)


def make_xsave_features_constants() -> str:
    """Used by :file:`Include/internal/pycore_cpuinfo_xsave_features.h`."""
    writer = util.CWriter()
    writer.comment("Constants for XSAVE components", style=DOXYGEN_STYLE)
    for feature_name, bit in XSAVE_FEATURES.items():
        if not 0 <= bit < 32:
            raise ValueError(f"invalid bit value for {feature_name!r}")
        key = get_constant_name(feature_name)
        writer.write(util.make_constant(key, bit, _NAME_MAXSIZE))
    return writer.build()
