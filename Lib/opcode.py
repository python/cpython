
"""
opcode module - potentially shared between dis and other modules which
operate on bytecodes (e.g. peephole optimizers).
"""


# Note that __all__ is further extended below
__all__ = ["cmp_op", "opname", "opmap", "stack_effect", "hascompare",
           "HAVE_ARGUMENT", "EXTENDED_ARG"]

import _opcode
from _opcode import stack_effect

import sys
# The build uses older versions of Python which do not have _opcode_metadata
if sys.version_info[:2] >= (3, 13):
    from _opcode_metadata import _specializations, _specialized_instructions

cmp_op = ('<', '<=', '==', '!=', '>', '>=')


ENABLE_SPECIALIZATION = True

def is_pseudo(op):
    return op >= MIN_PSEUDO_OPCODE and op <= MAX_PSEUDO_OPCODE

opmap = {}

# pseudo opcodes (used in the compiler) mapped to the values
# they can become in the actual code.
_pseudo_ops = {}

def def_op(name, op):
    opmap[name] = op

def pseudo_op(name, op, real_ops):
    def_op(name, op)
    _pseudo_ops[name] = real_ops


# Instruction opcodes for compiled code
# Blank lines correspond to available opcodes

def_op('CACHE', 0)
def_op('POP_TOP', 1)
def_op('PUSH_NULL', 2)
def_op('INTERPRETER_EXIT', 3)
def_op('END_FOR', 4)
def_op('END_SEND', 5)
def_op('TO_BOOL', 6)

def_op('NOP', 9)

def_op('UNARY_NEGATIVE', 11)
def_op('UNARY_NOT', 12)

def_op('UNARY_INVERT', 15)
def_op('EXIT_INIT_CHECK', 16)

# We reserve 17 as it is the initial value for the specializing counter
# This helps us catch cases where we attempt to execute a cache.
def_op('RESERVED', 17)

def_op('MAKE_FUNCTION', 24)
def_op('BINARY_SUBSCR', 25)
def_op('BINARY_SLICE', 26)
def_op('STORE_SLICE', 27)

def_op('GET_LEN', 30)
def_op('MATCH_MAPPING', 31)
def_op('MATCH_SEQUENCE', 32)
def_op('MATCH_KEYS', 33)

def_op('PUSH_EXC_INFO', 35)
def_op('CHECK_EXC_MATCH', 36)
def_op('CHECK_EG_MATCH', 37)

def_op('FORMAT_SIMPLE', 40)
def_op('FORMAT_WITH_SPEC', 41)

def_op('WITH_EXCEPT_START', 49)
def_op('GET_AITER', 50)
def_op('GET_ANEXT', 51)
def_op('BEFORE_ASYNC_WITH', 52)
def_op('BEFORE_WITH', 53)
def_op('END_ASYNC_FOR', 54)
def_op('CLEANUP_THROW', 55)

def_op('STORE_SUBSCR', 60)
def_op('DELETE_SUBSCR', 61)

def_op('GET_ITER', 68)
def_op('GET_YIELD_FROM_ITER', 69)

def_op('LOAD_BUILD_CLASS', 71)

def_op('LOAD_ASSERTION_ERROR', 74)
def_op('RETURN_GENERATOR', 75)

def_op('RETURN_VALUE', 83)

def_op('SETUP_ANNOTATIONS', 85)

def_op('LOAD_LOCALS', 87)

def_op('POP_EXCEPT', 89)

HAVE_ARGUMENT = 90             # real opcodes from here have an argument:

def_op('STORE_NAME', 90)       # Index in name list
def_op('DELETE_NAME', 91)      # ""
def_op('UNPACK_SEQUENCE', 92)   # Number of tuple items
def_op('FOR_ITER', 93)
def_op('UNPACK_EX', 94)
def_op('STORE_ATTR', 95)       # Index in name list
def_op('DELETE_ATTR', 96)      # ""
def_op('STORE_GLOBAL', 97)     # ""
def_op('DELETE_GLOBAL', 98)    # ""
def_op('SWAP', 99)
def_op('LOAD_CONST', 100)       # Index in const list
def_op('LOAD_NAME', 101)       # Index in name list
def_op('BUILD_TUPLE', 102)      # Number of tuple items
def_op('BUILD_LIST', 103)       # Number of list items
def_op('BUILD_SET', 104)        # Number of set items
def_op('BUILD_MAP', 105)        # Number of dict entries
def_op('LOAD_ATTR', 106)       # Index in name list
def_op('COMPARE_OP', 107)       # Comparison operator
def_op('IMPORT_NAME', 108)     # Index in name list
def_op('IMPORT_FROM', 109)     # Index in name list
def_op('JUMP_FORWARD', 110)    # Number of words to skip

