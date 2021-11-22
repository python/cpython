import argparse
import ast
import builtins
import collections
import contextlib
import os
import re
import time
import types
import typing

import umarshal

verbose = False


def make_string_literal(b: bytes) -> str:
    res = ['"']
    if b.isascii() and b.decode("ascii").isprintable():
        res.append(b.decode("ascii").replace("\\", "\\\\").replace("\"", "\\\""))
    else:
        for i in b:
            res.append(f"\\x{i:02x}")
    res.append('"')
    return "".join(res)


CO_FAST_LOCAL = 0x20
CO_FAST_CELL = 0x40
CO_FAST_FREE = 0x80


def get_localsplus(code: types.CodeType):
    a = collections.defaultdict(int)
    for name in code.co_varnames:
        a[name] |= CO_FAST_LOCAL
    for name in code.co_cellvars:
        a[name] |= CO_FAST_CELL
    for name in code.co_freevars:
        a[name] |= CO_FAST_FREE
    return tuple(a.keys()), bytes(a.values())


def get_localsplus_counts(code: types.CodeType,
                          names: tuple[str, ...],
                          kinds: bytes) -> tuple[int, int, int, int]:
    nlocals = 0
    nplaincellvars = 0
    ncellvars = 0
    nfreevars = 0
    for name, kind in zip(names, kinds, strict=True):
        if kind & CO_FAST_LOCAL:
            nlocals += 1
            if kind & CO_FAST_CELL:
                ncellvars += 1
        elif kind & CO_FAST_CELL:
            ncellvars += 1
            nplaincellvars += 1
        elif kind & CO_FAST_FREE:
            nfreevars += 1
    assert nlocals == len(code.co_varnames) == code.co_nlocals, \
        (nlocals, len(code.co_varnames), code.co_nlocals)
    assert ncellvars == len(code.co_cellvars)
    assert nfreevars == len(code.co_freevars)
    assert len(names) == nlocals + nplaincellvars + nfreevars
    return nlocals, nplaincellvars, ncellvars, nfreevars


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


