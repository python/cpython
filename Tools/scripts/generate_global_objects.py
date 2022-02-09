import contextlib
import glob
import os.path
import re
import sys


assert os.path.isabs(__file__), __file__
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
INTERNAL = os.path.join(ROOT, 'Include', 'internal')


STRING_LITERALS = {
    'empty': '',
    'dot': '.',
    'comma_sep': ', ',
    'percent': '%',
    'dbl_percent': '%%',

    '"anonymous" labels': None,
    'anon_dictcomp': '<dictcomp>',
    'anon_genexpr': '<genexpr>',
    'anon_lambda': '<lambda>',
    'anon_listcomp': '<listcomp>',
    'anon_module': '<module>',
    'anon_setcomp': '<setcomp>',
    'anon_string': '<string>',
    'dot_locals': '.<locals>',
}
IDENTIFIERS = [
    'Py_Repr',
    'TextIOWrapper',
    'WarningMessage',
    '_',
    '__IOBase_closed',
    '__abc_tpflags__',
    '__abs__',
    '__abstractmethods__',
    '__add__',
    '__aenter__',
    '__aexit__',
    '__aiter__',
    '__all__',
    '__and__',
    '__anext__',
    '__annotations__',
    '__args__',
    '__await__',
    '__bases__',
    '__bool__',
    '__build_class__',
    '__builtins__',
    '__bytes__',
    '__call__',
    '__cantrace__',
    '__class__',
    '__class_getitem__',
    '__classcell__',
    '__complex__',
    '__contains__',
    '__copy__',
    '__del__',
    '__delattr__',
    '__delete__',
    '__delitem__',
    '__dict__',
    '__dir__',
    '__divmod__',
    '__doc__',
    '__enter__',
    '__eq__',
    '__exit__',
    '__file__',
    '__float__',
    '__floordiv__',
    '__format__',
    '__fspath__',
    '__ge__',
    '__get__',
    '__getattr__',
    '__getattribute__',
    '__getinitargs__',
    '__getitem__',
    '__getnewargs__',
    '__getnewargs_ex__',
    '__getstate__',
    '__gt__',
    '__hash__',
    '__iadd__',
    '__iand__',
    '__ifloordiv__',
    '__ilshift__',
    '__imatmul__',
    '__imod__',
    '__import__',
    '__imul__',
    '__index__',
    '__init__',
    '__init_subclass__',
    '__instancecheck__',
    '__int__',
    '__invert__',
    '__ior__',
    '__ipow__',
    '__irshift__',
    '__isabstractmethod__',
    '__isub__',
    '__iter__',
    '__itruediv__',
    '__ixor__',
    '__le__',
    '__len__',
    '__length_hint__',
    '__loader__',
    '__lshift__',
    '__lt__',
    '__ltrace__',
    '__main__',
    '__matmul__',
    '__missing__',
    '__mod__',
    '__module__',
    '__mro_entries__',
    '__mul__',
    '__name__',
    '__ne__',
    '__neg__',
    '__new__',
    '__newobj__',
    '__newobj_ex__',
    '__next__',
    '__note__',
    '__or__',
    '__origin__',
    '__package__',
    '__parameters__',
    '__path__',
    '__pos__',
    '__pow__',
    '__prepare__',
    '__qualname__',
    '__radd__',
    '__rand__',
    '__rdivmod__',
    '__reduce__',
    '__reduce_ex__',
    '__repr__',
    '__reversed__',
    '__rfloordiv__',
    '__rlshift__',
    '__rmatmul__',
    '__rmod__',
    '__rmul__',
    '__ror__',
    '__round__',
    '__rpow__',
    '__rrshift__',
    '__rshift__',
    '__rsub__',
    '__rtruediv__',
    '__rxor__',
    '__set__',
    '__set_name__',
    '__setattr__',
    '__setitem__',
    '__setstate__',
    '__sizeof__',
    '__slotnames__',
    '__slots__',
    '__spec__',
    '__str__',
    '__sub__',
    '__subclasscheck__',
    '__subclasshook__',
    '__truediv__',
    '__trunc__',
    '__warningregistry__',
    '__weakref__',
    '__xor__',
    '_abc_impl',
    '_blksize',
    '_dealloc_warn',
    '_finalizing',
    '_find_and_load',
    '_fix_up_module',
    '_get_sourcefile',
    '_handle_fromlist',
    '_initializing',
    '_is_text_encoding',
    '_lock_unlock_module',
    '_showwarnmsg',
    '_shutdown',
    '_slotnames',
    '_strptime_time',
    '_uninitialized_submodules',
    '_warn_unawaited_coroutine',
    '_xoptions',
    'add',
    'append',
    'big',
    'buffer',
    'builtins',
    'clear',
    'close',
    'code',
    'copy',
    'copyreg',
    'decode',
    'default',
    'defaultaction',
    'difference_update',
    'dispatch_table',
    'displayhook',
    'enable',
    'encoding',
    'end_lineno',
    'end_offset',
    'errors',
    'excepthook',
    'extend',
    'filename',
    'fileno',
    'fillvalue',
    'filters',
    'find_class',
    'flush',
    'get',
    'get_source',
    'getattr',
    'ignore',
    'importlib',
    'intersection',
    'isatty',
    'items',
    'iter',
    'keys',
    'last_traceback',
    'last_type',
    'last_value',
    'latin1',
    'lineno',
    'little',
    'match',
    'metaclass',
    'mode',
    'modules',
    'mro',
    'msg',
    'n_fields',
    'n_sequence_fields',
    'n_unnamed_fields',
    'name',
    'obj',
    'offset',
    'onceregistry',
    'open',
    'parent',
    'partial',
    'path',
    'peek',
    'persistent_id',
    'persistent_load',
    'print_file_and_line',
    'ps1',
    'ps2',
    'raw',
    'read',
    'read1',
    'readable',
    'readall',
    'readinto',
    'readinto1',
    'readline',
    'reducer_override',
    'reload',
    'replace',
    'reset',
    'return',
    'reversed',
    'seek',
    'seekable',
    'send',
    'setstate',
    'sort',
    'stderr',
    'stdin',
    'stdout',
    'strict',
    'symmetric_difference_update',
    'tell',
    'text',
    'threading',
    'throw',
    'unraisablehook',
    'values',
    'version',
    'warnings',
    'warnoptions',
    'writable',
    'write',
    'zipimporter',
]


