#!/usr/bin/env python
"""Check exported symbols

Check that all symbols exported by CPython (libpython, stdlib extension
modules, and similar) start with Py or _Py, or are covered by an exception.
"""

import argparse
import dataclasses
import functools
import pathlib
import subprocess
import sys
import sysconfig

ALLOWED_PREFIXES = ('Py', '_Py')
if sys.platform == 'darwin':
    ALLOWED_PREFIXES += ('__Py',)

# mimalloc doesn't use static, but it's symbols are not exported
# from the shared library.  They do show up in the static library
# before its linked into an executable.
ALLOWED_STATIC_PREFIXES = ('mi_', '_mi_')

# "Legacy": some old symbols are prefixed by "PY_".
EXCEPTIONS = frozenset({
    'PY_TIMEOUT_MAX',
})

IGNORED_EXTENSION = "_ctypes_test"


@dataclasses.dataclass
class Library:
    path: pathlib.Path
    is_dynamic: bool

    @functools.cached_property
    def is_ignored(self):
        name_without_extemnsions = self.path.name.partition('.')[0]
        return name_without_extemnsions == IGNORED_EXTENSION


@dataclasses.dataclass
class Symbol:
    name: str
    type: str
    library: str

    def __str__(self):
        return f"{self.name!r} (type {self.type}) from {self.library.path}"

    @functools.cached_property
    def is_local(self):
        # If lowercase, the symbol is usually local; if uppercase, the symbol
        # is global (external).  There are however a few lowercase symbols that
        # are shown for special global symbols ("u", "v" and "w").
        if self.type.islower() and self.type not in "uvw":
            return True

        return False

    @functools.cached_property
    def is_smelly(self):
        if self.is_local:
            return False
        if self.name.startswith(ALLOWED_PREFIXES):
            return False
        if self.name in EXCEPTIONS:
            return False
        if not self.library.is_dynamic and self.name.startswith(
                ALLOWED_STATIC_PREFIXES):
            return False
        if self.library.is_ignored:
            return False
        return True

    @functools.cached_property
    def _sort_key(self):
        return self.name, self.library.path

    def __lt__(self, other_symbol):
        return self._sort_key < other_symbol._sort_key


def get_exported_symbols(library):
    # Only look at dynamic symbols
    args = ['nm', '--no-sort']
    if library.is_dynamic:
        args.append('--dynamic')
    args.append(library.path)
    proc = subprocess.run(args, stdout=subprocess.PIPE, encoding='utf-8')
    if proc.returncode:
        print("+", args)
        sys.stdout.write(proc.stdout)
        sys.exit(proc.returncode)

    stdout = proc.stdout.rstrip()
    if not stdout:
        raise Exception("command output is empty")

    symbols = []
    for line in stdout.splitlines():
        if not line:
            continue

        # Split lines like  '0000000000001b80 D PyTextIOWrapper_Type'
        parts = line.split(maxsplit=2)
        # Ignore lines like '                 U PyDict_SetItemString'
        # and headers like 'pystrtod.o:'
        if len(parts) < 3:
            continue

        symbol = Symbol(name=parts[-1], type=parts[1], library=library)
        if not symbol.is_local:
            symbols.append(symbol)

    return symbols


def get_extension_libraries():
    # This assumes pybuilddir.txt is in same directory as pyconfig.h.
    # In the case of out-of-tree builds, we can't assume pybuilddir.txt is
    # in the source folder.
    config_dir = pathlib.Path(sysconfig.get_config_h_filename()).parent
    try:
        config_dir = config_dir.relative_to(pathlib.Path.cwd(), walk_up=True)
    except ValueError:
        pass
    filename = config_dir / "pybuilddir.txt"
    pybuilddir = filename.read_text().strip()

    builddir = config_dir / pybuilddir
    result = []
    for path in sorted(builddir.glob('**/*.so')):
        if path.stem == IGNORED_EXTENSION:
            continue
        result.append(Library(path, is_dynamic=True))

    return result


def main():
    parser = argparse.ArgumentParser(
        description=__doc__.split('\n', 1)[-1])
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='be verbose (currently: print out all symbols)')
    args = parser.parse_args()

    libraries = []

    # static library
    try:
        LIBRARY = pathlib.Path(sysconfig.get_config_var('LIBRARY'))
    except TypeError as exc:
        raise Exception("failed to get LIBRARY sysconfig variable") from exc
    LIBRARY = pathlib.Path(LIBRARY)
    if LIBRARY.exists():
        libraries.append(Library(LIBRARY, is_dynamic=False))

    # dynamic library
    try:
        LDLIBRARY = pathlib.Path(sysconfig.get_config_var('LDLIBRARY'))
    except TypeError as exc:
        raise Exception("failed to get LDLIBRARY sysconfig variable") from exc
    if LDLIBRARY != LIBRARY:
        libraries.append(Library(LDLIBRARY, is_dynamic=True))

    # Check extension modules like _ssl.cpython-310d-x86_64-linux-gnu.so
    libraries.extend(get_extension_libraries())

    smelly_symbols = []
    for library in libraries:
        symbols = get_exported_symbols(library)
        if args.verbose:
            print(f"{library.path}: {len(symbols)} symbol(s) found")
        for symbol in sorted(symbols):
            if args.verbose:
                print("    -", symbol.name)
            if symbol.is_smelly:
                smelly_symbols.append(symbol)

    print()

    if smelly_symbols:
        print(f"Found {len(smelly_symbols)} smelly symbols in total!")
        for symbol in sorted(smelly_symbols):
            print(f"    - {symbol.name} from {symbol.library.path}")
        sys.exit(1)

    print(f"OK: all exported symbols of all libraries",
          f"are prefixed with {' or '.join(map(repr, ALLOWED_PREFIXES))}",
          f"or are covered by exceptions")


if __name__ == "__main__":
    main()
