"""Deep freeze

The script may be executed by _bootstrap_python interpreter.
Shared library extension modules are not available in that case.
Requires 3.11+ to be executed,
because relies on `code.co_qualname` and `code.co_exceptiontable`.
"""

from __future__ import annotations

import argparse
import builtins
import collections
import contextlib
import os
import re
import time
import types

import umarshal

TYPE_CHECKING = False
if TYPE_CHECKING:
    from collections.abc import Iterator
    from typing import Any, TextIO

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))

verbose = False

# This must be kept in sync with Tools/cases_generator/analyzer.py
RESUME = 128

def isprintable(b: bytes) -> bool:
    return all(0x20 <= c < 0x7f for c in b)


def make_string_literal(b: bytes) -> str:
    res = ['"']
    if isprintable(b):
        res.append(b.decode("ascii").replace("\\", "\\\\").replace("\"", "\\\""))
    else:
        for i in b:
            res.append(f"\\x{i:02x}")
    res.append('"')
    return "".join(res)


CO_FAST_LOCAL = 0x20
CO_FAST_CELL = 0x40
CO_FAST_FREE = 0x80

next_code_version = 1

def get_localsplus(code: types.CodeType) -> tuple[tuple[str, ...], bytes]:
    a: collections.defaultdict[str, int] = collections.defaultdict(int)
    for name in code.co_varnames:
        a[name] |= CO_FAST_LOCAL
    for name in code.co_cellvars:
        a[name] |= CO_FAST_CELL
    for name in code.co_freevars:
        a[name] |= CO_FAST_FREE
    return tuple(a.keys()), bytes(a.values())


def get_localsplus_counts(code: types.CodeType,
                          names: tuple[str, ...],
                          kinds: bytes) -> tuple[int, int, int]:
    nlocals = 0
    ncellvars = 0
    nfreevars = 0
    assert len(names) == len(kinds)
    for name, kind in zip(names, kinds):
        if kind & CO_FAST_LOCAL:
            nlocals += 1
            if kind & CO_FAST_CELL:
                ncellvars += 1
        elif kind & CO_FAST_CELL:
            ncellvars += 1
        elif kind & CO_FAST_FREE:
            nfreevars += 1
    assert nlocals == len(code.co_varnames) == code.co_nlocals, \
        (nlocals, len(code.co_varnames), code.co_nlocals)
    assert ncellvars == len(code.co_cellvars)
    assert nfreevars == len(code.co_freevars)
    return nlocals, ncellvars, nfreevars


PyUnicode_1BYTE_KIND = 1
PyUnicode_2BYTE_KIND = 2
PyUnicode_4BYTE_KIND = 4


def analyze_character_width(s: str) -> tuple[int, bool]:
    maxchar = ' '
    for c in s:
        maxchar = max(maxchar, c)
    ascii = False
    if maxchar <= '\xFF':
        kind = PyUnicode_1BYTE_KIND
        ascii = maxchar <= '\x7F'
    elif maxchar <= '\uFFFF':
        kind = PyUnicode_2BYTE_KIND
    else:
        kind = PyUnicode_4BYTE_KIND
    return kind, ascii


def removesuffix(base: str, suffix: str) -> str:
    if base.endswith(suffix):
        return base[:len(base) - len(suffix)]
    return base