#######################################
# helpers

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


#######################################
# the global objects

START = '/* The following is auto-generated by Tools/scripts/generate_global_objects.py. */'
END = '/* End auto-generated code */'


def generate_global_strings():
    filename = os.path.join(INTERNAL, 'pycore_global_strings.h')

    # Read the non-generated part of the file.
    with open(filename) as infile:
        before = ''.join(iter_to_marker(infile, START))[:-1]
        for _ in iter_to_marker(infile, END):
            pass
        after = infile.read()[:-1]

    # Generate the file.
    with open(filename, 'w', encoding='utf-8') as outfile:
        printer = Printer(outfile)
        printer.write(before)
        printer.write(START)
        with printer.block('struct _Py_global_strings', ';'):
            with printer.block('struct', ' literals;'):
                for name, literal in STRING_LITERALS.items():
                    if literal is None:
                        outfile.write('\n')
                        printer.write(f'// {name}')
                    else:
                        printer.write(f'STRUCT_FOR_STR({name}, "{literal}")')
            outfile.write('\n')
            with printer.block('struct', ' identifiers;'):
                for name in sorted(IDENTIFIERS):
                    assert name.isidentifier(), name
                    printer.write(f'STRUCT_FOR_ID({name})')
        printer.write(END)
        printer.write(after)


def generate_runtime_init():
    # First get some info from the declarations.
    nsmallposints = None
    nsmallnegints = None
    with open(os.path.join(INTERNAL, 'pycore_global_objects.h')) as infile:
        for line in infile:
            if line.startswith('#define _PY_NSMALLPOSINTS'):
                nsmallposints = int(line.split()[-1])
            elif line.startswith('#define _PY_NSMALLNEGINTS'):
                nsmallnegints = int(line.split()[-1])
                break
        else:
            raise NotImplementedError
    assert nsmallposints and nsmallnegints

    # Then target the runtime initializer.
    filename = os.path.join(INTERNAL, 'pycore_runtime_init.h')

    # Read the non-generated part of the file.
    with open(filename) as infile:
        before = ''.join(iter_to_marker(infile, START))[:-1]
        for _ in iter_to_marker(infile, END):
            pass
        after = infile.read()[:-1]

    # Generate the file.
    with open(filename, 'w', encoding='utf-8') as outfile:
        printer = Printer(outfile)
        printer.write(before)
        printer.write(START)
        with printer.block('#define _Py_global_objects_INIT', continuation=True):
            with printer.block('.singletons =', ','):
                # Global int objects.
                with printer.block('.small_ints =', ','):
                    for i in range(-nsmallnegints, nsmallposints):
                        printer.write(f'_PyLong_DIGIT_INIT({i}),')
                printer.write('')
                # Global bytes objects.
                printer.write('.bytes_empty = _PyBytes_SIMPLE_INIT(0, 0),')
                with printer.block('.bytes_characters =', ','):
                    for i in range(256):
                        printer.write(f'_PyBytes_CHAR_INIT({i}),')
                printer.write('')
                # Global strings.
                with printer.block('.strings =', ','):
                    with printer.block('.literals =', ','):
                        for name, literal in STRING_LITERALS.items():
                            if literal is None:
                                printer.write('')
                            else:
                                printer.write(f'INIT_STR({name}, "{literal}"),')
                    with printer.block('.identifiers =', ','):
                        for name in sorted(IDENTIFIERS):
                            assert name.isidentifier(), name
                            printer.write(f'INIT_ID({name}),')
        printer.write(END)
        printer.write(after)


