"""Support functions for loading the reference count data file."""
__version__ = '$Revision$'

import os
import string
import sys


# Determine the expected location of the reference count file:
try:
    p = os.path.dirname(__file__)
except NameError:
    p = sys.path[0]
p = os.path.normpath(os.path.join(os.getcwd(), p, os.pardir,
                                  "api", "refcounts.dat"))
DEFAULT_PATH = p
del p


def load(path=DEFAULT_PATH):
    return loadfile(open(path))


def loadfile(fp):
    d = {}
    while 1:
        line = fp.readline()
        if not line:
            break
        line = string.strip(line)
        if line[:1] in ("", "#"):
            # blank lines and comments
            continue
        parts = string.split(line, ":", 4)
        function, type, arg, refcount, comment = parts
        if refcount == "null":
            refcount = None
        elif refcount:
            refcount = int(refcount)
        else:
            refcount = None
        #
        # Get the entry, creating it if needed:
        #
        try:
            entry = d[function]
        except KeyError:
            entry = d[function] = Entry(function)
        #
        # Update the entry with the new parameter or the result information.
        #
        if arg:
            entry.args.append((arg, type, refcount))
        else:
            entry.result_type = type
            entry.result_refs = refcount
    return d


class Entry:
    def __init__(self, name):
        self.name = name
        self.args = []
        self.result_type = ''
        self.result_refs = None


def dump(d):
    """Dump the data in the 'canonical' format, with functions in
    sorted order."""
    items = d.items()
    items.sort()
    first = 1
    for k, entry in items:
        if first:
            first = 0
        else:
            print
        s = entry.name + ":%s:%s:%s:"
        if entry.result_refs is None:
            r = ""
        else:
            r = entry.result_refs
        print s % (entry.result_type, "", r)
        for t, n, r in entry.args:
            if r is None:
                r = ""
            print s % (t, n, r)


def main():
    d = load()
    dump(d)


if __name__ == "__main__":
    main()
