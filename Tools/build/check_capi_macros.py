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
IGNORED_FILENAME = os.path.join(TOOLS_BUILD_DIR, 'check_capi_macros_ignored.txt')

DEFINE_REGEX = re.compile(r'\s*# *define\s+(.*)')
PYTHON_PREFIX = re.compile(r'(Py|PY|_Py|_PY)')
DEFINE_NAME_REGEX = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)\b')
UNDEF_REGEX = re.compile(r'#undef (.*)')


def parse_file(filename, names):
    with open(filename, encoding='utf8') as fp:
        for line in fp:
            match = DEFINE_REGEX.match(line)
            if not match:
                continue
            macro = match.group(1)

            if PYTHON_PREFIX.match(macro):
                continue

            match = DEFINE_NAME_REGEX.match(macro)
            if not match:
                print(f"ERROR: {filename}: Unable to parse {line!r}")
                sys.exit(1)
            name = match.group(1)
            names.append((name, filename))


def parse_pyconfig_in(filename, names):
    with open(filename, encoding='utf8') as fp:
        for line in fp:
            match = UNDEF_REGEX.match(line)
            if not match:
                continue
            name = match.group(1)
            names.append((name, filename))


def get_ignored_names():
    ignored = set()
    with open(IGNORED_FILENAME, encoding='utf8') as fp:
        for line in fp:
            name = line.strip()
            if name.startswith('#'):
                # Ignore comment
                continue
            if name:
                ignored.add(name)
    return ignored


def main():
    failure = False

    include_dir = os.path.join(SRC_DIR, 'Include')
    files = glob.glob(os.path.join(include_dir, '*.h'))
    files.extend(glob.glob(os.path.join(include_dir, 'cpython', '*.h')))
    names = []  # list of (name: str, filename: str)
    for filename in files:
        parse_file(filename, names)

    filename = os.path.join(SRC_DIR, 'pyconfig.h.in')
    parse_file(filename, names)
    parse_pyconfig_in(filename, names)
    names.sort()

    names_set = {name for name, filename in names}
    ignored = get_ignored_names()
    outdated = ignored - names_set
    if outdated:
        print(f"ERROR: {IGNORED_FILENAME} is outdated, "
              "the macros can be removed:")
        print()
        for name in sorted(outdated):
            print(f" - {name}")
        print()
        print(f"Total: {len(outdated)} macros")
        print()
        failure = True

    new_macros = names_set - ignored
    if new_macros:
        print('ERROR: the Python C API defines the following macros '
              'with a name not starting with "Py":')
        print()
        count = 0
        for name, filename in names:
            if name in ignored:
                continue
            print(f"- {name} defined by {filename}")
            count += 1
        print()
        print(f"Total: {count} macros")

    if not failure:
        print("OK: the Python C API only defines macros with names "
              f"starting with Py (ignoring {len(ignored)} macros)")
        sys.exit(0)

    sys.exit(1)


if __name__ == "__main__":
    main()
