
"""
opcode module - potentially shared between dis and other modules which
operate on bytecodes (e.g. peephole optimizers).
"""


# Note that __all__ is further extended below
__all__ = ["cmp_op", "stack_effect", "hascompare"]

import _opcode
from _opcode import stack_effect

import sys
# The build uses older versions of Python which do not have _opcode_metadata
if sys.version_info[:2] >= (3, 13):
    from _opcode_metadata import _specializations, _specialized_opmap
    from _opcode_metadata import opmap, HAVE_ARGUMENT, MIN_INSTRUMENTED_OPCODE
    EXTENDED_ARG = opmap['EXTENDED_ARG']

    opname = ['<%r>' % (op,) for op in range(max(opmap.values()) + 1)]
    for op, i in opmap.items():
        opname[i] = op

    __all__.extend(["opname", "opmap", "HAVE_ARGUMENT", "EXTENDED_ARG"])

cmp_op = ('<', '<=', '==', '!=', '>', '>=')

# The build uses older versions of Python which do not have _opcode.has_* functions
if sys.version_info[:2] >= (3, 13):
    # These lists are documented as part of the dis module's API
    hasarg = [op for op in opmap.values() if _opcode.has_arg(op)]
    hasconst = [op for op in opmap.values() if _opcode.has_const(op)]
    hasname = [op for op in opmap.values() if _opcode.has_name(op)]
    hasjump = [op for op in opmap.values() if _opcode.has_jump(op)]
    hasjrel = hasjump  # for backward compatibility
    hasjabs = []
    hasfree = [op for op in opmap.values() if _opcode.has_free(op)]
    haslocal = [op for op in opmap.values() if _opcode.has_local(op)]
    hasexc = [op for op in opmap.values() if _opcode.has_exc(op)]

    __all__.extend(["hasarg", "hasconst", "hasname", "hasjump", "hasjrel",
                    "hasjabs", "hasfree", "haslocal", "hasexc"])

    _intrinsic_1_descs = _opcode.get_intrinsic1_descs()
    _intrinsic_2_descs = _opcode.get_intrinsic2_descs()

    hascompare = [opmap["COMPARE_OP"]]

_nb_ops = [
    ("NB_ADD", "+"),
    ("NB_AND", "&"),
    ("NB_FLOOR_DIVIDE", "//"),
    ("NB_LSHIFT", "<<"),
    ("NB_MATRIX_MULTIPLY", "@"),
    ("NB_MULTIPLY", "*"),
    ("NB_REMAINDER", "%"),
    ("NB_OR", "|"),
    ("NB_POWER", "**"),
    ("NB_RSHIFT", ">>"),
    ("NB_SUBTRACT", "-"),
    ("NB_TRUE_DIVIDE", "/"),
    ("NB_XOR", "^"),
    ("NB_INPLACE_ADD", "+="),
    ("NB_INPLACE_AND", "&="),
    ("NB_INPLACE_FLOOR_DIVIDE", "//="),
    ("NB_INPLACE_LSHIFT", "<<="),
    ("NB_INPLACE_MATRIX_MULTIPLY", "@="),
    ("NB_INPLACE_MULTIPLY", "*="),
    ("NB_INPLACE_REMAINDER", "%="),
    ("NB_INPLACE_OR", "|="),
    ("NB_INPLACE_POWER", "**="),
    ("NB_INPLACE_RSHIFT", ">>="),
    ("NB_INPLACE_SUBTRACT", "-="),
    ("NB_INPLACE_TRUE_DIVIDE", "/="),
    ("NB_INPLACE_XOR", "^="),
]


_cache_format = {
    "LOAD_GLOBAL": {
        "counter": 1,
        "index": 1,
        "module_keys_version": 1,
        "builtin_keys_version": 1,
    },
    "BINARY_OP": {
        "counter": 1,
    },
    "UNPACK_SEQUENCE": {
        "counter": 1,
    },
    "COMPARE_OP": {
        "counter": 1,
    },
    "BINARY_SUBSCR": {
        "counter": 1,
    },
    "FOR_ITER": {
        "counter": 1,
    },
    "LOAD_SUPER_ATTR": {
        "counter": 1,
    },
    "LOAD_ATTR": {
        "counter": 1,
        "version": 2,
        "keys_version": 2,
        "descr": 4,
    },
    "STORE_ATTR": {
        "counter": 1,
        "version": 2,
        "index": 1,
    },
    "CALL": {
        "counter": 1,
        "func_version": 2,
    },
    "STORE_SUBSCR": {
        "counter": 1,
    },
    "SEND": {
        "counter": 1,
    },
    "JUMP_BACKWARD": {
        "counter": 1,
    },
    "TO_BOOL": {
        "counter": 1,
        "version": 2,
    },
}

_inline_cache_entries = {
    name : sum(value.values()) for (name, value) in _cache_format.items()
}
