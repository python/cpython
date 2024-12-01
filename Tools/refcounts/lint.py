"""Lint Doc/data/refcounts.dat."""

from __future__ import annotations

import itertools
import re
import tomllib
from argparse import ArgumentParser
from dataclasses import dataclass, field
from enum import auto as _auto, Enum
from pathlib import Path
from typing import TYPE_CHECKING, LiteralString, NamedTuple

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Mapping

C_ELLIPSIS: LiteralString = '...'

MATCH_TODO: Callable[[str], re.Match | None]
MATCH_TODO = re.compile(r'^#\s*TODO:\s*(\w+)$').match

OBJECT_TYPES: frozenset[str] = frozenset()

for qualifier, object_type, suffix in itertools.product(
    ('const ', ''),
    (
        'PyObject',
        'PyLongObject', 'PyTypeObject',
        'PyCodeObject', 'PyFrameObject',
        'PyModuleObject', 'PyVarObject',
    ),
    ('*', '**', '* const *', '* const*'),
):
    OBJECT_TYPES |= {
        f'{qualifier}{object_type}{suffix}',
        f'{qualifier}{object_type} {suffix}',
    }
del suffix, object_type, qualifier

IGNORE_LIST: frozenset[str] = frozenset((
    # part of the stable ABI but should not be used at all
    'PyUnicode_GetSize',
    # part of the stable ABI but completely removed
    '_PyState_AddModule',
))

def flno_(lineno: int) -> str:
    # Format the line so that users can C/C from the terminal
    # the line number and jump with their editor using Ctrl+G.
    return f'{lineno:>5} '

class RefType(Enum):
    UNKNOWN = _auto()
    UNUSED = _auto()
    DECREF = _auto()
    BORROW = _auto()
    INCREF = _auto()
    STEALS = _auto()
    NULL = _auto()  # for return values only

class LineInfo(NamedTuple):
    func: str
    ctype: str | None
    name: str | None
    reftype: RefType | None
    comment: str

    raw_func: str
    raw_ctype: str
    raw_name: str
    raw_reftype: str

    strip_func: bool
    strip_ctype: bool
    strip_name: bool
    strip_reftype: bool

@dataclass(slots=True)
class Return:
    ctype: str | None
    reftype: RefType | None
    comment: str

@dataclass(slots=True)
class Param:
    name: str
    lineno: int

    ctype: str | None
    reftype: RefType | None
    comment: str

@dataclass(slots=True)
class Signature:
    name: str
    lineno: int
    rparam: Return
    params: dict[str, Param] = field(default_factory=dict)

class FileView(NamedTuple):
    signatures: Mapping[str, Signature]
    incomplete: frozenset[str]

def parse_line(line: str) -> LineInfo | None:
    parts = line.split(':', maxsplit=4)
    if len(parts) != 5:
        return None

    raw_func, raw_ctype, raw_name, raw_reftype, comment = parts

    func = raw_func.strip()
    strip_func = func != raw_func
    if not func:
        return None

    clean_ctype = raw_ctype.strip()
    ctype = clean_ctype or None
    strip_ctype = clean_ctype != raw_ctype

    clean_name = raw_name.strip()
    name = clean_name or None
    strip_name = clean_name != raw_name

    clean_reftype = raw_reftype.strip()
    strip_reftype = clean_reftype != raw_reftype

    if clean_reftype == '-1':
        reftype = RefType.DECREF
    elif clean_reftype == '0':
        reftype = RefType.BORROW
    elif clean_reftype == '+1':
        reftype = RefType.INCREF
    elif clean_reftype == '$':
        reftype = RefType.STEALS
    elif clean_reftype == 'null':
        reftype = RefType.NULL
    elif not clean_reftype:
        reftype = RefType.UNUSED
    else:
        reftype = RefType.UNKNOWN

    comment = comment.strip()
    return LineInfo(func, ctype, name, reftype, comment,
                    raw_func, raw_ctype, raw_name, raw_reftype,
                    strip_func, strip_ctype, strip_name, strip_reftype)

class ParserReporter:
    def __init__(self) -> None:
        self.count = 0

    def info(self, lineno: int, message: str) -> None:
        self.count += 1
        print(f'{flno_(lineno)} {message}')

    warn = error = info

