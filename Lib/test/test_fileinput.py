'''
Tests for fileinput module.
Nick Mathewson
'''

from test.test_support import verify, verbose, TESTFN
import sys, os, re
from StringIO import StringIO
from fileinput import FileInput

# The fileinput module has 2 interfaces: the FileInput class which does
# all the work, and a few functions (input, etc.) that use a global _state
# variable.  We only test the FileInput class, since the other functions
# only provide a thin facade over FileInput.

# Write lines (a list of lines) to temp file number i, and return the
# temp file's name.
def writeTmp(i, lines):
    name = TESTFN + str(i)
    f = open(name, 'w')
    f.writelines(lines)
    f.close()
    return name

pat = re.compile(r'LINE (\d+) OF FILE (\d+)')

def remove_tempfiles(*names):
    for name in names:
        try:
            os.unlink(name)
        except:
            pass

def runTests(t1, t2, t3, t4, bs=0, round=0):
    start = 1 + round*6
    if verbose:
        print '%s. Simple iteration (bs=%s)' % (start+0, bs)
    fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
    lines = list(fi)
    fi.close()
    verify(len(lines) == 31)
    verify(lines[4] == 'Line 5 of file 1\n')
    verify(lines[30] == 'Line 1 of file 4\n')
    verify(fi.lineno() == 31)
    verify(fi.filename() == t4)

    if verbose:
        print '%s. Status variables (bs=%s)' % (start+1, bs)
    fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
    s = "x"
    while s and s != 'Line 6 of file 2\n':
        s = fi.readline()
    verify(fi.filename() == t2)
    verify(fi.lineno() == 21)
    verify(fi.filelineno() == 6)
    verify(not fi.isfirstline())
    verify(not fi.isstdin())

    if verbose:
        print '%s. Nextfile (bs=%s)' % (start+2, bs)
    fi.nextfile()
    verify(fi.readline() == 'Line 1 of file 3\n')
    verify(fi.lineno() == 22)
    fi.close()

    if verbose:
        print '%s. Stdin (bs=%s)' % (start+3, bs)
    fi = FileInput(files=(t1, t2, t3, t4, '-'), bufsize=bs)
    savestdin = sys.stdin
    try:
        sys.stdin = StringIO("Line 1 of stdin\nLine 2 of stdin\n")
        lines = list(fi)
        verify(len(lines) == 33)
        verify(lines[32] == 'Line 2 of stdin\n')
        verify(fi.filename() == '<stdin>')
        fi.nextfile()
    finally:
        sys.stdin = savestdin

    if verbose:
        print '%s. Boundary conditions (bs=%s)' % (start+4, bs)
    fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
    verify(fi.lineno() == 0)
    verify(fi.filename() == None)
    fi.nextfile()
    verify(fi.lineno() == 0)
    verify(fi.filename() == None)

    if verbose:
        print '%s. Inplace (bs=%s)' % (start+5, bs)
    savestdout = sys.stdout
    try:
        fi = FileInput(files=(t1, t2, t3, t4), inplace=1, bufsize=bs)
        for line in fi:
            line = line[:-1].upper()
            print line
        fi.close()
    finally:
        sys.stdout = savestdout

    fi = FileInput(files=(t1, t2, t3, t4), bufsize=bs)
    for line in fi:
        verify(line[-1] == '\n')
        m = pat.match(line[:-1])
        verify(m != None)
        verify(int(m.group(1)) == fi.filelineno())
    fi.close()


def writeFiles():
    global t1, t2, t3, t4
    t1 = writeTmp(1, ["Line %s of file 1\n" % (i+1) for i in range(15)])
    t2 = writeTmp(2, ["Line %s of file 2\n" % (i+1) for i in range(10)])
    t3 = writeTmp(3, ["Line %s of file 3\n" % (i+1) for i in range(5)])
    t4 = writeTmp(4, ["Line %s of file 4\n" % (i+1) for i in range(1)])

# First, run the tests with default and teeny buffer size.
for round, bs in (0, 0), (1, 30):
    try:
        writeFiles()
        runTests(t1, t2, t3, t4, bs, round)
    finally:
        remove_tempfiles(t1, t2, t3, t4)

# Next, check for proper behavior with 0-byte files.
if verbose:
    print "13. 0-byte files"
try:
    t1 = writeTmp(1, [""])
    t2 = writeTmp(2, [""])
    t3 = writeTmp(3, ["The only line there is.\n"])
    t4 = writeTmp(4, [""])
    fi = FileInput(files=(t1, t2, t3, t4))
    line = fi.readline()
    verify(line == 'The only line there is.\n')
    verify(fi.lineno() == 1)
    verify(fi.filelineno() == 1)
    verify(fi.filename() == t3)
    line = fi.readline()
    verify(not line)
    verify(fi.lineno() == 1)
    verify(fi.filelineno() == 0)
    verify(fi.filename() == t4)
    fi.close()
finally:
    remove_tempfiles(t1, t2, t3, t4)

if verbose:
    print "14. Files that don't end with newline"
try:
    t1 = writeTmp(1, ["A\nB\nC"])
    t2 = writeTmp(2, ["D\nE\nF"])
    fi = FileInput(files=(t1, t2))
    lines = list(fi)
    verify(lines == ["A\n", "B\n", "C", "D\n", "E\n", "F"])
    verify(fi.filelineno() == 3)
    verify(fi.lineno() == 6)
finally:
    remove_tempfiles(t1, t2)
