#! /usr/bin/env python

"""fixdiv - tool to fix division operators.

To use this tool, first run `python -Dwarn yourscript.py 2>warnings'.
This runs the script `yourscript.py' while writing warning messages
about all uses of the classic division operator to the file
`warnings'.  The warnings look like this:

  <file>:<line>: DeprecationWarning: classic <type> division

The warnings are written to stderr, so you must use `2>' for the I/O
redirect.  I know of no way to redirect stderr on Windows in a DOS
box, so you will have to modify the script to set sys.stderr to some
kind of log file if you want to do this on Windows.

The warnings are not limited to the script; modules imported by the
script may also trigger warnings.  In fact a useful technique is to
write a test script specifically intended to exercise all code in a
particular module or set of modules.

Then run `python fixdiv.py warnings'.  This first reads the warnings,
looking for classic division warnings, and sorts them by file name and
line number.  Then, for each file that received at least one warning,
it parses the file and tries to match the warnings up to the division
operators found in the source code.  If it is successful, it writes a
recommendation to stdout in the form of a context diff.  If it is not
successful, it writes observations to stdout instead.

There are several possible recommendations and observations:

- A / operator was found that can remain unchanged.  This is the
  recommendation when only float and/or complex arguments were seen.

- A / operator was found that should be changed to //.  This is the
  recommendation when only int and/or long arguments were seen.

- A / operator was found for which int or long as well as float or
  complex arguments were seen.  This is highly unlikely; if it occurs,
  you may have to restructure the code to keep the classic semantics,
  or maybe you don't care about the classic semantics.

- A / operator was found for which no warnings were seen.  This could
  be code that was never executed, or code that was only executed with
  with user-defined objects as arguments.  You will have to
  investigate further.  Note that // can be overloaded separately from
  /, using __floordiv__.  True division can also be separately
  overloaded, using __truediv__.  Classic division should be the same
  as either of those.  (XXX should I add a warning for division on
  user-defined objects, to disambiguate this case from code that was
  never executed?)

- A warning was seen for a line not containing a / operator.  This is
  an anomaly that shouldn't happen; the most likely cause is a change
  to the file between the time the test script was run to collect
  warnings and the time fixdiv was run.

- More than one / operator was found on one line, or in a statement
  split across multiple lines.  Because the warnings framework doesn't
  (and can't) show the offset within the line, and the code generator
  doesn't always give the correct line number for operations in a
  multi-line statement, it's not clear whether both were executed.  In
  practice, they usually are, so the default action is make the same
  recommendation for all / operators, based on the above criteria.
  The -m option issues warnings for these cases instead.

Notes:

- The augmented assignment operator /= is handled the same way as the
  / operator.

- This tool never looks at the // operator; no warnings are ever
  generated for use of this operator.

- This tool never looks at the / operator when a future division
  statement is in effect; no warnings are generated in this case, and
  because the tool only looks at files for which at least one classic
  division warning was seen, it will never look at files containing a
  future division statement.

- Warnings may be issued for code not read from a file, but executed
  using an exec statement or the eval() function.  These will have
  <string> in the filename position.  The fixdiv script will attempt
  and fail to open a file named "<string>", and issue a warning about
  this failure.  You're on your own to deal with this.  You could make
  all recommended changes and add a future division statement to all
  affected files, and then re-run the test script; it should not issue
  any warnings.  If there are any, and you have a hard time tracking
  down where they are generated, you can use the -Werror option to
  force an error instead of a first warning, generating a traceback.

- The tool should be run from the same directory as that from which
  the original script was run, otherwise it won't be able to open
  files given by relative pathnames.
"""

import sys
import getopt
import re
import tokenize
from pprint import pprint

multi_ok = 1

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hm")
    except getopt.error, msg:
        usage(msg)
        return 2
    for o, a in opts:
        if o == "-h":
            print __doc__
            return
        if o == "-m":
            global multi_ok
            multi_ok = 0
    if not args:
        usage("at least one file argument is required")
        return 2
    if args[1:]:
        sys.stderr.write("%s: extra file arguments ignored\n", sys.argv[0])
    warnings = readwarnings(args[0])
    if warnings is None:
        return 1
    files = warnings.keys()
    if not files:
        print "No classic division warnings read from", args[0]
        return
    files.sort()
    exit = None
    for file in files:
        x = process(file, warnings[file])
        exit = exit or x
    return exit

def usage(msg):
    sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.stderr.write("Usage: %s [-m] warnings\n" % sys.argv[0])
    sys.stderr.write("Try `%s -h' for more information.\n" % sys.argv[0])

PATTERN = ("^(.+?):(\d+): DeprecationWarning: "
           "classic (int|long|float|complex) division$")

