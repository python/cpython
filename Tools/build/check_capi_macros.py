"""
Check that all macros defined by the Python C API have a name starting with
"Py". Ignore names listed by check_capi_macros_ignored.txt: macros with an
invalid name, added before this script was created.

Python C API:

* Include/*.h
* Include/cpython/*.h
* pyconfig.h.in
"""

import difflib
import glob
import os.path
import re
import sys

EXCLUDE_HEADERS = {
    # Header files not included by Python.h and ignored by this script
    'dynamic_annotations.h',
    'errcode.h',
    'opcode.h',
    'opcode_ids.h',
    'osdefs.h',
    'pyexpat.h',
    'structmember.h',

    # Header files not included by Python.h but parsed by this script:
    # - datetime.h
    # - frameobject.h
    # - marshal.h
    # - py_curses.h
    # - pydtrace.h
}

TOOLS_BUILD_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(TOOLS_BUILD_DIR))
IGNORED_FILENAME = os.path.join(TOOLS_BUILD_DIR, 'check_capi_macros_ignored.txt')

DEFINE_REGEX = re.compile(r'\s*#\s*(?P<directive>define|undef)\s+(?P<macro>.*)')
PYTHON_PREFIX = re.compile(r'(Py|PY|_Py|_PY)')
NAME_REGEX = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)\b')

CAUSES_BY_DIRECTIVE = {
    "define": "defined",
    "undef": "undefined",
}


def parse_file(filename, names):
    with open(filename, encoding='utf8') as fp:
        for lineno, line in enumerate(fp, start=1):
            # Check for '#define MACRO'
            match = DEFINE_REGEX.match(line)
            if not match:
                continue
            macro = match['macro']
            directive = match['directive']
            cause = CAUSES_BY_DIRECTIVE.get(directive, directive)

            if PYTHON_PREFIX.match(macro):
                continue

            match = NAME_REGEX.match(macro)
            if not match:
                print(f"ERROR: {filename}: Unable to parse {line!r}")
                sys.exit(1)
            name = match[1]
            names.append((name, filename, lineno, cause))


def get_ignored_names():
    ignored = []
    with open(IGNORED_FILENAME, encoding='utf8') as fp:
        for line in fp:
            name = line.strip()
            if name.startswith('#'):
                # Ignore comment
                continue
            if name:
                ignored.append(name)
    return ignored


def main():
    failure = False

    # Parse header files
    include_dir = os.path.join(SRC_DIR, 'Include')
    files = glob.glob(os.path.join(include_dir, '*.h'))
    files = [filename for filename in files
             if os.path.basename(filename) not in EXCLUDE_HEADERS]
    files.extend(glob.glob(os.path.join(include_dir, 'cpython', '*.h')))
    files.append(os.path.join(SRC_DIR, 'pyconfig.h.in'))
    names = []  # list of (name: str, filename: str, lineno: int, cause: str)
    for filename in files:
        parse_file(filename, names)

    # Parse ignore list
    ignored = get_ignored_names()

    # Check if the sorted list has duplicated entries
    if len(set(ignored)) != len(ignored):
        print(f"ERROR: {IGNORED_FILENAME} list contains duplicated entries:")
        print()
        seen = set()
        for name in ignored:
            if name not in seen:
                seen.add(name)
                continue
            print(f"- {name}")
        print()
        failure = True

    # Check if the sorted list is sorted
    ignored_sorted = sorted(ignored)
    if ignored_sorted != ignored:
        print(f"ERROR: {IGNORED_FILENAME} list is not sorted")
        print()
        diff = difflib.unified_diff(ignored, ignored_sorted,
                                    fromfile=IGNORED_FILENAME,
                                    tofile=IGNORED_FILENAME,
                                    lineterm='')
        for line in diff:
            print(line)
        print()
        failure = True

    # Check for outdated ignore list
    names_set = {name for name, filename, lineno, cause in names}
    ignored = set(ignored)
    outdated = ignored - names_set
    if outdated:
        print(f"ERROR: {IGNORED_FILENAME} is outdated, "
              "the following macros can be removed:")
        print()
        for name in sorted(outdated):
            print(f"- {name}")
        print()
        print(f"Total: {len(outdated)} macros")
        print()
        failure = True

    # Check for new macros
    new_macros = names_set - ignored
    if new_macros:
        print('ERROR: the Python C API defines the following new macros:')
        print()
        count = 0
        for name, filename, lineno, cause in sorted(names):
            if name in ignored:
                continue
            print(f"- {name} {cause} at {filename}:{lineno}")
            count += 1
        print()
        print(f"Total: {count} macros")
        failure = True

    if not failure:
        print("OK: the ignore list is up to date and sorted")
        print("OK: the Python C API only defines macros with names "
              f"starting with Py (ignoring {len(ignored)} macros)")
        sys.exit(0)

    sys.exit(1)


if __name__ == "__main__":
    main()
