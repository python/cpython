import contextlib
import io
import os.path
import re

import consts_getter

SCRIPT_NAME = 'Tools/build/generate_global_objects.py'
__file__ = os.path.abspath(__file__)
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
INTERNAL = os.path.join(ROOT, 'Include', 'internal')


IGNORED = {
    'ACTION',  # Python/_warnings.c
    'ATTR',  # Python/_warnings.c and Objects/funcobject.c
    'DUNDER',  # Objects/typeobject.c
    'RDUNDER',  # Objects/typeobject.c
    'SPECIAL',  # Objects/weakrefobject.c
    'NAME',  # Objects/typeobject.c
}
IDENTIFIERS = [
    # from ADD() Python/_warnings.c
    'default',
    'ignore',

    # from GET_WARNINGS_ATTR() in Python/_warnings.c
    'WarningMessage',
    '_showwarnmsg',
    '_warn_unawaited_coroutine',
    'defaultaction',
    'filters',
    'onceregistry',

    # from WRAP_METHOD() in Objects/weakrefobject.c
    '__bytes__',
    '__reversed__',

    # from COPY_ATTR() in Objects/funcobject.c
    '__module__',
    '__name__',
    '__qualname__',
    '__doc__',
    '__annotations__',

    # from SLOT* in Objects/typeobject.c
    '__abs__',
    '__add__',
    '__aiter__',
    '__and__',
    '__anext__',
    '__await__',
    '__bool__',
    '__call__',
    '__contains__',
    '__del__',
    '__delattr__',
    '__delete__',
    '__delitem__',
    '__eq__',
    '__float__',
    '__floordiv__',
    '__ge__',
    '__get__',
    '__getattr__',
    '__getattribute__',
    '__getitem__',
    '__gt__',
    '__hash__',
    '__iadd__',
    '__iand__',
    '__ifloordiv__',
    '__ilshift__',
    '__imatmul__',
    '__imod__',
    '__imul__',
    '__index__',
    '__init__',
    '__int__',
    '__invert__',
    '__ior__',
    '__ipow__',
    '__irshift__',
    '__isub__',
    '__iter__',
    '__itruediv__',
    '__ixor__',
    '__le__',
    '__len__',
    '__lshift__',
    '__lt__',
    '__matmul__',
    '__mod__',
    '__mul__',
    '__ne__',
    '__neg__',
    '__new__',
    '__next__',
    '__or__',
    '__pos__',
    '__pow__',
    '__radd__',
    '__rand__',
    '__repr__',
    '__rfloordiv__',
    '__rlshift__',
    '__rmatmul__',
    '__rmod__',
    '__rmul__',
    '__ror__',
    '__rpow__',
    '__rrshift__',
    '__rshift__',
    '__rsub__',
    '__rtruediv__',
    '__rxor__',
    '__set__',
    '__setattr__',
    '__setitem__',
    '__str__',
    '__sub__',
    '__truediv__',
    '__xor__',
    '__divmod__',
    '__rdivmod__',
    '__buffer__',
    '__release_buffer__',

    #Workarounds for GH-108918
    'alias',
    'args',
    'exc_type',
    'exc_value',
    'self',
    'traceback',
]

NON_GENERATED_IMMORTAL_OBJECTS = [
    # The generated ones come from generate_runtime_init().

    # b'' (empty bytes string)
    ('_PyStaticObject_CheckBytesSingleton({}, 0, 0);',
     '(PyObject *)&_Py_SINGLETON(bytes_empty)'),
    # () (empty tuple)
    ('_PyStaticObject_CheckSingleton({}, &PyTuple_Type);',
     '(PyObject *)&_Py_SINGLETON(tuple_empty)'),
    ('_PyStaticObject_CheckSingleton({}, &_PyHamt_BitmapNode_Type);',
     '(PyObject *)&_Py_SINGLETON(hamt_bitmap_node_empty)'),
    ('_PyStaticObject_CheckSingleton({}, &_PyHamt_Type);',
     '(PyObject *)&_Py_INTERP_SINGLETON(interp, hamt_empty)'),
    ('_PyStaticObject_CheckSingleton({}, &_PyContextTokenMissing_Type);',
     '(PyObject *)&_Py_SINGLETON(context_token_missing)'),
    # False
    ('_PyStaticObject_CheckLongSingleton({}, 0, 1);',
     '(PyObject *)&_Py_FalseStruct '),
    # True
    ('_PyStaticObject_CheckLongSingleton({}, 1, 1);',
     '(PyObject *)&_Py_TrueStruct'),
    # None
    ('_PyStaticObject_CheckSingleton({}, &_PyNone_Type);',
     '(PyObject *)&_Py_NoneStruct'),
    # Ellipsis (...)
    ('_PyStaticObject_CheckSingleton({}, &PyEllipsis_Type);',
     '(PyObject *)&_Py_EllipsisObject'),
    # Py_NotImplemented
    ('_PyStaticObject_CheckSingleton({}, &_PyNotImplemented_Type);',
     '(PyObject *)&_Py_NotImplementedStruct'),
]


