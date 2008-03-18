#! /usr/bin/env python

# Released to the public domain, by Tim Peters, 03 October 2000.

"""reindent [-d][-r][-v] [ path ... ]

-d (--dryrun)   Dry run.   Analyze, but don't make any changes to, files.
-r (--recurse)  Recurse.   Search for all .py files in subdirectories too.
-n (--nobackup) No backup. Does not make a ".bak" file before reindenting.
-v (--verbose)  Verbose.   Print informative msgs; else no output.
-h (--help)     Help.      Print this usage information and exit.

Change Python (.py) files to use 4-space indents and no hard tab characters.
Also trim excess spaces and tabs from ends of lines, and remove empty lines
at the end of files.  Also ensure the last line ends with a newline.

If no paths are given on the command line, reindent operates as a filter,
reading a single source file from standard input and writing the transformed
source to standard output.  In this case, the -d, -r and -v flags are
ignored.

You can pass one or more file and/or directory paths.  When a directory
path, all .py files within the directory will be examined, and, if the -r
option is given, likewise recursively for subdirectories.

If output is not to standard output, reindent overwrites files in place,
renaming the originals with a .bak extension.  If it finds nothing to
change, the file is left alone.  If reindent does change a file, the changed
file is a fixed-point for future runs (i.e., running reindent on the
resulting .py file won't change it again).

The hard part of reindenting is figuring out what to do with comment
lines.  So long as the input files get a clean bill of health from
tabnanny.py, reindent should do a good job.

The backup file is a copy of the one that is being reindented. The ".bak"
file is generated with shutil.copy(), but some corner cases regarding
user/group and permissions could leave the backup file more readable that
you'd prefer. You can always use the --nobackup option to prevent this.
"""

__version__ = "1"

import tokenize
import os, shutil
import sys

verbose    = 0
recurse    = 0
dryrun     = 0
makebackup = True

def usage(msg=None):
    if msg is not None:
        print(msg, file=sys.stderr)
    print(__doc__, file=sys.stderr)

def errprint(*args):
    sep = ""
    for arg in args:
        sys.stderr.write(sep + str(arg))
        sep = " "
    sys.stderr.write("\n")

def main():
    import getopt
    global verbose, recurse, dryrun, makebackup
    try:
        opts, args = getopt.getopt(sys.argv[1:], "drnvh",
                        ["dryrun", "recurse", "nobackup", "verbose", "help"])
    except getopt.error as msg:
        usage(msg)
        return
    for o, a in opts:
        if o in ('-d', '--dryrun'):
            dryrun += 1
        elif o in ('-r', '--recurse'):
            recurse += 1
        elif o in ('-n', '--nobackup'):
            makebackup = False
        elif o in ('-v', '--verbose'):
            verbose += 1
        elif o in ('-h', '--help'):
            usage()
            return
    if not args:
        r = Reindenter(sys.stdin)
        r.run()
        r.write(sys.stdout)
        return
    for arg in args:
        check(arg)

def check(file):
    if os.path.isdir(file) and not os.path.islink(file):
        if verbose:
            print("listing directory", file)
        names = os.listdir(file)
        for name in names:
            fullname = os.path.join(file, name)
            if ((recurse and os.path.isdir(fullname) and
                 not os.path.islink(fullname))
                or name.lower().endswith(".py")):
                check(fullname)
        return

    if verbose:
        print("checking", file, "...", end=' ')
    try:
        f = open(file)
    except IOError as msg:
        errprint("%s: I/O Error: %s" % (file, str(msg)))
        return

    r = Reindenter(f)
    f.close()
    if r.run():
        if verbose:
            print("changed.")
            if dryrun:
                print("But this is a dry run, so leaving it alone.")
        if not dryrun:
            bak = file + ".bak"
            if makebackup:
                shutil.copyfile(file, bak)
                if verbose:
                    print("backed up", file, "to", bak)
            f = open(file, "w")
            r.write(f)
            f.close()
            if verbose:
                print("wrote new", file)
        return True
    else:
        if verbose:
            print("unchanged.")
        return False

def _rstrip(line, JUNK='\n \t'):
    """Return line stripped of trailing spaces, tabs, newlines.

    Note that line.rstrip() instead also strips sundry control characters,
    but at least one known Emacs user expects to keep junk like that, not
    mentioning Barry by name or anything <wink>.
    """

    i = len(line)
    while i > 0 and line[i-1] in JUNK:
        i -= 1
    return line[:i]

