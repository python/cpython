#! /usr/bin/env python

"""fixdiv - tool to fix division operators.

To use this tool, first run `python -Dwarn yourscript.py 2>warnings'.
This runs the script `yourscript.py' while writing warning messages
about all uses of the classic division operator to the file
`warnings'.  (The warnings are written to stderr, so you must use `2>'
for the I/O redirect.  I don't yet know how to do this on Windows.)

Then run `python fixdiv.py warnings'.  This first reads the warnings,
looking for classic division warnings, and sorts them by file name and
line number.  Then, for each file that received at least one warning,
it parses the file and tries to match the warnings up to the division
operators found in the source code.  If it is successful, it writes a
recommendation to stdout in the form of a context diff.  If it is not
successful, it writes recommendations to stdout instead.
"""

import sys
import getopt
import re
import tokenize
from pprint import pprint

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h")
    except getopt.error, msg:
        usage(2, msg)
    for o, a in opts:
        if o == "-h":
            help()
    if not args:
        usage(2, "at least one file argument is required")
    if args[1:]:
        sys.stderr.write("%s: extra file arguments ignored\n", sys.argv[0])
    readwarnings(args[0])

def usage(exit, msg=None):
    if msg:
        sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.stderr.write("Usage: %s warnings\n" % sys.argv[0])
    sys.stderr.write("Try `%s -h' for more information.\n" % sys.argv[0])
    sys.exit(exit)

def help():
    print __doc__
    sys.exit(0)

def readwarnings(warningsfile):
    pat = re.compile(
        "^(.+?):(\d+): DeprecationWarning: classic ([a-z]+) division$")
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
        m = pat.match(line)
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
    files = warnings.keys()
    files.sort()
    for file in files:
        process(file, warnings[file])

def process(file, list):
    print "-"*70
    if not list:
        sys.stderr.write("no division warnings for %s\n" % file)
        return
    try:
        fp = open(file)
    except IOError, msg:
        sys.stderr.write("can't open: %s\n" % msg)
        return
    print "Processing:", file
    f = FileContext(fp)
    list.sort()
    index = 0 # list[:index] has been processed, list[index:] is still to do
    orphans = [] # subset of list for which no / operator was found
    unknown = [] # lines with / operators for which no warnings were seen
    g = tokenize.generate_tokens(f.readline)
    while 1:
        startlineno, endlineno, slashes = lineinfo = scanline(g)
        if startlineno is None:
            break
        assert startlineno <= endlineno is not None
        while index < len(list) and list[index][0] < startlineno:
            orphans.append(list[index])
            index += 1
        warnings = []
        while index < len(list) and list[index][0] <= endlineno:
            warnings.append(list[index])
            index += 1
        if not slashes and not warnings:
            pass
        elif slashes and not warnings:
            report(slashes, "Unexecuted code")
        elif warnings and not slashes:
            reportphantomwarnings(warnings, f)
        else:
            if len(slashes) > 1:
                report(slashes, "More than one / operator")
            else:
                (row, col), line = slashes[0]
                line = chop(line)
                if line[col:col+1] != "/":
                    print "*** Can't find the / operator in line %d:" % row
                    print "*", line
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
        ## if type in (tokenize.NEWLINE, tokenize.NL, tokenize.COMMENT):
        if type == tokenize.NEWLINE:
            break
    return startlineno, endlineno, slashes

def chop(line):
    if line.endswith("\n"):
        return line[:-1]
    else:
        return line

if __name__ == "__main__":
    main()
