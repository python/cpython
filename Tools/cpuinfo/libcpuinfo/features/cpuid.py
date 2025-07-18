"""
Generate an enumeration describing masks to apply on CPUID output registers.

Constants are _Py_CPUID_MASK_<REGISTER>_L<LEAF>[S<SUBLEAF>]_<FEATURE>,
where <> (resp. []) denotes a required (resp. optional) group and:

- REGISTER is EAX, EBX, ECX or EDX,
- LEAF is the initial value of the EAX register (1 or 7),
- SUBLEAF is the initial value of the ECX register (omitted if 0), and
- FEATURE is a SIMD feature (with one or more specialized instructions).

For maintainability, the flags are ordered by registers, leafs, subleafs,
and bits. See https://en.wikipedia.org/wiki/CPUID for the values.

Note 1: The LEAF is also called the 'page' or the 'level'.
Note 2: The SUBLEAF is also referred to as the 'count'.

The LEAF value should only 1 or 7 as other values may have different
meanings depending on the underlying architecture.

.. seealso:: :file:`Include/internal/pycore_cpuinfo_cpuid_features.h`
"""

from __future__ import annotations

__all__ = ["make_cpuid_features_constants"]

from typing import TYPE_CHECKING

import libcpuinfo.util as util
from libcpuinfo.util import DOXYGEN_STYLE

if TYPE_CHECKING:
    from typing import Final

    type Leaf = int
    type SubLeaf = int
    type Registry = str
    type FeatureFamily = tuple[Leaf, SubLeaf, Registry]

    type Feature = str
    type BitIndex = int

CPUID_FEATURES: Final[dict[FeatureFamily, dict[Feature, BitIndex]]] = {
    # See https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits.
    (1, 0, "ECX"): {
        "SSE3": 0,
        "PCLMULQDQ": 1,
        "SSSE3": 9,
        "FMA": 12,
        "SSE4_1": 19,
        "SSE4_2": 20,
        "POPCNT": 23,
        "XSAVE": 26,
        "OSXSAVE": 27,
        "AVX": 28,
    },
    (1, 0, "EDX"): {
        "CMOV": 15,
        "SSE": 25,
        "SSE2": 26,
    },
    # See https://en.wikipedia.org/wiki/CPUID#EAX=7,_ECX=0:_Extended_Features.
    (7, 0, "EBX"): {
        "AVX2": 5,
        "AVX512_F": 16,
        "AVX512_DQ": 17,
        "AVX512_IFMA": 21,
        "AVX512_PF": 26,
        "AVX512_ER": 27,
        "AVX512_CD": 28,
        "AVX512_BW": 30,
        "AVX512_VL": 31,
    },
    (7, 0, "ECX"): {
        "AVX512_VBMI": 1,
        "AVX512_VBMI2": 6,
        "AVX512_VNNI": 11,
        "AVX512_BITALG": 12,
        "AVX512_VPOPCNTDQ": 14,
    },
    (7, 0, "EDX"): {
        "AVX512_4VNNIW": 2,
        "AVX512_4FMAPS": 3,
        "AVX512_VP2INTERSECT": 8,
    },
    # See https://en.wikipedia.org/wiki/CPUID#EAX=7,_ECX=1:_Extended_Features.
    (7, 1, "EAX"): {
        "AVX_VNNI": 4,
        "AVX_IFMA": 23,
    },
    (7, 1, "EDX"): {
        "AVX_VNNI_INT8": 4,
        "AVX_NE_CONVERT": 5,
        "AVX_VNNI_INT16": 10,
    },
}


def get_constant_name(
    leaf: Leaf, subleaf: SubLeaf, registry: Registry, name: Feature
) -> str:
    node = f"L{leaf}S{subleaf}" if subleaf else f"L{leaf}"
    return f"_Py_CPUID_MASK_{registry}_{node}_{name}"


_NAME_MAXSIZE: Final[int] = util.next_block(
    max(
        len(get_constant_name(*family, name))
        for family, values in CPUID_FEATURES.items()
        for name in values
    )
)


def make_cpuid_features_constants() -> str:
    """Used by :file:`Include/internal/pycore_cpuinfo_cpuid_features.h`."""
    writer = util.CWriter()
    writer.comment("Constants for CPUID features", style=DOXYGEN_STYLE)
    for family, values in CPUID_FEATURES.items():
        leaf, subleaf, registry = family
        writer.comment(f"CPUID (LEAF={leaf}, SUBLEAF={subleaf}) [{registry}]")
        for feature_name, bit in values.items():
            if not feature_name:
                raise ValueError(f"invalid entry for {family}")
            if not 0 <= bit < 32:
                raise ValueError(f"invalid bit value for {feature_name!r}")
            key = get_constant_name(leaf, subleaf, registry, feature_name)
            writer.write(util.make_constant(key, bit, _NAME_MAXSIZE))
        writer.write_blankline()
    return writer.build()