#######################################
# helpers

def iter_files():
    for name in ('Modules', 'Objects', 'Parser', 'PC', 'Programs', 'Python'):
        root = os.path.join(ROOT, name)
        for dirname, _, files in os.walk(root):
            for name in files:
                if not name.endswith(('.c', '.h')):
                    continue
                yield os.path.join(dirname, name)


def iter_global_strings():
    id_regex = re.compile(r'\b_Py_ID\((\w+)\)')
    str_regex = re.compile(r'\b_Py_DECLARE_STR\((\w+), "(.*?)"\)')
    for filename in iter_files():
        try:
            infile = open(filename, encoding='utf-8')
        except FileNotFoundError:
            # The file must have been a temporary file.
            continue
        with infile:
            for lno, line in enumerate(infile, 1):
                for m in id_regex.finditer(line):
                    identifier, = m.groups()
                    yield identifier, None, filename, lno, line
                for m in str_regex.finditer(line):
                    varname, string = m.groups()
                    yield varname, string, filename, lno, line


def iter_to_marker(lines, marker):
    for line in lines:
        if line.rstrip() == marker:
            break
        yield line


class Printer:

    def __init__(self, file):
        self.level = 0
        self.file = file
        self.continuation = [False]

    @contextlib.contextmanager
    def indent(self):
        save_level = self.level
        try:
            self.level += 1
            yield
        finally:
            self.level = save_level

    def write(self, arg):
        eol = '\n'
        if self.continuation[-1]:
            eol = f' \\{eol}' if arg else f'\\{eol}'
        self.file.writelines(("    "*self.level, arg, eol))

    @contextlib.contextmanager
    def block(self, prefix, suffix="", *, continuation=None):
        if continuation is None:
            continuation = self.continuation[-1]
        self.continuation.append(continuation)

        self.write(prefix + " {")
        with self.indent():
            yield
        self.continuation.pop()
        self.write("}" + suffix)


@contextlib.contextmanager
def open_for_changes(filename):
    """Like open() but only write to the file if it changed."""
    with open(filename) as infile:
        orig = infile.read()

    outfile = io.StringIO()
    yield outfile
    text = outfile.getvalue()
    if text != orig:
        with open(filename, 'w', encoding='utf-8') as outfile:
            outfile.write(text)
    else:
        print(f'# not changed: {filename}')


#######################################
# the global objects

START = f'/* The following is auto-generated by {SCRIPT_NAME}. */'
END = '/* End auto-generated code */'


def generate_global_strings(identifiers, strings):
    filename = os.path.join(INTERNAL, 'pycore_global_strings.h')

    # Read the non-generated part of the file.
    with open(filename) as infile:
        orig = infile.read()
    lines = iter(orig.rstrip().splitlines())
    before = '\n'.join(iter_to_marker(lines, START))
    for _ in iter_to_marker(lines, END):
        pass
    after = '\n'.join(lines)

    # Generate the file.
    with open_for_changes(filename) as outfile:
        printer = Printer(outfile)
        printer.write(before)
        printer.write(START)
        with printer.block('struct _Py_global_strings', ';'):
            with printer.block('struct', ' literals;'):
                for literal, name in sorted(strings.items(), key=lambda x: x[1]):
                    printer.write(f'STRUCT_FOR_STR({name}, "{literal}")')
            outfile.write('\n')
            with printer.block('struct', ' identifiers;'):
                for name in sorted(identifiers):
                    assert name.isidentifier(), name
                    printer.write(f'STRUCT_FOR_ID({name})')
            with printer.block('struct', ' ascii[128];'):
                printer.write("PyASCIIObject _ascii;")
                printer.write("uint8_t _data[2];")
            with printer.block('struct', ' latin1[128];'):
                printer.write("PyCompactUnicodeObject _latin1;")
                printer.write("uint8_t _data[2];")
        printer.write(END)
        printer.write(after)


