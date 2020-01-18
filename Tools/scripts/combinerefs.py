#! /usr/bin/env python3

"""
combinerefs path

A helper for analyzing PYTHONDUMPREFS output.

When the PYTHONDUMPREFS envar is set in a debug build, at Python shutdown
time Py_FinalizeEx() prints the list of all live objects twice:  first it
prints the repr() of each object while the interpreter is still fully intact.
After cleaning up everything it can, it prints all remaining live objects
again, but the second time just prints their addresses, refcounts, and type
names (because the interpreter has been torn down, calling repr methods at
this point can get into infinite loops or blow up).

Save all this output into a file, then run this script passing the path to
that file.  The script finds both output chunks, combines them, then prints
a line of output for each object still alive at the end:

    address refcnt typename repr

address is the address of the object, in whatever format the platform C
produces for a %p format code.

refcnt is of the form

    "[" ref "]"

when the object's refcount is the same in both PYTHONDUMPREFS output blocks,
or

    "[" ref_before "->" ref_after "]"

if the refcount changed.

typename is object->ob_type->tp_name, extracted from the second PYTHONDUMPREFS
output block.

repr is repr(object), extracted from the first PYTHONDUMPREFS output block.
CAUTION:  If object is a container type, it may not actually contain all the
objects shown in the repr:  the repr was captured from the first output block,
and some of the containees may have been released since then.  For example,
it's common for the line showing the dict of interned strings to display
strings that no longer exist at the end of Py_FinalizeEx; this can be recognized
(albeit painfully) because such containees don't have a line of their own.

The objects are listed in allocation order, with most-recently allocated
printed first, and the first object allocated printed last.


Simple examples:

    00857060 [14] str '__len__'

The str object '__len__' is alive at shutdown time, and both PYTHONDUMPREFS
output blocks said there were 14 references to it.  This is probably due to
C modules that intern the string "__len__" and keep a reference to it in a
file static.

    00857038 [46->5] tuple ()

46-5 = 41 references to the empty tuple were removed by the cleanup actions
between the times PYTHONDUMPREFS produced output.

    00858028 [1025->1456] str '<dummy key>'

The string '<dummy key>', which is used in dictobject.c to overwrite a real
key that gets deleted, grew several hundred references during cleanup.  It
suggests that stuff did get removed from dicts by cleanup, but that the dicts
themselves are staying alive for some reason. """

import re
import sys

# Generate lines from fileiter.  If whilematch is true, continue reading
# while the regexp object pat matches line.  If whilematch is false, lines
# are read so long as pat doesn't match them.  In any case, the first line
# that doesn't match pat (when whilematch is true), or that does match pat
# (when whilematch is false), is lost, and fileiter will resume at the line
# following it.
def read(fileiter, pat, whilematch):
    for line in fileiter:
        if bool(pat.match(line)) == whilematch:
            yield line
        else:
            break

def combinefile(f):
    fi = iter(f)

    for line in read(fi, re.compile(r'^Remaining objects:$'), False):
        pass

    crack = re.compile(r'([a-zA-Z\d]+) \[(\d+)\] (.*)')
    addr2rc = {}
    addr2guts = {}
    before = 0
    for line in read(fi, re.compile(r'^Remaining object addresses:$'), False):
        m = crack.match(line)
        if m:
            addr, addr2rc[addr], addr2guts[addr] = m.groups()
            before += 1
        else:
            print('??? skipped:', line)

    after = 0
    for line in read(fi, crack, True):
        after += 1
        m = crack.match(line)
        assert m
        addr, rc, guts = m.groups() # guts is type name here
        if addr not in addr2rc:
            print('??? new object created while tearing down:', line.rstrip())
            continue
        print(addr, end=' ')
        if rc == addr2rc[addr]:
            print('[%s]' % rc, end=' ')
        else:
            print('[%s->%s]' % (addr2rc[addr], rc), end=' ')
        print(guts, addr2guts[addr])

    print("%d objects before, %d after" % (before, after))

def combine(fname):
    with open(fname) as f:
        combinefile(f)

if __name__ == '__main__':
    combine(sys.argv[1])
