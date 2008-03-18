#! /usr/bin/env python

# Released to the public domain, by Tim Peters, 28 February 2000.

"""checkappend.py -- search for multi-argument .append() calls.

Usage:  specify one or more file or directory paths:
    checkappend [-v] file_or_dir [file_or_dir] ...

Each file_or_dir is checked for multi-argument .append() calls.  When
a directory, all .py files in the directory, and recursively in its
subdirectories, are checked.

Use -v for status msgs.  Use -vv for more status msgs.

In the absence of -v, the only output is pairs of the form

    filename(linenumber):
    line containing the suspicious append

Note that this finds multi-argument append calls regardless of whether
they're attached to list objects.  If a module defines a class with an
append method that takes more than one argument, calls to that method
will be listed.

Note that this will not find multi-argument list.append calls made via a
bound method object.  For example, this is not caught:

    somelist = []
    push = somelist.append
    push(1, 2, 3)
"""

__version__ = 1, 0, 0

import os
import sys
import getopt
import tokenize

verbose = 0

def errprint(*args):
    msg = ' '.join(args)
    sys.stderr.write(msg)
    sys.stderr.write("\n")

def main():
    args = sys.argv[1:]
    global verbose
    try:
        opts, args = getopt.getopt(sys.argv[1:], "v")
    except getopt.error as msg:
        errprint(str(msg) + "\n\n" + __doc__)
        return
    for opt, optarg in opts:
        if opt == '-v':
            verbose = verbose + 1
    if not args:
        errprint(__doc__)
        return
    for arg in args:
        check(arg)

def check(file):
    if os.path.isdir(file) and not os.path.islink(file):
        if verbose:
            print("%r: listing directory" % (file,))
        names = os.listdir(file)
        for name in names:
            fullname = os.path.join(file, name)
            if ((os.path.isdir(fullname) and
                 not os.path.islink(fullname))
                or os.path.normcase(name[-3:]) == ".py"):
                check(fullname)
        return

    try:
        f = open(file)
    except IOError as msg:
        errprint("%r: I/O Error: %s" % (file, msg))
        return

    if verbose > 1:
        print("checking %r ..." % (file,))

    ok = AppendChecker(file, f).run()
    if verbose and ok:
        print("%r: Clean bill of health." % (file,))

[FIND_DOT,
 FIND_APPEND,
 FIND_LPAREN,
 FIND_COMMA,
 FIND_STMT]   = range(5)

class AppendChecker:
    def __init__(self, fname, file):
        self.fname = fname
        self.file = file
        self.state = FIND_DOT
        self.nerrors = 0

    def run(self):
        try:
            tokens = tokenize.generate_tokens(self.file.readline)
            for _token in tokens:
                self.tokeneater(*_token)
        except tokenize.TokenError as msg:
            errprint("%r: Token Error: %s" % (self.fname, msg))
            self.nerrors = self.nerrors + 1
        return self.nerrors == 0

    def tokeneater(self, type, token, start, end, line,
                NEWLINE=tokenize.NEWLINE,
                JUNK=(tokenize.COMMENT, tokenize.NL),
                OP=tokenize.OP,
                NAME=tokenize.NAME):

        state = self.state

        if type in JUNK:
            pass

        elif state is FIND_DOT:
            if type is OP and token == ".":
                state = FIND_APPEND

        elif state is FIND_APPEND:
            if type is NAME and token == "append":
                self.line = line
                self.lineno = start[0]
                state = FIND_LPAREN
            else:
                state = FIND_DOT

        elif state is FIND_LPAREN:
            if type is OP and token == "(":
                self.level = 1
                state = FIND_COMMA
            else:
                state = FIND_DOT

        elif state is FIND_COMMA:
            if type is OP:
                if token in ("(", "{", "["):
                    self.level = self.level + 1
                elif token in (")", "}", "]"):
                    self.level = self.level - 1
                    if self.level == 0:
                        state = FIND_DOT
                elif token == "," and self.level == 1:
                    self.nerrors = self.nerrors + 1
                    print("%s(%d):\n%s" % (self.fname, self.lineno,
                                           self.line))
                    # don't gripe about this stmt again
                    state = FIND_STMT

        elif state is FIND_STMT:
            if type is NEWLINE:
                state = FIND_DOT

        else:
            raise SystemError("unknown internal state '%r'" % (state,))

        self.state = state

if __name__ == '__main__':
    main()
