#! /usr/bin/env python

"""cleanfuture [-d][-r][-v] path ...

-d  Dry run.  Analyze, but don't make any changes to, files.
-r  Recurse.  Search for all .py files in subdirectories too.
-v  Verbose.  Print informative msgs.

Search Python (.py) files for future statements, and remove the features
from such statements that are already mandatory in the version of Python
you're using.

Pass one or more file and/or directory paths.  When a directory path, all
.py files within the directory will be examined, and, if the -r option is
given, likewise recursively for subdirectories.

Overwrites files in place, renaming the originals with a .bak extension. If
cleanfuture finds nothing to change, the file is left alone.  If cleanfuture
does change a file, the changed file is a fixed-point (i.e., running
cleanfuture on the resulting .py file won't change it again, at least not
until you try it again with a later Python release).

Limitations:  You can do these things, but this tool won't help you then:

+ A future statement cannot be mixed with any other statement on the same
  physical line (separated by semicolon).

+ A future statement cannot contain an "as" clause.

Example:  Assuming you're using Python 2.2, if a file containing

from __future__ import nested_scopes, generators

is analyzed by cleanfuture, the line is rewritten to

from __future__ import generators

because nested_scopes is no longer optional in 2.2 but generators is.
"""

import __future__
import tokenize
import os
import sys

dryrun  = 0
recurse = 0
verbose = 0

def errprint(*args):
    strings = map(str, args)
    msg = ' '.join(strings)
    if msg[-1:] != '\n':
        msg += '\n'
    sys.stderr.write(msg)

def main():
    import getopt
    global verbose, recurse, dryrun
    try:
        opts, args = getopt.getopt(sys.argv[1:], "drv")
    except getopt.error, msg:
        errprint(msg)
        return
    for o, a in opts:
        if o == '-d':
            dryrun += 1
        elif o == '-r':
            recurse += 1
        elif o == '-v':
            verbose += 1
    if not args:
        errprint("Usage:", __doc__)
        return
    for arg in args:
        check(arg)

def check(file):
    if os.path.isdir(file) and not os.path.islink(file):
        if verbose:
            print "listing directory", file
        names = os.listdir(file)
        for name in names:
            fullname = os.path.join(file, name)
            if ((recurse and os.path.isdir(fullname) and
                 not os.path.islink(fullname))
                or name.lower().endswith(".py")):
                check(fullname)
        return

    if verbose:
        print "checking", file, "...",
    try:
        f = open(file)
    except IOError, msg:
        errprint("%r: I/O Error: %s" % (file, str(msg)))
        return

    ff = FutureFinder(f, file)
    changed = ff.run()
    if changed:
        ff.gettherest()
    f.close()
    if changed:
        if verbose:
            print "changed."
            if dryrun:
                print "But this is a dry run, so leaving it alone."
        for s, e, line in changed:
            print "%r lines %d-%d" % (file, s+1, e+1)
            for i in range(s, e+1):
                print ff.lines[i],
            if line is None:
                print "-- deleted"
            else:
                print "-- change to:"
                print line,
        if not dryrun:
            bak = file + ".bak"
            if os.path.exists(bak):
                os.remove(bak)
            os.rename(file, bak)
            if verbose:
                print "renamed", file, "to", bak
            g = open(file, "w")
            ff.write(g)
            g.close()
            if verbose:
                print "wrote new", file
    else:
        if verbose:
            print "unchanged."

class FutureFinder:

    def __init__(self, f, fname):
        self.f = f
        self.fname = fname
        self.ateof = 0
        self.lines = [] # raw file lines

        # List of (start_index, end_index, new_line) triples.
        self.changed = []

    # Line-getter for tokenize.
    def getline(self):
        if self.ateof:
            return ""
        line = self.f.readline()
        if line == "":
            self.ateof = 1
        else:
            self.lines.append(line)
        return line

    def run(self):
        STRING = tokenize.STRING
        NL = tokenize.NL
        NEWLINE = tokenize.NEWLINE
        COMMENT = tokenize.COMMENT
        NAME = tokenize.NAME
        OP = tokenize.OP

        changed = self.changed
        get = tokenize.generate_tokens(self.getline).next
        type, token, (srow, scol), (erow, ecol), line = get()

        # Chew up initial comments and blank lines (if any).
        while type in (COMMENT, NL, NEWLINE):
            type, token, (srow, scol), (erow, ecol), line = get()

        # Chew up docstring (if any -- and it may be implicitly catenated!).
        while type is STRING:
            type, token, (srow, scol), (erow, ecol), line = get()

        # Analyze the future stmts.
        while 1:
            # Chew up comments and blank lines (if any).
            while type in (COMMENT, NL, NEWLINE):
                type, token, (srow, scol), (erow, ecol), line = get()

            if not (type is NAME and token == "from"):
                break
            startline = srow - 1    # tokenize is one-based
            type, token, (srow, scol), (erow, ecol), line = get()

            if not (type is NAME and token == "__future__"):
                break
            type, token, (srow, scol), (erow, ecol), line = get()

            if not (type is NAME and token == "import"):
                break
            type, token, (srow, scol), (erow, ecol), line = get()

            # Get the list of features.
            features = []
            while type is NAME:
                features.append(token)
                type, token, (srow, scol), (erow, ecol), line = get()

                if not (type is OP and token == ','):
                    break
                type, token, (srow, scol), (erow, ecol), line = get()

            # A trailing comment?
            comment = None
            if type is COMMENT:
                comment = token
                type, token, (srow, scol), (erow, ecol), line = get()

            if type is not NEWLINE:
                errprint("Skipping file %r; can't parse line %d:\n%s" %
                         (self.fname, srow, line))
                return []

            endline = srow - 1

            # Check for obsolete features.
            okfeatures = []
            for f in features:
                object = getattr(__future__, f, None)
                if object is None:
                    # A feature we don't know about yet -- leave it in.
                    # They'll get a compile-time error when they compile
                    # this program, but that's not our job to sort out.
                    okfeatures.append(f)
                else:
                    released = object.getMandatoryRelease()
                    if released is None or released <= sys.version_info:
                        # Withdrawn or obsolete.
                        pass
                    else:
                        okfeatures.append(f)

            # Rewrite the line if at least one future-feature is obsolete.
            if len(okfeatures) < len(features):
                if len(okfeatures) == 0:
                    line = None
                else:
                    line = "from __future__ import "
                    line += ', '.join(okfeatures)
                    if comment is not None:
                        line += ' ' + comment
                    line += '\n'
                changed.append((startline, endline, line))

            # Loop back for more future statements.

        return changed

    def gettherest(self):
        if self.ateof:
            self.therest = ''
        else:
            self.therest = self.f.read()

    def write(self, f):
        changed = self.changed
        assert changed
        # Prevent calling this again.
        self.changed = []
        # Apply changes in reverse order.
        changed.reverse()
        for s, e, line in changed:
            if line is None:
                # pure deletion
                del self.lines[s:e+1]
            else:
                self.lines[s:e+1] = [line]
        f.writelines(self.lines)
        # Copy over the remainder of the file.
        if self.therest:
            f.write(self.therest)

if __name__ == '__main__':
    main()