def_op('POP_JUMP_IF_FALSE', 114)
def_op('POP_JUMP_IF_TRUE', 115)
def_op('LOAD_GLOBAL', 116)     # Index in name list
def_op('IS_OP', 117)
def_op('CONTAINS_OP', 118)
def_op('RERAISE', 119)
def_op('COPY', 120)
def_op('RETURN_CONST', 121)
def_op('BINARY_OP', 122)
def_op('SEND', 123)            # Number of words to skip
def_op('LOAD_FAST', 124)        # Local variable number, no null check
def_op('STORE_FAST', 125)       # Local variable number
def_op('DELETE_FAST', 126)      # Local variable number
def_op('LOAD_FAST_CHECK', 127)  # Local variable number
def_op('POP_JUMP_IF_NOT_NONE', 128)
def_op('POP_JUMP_IF_NONE', 129)
def_op('RAISE_VARARGS', 130)    # Number of raise arguments (1, 2, or 3)
def_op('GET_AWAITABLE', 131)
def_op('BUILD_SLICE', 133)      # Number of items
def_op('JUMP_BACKWARD_NO_INTERRUPT', 134) # Number of words to skip (backwards)
def_op('MAKE_CELL', 135)
def_op('LOAD_DEREF', 137)
def_op('STORE_DEREF', 138)
def_op('DELETE_DEREF', 139)
def_op('JUMP_BACKWARD', 140)    # Number of words to skip (backwards)
def_op('LOAD_SUPER_ATTR', 141)
def_op('CALL_FUNCTION_EX', 142)  # Flags
def_op('LOAD_FAST_AND_CLEAR', 143)  # Local variable number
def_op('EXTENDED_ARG', 144)
EXTENDED_ARG = opmap['EXTENDED_ARG']
def_op('LIST_APPEND', 145)
def_op('SET_ADD', 146)
def_op('MAP_ADD', 147)
def_op('COPY_FREE_VARS', 149)
def_op('YIELD_VALUE', 150)
def_op('RESUME', 151)   # This must be kept in sync with deepfreeze.py
def_op('MATCH_CLASS', 152)

def_op('BUILD_CONST_KEY_MAP', 156)
def_op('BUILD_STRING', 157)
def_op('CONVERT_VALUE', 158)

def_op('LIST_EXTEND', 162)
def_op('SET_UPDATE', 163)
def_op('DICT_MERGE', 164)
def_op('DICT_UPDATE', 165)

def_op('LOAD_FAST_LOAD_FAST', 168)
def_op('STORE_FAST_LOAD_FAST', 169)
def_op('STORE_FAST_STORE_FAST', 170)
def_op('CALL', 171)
def_op('KW_NAMES', 172)
def_op('CALL_INTRINSIC_1', 173)
def_op('CALL_INTRINSIC_2', 174)
def_op('LOAD_FROM_DICT_OR_GLOBALS', 175)
def_op('LOAD_FROM_DICT_OR_DEREF', 176)
def_op('SET_FUNCTION_ATTRIBUTE', 177)    # Attribute

# Optimizer hook
def_op('ENTER_EXECUTOR', 230)

# Instrumented instructions
MIN_INSTRUMENTED_OPCODE = 237

def_op('INSTRUMENTED_LOAD_SUPER_ATTR', 237)
def_op('INSTRUMENTED_POP_JUMP_IF_NONE', 238)
def_op('INSTRUMENTED_POP_JUMP_IF_NOT_NONE', 239)
def_op('INSTRUMENTED_RESUME', 240)
def_op('INSTRUMENTED_CALL', 241)
def_op('INSTRUMENTED_RETURN_VALUE', 242)
def_op('INSTRUMENTED_YIELD_VALUE', 243)
def_op('INSTRUMENTED_CALL_FUNCTION_EX', 244)
def_op('INSTRUMENTED_JUMP_FORWARD', 245)
def_op('INSTRUMENTED_JUMP_BACKWARD', 246)
def_op('INSTRUMENTED_RETURN_CONST', 247)
def_op('INSTRUMENTED_FOR_ITER', 248)
def_op('INSTRUMENTED_POP_JUMP_IF_FALSE', 249)
def_op('INSTRUMENTED_POP_JUMP_IF_TRUE', 250)
def_op('INSTRUMENTED_END_FOR', 251)
def_op('INSTRUMENTED_END_SEND', 252)
def_op('INSTRUMENTED_INSTRUCTION', 253)
def_op('INSTRUMENTED_LINE', 254)
# 255 is reserved


MIN_PSEUDO_OPCODE = 256

pseudo_op('SETUP_FINALLY', 256, ['NOP'])
pseudo_op('SETUP_CLEANUP', 257, ['NOP'])
pseudo_op('SETUP_WITH', 258, ['NOP'])
pseudo_op('POP_BLOCK', 259, ['NOP'])

pseudo_op('JUMP', 260, ['JUMP_FORWARD', 'JUMP_BACKWARD'])
pseudo_op('JUMP_NO_INTERRUPT', 261, ['JUMP_FORWARD', 'JUMP_BACKWARD_NO_INTERRUPT'])

pseudo_op('LOAD_METHOD', 262, ['LOAD_ATTR'])
pseudo_op('LOAD_SUPER_METHOD', 263, ['LOAD_SUPER_ATTR'])
pseudo_op('LOAD_ZERO_SUPER_METHOD', 264, ['LOAD_SUPER_ATTR'])
pseudo_op('LOAD_ZERO_SUPER_ATTR', 265, ['LOAD_SUPER_ATTR'])

pseudo_op('STORE_FAST_MAYBE_NULL', 266, ['STORE_FAST'])
pseudo_op('LOAD_CLOSURE', 267, ['LOAD_FAST'])

MAX_PSEUDO_OPCODE = MIN_PSEUDO_OPCODE + len(_pseudo_ops) - 1

del def_op, pseudo_op

opname = ['<%r>' % (op,) for op in range(MAX_PSEUDO_OPCODE + 1)]
for op, i in opmap.items():
    opname[i] = op

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

_inline_cache_entries = [
    sum(_cache_format.get(opname[opcode], {}).values()) for opcode in range(256)
]
