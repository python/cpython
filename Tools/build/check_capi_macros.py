"""
Check that all macros defined by the Python C API have a name starting with
"Py". Ignore names listed by check_capi_macros_ignored.txt: macros with an
invalid name, added before this script was created.

Python C API:

* Include/*.h
* Include/cpython/*.h
* pyconfig.h.in
"""

import glob
import os.path
import re
import sys

TOOLS_BUILD_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(TOOLS_BUILD_DIR))

DEFINE_REGEX = re.compile(r'^\s*# *define\s+(.*)')
PYTHON_PREFIX = re.compile(r'^(Py|PY|_Py|_PY)')
DEFINE_NAME_REGEX = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)\b')
UNDEF_REGEX = re.compile(r'#undef (.*)$')


def parse_file(filename, names, ignored):
    with open(filename, encoding='utf8') as fp:
        for line in fp:
            match = DEFINE_REGEX.match(line)
            if not match:
                continue
            macro = match.group(1)

            if PYTHON_PREFIX.match(macro):
                continue

            match = DEFINE_NAME_REGEX.search(macro)
            if not match:
                print(f"ERROR: {filename}: Unable to parse {line!r}")
                sys.exit(1)
            name = match.group(1)
            if name in ignored:
                continue

            names.append((name, filename))


def parse_pyconfig_in(filename, names, ignored):
    with open(filename, encoding='utf8') as fp:
        for line in fp:
            match = UNDEF_REGEX.match(line)
            if not match:
                continue
            name = match.group(1)
            if name in ignored:
                continue
            names.append((name, filename))


def get_ignored_names():
    filename = os.path.join(TOOLS_BUILD_DIR, 'check_capi_macros_ignored.txt')
    ignored = set()
    with open(filename, encoding='utf8') as fp:
        for line in fp:
            name = line.strip()
            if name.startswith('#'):
                # Ignore comment
                continue
            if name:
                ignored.add(name)
    return ignored


def main():
    ignored = get_ignored_names()

    include_dir = os.path.join(SRC_DIR, 'Include')
    files = glob.glob(os.path.join(include_dir, '*.h'))
    files.extend(glob.glob(os.path.join(include_dir, 'cpython', '*.h')))
    names = []
    for filename in files:
        parse_file(filename, names, ignored)

    filename = os.path.join(SRC_DIR, 'pyconfig.h.in')
    parse_file(filename, names, ignored)
    parse_pyconfig_in(filename, names, ignored)
    names.sort()

    if not names:
        print("OK: the Python C API only defines macros with names "
              f"starting with Py (ignoring {len(ignored)} macros)")
        sys.exit(0)

    print('ERROR: the Python C API defines the following macros '
          'with a name not starting with "Py":')
    print()
    for name, filename in names:
        print(f"- {name} defined by {filename}")
    print()
    print(f"Total: {len(names)} macros")
    sys.exit(1)


if __name__ == "__main__":
    main()