def generate_runtime_init(identifiers, strings):
    nsmallnegints, nsmallposints = consts_getter.get_nsmallnegints_and_nsmallposints()

    # Then target the runtime initializer.
    filename = os.path.join(INTERNAL, 'pycore_runtime_init_generated.h')

    # Read the non-generated part of the file.
    with open(filename) as infile:
        orig = infile.read()
    lines = iter(orig.rstrip().splitlines())
    before = '\n'.join(iter_to_marker(lines, START))
    for _ in iter_to_marker(lines, END):
        pass
    after = '\n'.join(lines)

    # Generate the file.
    with open_for_changes(filename) as outfile:
        immortal_objects = []
        printer = Printer(outfile)
        printer.write(before)
        printer.write(START)
        with printer.block('#define _Py_small_ints_INIT', continuation=True):
            for i in range(-nsmallnegints, nsmallposints):
                printer.write(f'_PyLong_DIGIT_INIT({i}),')
                immortal_objects.append((
                    None,
                    f'(PyObject *)&_Py_SINGLETON(small_ints)[_PY_NSMALLNEGINTS + {i}]',
                ))
        printer.write('')
        with printer.block('#define _Py_bytes_characters_INIT', continuation=True):
            for i in range(256):
                printer.write(f'_PyBytes_CHAR_INIT({i}),')
                immortal_objects.append((
                    None,
                    f'(PyObject *)&_Py_SINGLETON(bytes_characters)[{i}]',
                ))
        printer.write('')
        with printer.block('#define _Py_str_literals_INIT', continuation=True):
            for literal, name in sorted(strings.items(), key=lambda x: x[1]):
                printer.write(f'INIT_STR({name}, "{literal}"),')
                escaped = literal.replace("{", "{{").replace("}", "}}")
                immortal_objects.append((
                    f'_PyStaticObject_CheckUnicodeSingleton({{}}, "{escaped}", {len(literal)});',
                    f'(PyObject *)&_Py_STR({name})',
                ))
        printer.write('')
        with printer.block('#define _Py_str_identifiers_INIT', continuation=True):
            for name in sorted(identifiers):
                assert name.isidentifier(), name
                printer.write(f'INIT_ID({name}),')
                assert all(ch not in "{}" for ch in name)
                immortal_objects.append((
                    f'_PyStaticObject_CheckUnicodeSingleton({{}}, "{name}", {len(name)});',
                    f'(PyObject *)&_Py_ID({name})',
                ))
        printer.write('')
        with printer.block('#define _Py_str_ascii_INIT', continuation=True):
            for i in range(128):
                printer.write(f'_PyASCIIObject_INIT("\\x{i:02x}"),')
                immortal_objects.append((
                    None,
                    f'(PyObject *)&_Py_SINGLETON(strings).ascii[{i}]',
                ))
        printer.write('')
        with printer.block('#define _Py_str_latin1_INIT', continuation=True):
            for i in range(128, 256):
                utf8 = ['"']
                for c in chr(i).encode('utf-8'):
                    utf8.append(f"\\x{c:02x}")
                utf8.append('"')
                printer.write(f'_PyUnicode_LATIN1_INIT("\\x{i:02x}", {"".join(utf8)}),')
                immortal_objects.append((
                    None,
                    f'(PyObject *)&_Py_SINGLETON(strings).latin1[{i} - 128]',
                ))
        printer.write(END)
        printer.write(after)
        return immortal_objects


def generate_static_strings_initializer(identifiers, strings):
    # Target the runtime initializer.
    filename = os.path.join(INTERNAL, 'pycore_unicodeobject_generated.h')

    # Read the non-generated part of the file.
    with open(filename) as infile:
        orig = infile.read()
    lines = iter(orig.rstrip().splitlines())
    before = '\n'.join(iter_to_marker(lines, START))
    for _ in iter_to_marker(lines, END):
        pass
    after = '\n'.join(lines)

    # Generate the file.
    with open_for_changes(filename ) as outfile:
        printer = Printer(outfile)
        printer.write(before)
        printer.write(START)
        printer.write("static inline void")
        with printer.block("_PyUnicode_InitStaticStrings(PyInterpreterState *interp)"):
            printer.write(f'PyObject *string;')
            for i in sorted(identifiers):
                # This use of _Py_ID() is ignored by iter_global_strings()
                # since iter_files() ignores .h files.
                printer.write(f'string = &_Py_ID({i});')
                printer.write(f'_PyUnicode_InternStatic(interp, &string);')
                printer.write(f'assert(_PyUnicode_CheckConsistency(string, 1));')
                printer.write(f'assert(PyUnicode_GET_LENGTH(string) != 1);')
            for value, name in sorted(strings.items()):
                printer.write(f'string = &_Py_STR({name});')
                printer.write(f'_PyUnicode_InternStatic(interp, &string);')
                printer.write(f'assert(_PyUnicode_CheckConsistency(string, 1));')
                printer.write(f'assert(PyUnicode_GET_LENGTH(string) != 1);')
        printer.write(END)
        printer.write(after)


