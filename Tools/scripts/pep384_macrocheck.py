"""
pep384_macrocheck.py

This program tries to locate errors in the relevant Python header
files where macros access type fields when they are reachable from
the limited API.

The idea is to search macros with the string "->tp_" in it.
When the macro name does not begin with an underscore,
then we have found a dormant error.

Christian Tismer
2018-06-02
"""

import sys
import os
import re


DEBUG = False

def dprint(*args, **kw):
    if DEBUG:
        print(*args, **kw)

def parse_headerfiles(startpath):
    """
    Scan all header files which are reachable fronm Python.h
    """
    search = "Python.h"
    name = os.path.join(startpath, search)
    if not os.path.exists(name):
        raise ValueError("file {} was not found in {}\n"
            "Please give the path to Python's include directory."
            .format(search, startpath))
    errors = 0
    with open(name) as python_h:
        while True:
            line = python_h.readline()
            if not line:
                break
            found = re.match(r'^\s*#\s*include\s*"(\w+\.h)"', line)
            if not found:
                continue
            include = found.group(1)
            dprint("Scanning", include)
            name = os.path.join(startpath, include)
            if not os.path.exists(name):
                name = os.path.join(startpath, "../PC", include)
            errors += parse_file(name)
    return errors

def ifdef_level_gen():
    """
    Scan lines for #ifdef and track the level.
    """
    level = 0
    ifdef_pattern = r"^\s*#\s*if"  # covers ifdef and ifndef as well
    endif_pattern = r"^\s*#\s*endif"
    while True:
        line = yield level
        if re.match(ifdef_pattern, line):
            level += 1
        elif re.match(endif_pattern, line):
            level -= 1

def limited_gen():
    """
    Scan lines for Py_LIMITED_API yes(1) no(-1) or nothing (0)
    """
    limited = [0]   # nothing
    unlimited_pattern = r"^\s*#\s*ifndef\s+Py_LIMITED_API"
    limited_pattern = "|".join([
        r"^\s*#\s*ifdef\s+Py_LIMITED_API",
        r"^\s*#\s*(el)?if\s+!\s*defined\s*\(\s*Py_LIMITED_API\s*\)\s*\|\|",
        r"^\s*#\s*(el)?if\s+defined\s*\(\s*Py_LIMITED_API"
        ])
    else_pattern =      r"^\s*#\s*else"
    ifdef_level = ifdef_level_gen()
    status = next(ifdef_level)
    wait_for = -1
    while True:
        line = yield limited[-1]
        new_status = ifdef_level.send(line)
        dir = new_status - status
        status = new_status
        if dir == 1:
            if re.match(unlimited_pattern, line):
                limited.append(-1)
                wait_for = status - 1
            elif re.match(limited_pattern, line):
                limited.append(1)
                wait_for = status - 1
        elif dir == -1:
            # this must have been an endif
            if status == wait_for:
                limited.pop()
                wait_for = -1
        else:
            # it could be that we have an elif
            if re.match(limited_pattern, line):
                limited.append(1)
                wait_for = status - 1
            elif re.match(else_pattern, line):
                limited.append(-limited.pop())  # negate top

def parse_file(fname):
    errors = 0
    with open(fname) as f:
        lines = f.readlines()
    type_pattern = r"^.*?->\s*tp_"
    define_pattern = r"^\s*#\s*define\s+(\w+)"
    limited = limited_gen()
    status = next(limited)
    for nr, line in enumerate(lines):
        status = limited.send(line)
        line = line.rstrip()
        dprint(fname, nr, status, line)
        if status != -1:
            if re.match(define_pattern, line):
                name = re.match(define_pattern, line).group(1)
                if not name.startswith("_"):
                    # found a candidate, check it!
                    macro = line + "\n"
                    idx = nr
                    while line.endswith("\\"):
                        idx += 1
                        line = lines[idx].rstrip()
                        macro += line + "\n"
                    if re.match(type_pattern, macro, re.DOTALL):
                        # this type field can reach the limited API
                        report(fname, nr + 1, macro)
                        errors += 1
    return errors

def report(fname, nr, macro):
    f = sys.stderr
    print(fname + ":" + str(nr), file=f)
    print(macro, file=f)

if __name__ == "__main__":
    p = sys.argv[1] if sys.argv[1:] else "../../Include"
    errors = parse_headerfiles(p)
    if errors:
        # somehow it makes sense to raise a TypeError :-)
        raise TypeError("These {} locations contradict the limited API."
                        .format(errors))