def parse(lines: Iterable[str]) -> FileView:
    signatures: dict[str, Signature] = {}
    incomplete: set[str] = set()

    w = ParserReporter()

    for lineno, line in enumerate(map(str.strip, lines), 1):
        if not line:
            continue
        if line.startswith('#'):
            if match := MATCH_TODO(line):
                incomplete.add(match.group(1))
            continue

        e = parse_line(line)
        if e is None:
            w.error(lineno, f'cannot parse: {line!r}')
            continue

        if e.strip_func:
            w.warn(lineno, f'[func] whitespaces around {e.raw_func!r}')
        if e.strip_ctype:
            w.warn(lineno, f'[type] whitespaces around {e.raw_ctype!r}')
        if e.strip_name:
            w.warn(lineno, f'[name] whitespaces around {e.raw_name!r}')
        if e.strip_reftype:
            w.warn(lineno, f'[ref] whitespaces around {e.raw_reftype!r}')

        func, name = e.func, e.name
        ctype, reftype = e.ctype, e.reftype
        comment = e.comment

        if func not in signatures:
            # process return value
            if name is not None:
                w.warn(lineno, f'named return value in {line!r}')
            ret_param = Return(ctype, reftype, comment)
            signatures[func] = Signature(func, lineno, ret_param)
        else:
            # process parameter
            if name is None:
                w.error(lineno, f'missing parameter name in {line!r}')
                continue
            sig: Signature = signatures[func]
            if name in sig.params:
                w.error(lineno, f'duplicated parameter name in {line!r}')
                continue
            sig.params[name] = Param(name, lineno, ctype, reftype, comment)

    if w.count:
        print()
        print(f'Found {w.count} issue(s)')

    return FileView(signatures, frozenset(incomplete))

class CheckerWarnings:
    def __init__(self) -> None:
        self.count = 0

    def block(self, sig: Signature, message: str) -> None:
        self.count += 1
        print(f'{flno_(sig.lineno)} {sig.name:50} {message}')

    def param(self, sig: Signature, param: Param, message: str) -> None:
        self.count += 1
        fullname = f'{sig.name}[{param.name}]'
        print(f'{flno_(param.lineno)} {fullname:50} {message}')

def check(view: FileView) -> None:
    w = CheckerWarnings()

    for sig in view.signatures.values():  # type: Signature
        # check the return value
        rparam = sig.rparam
        if not rparam.ctype:
            w.block(sig, 'missing return value type')
        if rparam.reftype is RefType.UNKNOWN:
            w.block(sig, 'unknown return value type')
        # check the parameters
        for name, param in sig.params.items():  # type: (str, Param)
            ctype, reftype = param.ctype, param.reftype
            if ctype in OBJECT_TYPES and reftype is RefType.UNUSED:
                w.param(sig, param, 'missing reference count management')
            if ctype not in OBJECT_TYPES and reftype is not RefType.UNUSED:
                w.param(sig, param, 'unused reference count management')
            if name != C_ELLIPSIS and not name.isidentifier():
                # Python accepts the same identifiers as in C
                w.param(sig, param, 'invalid parameter name')

    if w.count:
        print()
        print(f'Found {w.count} issue(s)')
    names = view.signatures.keys()
    if sorted(names) != list(names):
        print('Entries are not sorted')

def check_structure(view: FileView, stable_abi_file: str) -> None:
    stable_abi_str = Path(stable_abi_file).read_text()
    stable_abi = tomllib.loads(stable_abi_str)
    expect = stable_abi['function'].keys()
    # check if there are missing entries (those marked as "TODO" are ignored)
    actual = IGNORE_LIST | view.incomplete | view.signatures.keys()
    if missing := (expect - actual):
        print('[!] missing stable ABI entries:')
        for name in sorted(missing):
            print(name)

def _create_parser() -> ArgumentParser:
    parser = ArgumentParser(prog='lint.py')
    parser.add_argument('file', help='the file to check')
    parser.add_argument('--stable-abi', help='the stable ABI TOML file to use')
    return parser

def main() -> None:
    parser = _create_parser()
    args = parser.parse_args()
    lines = Path(args.file).read_text().splitlines()
    print(' PARSING '.center(80, '-'))
    view = parse(lines)
    print(' CHECKING '.center(80, '-'))
    check(view)
    if args.stable_abi:
        print(' CHECKING STABLE ABI '.center(80, '-'))
        check_structure(view, args.stable_abi)

if __name__ == '__main__':
    main()