def generate_global_object_finalizers(generated_immortal_objects):
    nsmallnegints, nsmallposints = consts_getter.get_nsmallnegints_and_nsmallposints()

    # Target the runtime initializer.
    filename = os.path.join(INTERNAL, 'pycore_global_objects_fini_generated.h')

    # Generate the file.
    with open_for_changes(filename) as outfile:
        printer = Printer(outfile)
        printer.write("""
// File automatically generated by %s

#ifndef Py_INTERNAL_GLOBAL_OBJECTS_FINI_GENERATED_INIT_H
#define Py_INTERNAL_GLOBAL_OBJECTS_FINI_GENERATED_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_global_objects_fini.h"  // _PyStaticObject_CheckSingleton()
#include "pycore_hamt.h"          // _PyHamt_BitmapNode_Type

#ifdef Py_DEBUG
        """.strip() % SCRIPT_NAME)
        printer.write('')
        printer.write('// Check static objects consistency: detect if a C extension modified')
        printer.write('// an immutable singleton by mistake.')
        printer.write("static inline void")
        with printer.block(
                "_PyStaticObjects_CheckAll(PyInterpreterState *interp)"):
            printer.write('/* generated runtime-global */')
            printer.write('// (see pycore_runtime_init_generated.h)')
            for fmt, ref in generated_immortal_objects:
                if fmt is not None:
                    printer.write(fmt.format(ref))

            with printer.block(f'for (int i={-nsmallnegints}; i < {nsmallposints}; i++)'):
                obj = '(PyObject *)&_Py_SINGLETON(small_ints)[_PY_NSMALLNEGINTS + i]'
                printer.write(f'_PyStaticObject_CheckLongSingleton({obj}, i, 0);')

            with printer.block(f'for (int i=0; i <= 255; i++)'):
                obj = '(PyObject *)&_Py_SINGLETON(bytes_characters)[i]'
                printer.write(f'_PyStaticObject_CheckBytesSingleton({obj}, 1, i);')

            with printer.block(f'for (int i=0; i <= 127; i++)'):
                obj = '(PyObject *)&_Py_SINGLETON(strings).ascii[i]'
                printer.write(f'_PyStaticObject_CheckUnicodeCharSingleton({obj}, i);')

            with printer.block(f'for (int i=128; i <= 255; i++)'):
                obj = '(PyObject *)&_Py_SINGLETON(strings).latin1[i - 128]'
                printer.write(f'_PyStaticObject_CheckUnicodeCharSingleton({obj}, i);')

            printer.write('/* non-generated */')
            for fmt, ref in NON_GENERATED_IMMORTAL_OBJECTS:
                printer.write(fmt.format(ref))
        printer.write('')
        printer.write('#endif  // Py_DEBUG')
        printer.write('')
        printer.write("""
/* End auto-generated code */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_FINI_GENERATED_INIT_H */
        """.strip())


def get_identifiers_and_strings() -> 'tuple[set[str], dict[str, str]]':
    identifiers = set(IDENTIFIERS)
    strings = {}
    # Note that we store strings as they appear in C source, so the checks here
    # can be defeated, e.g.:
    # - "a" and "\0x61" won't be reported as duplicate.
    # - "\n" appears as 2 characters.
    # Probably not worth adding a C string parser.
    for name, string, *_ in iter_global_strings():
        if string is None:
            if name not in IGNORED:
                identifiers.add(name)
        else:
            if len(string) == 1 and ord(string) < 256:
                # Give a nice message for common mistakes.
                # To cover tricky cases (like "\n") we also generate C asserts.
                raise ValueError(
                    'do not use &_Py_ID or &_Py_STR for one-character latin-1 '
                    f'strings, use _Py_LATIN1_CHR instead: {string!r}')
            if string not in strings:
                strings[string] = name
            elif name != strings[string]:
                raise ValueError(f'name mismatch for string {string!r} ({name!r} != {strings[string]!r}')
    overlap = identifiers & set(strings.keys())
    if overlap:
        raise ValueError(
            'do not use both _Py_ID and _Py_DECLARE_STR for the same string: '
            + repr(overlap))
    return identifiers, strings


#######################################
# the script

def main() -> None:
    identifiers, strings = get_identifiers_and_strings()

    generate_global_strings(identifiers, strings)
    generated_immortal_objects = generate_runtime_init(identifiers, strings)
    generate_static_strings_initializer(identifiers, strings)
    generate_global_object_finalizers(generated_immortal_objects)


if __name__ == '__main__':
    main()
