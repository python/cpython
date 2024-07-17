
import argparse
import sys

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    analyze_files,
    StackItem,
    analysis_error,
)
from typing import Iterator
from lexer import Token, LPAREN, IDENTIFIER
from generators_common import DEFAULT_INPUT

NON_ESCAPING_FUNCTIONS = (
    "Py_INCREF",
    "_PyManagedDictPointer_IsValues",
    "_PyObject_GetManagedDict",
    "_PyObject_ManagedDictPointer",
    "_PyObject_InlineValues",
    "_PyDictValues_AddToInsertionOrder",
    "Py_DECREF",
    "Py_XDECREF",
    "_Py_DECREF_SPECIALIZED",
    "DECREF_INPUTS_AND_REUSE_FLOAT",
    "PyUnicode_Append",
    "_PyLong_IsZero",
    "Py_ARRAY_LENGTH",
    "Py_Unicode_GET_LENGTH",
    "PyUnicode_READ_CHAR",
    "_Py_SINGLETON",
    "PyUnicode_GET_LENGTH",
    "_PyLong_IsCompact",
    "_PyLong_IsNonNegativeCompact",
    "_PyLong_CompactValue",
    "_PyLong_DigitCount",
    "_Py_NewRef",
    "_Py_IsImmortal",
    "PyLong_FromLong",
    "_Py_STR",
    "_PyLong_Add",
    "_PyLong_Multiply",
    "_PyLong_Subtract",
    "Py_NewRef",
    "_PyList_ITEMS",
    "_PyTuple_ITEMS",
    "_Py_atomic_load_uintptr_relaxed",
    "_PyFrame_GetCode",
    "_PyThreadState_HasStackSpace",
    "_PyUnicode_Equal",
    "_PyFrame_SetStackPointer",
    "_PyType_HasFeature",
    "PyUnicode_Concat",
    "PySlice_New",
    "_Py_LeaveRecursiveCallPy",
    "maybe_lltrace_resume_frame",
    "_PyUnicode_JoinArray",
    "_PyEval_FrameClearAndPop",
    "_PyFrame_StackPush",
    "PyCell_New",
    "PyFloat_AS_DOUBLE",
    "_PyFrame_PushUnchecked",
    "Py_FatalError",
    "assert",
    "Py_Is",
    "Py_IsTrue",
    "Py_IsNone",
    "Py_IsFalse",
    "_PyFrame_GetStackPointer"
    "_PyCode_CODE",
    "PyCFunction_GET_FLAGS",
    "_PyErr_Occurred",
    "_Py_LeaveRecursiveCallTstate",
    "_Py_EnterRecursiveCallTstateUnchecked",
    "PyStackRef_FromPyObjectSteal",
    "PyStackRef_AsPyObjectBorrow",
    "PyStackRef_AsPyObjectSteal",
    "PyStackRef_CLOSE",
    "PyStackRef_DUP",
    "PyStackRef_CLEAR",
    "PyStackRef_IsNull",
    "PyStackRef_TYPE",
    "PyStackRef_False",
    "PyStackRef_True",
    "PyStackRef_None",
    "PyStackRef_Is",
    "PyStackRef_FromPyObjectNew",
    "PyStackRef_AsPyObjectNew",
    "PyStackRef_FromPyObjectImmortal",
    "STACKREFS_TO_PYOBJECTS",
    "STACKREFS_TO_PYOBJECTS_CLEANUP",
    "CONVERSION_FAILED",
)

FLOW_CONTROL = {
    "ESCAPING_CALL",
    "ERROR_IF",
    "DEOPT_IF",
    "ERROR_NO_POP",
}

DECREFS = {
    "Py_DECREF",
    "Py_XDECREF",
    "Py_CLEAR",
    "DECREF_INPUTS",
}

def check_escaping_call(tkn_iter: Iterator[Token]) -> bool:
    res = 0
    assert(next(tkn_iter).kind == "LPAREN")
    parens = 1
    for tkn in tkn_iter:
        if tkn.kind == "LPAREN":
            parens += 1
        elif tkn.kind == "RPAREN":
            parens -= 1
            if parens == 0:
                return
        elif tkn.kind == "GOTO":
            print(f"`goto` in 'ESCAPING_CALL' on line {tkn.line}")
            res = 1
        elif tkn.kind == IDENTIFIER:
            if tkn.text in FLOW_CONTROL:
                print(f"Exiting flow control in 'ESCAPING_CALL' on line {tkn.line}")
                res = 1
            if tkn.text in DECREFS:
                print(f"DECREF in 'ESCAPING_CALL' on line {tkn.line}")
                res = 1
    return res

def is_macro_name(name: str) -> bool:
    if name[0] == "_":
        name = name[1:]
    if name.startswith("Py"):
        name = name[2:]
    return name == name.upper()

def is_getter(name: str) -> bool:
    return "GET" in name

def check_for_unmarked_escapes(uop: Uop) -> None:
    res = 0
    tkns = iter(uop.body)
    for tkn in tkns:
        if tkn.kind != IDENTIFIER:
            continue
        try:
            next_tkn = next(tkns)
        except StopIteration:
            return False
        if next_tkn.kind != LPAREN:
            continue
        if tkn.text == "ESCAPING_CALL":
            if check_escaping_call(tkns):
                res = 1
        if is_macro_name(tkn.text):
            continue
        if is_getter(tkn.text):
            continue
        if tkn.text.endswith("Check") or tkn.text.endswith("CheckExact"):
            continue
        if "backoff_counter" in tkn.text:
            continue
        if tkn.text not in NON_ESCAPING_FUNCTIONS:
            print(f"Unmarked escaping function '{tkn.text}' at {tkn.filename}:{tkn.line}")
            res = 1
    return res

def verify_uop(uop: Uop) -> None:
    return check_for_unmarked_escapes(uop)

def verify(analysis: Analysis) -> None:
    res = 0
    for uop in analysis.uops.values():
        res |= verify_uop(uop)
    return res


arg_parser = argparse.ArgumentParser(
    description="Verify the bytecode description file.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    sys.exit(verify(analyze_files(args.input)))