class Printer:

    def __init__(self, file: typing.TextIO):
        self.level = 0
        self.file = file
        self.cache: dict[tuple[type, object], str] = {}
        self.hits, self.misses = 0, 0
        self.patchups: list[str] = []
        self.write('#include "Python.h"')
        self.write('#include "internal/pycore_gc.h"')
        self.write('#include "internal/pycore_code.h"')
        self.write("")

    @contextlib.contextmanager
    def indent(self) -> None:
        save_level = self.level
        try:
            self.level += 1
            yield
        finally:
            self.level = save_level

    def write(self, arg: str) -> None:
        self.file.writelines(("    "*self.level, arg, "\n"))

    @contextlib.contextmanager
    def block(self, prefix: str, suffix: str = "") -> None:
        self.write(prefix + " {")
        with self.indent():
            yield
        self.write("}" + suffix)

    def object_head(self, typename: str) -> None:
        with self.block(".ob_base =", ","):
            self.write(f".ob_refcnt = 999999999,")
            self.write(f".ob_type = &{typename},")

    def object_var_head(self, typename: str, size: int) -> None:
        with self.block(".ob_base =", ","):
            self.object_head(typename)
            self.write(f".ob_size = {size},")

    def field(self, obj: object, name: str) -> None:
        self.write(f".{name} = {getattr(obj, name)},")

    def generate_bytes(self, name: str, b: bytes) -> str:
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
                        self.write(".ready = 1,")
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
                            self.write(".ready = 1,")
                with self.block(f"._data =", ","):
                    for i in range(0, len(s), 16):
                        data = s[i:i+16]
                        self.write(", ".join(map(str, map(ord, data))) + ",")
                if kind == PyUnicode_2BYTE_KIND:
                    self.patchups.append("if (sizeof(wchar_t) == 2) {")
                    self.patchups.append(f"    {name}._compact._base.wstr = (wchar_t *) {name}._data;")
                    self.patchups.append(f"    {name}._compact.wstr_length = {len(s)};")
                    self.patchups.append("}")
                if kind == PyUnicode_4BYTE_KIND:
                    self.patchups.append("if (sizeof(wchar_t) == 4) {")
                    self.patchups.append(f"    {name}._compact._base.wstr = (wchar_t *) {name}._data;")
                    self.patchups.append(f"    {name}._compact.wstr_length = {len(s)};")
                    self.patchups.append("}")
                return f"& {name}._compact._base.ob_base"


    def generate_code(self, name: str, code: types.CodeType) -> str:
        # The ordering here matches PyCode_NewWithPosOnlyArgs()
        # (but see below).
        co_code = self.generate(name + "_code", code.co_code)
        co_consts = self.generate(name + "_consts", code.co_consts)
        co_names = self.generate(name + "_names", code.co_names)
        co_varnames = self.generate(name + "_varnames", code.co_varnames)
        co_freevars = self.generate(name + "_freevars", code.co_freevars)
        co_cellvars = self.generate(name + "_cellvars", code.co_cellvars)
        co_filename = self.generate(name + "_filename", code.co_filename)
        co_name = self.generate(name + "_name", code.co_name)
        co_qualname = self.generate(name + "_qualname", code.co_qualname)
        co_linetable = self.generate(name + "_linetable", code.co_linetable)
        co_endlinetable = self.generate(name + "_endlinetable", code.co_endlinetable)
        co_columntable = self.generate(name + "_columntable", code.co_columntable)
        co_exceptiontable = self.generate(name + "_exceptiontable", code.co_exceptiontable)
        # These fields are not directly accessible
        localsplusnames, localspluskinds = get_localsplus(code)
        co_localsplusnames = self.generate(name + "_localsplusnames", localsplusnames)
        co_localspluskinds = self.generate(name + "_localspluskinds", localspluskinds)
        # Derived values
        nlocals, nplaincellvars, ncellvars, nfreevars = \
            get_localsplus_counts(code, localsplusnames, localspluskinds)
        with self.block(f"static struct PyCodeObject {name} =", ";"):
            self.object_head("PyCode_Type")
            # But the ordering here must match that in cpython/code.h
            # (which is a pain because we tend to reorder those for perf)
            # otherwise MSVC doesn't like it.
            self.write(f".co_consts = {co_consts},")
            self.write(f".co_names = {co_names},")
            self.write(f".co_firstinstr = (_Py_CODEUNIT *) {co_code.removesuffix('.ob_base.ob_base')}.ob_sval,")
            self.write(f".co_exceptiontable = {co_exceptiontable},")
            self.field(code, "co_flags")
            self.write(".co_warmup = QUICKENING_INITIAL_WARMUP_VALUE,")
            self.field(code, "co_argcount")
            self.field(code, "co_posonlyargcount")
            self.field(code, "co_kwonlyargcount")
            self.field(code, "co_stacksize")
            self.field(code, "co_firstlineno")
            self.write(f".co_code = {co_code},")
            self.write(f".co_localsplusnames = {co_localsplusnames},")
            self.write(f".co_localspluskinds = {co_localspluskinds},")
            self.write(f".co_filename = {co_filename},")
            self.write(f".co_name = {co_name},")
            self.write(f".co_qualname = {co_qualname},")
            self.write(f".co_linetable = {co_linetable},")
            self.write(f".co_endlinetable = {co_endlinetable},")
            self.write(f".co_columntable = {co_columntable},")
            self.write(f".co_nlocalsplus = {len(localsplusnames)},")
            self.field(code, "co_nlocals")
            self.write(f".co_nplaincellvars = {nplaincellvars},")
            self.write(f".co_ncellvars = {ncellvars},")
            self.write(f".co_nfreevars = {nfreevars},")
            self.write(f".co_varnames = {co_varnames},")
            self.write(f".co_cellvars = {co_cellvars},")
            self.write(f".co_freevars = {co_freevars},")
        return f"& {name}.ob_base"

    def generate_tuple(self, name: str, t: tuple[object, ...]) -> str:
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
        sign = -1 if i < 0 else 0 if i == 0 else +1
        i = abs(i)
        digits: list[int] = []
        while i:
            i, rem = divmod(i, digit)
            digits.append(rem)
        self.write("static")
        with self.indent():
            with self.block("struct"):
                self.write("PyObject_VAR_HEAD")
                self.write(f"digit ob_digit[{max(1, len(digits))}];")
        with self.block(f"{name} =", ";"):
            self.object_var_head("PyLong_Type", sign*len(digits))
            if digits:
                ds = ", ".join(map(str, digits))
                self.write(f".ob_digit = {{ {ds} }},")

    def generate_int(self, name: str, i: int) -> str:
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
        return f"& {name}.ob_base.ob_base"

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

    def generate_frozenset(self, name: str, fs: frozenset[object]) -> str:
        ret = self.generate_tuple(name, tuple(sorted(fs)))
        self.write("// TODO: The above tuple should be a frozenset")
        return ret

    def generate(self, name: str, obj: object) -> str:
        # Use repr() in the key to distinguish -0.0 from +0.0
        key = (type(obj), obj, repr(obj))
        if key in self.cache:
            self.hits += 1
            # print(f"Cache hit {key!r:.40}: {self.cache[key]!r:.40}")
            return self.cache[key]
        self.misses += 1
        match obj:
            case types.CodeType() | umarshal.Code() as code:
                val = self.generate_code(name, code)
            case tuple(t):
                val = self.generate_tuple(name, t)
            case str(s):
                val = self.generate_unicode(name, s)
            case bytes(b):
                val = self.generate_bytes(name, b)
            case True:
                return "Py_True"
            case False:
                return "Py_False"
            case int(i):
                val = self.generate_int(name, i)
            case float(x):
                val = self.generate_float(name, x)
            case complex() as z:
                val = self.generate_complex(name, z)
            case frozenset(fs):
                val = self.generate_frozenset(name, fs)
            case builtins.Ellipsis:
                return "Py_Ellipsis"
            case None:
                return "Py_None"
            case _:
                raise TypeError(
                    f"Cannot generate code for {type(obj).__name__} object")
        # print(f"Cache store {key!r:.40}: {val!r:.40}")
        self.cache[key] = val
        return val