def readwarnings(warningsfile):
    prog = re.compile(PATTERN)
    try:
        f = open(warningsfile)
    except IOError, msg:
        sys.stderr.write("can't open: %s\n" % msg)
        return
    warnings = {}
    while 1:
        line = f.readline()
        if not line:
            break
        m = prog.match(line)
        if not m:
            if line.find("division") >= 0:
                sys.stderr.write("Warning: ignored input " + line)
            continue
        file, lineno, what = m.groups()
        list = warnings.get(file)
        if list is None:
            warnings[file] = list = []
        list.append((int(lineno), intern(what)))
    f.close()
    return warnings

def process(file, list):
    print "-"*70
    assert list # if this fails, readwarnings() is broken
    try:
        fp = open(file)
    except IOError, msg:
        sys.stderr.write("can't open: %s\n" % msg)
        return 1
    print "Index:", file
    f = FileContext(fp)
    list.sort()
    index = 0 # list[:index] has been processed, list[index:] is still to do
    g = tokenize.generate_tokens(f.readline)
    while 1:
        startlineno, endlineno, slashes = lineinfo = scanline(g)
        if startlineno is None:
            break
        assert startlineno <= endlineno is not None
        orphans = []
        while index < len(list) and list[index][0] < startlineno:
            orphans.append(list[index])
            index += 1
        if orphans:
            reportphantomwarnings(orphans, f)
        warnings = []
        while index < len(list) and list[index][0] <= endlineno:
            warnings.append(list[index])
            index += 1
        if not slashes and not warnings:
            pass
        elif slashes and not warnings:
            report(slashes, "No conclusive evidence")
        elif warnings and not slashes:
            reportphantomwarnings(warnings, f)
        else:
            if len(slashes) > 1:
                if not multi_ok:
                    report(slashes, "More than one / operator per statement")
                    continue
            intlong = []
            floatcomplex = []
            bad = []
            for lineno, what in warnings:
                if what in ("int", "long"):
                    intlong.append(what)
                elif what in ("float", "complex"):
                    floatcomplex.append(what)
                else:
                    bad.append(what)
            lastrow = None
            for (row, col), line in slashes:
                if row == lastrow:
                    continue
                lastrow = row
                line = chop(line)
                if line[col:col+1] != "/":
                    print "*** Can't find the / operator in line %d:" % row
                    print "*", line
                    continue
                if bad:
                    print "*** Bad warning for line %d:" % row, bad
                    print "*", line
                elif intlong and not floatcomplex:
                        print "%dc%d" % (row, row)
                        print "<", line
                        print "---"
                        print ">", line[:col] + "/" + line[col:]
                elif floatcomplex and not intlong:
                    print "True division / operator at line %d:" % row
                    print "=", line
    fp.close()

def reportphantomwarnings(warnings, f):
    blocks = []
    lastrow = None
    lastblock = None
    for row, what in warnings:
        if row != lastrow:
            lastblock = [row]
            blocks.append(lastblock)
        lastblock.append(what)
    for block in blocks:
        row = block[0]
        whats = "/".join(block[1:])
        print "*** Phantom %s warnings for line %d:" % (whats, row)
        f.report(row, mark="*")

def report(slashes, message):
    lastrow = None
    for (row, col), line in slashes:
        if row != lastrow:
            print "*** %s on line %d:" % (message, row)
            print "*", chop(line)
            lastrow = row

class FileContext:
    def __init__(self, fp, window=5, lineno=1):
        self.fp = fp
        self.window = 5
        self.lineno = 1
        self.eoflookahead = 0
        self.lookahead = []
        self.buffer = []
    def fill(self):
        while len(self.lookahead) < self.window and not self.eoflookahead:
            line = self.fp.readline()
            if not line:
                self.eoflookahead = 1
                break
            self.lookahead.append(line)
    def readline(self):
        self.fill()
        if not self.lookahead:
            return ""
        line = self.lookahead.pop(0)
        self.buffer.append(line)
        self.lineno += 1
        return line
    def truncate(self):
        del self.buffer[-window:]
    def __getitem__(self, index):
        self.fill()
        bufstart = self.lineno - len(self.buffer)
        lookend = self.lineno + len(self.lookahead)
        if bufstart <= index < self.lineno:
            return self.buffer[index - bufstart]
        if self.lineno <= index < lookend:
            return self.lookahead[index - self.lineno]
        raise KeyError
    def report(self, first, last=None, mark="*"):
        if last is None:
            last = first
        for i in range(first, last+1):
            try:
                line = self[first]
            except KeyError:
                line = "<missing line>"
            print mark, chop(line)

def scanline(g):
    slashes = []
    startlineno = None
    endlineno = None
    for type, token, start, end, line in g:
        endlineno = end[0]
        if startlineno is None:
            startlineno = endlineno
        if token in ("/", "/="):
            slashes.append((start, line))
        if type == tokenize.NEWLINE:
            break
    return startlineno, endlineno, slashes

def chop(line):
    if line.endswith("\n"):
        return line[:-1]
    else:
        return line

if __name__ == "__main__":
    sys.exit(main())
