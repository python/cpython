"""Opcode utilities for bytecode-level profiler visualization.

This module provides utilities to get opcode names and detect specialization
status using the opcode module's metadata. Used by heatmap and flamegraph
collectors to display which bytecode instructions are executing at each
source line, including Python's adaptive specialization optimizations.
"""

import opcode

# Build opcode name mapping: opcode number -> opcode name
# This includes both standard opcodes and specialized variants (Python 3.11+)
_OPCODE_NAMES = dict(enumerate(opcode.opname))
if hasattr(opcode, "_specialized_opmap"):
    for name, op in opcode._specialized_opmap.items():
        _OPCODE_NAMES[op] = name

# Build deopt mapping: specialized opcode number -> base opcode number
# Python 3.11+ uses adaptive specialization where generic opcodes like
# LOAD_ATTR can be replaced at runtime with specialized variants like
# LOAD_ATTR_INSTANCE_VALUE. This mapping lets us show both forms.
_DEOPT_MAP = {}
if hasattr(opcode, "_specializations") and hasattr(
    opcode, "_specialized_opmap"
):
    for base_name, variant_names in opcode._specializations.items():
        base_opcode = opcode.opmap.get(base_name)
        if base_opcode is not None:
            for variant_name in variant_names:
                variant_opcode = opcode._specialized_opmap.get(variant_name)
                if variant_opcode is not None:
                    _DEOPT_MAP[variant_opcode] = base_opcode


def get_opcode_info(opcode_num):
    """Get opcode name and specialization info from an opcode number.

    Args:
        opcode_num: The opcode number (0-255 or higher for specialized)

    Returns:
        A dict with keys:
        - 'opname': The opcode name (e.g., 'LOAD_ATTR_INSTANCE_VALUE')
        - 'base_opname': The base opcode name (e.g., 'LOAD_ATTR')
        - 'is_specialized': True if this is a specialized instruction
    """
    opname = _OPCODE_NAMES.get(opcode_num)
    if opname is None:
        return {
            "opname": f"<{opcode_num}>",
            "base_opname": f"<{opcode_num}>",
            "is_specialized": False,
        }

    base_opcode = _DEOPT_MAP.get(opcode_num)
    if base_opcode is not None:
        base_opname = _OPCODE_NAMES.get(base_opcode, f"<{base_opcode}>")
        return {
            "opname": opname,
            "base_opname": base_opname,
            "is_specialized": True,
        }

    return {
        "opname": opname,
        "base_opname": opname,
        "is_specialized": False,
    }


def format_opcode(opcode_num):
    """Format an opcode for display, showing base opcode for specialized ones.

    Args:
        opcode_num: The opcode number (0-255 or higher for specialized)

    Returns:
        A formatted string like 'LOAD_ATTR' or 'LOAD_ATTR_INSTANCE_VALUE (LOAD_ATTR)'
    """
    info = get_opcode_info(opcode_num)
    if info["is_specialized"]:
        return f"{info['opname']} ({info['base_opname']})"
    return info["opname"]


def get_opcode_mapping():
    """Get opcode name and deopt mappings for JavaScript consumption.

    Returns:
        A dict with keys:
        - 'names': Dict mapping opcode numbers to opcode names
        - 'deopt': Dict mapping specialized opcode numbers to base opcode numbers
    """
    return {"names": _OPCODE_NAMES, "deopt": _DEOPT_MAP}
