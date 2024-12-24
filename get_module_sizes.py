#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
from typing import Dict, Iterable

TABLE_PATTERN = re.compile(r"""
    \s*(?P<number>\d+):\s+       # Match the symbol number. Allow spaces because symbol numbers are aligned to the right.
    (?P<address>[a-zA-Z0-9]+)\s+ # Symbol address in file.
    (?P<size>\d+)\s+             # Symbol size.
    (?P<type>\w+)\s+             # Symbol type.
    (?P<bind>\w+)\s+             # Symbol bind.
    (?P<vis>\w+)\s+              # Symbol Vis(ibility, I think).
    (?P<ndx>\w+)\s+              # Symbol NDX.
    (?P<name>[\w.]+)             # Symbol name.
""", re.X)

MODULES= [
        'abc',
        'aifc',
        '_aix_support',
        'antigravity',
        'argparse',
        'ast',
        'base64',
        'bdb',
        'bisect',
        'calendar',
        'cgi',
        'cgitb',
        'chunk',
        'cmd',
        'codecs',
        'codeop',
        'code',
        'collections',
        '_collections_abc',
        'colorsys',
        '_compat_pickle',
        'compileall',
        '_compression',
        'concurrent',
        'configparser',
        'contextlib',
        'contextvars',
        'copy',
        'copyreg',
        'cProfile',
        'crypt',
        'csv',
        'dataclasses',
        'datetime',
        'dbm',
        'decimal',
        'difflib',
        'dis',
        'doctest',
        'email',
        'encodings',
        'ensurepip',
        'enum',
        'filecmp',
        'fileinput',
        'fnmatch',
        'fractions',
        'ftplib',
        'functools',
        '__future__',
        'genericpath',
        'getopt',
        'getpass',
        'gettext',
        'glob',
        'graphlib',
        'gzip',
        'hashlib',
        'heapq',
        'hmac',
        'html',
        'http',
        'idlelib',
        'imaplib',
        'imghdr',
        'importlib',
        'inspect',
        'io',
        'ipaddress',
        'json',
        'keyword',
        'lib2to3',
        'linecache',
        'locale',
        'logging',
        'lzma',
        'mailbox',
        'mailcap',
        '_markupbase',
        'mimetypes',
        'modulefinder',
        'msilib',
        'multiprocessing',
        'netrc',
        'nntplib',
        'ntpath',
        'nturl2path',
        'numbers',
        'opcode',
        'operator',
        'optparse',
        'os',
        '_osx_support',
        'pathlib',
        'pdb',
        '__phello__',
        'pickle',
        'pickletools',
        'pipes',
        'pkgutil',
        'platform',
        'plistlib',
        'poplib',
        'posixpath',
        'pprint',
        'profile',
        'pstats',
        'pty',
        '_py_abc',
        'pyclbr',
        'py_compile',
        '_pydatetime',
        '_pydecimal',
        'pydoc_data',
        'pydoc',
        '_pyio',
        '_pylong',
        'queue',
        'quopri',
        'random',
        're',
        'reprlib',
        'rlcompleter',
        'sched',
        'selectors',
        'shelve',
        'shlex',
        'shutil',
        'signal',
        'smtplib',
        'sndhdr',
        'socket',
        'socketserver',
        'statistics',
        'stat',
        'stringprep',
        'string',
        '_strptime',
        'struct',
        'subprocess',
        'sunau',
        'symtable',
        'sysconfig',
        'tabnanny',
        'tarfile',
        'telnetlib',
        'tempfile',
        'textwrap',
        'this',
        '_threading_local',
        'threading',
        'timeit',
        'tokenize',
        'token',
        'tomllib',
        'traceback',
        'tracemalloc',
        'trace',
        'tty',
        'types',
        'typing',
        'urllib',
        'uuid',
        'uu',
        'warnings',
        'wave',
        'weakref',
        '_weakrefset',
        'webbrowser',
        'wsgiref',
        'xdrlib',
        'zipapp',
        'zipfile',
        'zoneinfo',
        '__hello__',

        'site',
        '_sitebuiltins',
        'runpy',

        'gdb',
        'pygments',

        'zipimport',

        'const_str',
        'const_int',
]

def print_warning(message: str, prefix: str = "Warning: ", color: str = "\033[33m"):
    ANSI_RESET = "\033[0m"
    print(f"{color}{prefix}{ANSI_RESET}{message}", file=sys.stderr)

def human_bytes(num_bytes: float, byte_step: int = 1024) -> str:
    """Return the given bytes as a human friendly string."""
    PREFIXES = ['B', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y']

    num_step = num_bytes
    chosen_prefix = None
    for chosen_prefix in PREFIXES:
        new_num_step = num_step / byte_step
        if new_num_step < 1.0:
            break

        num_step = new_num_step

    if chosen_prefix != 'B':
        chosen_prefix += ("i" if byte_step == 1024 else "") + "B"

    return f"{num_step:.2f} {chosen_prefix}"

def get_module_sizes(object_file: str, module_list: Iterable[str] = None) -> Dict[str, int]:
    module_list = module_list or MODULES.copy()

    symbol_info = subprocess.run(["readelf", "-sW", "--sym-base=10", object_file], check=True, capture_output=True).stdout.decode()

    module_sizes = {}
    for symbol_str in symbol_info.splitlines():
        symbol_match = TABLE_PATTERN.search(symbol_str)
        if symbol_match is None:
            print_warning(f"Couldn't match table to line: {symbol_str!r}")
            continue

        symbol_name, symbol_size = symbol_match.group("name"), int(symbol_match.group("size"))
        for existing_module in module_list:
            if symbol_name.startswith((f"{existing_module}_", f"_Py_get_{existing_module}_")):
                module_sizes[existing_module] = module_sizes.get(existing_module, 0) + symbol_size
                break
        else:
            print_warning(f"Can't match symbol {symbol_name} (size: {human_bytes(symbol_size)}) to module")

    return module_sizes

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("object_file")
    parser.add_argument("--total", action='store_true')
    args = parser.parse_args()

    module_sizes = get_module_sizes(args.object_file)
    sorted_module_sizes = sorted(module_sizes.items(), key=lambda module_tuple: module_tuple[1])

    bytes_total = 0
    for module_name, module_size in sorted_module_sizes:
        print(f"{human_bytes(module_size)}\t{module_name}")
        bytes_total += module_size

    if args.total:
        print(f"Total:\t{human_bytes(bytes_total)}")

if __name__ == "__main__":
    main()