#######################################
# checks

def err(msg):
    print(msg, file=sys.stderr)


GETTER_RE = re.compile(r'''
    ^
    .*?
    (?:
        (?:
            _Py_ID
            [(]
            ( \w+ )  # <identifier>
            [)]
         )
        |
        (?:
            _Py_STR
            [(]
            ( \w+ )  # <literal>
            [)]
         )
     )
''', re.VERBOSE)
TYPESLOTS_RE = re.compile(r'''
    ^
    .*?
    (?:
        (?:
            SLOT0 [(] .*?, \s*
            ( \w+ )  # <slot0>
            [)]
         )
        |
        (?:
            SLOT1 [(] .*?, \s*
            ( \w+ )  # <slot1>
            , .* [)]
         )
        |
        (?:
            SLOT1BIN [(] .*?, .*?, \s*
            ( \w+ )  # <slot1bin>
            , \s*
            ( \w+ )  # <reverse>
            [)]
         )
        |
        (?:
            SLOT1BINFULL [(] .*?, .*?, .*?, \s*
            ( \w+ )  # <slot1binfull>
            , \s*
            ( \w+ )  # <fullreverse>
            [)]
         )
        |
        ( SLOT \d .* [^)] $ )  # <wrapped>
     )
''', re.VERBOSE)

def check_orphan_strings():
    literals = set(n for n, s in STRING_LITERALS.items() if s)
    identifiers = set(IDENTIFIERS)
    files = glob.iglob(os.path.join(ROOT, '**', '*.[ch]'), recursive=True)
    for i, filename in enumerate(files, start=1):
        print('.', end='')
        if i % 5 == 0:
            print(' ', end='')
        if i % 20 == 0:
            print()
        if i % 100 == 0:
            print()
        with open(filename) as infile:
            wrapped = None
            for line in infile:
                identifier = literal = reverse = None

                line = line.splitlines()[0]
                if wrapped:
                    line = f'{wrapped.rstrip()} {line}'
                    wrapped = None

                if os.path.basename(filename) == '_warnings.c':
                    m = re.match(r'^.* = GET_WARNINGS_ATTR[(][^,]*, (\w+),', line)
                    if m:
                        identifier, = m.groups()
                elif os.path.basename(filename) == 'typeobject.c':
                    m = TYPESLOTS_RE.match(line)
                    if m:
                        (slot0,
                         slot1,
                         slot1bin, reverse,
                         slot1binfull, fullreverse,
                         wrapped,
                         ) = m.groups()
                        identifier = slot0 or slot1 or slot1bin or slot1binfull
                        reverse = reverse or fullreverse

                if not identifier and not literal:
                    m = GETTER_RE.match(line)
                    if not m:
                        continue
                    identifier, literal = m.groups()

                if literal:
                    if literals and literal in literals:
                        literals.remove(literal)
                if identifier:
                    if identifiers and identifier in identifiers:
                        identifiers.remove(identifier)
                if reverse:
                    if identifiers and reverse in identifiers:
                        identifiers.remove(reverse)
                if not literals and not identifiers:
                    break
            else:
                continue
            break
    if i % 20:
        print()
    if not literals and not identifiers:
        return
    print('ERROR:', file=sys.stderr)
    if literals:
        err(' unused global string literals:')
        for name in sorted(literals):
            err(f'   {name}')
    if identifiers:
        if literals:
            print()
        err(' unused global identifiers:')
        for name in sorted(identifiers):
            err(f'   {name}')


#######################################
# the script

def main(*, check=False) -> None:
    generate_global_strings()
    generate_runtime_init()

    if check:
        check_orphan_strings()


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    main(**vars(args))