EPILOGUE = """
PyObject *
_Py_get_%%NAME%%_toplevel(void)
{
    do_patchups();
    return (PyObject *) &toplevel;
}
"""

FROZEN_COMMENT = "/* Auto-generated by Programs/_freeze_module.c */"

FROZEN_DATA_LINE = r"\s*(\d+,\s*)+\s*"


def is_frozen_header(source: str) -> bool:
    return source.startswith(FROZEN_COMMENT)


def decode_frozen_data(source: str) -> types.CodeType:
    lines = source.splitlines()
    while lines and re.match(FROZEN_DATA_LINE, lines[0]) is None:
        del lines[0]
    while lines and re.match(FROZEN_DATA_LINE, lines[-1]) is None:
        del lines[-1]
    values: tuple[int, ...] = ast.literal_eval("".join(lines))
    data = bytes(values)
    return umarshal.loads(data)


def generate(source: str, filename: str, modname: str, file: typing.TextIO) -> None:
    if is_frozen_header(source):
        code = decode_frozen_data(source)
    else:
        code = compile(source, filename, "exec")
    printer = Printer(file)
    printer.generate("toplevel", code)
    printer.write("")
    with printer.block("static void do_patchups()"):
        for p in printer.patchups:
            printer.write(p)
    here = os.path.dirname(__file__)
    printer.write(EPILOGUE.replace("%%NAME%%", modname.replace(".", "_")))
    if verbose:
        print(f"Cache hits: {printer.hits}, misses: {printer.misses}")


parser = argparse.ArgumentParser()
parser.add_argument("-m", "--module", help="Defaults to basename(file)")
parser.add_argument("-o", "--output", help="Defaults to MODULE.c")
parser.add_argument("-v", "--verbose", action="store_true", help="Print diagnostics")
parser.add_argument("file", help="Input file (required)")


@contextlib.contextmanager
def report_time(label: str):
    t0 = time.time()
    try:
        yield
    finally:
        t1 = time.time()
    if verbose:
        print(f"{label}: {t1-t0:.3f} sec")


def main() -> None:
    global verbose
    args = parser.parse_args()
    verbose = args.verbose
    with open(args.file, encoding="utf-8") as f:
        source = f.read()
    modname = args.module or os.path.basename(args.file).removesuffix(".py")
    output = args.output or modname + ".c"
    with open(output, "w", encoding="utf-8") as file:
        with report_time("generate"):
            generate(source, f"<frozen {modname}>", modname, file)
    if verbose:
        print(f"Wrote {os.path.getsize(output)} bytes to {output}")


if __name__ == "__main__":
    main()