class Printer:

    def __init__(self, file: TextIO) -> None:
        self.level = 0
        self.file = file
        self.cache: dict[tuple[type, object, str], str] = {}
        self.hits, self.misses = 0, 0
        self.finis: list[str] = []
        self.inits: list[str] = []
        self.identifiers, self.strings = self.get_identifiers_and_strings()
        self.write('#include "Python.h"')
        self.write('#include "internal/pycore_object.h"')
        self.write('#include "internal/pycore_gc.h"')
        self.write('#include "internal/pycore_code.h"')
        self.write('#include "internal/pycore_frame.h"')
        self.write('#include "internal/pycore_long.h"')
        self.write("")

    def get_identifiers_and_strings(self) -> tuple[set[str], dict[str, str]]:
        filename = os.path.join(ROOT, "Include", "internal", "pycore_global_strings.h")
        with open(filename) as fp:
            lines = fp.readlines()
        identifiers: set[str] = set()
        strings: dict[str, str] = {}
        for line in lines:
            if m := re.search(r"STRUCT_FOR_ID\((\w+)\)", line):
                identifiers.add(m.group(1))
            if m := re.search(r'STRUCT_FOR_STR\((\w+), "(.*?)"\)', line):
                strings[m.group(2)] = m.group(1)
        return identifiers, strings

    @contextlib.contextmanager
    def indent(self) -> Iterator[None]:
        save_level = self.level
        try:
            self.level += 1
            yield
        finally:
            self.level = save_level

    def write(self, arg: str) -> None:
        self.file.writelines(("    "*self.level, arg, "\n"))

    @contextlib.contextmanager
    def block(self, prefix: str, suffix: str = "") -> Iterator[None]:
        self.write(prefix + " {")
        with self.indent():
            yield
        self.write("}" + suffix)

    def object_head(self, typename: str) -> None:
        self.write(f".ob_base = _PyObject_HEAD_INIT(&{typename}),")

    def object_var_head(self, typename: str, size: int) -> None:
        self.write(f".ob_base = _PyVarObject_HEAD_INIT(&{typename}, {size}),")

    def field(self, obj: object, name: str) -> None:
        self.write(f".{name} = {getattr(obj, name)},")

    def generate_bytes(self, name: str, b: bytes) -> str:
        if b == b"":
            return "(PyObject *)&_Py_SINGLETON(bytes_empty)"
        if len(b) == 1:
            return f"(PyObject *)&_Py_SINGLETON(bytes_characters[{b[0]}])"
        self.write("static")
        with self.indent():
            with self.block("struct"):
                self.write("PyObject_VAR_HEAD")
                self.write("Py_hash_t ob_shash;")
                self.write(f"char ob_sval[{len(b) + 1}];")
        with self.block(f"{name} =", ";"):
            self.object_var_head("PyBytes_Type", len(b))
            self.write(".ob_shash = -1,")
            self.write(f".ob_sval = {make_string_literal(b)},")
        return f"& {name}.ob_base.ob_base"

    def generate_unicode(self, name: str, s: str) -> str:
        if s in self.strings:
            return f"&_Py_STR({self.strings[s]})"
        if s in self.identifiers:
            return f"&_Py_ID({s})"
        if len(s) == 1:
            c = ord(s)
            if c < 128:
                return f"(PyObject *)&_Py_SINGLETON(strings).ascii[{c}]"
            elif c < 256:
                return f"(PyObject *)&_Py_SINGLETON(strings).latin1[{c - 128}]"
        if re.match(r'\A[A-Za-z0-9_]+\Z', s):
            name = f"const_str_{s}"
        kind, ascii = analyze_character_width(s)
        if kind == PyUnicode_1BYTE_KIND:
            datatype = "uint8_t"
        elif kind == PyUnicode_2BYTE_KIND:
            datatype = "uint16_t"
        else:
            datatype = "uint32_t"
        self.write("static")
        with self.indent():
            with self.block("struct"):
                if ascii:
                    self.write("PyASCIIObject _ascii;")
                else:
                    self.write("PyCompactUnicodeObject _compact;")
                self.write(f"{datatype} _data[{len(s)+1}];")
        with self.block(f"{name} =", ";"):
            if ascii:
                with self.block("._ascii =", ","):
                    self.object_head("PyUnicode_Type")
                    self.write(f".length = {len(s)},")
                    self.write(".hash = -1,")
                    with self.block(".state =", ","):
                        self.write(".kind = 1,")
                        self.write(".compact = 1,")
                        self.write(".ascii = 1,")
                        self.write(".statically_allocated = 1,")
                self.write(f"._data = {make_string_literal(s.encode('ascii'))},")
                return f"& {name}._ascii.ob_base"
            else:
                with self.block("._compact =", ","):
                    with self.block("._base =", ","):
                        self.object_head("PyUnicode_Type")
                        self.write(f".length = {len(s)},")
                        self.write(".hash = -1,")
                        with self.block(".state =", ","):
                            self.write(f".kind = {kind},")
                            self.write(".compact = 1,")
                            self.write(".ascii = 0,")
                            self.write(".statically_allocated = 1,")
                    utf8 = s.encode('utf-8')
                    self.write(f'.utf8 = {make_string_literal(utf8)},')
                    self.write(f'.utf8_length = {len(utf8)},')
                with self.block(f"._data =", ","):
                    for i in range(0, len(s), 16):
                        data = s[i:i+16]
                        self.write(", ".join(map(str, map(ord, data))) + ",")
                return f"& {name}._compact._base.ob_base"


    def generate_code(self, name: str, code: types.CodeType) -> str:
        global next_code_version
        # The ordering here matches PyCode_NewWithPosOnlyArgs()
        # (but see below).
        co_consts = self.generate(name + "_consts", code.co_consts)
        co_names = self.generate(name + "_names", code.co_names)
        co_filename = self.generate(name + "_filename", code.co_filename)
        co_name = self.generate(name + "_name", code.co_name)
        co_linetable = self.generate(name + "_linetable", code.co_linetable)
        # We use 3.10 for type checking, but this module requires 3.11
        # TODO: bump python version for this script.
        co_qualname = self.generate(
            name + "_qualname",
            code.co_qualname,  # type: ignore[attr-defined]
        )
        co_exceptiontable = self.generate(
            name + "_exceptiontable",
            code.co_exceptiontable,  # type: ignore[attr-defined]
        )
        # These fields are not directly accessible
        localsplusnames, localspluskinds = get_localsplus(code)
        co_localsplusnames = self.generate(name + "_localsplusnames", localsplusnames)
        co_localspluskinds = self.generate(name + "_localspluskinds", localspluskinds)
        # Derived values
        nlocals, ncellvars, nfreevars = \
            get_localsplus_counts(code, localsplusnames, localspluskinds)
        co_code_adaptive = make_string_literal(code.co_code)
        self.write("static")
        with self.indent():
            self.write(f"struct _PyCode_DEF({len(code.co_code)})")
        with self.block(f"{name} =", ";"):
            self.object_var_head("PyCode_Type", len(code.co_code) // 2)
            # But the ordering here must match that in cpython/code.h
            # (which is a pain because we tend to reorder those for perf)
            # otherwise MSVC doesn't like it.
            self.write(f".co_consts = {co_consts},")
            self.write(f".co_names = {co_names},")
            self.write(f".co_exceptiontable = {co_exceptiontable},")
            self.field(code, "co_flags")
            self.field(code, "co_argcount")
            self.field(code, "co_posonlyargcount")
            self.field(code, "co_kwonlyargcount")
            # The following should remain in sync with _PyFrame_NumSlotsForCodeObject
            self.write(f".co_framesize = {code.co_stacksize + len(localsplusnames)} + FRAME_SPECIALS_SIZE,")
            self.field(code, "co_stacksize")
            self.field(code, "co_firstlineno")
            self.write(f".co_nlocalsplus = {len(localsplusnames)},")
            self.field(code, "co_nlocals")
            self.write(f".co_ncellvars = {ncellvars},")
            self.write(f".co_nfreevars = {nfreevars},")
            self.write(f".co_version = {next_code_version},")
            next_code_version += 1
            self.write(f".co_localsplusnames = {co_localsplusnames},")
            self.write(f".co_localspluskinds = {co_localspluskinds},")
            self.write(f".co_filename = {co_filename},")
            self.write(f".co_name = {co_name},")
            self.write(f".co_qualname = {co_qualname},")
            self.write(f".co_linetable = {co_linetable},")
            self.write(f"._co_cached = NULL,")
            self.write(f".co_code_adaptive = {co_code_adaptive},")
            first_traceable = 0
            for op in code.co_code[::2]:
                if op == RESUME:
                    break
                first_traceable += 1
            self.write(f"._co_firsttraceable = {first_traceable},")
        name_as_code = f"(PyCodeObject *)&{name}"
        self.finis.append(f"_PyStaticCode_Fini({name_as_code});")
        self.inits.append(f"_PyStaticCode_Init({name_as_code})")
        return f"& {name}.ob_base.ob_base"

    def generate_tuple(self, name: str, t: tuple[object, ...]) -> str:
        if len(t) == 0:
            return f"(PyObject *)& _Py_SINGLETON(tuple_empty)"
        items = [self.generate(f"{name}_{i}", it) for i, it in enumerate(t)]
        self.write("static")
        with self.indent():
            with self.block("struct"):
                self.write("PyGC_Head _gc_head;")
                with self.block("struct", "_object;"):
                    self.write("PyObject_VAR_HEAD")
                    if t:
                        self.write(f"PyObject *ob_item[{len(t)}];")
        with self.block(f"{name} =", ";"):
            with self.block("._object =", ","):
                self.object_var_head("PyTuple_Type", len(t))
                if items:
                    with self.block(f".ob_item =", ","):
                        for item in items:
                            self.write(item + ",")
        return f"& {name}._object.ob_base.ob_base"

    def _generate_int_for_bits(self, name: str, i: int, digit: int) -> None:
        sign = (i > 0) - (i < 0)
        i = abs(i)
        digits: list[int] = []
        while i:
            i, rem = divmod(i, digit)
            digits.append(rem)
        self.write("static")
        with self.indent():
            with self.block("struct"):
                self.write("PyObject ob_base;")
                self.write("uintptr_t lv_tag;")
                self.write(f"digit ob_digit[{max(1, len(digits))}];")
        with self.block(f"{name} =", ";"):
            self.object_head("PyLong_Type")
            self.write(f".lv_tag = TAG_FROM_SIGN_AND_SIZE({sign}, {len(digits)}),")
            if digits:
                ds = ", ".join(map(str, digits))
                self.write(f".ob_digit = {{ {ds} }},")

    def generate_int(self, name: str, i: int) -> str:
        if -5 <= i <= 256:
            return f"(PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + {i}]"
        if i >= 0:
            name = f"const_int_{i}"
        else:
            name = f"const_int_negative_{abs(i)}"
        if abs(i) < 2**15:
            self._generate_int_for_bits(name, i, 2**15)
        else:
            connective = "if"
            for bits_in_digit in 15, 30:
                self.write(f"#{connective} PYLONG_BITS_IN_DIGIT == {bits_in_digit}")
                self._generate_int_for_bits(name, i, 2**bits_in_digit)
                connective = "elif"
            self.write("#else")
            self.write('#error "PYLONG_BITS_IN_DIGIT should be 15 or 30"')
            self.write("#endif")
            # If neither clause applies, it won't compile
        return f"& {name}.ob_base"

    def generate_float(self, name: str, x: float) -> str:
        with self.block(f"static PyFloatObject {name} =", ";"):
            self.object_head("PyFloat_Type")
            self.write(f".ob_fval = {x},")
        return f"&{name}.ob_base"

    def generate_complex(self, name: str, z: complex) -> str:
        with self.block(f"static PyComplexObject {name} =", ";"):
            self.object_head("PyComplex_Type")
            self.write(f".cval = {{ {z.real}, {z.imag} }},")
        return f"&{name}.ob_base"

    def generate_frozenset(self, name: str, fs: frozenset[Any]) -> str:
        try:
            fs_sorted = sorted(fs)
        except TypeError:
            # frozen set with incompatible types, fallback to repr()
            fs_sorted = sorted(fs, key=repr)
        ret = self.generate_tuple(name, tuple(fs_sorted))
        self.write("// TODO: The above tuple should be a frozenset")
        return ret

    def generate_file(self, module: str, code: object)-> None:
        module = module.replace(".", "_")
        self.generate(f"{module}_toplevel", code)
        self.write(EPILOGUE.format(name=module))

    def generate(self, name: str, obj: object) -> str:
        # Use repr() in the key to distinguish -0.0 from +0.0
        key = (type(obj), obj, repr(obj))
        if key in self.cache:
            self.hits += 1
            # print(f"Cache hit {key!r:.40}: {self.cache[key]!r:.40}")
            return self.cache[key]
        self.misses += 1
        if isinstance(obj, types.CodeType) :
            val = self.generate_code(name, obj)
        elif isinstance(obj, tuple):
            val = self.generate_tuple(name, obj)
        elif isinstance(obj, str):
            val = self.generate_unicode(name, obj)
        elif isinstance(obj, bytes):
            val = self.generate_bytes(name, obj)
        elif obj is True:
            return "Py_True"
        elif obj is False:
            return "Py_False"
        elif isinstance(obj, int):
            val = self.generate_int(name, obj)
        elif isinstance(obj, float):
            val = self.generate_float(name, obj)
        elif isinstance(obj, complex):
            val = self.generate_complex(name, obj)
        elif isinstance(obj, frozenset):
            val = self.generate_frozenset(name, obj)
        elif obj is builtins.Ellipsis:
            return "Py_Ellipsis"
        elif obj is None:
            return "Py_None"
        else:
            raise TypeError(
                f"Cannot generate code for {type(obj).__name__} object")
        # print(f"Cache store {key!r:.40}: {val!r:.40}")
        self.cache[key] = val
        return val


EPILOGUE = """
PyObject *
_Py_get_{name}_toplevel(void)
{{
    return Py_NewRef((PyObject *) &{name}_toplevel);
}}
"""

FROZEN_COMMENT_C = "/* Auto-generated by Programs/_freeze_module.c */"
FROZEN_COMMENT_PY = "/* Auto-generated by Programs/_freeze_module.py */"

FROZEN_DATA_LINE = r"\s*(\d+,\s*)+\s*"


def is_frozen_header(source: str) -> bool:
    return source.startswith((FROZEN_COMMENT_C, FROZEN_COMMENT_PY))


def decode_frozen_data(source: str) -> types.CodeType:
    values: list[int] = []
    for line in source.splitlines():
        if re.match(FROZEN_DATA_LINE, line):
            values.extend([int(x) for x in line.split(",") if x.strip()])
    data = bytes(values)
    return umarshal.loads(data)  # type: ignore[no-any-return]


def generate(args: list[str], output: TextIO) -> None:
    printer = Printer(output)
    for arg in args:
        file, modname = arg.rsplit(':', 1)
        with open(file, encoding="utf8") as fd:
            source = fd.read()
            if is_frozen_header(source):
                code = decode_frozen_data(source)
            else:
                code = compile(fd.read(), f"<frozen {modname}>", "exec")
            printer.generate_file(modname, code)
    with printer.block(f"void\n_Py_Deepfreeze_Fini(void)"):
        for p in printer.finis:
            printer.write(p)
    with printer.block(f"int\n_Py_Deepfreeze_Init(void)"):
        for p in printer.inits:
            with printer.block(f"if ({p} < 0)"):
                printer.write("return -1;")
        printer.write("return 0;")
    printer.write(f"\nuint32_t _Py_next_func_version = {next_code_version};\n")
    if verbose:
        print(f"Cache hits: {printer.hits}, misses: {printer.misses}")


parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output", help="Defaults to deepfreeze.c", default="deepfreeze.c")
parser.add_argument("-v", "--verbose", action="store_true", help="Print diagnostics")
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument("-f", "--file", help="read rule lines from a file")
group.add_argument('args', nargs="*", default=(),
                   help="Input file and module name (required) in file:modname format")

@contextlib.contextmanager
def report_time(label: str) -> Iterator[None]:
    t0 = time.perf_counter()
    try:
        yield
    finally:
        t1 = time.perf_counter()
    if verbose:
        print(f"{label}: {t1-t0:.3f} sec")


def main() -> None:
    global verbose
    args = parser.parse_args()
    verbose = args.verbose
    output = args.output

    if args.file:
        if verbose:
            print(f"Reading targets from {args.file}")
        with open(args.file, encoding="utf-8-sig") as fin:
            rules = [x.strip() for x in fin]
    else:
        rules = args.args

    with open(output, "w", encoding="utf-8") as file:
        with report_time("generate"):
            generate(rules, file)
    if verbose:
        print(f"Wrote {os.path.getsize(output)} bytes to {output}")


if __name__ == "__main__":
    main()