class Reindenter:

    def __init__(self, f):
        self.find_stmt = 1  # next token begins a fresh stmt?
        self.level = 0      # current indent level

        # Raw file lines.
        self.raw = f.readlines()

        # File lines, rstripped & tab-expanded.  Dummy at start is so
        # that we can use tokenize's 1-based line numbering easily.
        # Note that a line is all-blank iff it's "\n".
        self.lines = [_rstrip(line).expandtabs() + "\n"
                      for line in self.raw]
        self.lines.insert(0, None)
        self.index = 1  # index into self.lines of next line

        # List of (lineno, indentlevel) pairs, one for each stmt and
        # comment line.  indentlevel is -1 for comment lines, as a
        # signal that tokenize doesn't know what to do about them;
        # indeed, they're our headache!
        self.stats = []

    def run(self):
        tokenize.tokenize(self.getline, self.tokeneater)
        # Remove trailing empty lines.
        lines = self.lines
        while lines and lines[-1] == "\n":
            lines.pop()
        # Sentinel.
        stats = self.stats
        stats.append((len(lines), 0))
        # Map count of leading spaces to # we want.
        have2want = {}
        # Program after transformation.
        after = self.after = []
        # Copy over initial empty lines -- there's nothing to do until
        # we see a line with *something* on it.
        i = stats[0][0]
        after.extend(lines[1:i])
        for i in range(len(stats)-1):
            thisstmt, thislevel = stats[i]
            nextstmt = stats[i+1][0]
            have = getlspace(lines[thisstmt])
            want = thislevel * 4
            if want < 0:
                # A comment line.
                if have:
                    # An indented comment line.  If we saw the same
                    # indentation before, reuse what it most recently
                    # mapped to.
                    want = have2want.get(have, -1)
                    if want < 0:
                        # Then it probably belongs to the next real stmt.
                        for j in range(i+1, len(stats)-1):
                            jline, jlevel = stats[j]
                            if jlevel >= 0:
                                if have == getlspace(lines[jline]):
                                    want = jlevel * 4
                                break
                    if want < 0:           # Maybe it's a hanging
                                           # comment like this one,
                        # in which case we should shift it like its base
                        # line got shifted.
                        for j in range(i-1, -1, -1):
                            jline, jlevel = stats[j]
                            if jlevel >= 0:
                                want = have + getlspace(after[jline-1]) - \
                                       getlspace(lines[jline])
                                break
                    if want < 0:
                        # Still no luck -- leave it alone.
                        want = have
                else:
                    want = 0
            assert want >= 0
            have2want[have] = want
            diff = want - have
            if diff == 0 or have == 0:
                after.extend(lines[thisstmt:nextstmt])
            else:
                for line in lines[thisstmt:nextstmt]:
                    if diff > 0:
                        if line == "\n":
                            after.append(line)
                        else:
                            after.append(" " * diff + line)
                    else:
                        remove = min(getlspace(line), -diff)
                        after.append(line[remove:])
        return self.raw != self.after

    def write(self, f):
        f.writelines(self.after)

    # Line-getter for tokenize.
    def getline(self):
        if self.index >= len(self.lines):
            line = ""
        else:
            line = self.lines[self.index]
            self.index += 1
        return line

    # Line-eater for tokenize.
    def tokeneater(self, type, token, slinecol, end, line,
                   INDENT=tokenize.INDENT,
                   DEDENT=tokenize.DEDENT,
                   NEWLINE=tokenize.NEWLINE,
                   COMMENT=tokenize.COMMENT,
                   NL=tokenize.NL):

        if type == NEWLINE:
            # A program statement, or ENDMARKER, will eventually follow,
            # after some (possibly empty) run of tokens of the form
            #     (NL | COMMENT)* (INDENT | DEDENT+)?
            self.find_stmt = 1

        elif type == INDENT:
            self.find_stmt = 1
            self.level += 1

        elif type == DEDENT:
            self.find_stmt = 1
            self.level -= 1

        elif type == COMMENT:
            if self.find_stmt:
                self.stats.append((slinecol[0], -1))
                # but we're still looking for a new stmt, so leave
                # find_stmt alone

        elif type == NL:
            pass

        elif self.find_stmt:
            # This is the first "real token" following a NEWLINE, so it
            # must be the first token of the next program statement, or an
            # ENDMARKER.
            self.find_stmt = 0
            if line:   # not endmarker
                self.stats.append((slinecol[0], self.level))

# Count number of leading blanks.
def getlspace(line):
    i, n = 0, len(line)
    while i < n and line[i] == " ":
        i += 1
    return i

if __name__ == '__main__':
    main()
