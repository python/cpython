#! /usr/bin/env python3

# Perform massive identifier substitution on C source files.
# This actually tokenizes the files (to some extent) so it can
# avoid making substitutions inside strings or comments.
# Inside strings, substitutions are never made; inside comments,
# it is a user option (off by default).
#
# The substitutions are read from one or more files whose lines,
# when not empty, after stripping comments starting with #,
# must contain exactly two words separated by whitespace: the
# old identifier and its replacement.
#
# The option -r reverses the sense of the substitutions (this may be
# useful to undo a particular substitution).
#
# If the old identifier is prefixed with a '*' (with no intervening
# whitespace), then it will not be substituted inside comments.
#
# Command line arguments are files or directories to be processed.
# Directories are searched recursively for files whose name looks
# like a C file (ends in .h or .c).  The special filename '-' means
# operate in filter mode: read stdin, write stdout.
#
# Symbolic links are always ignored (except as explicit directory
# arguments).
#
# The original files are kept as back-up with a "~" suffix.
#
# Changes made are reported to stdout in a diff-like format.
#
# NB: by changing only the function fixline() you can turn this
# into a program for different changes to C source files; by
# changing the function wanted() you can make a different selection of
# files.

import sys
import re
import os
from stat import *
import getopt

err = sys.stderr.write
dbg = err
rep = sys.stdout.write

def usage():
    progname = sys.argv[0]
    err('Usage: ' + progname +
              ' [-c] [-r] [-s file] ... file-or-directory ...\n')
    err('\n')
    err('-c           : substitute inside comments\n')
    err('-r           : reverse direction for following -s options\n')
    err('-s substfile : add a file of substitutions\n')
    err('\n')
    err('Each non-empty non-comment line in a substitution file must\n')
    err('contain exactly two words: an identifier and its replacement.\n')
    err('Comments start with a # character and end at end of line.\n')
    err('If an identifier is preceded with a *, it is not substituted\n')
    err('inside a comment even when -c is specified.\n')

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'crs:')
    except getopt.error as msg:
        err('Options error: ' + str(msg) + '\n')
        usage()
        sys.exit(2)
    bad = 0
    if not args: # No arguments
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-c':
            setdocomments()
        if opt == '-r':
            setreverse()
        if opt == '-s':
            addsubst(arg)
    for arg in args:
        if os.path.isdir(arg):
            if recursedown(arg): bad = 1
        elif os.path.islink(arg):
            err(arg + ': will not process symbolic links\n')
            bad = 1
        else:
            if fix(arg): bad = 1
    sys.exit(bad)

# Change this regular expression to select a different set of files
Wanted = '^[a-zA-Z0-9_]+\.[ch]$'
def wanted(name):
    return re.match(Wanted, name) >= 0

def recursedown(dirname):
    dbg('recursedown(%r)\n' % (dirname,))
    bad = 0
    try:
        names = os.listdir(dirname)
    except OSError as msg:
        err(dirname + ': cannot list directory: ' + str(msg) + '\n')
        return 1
    names.sort()
    subdirs = []
    for name in names:
        if name in (os.curdir, os.pardir): continue
        fullname = os.path.join(dirname, name)
        if os.path.islink(fullname): pass
        elif os.path.isdir(fullname):
            subdirs.append(fullname)
        elif wanted(name):
            if fix(fullname): bad = 1
    for fullname in subdirs:
        if recursedown(fullname): bad = 1
    return bad

def fix(filename):
##  dbg('fix(%r)\n' % (filename,))
    if filename == '-':
        # Filter mode
        f = sys.stdin
        g = sys.stdout
    else:
        # File replacement mode
        try:
            f = open(filename, 'r')
        except IOError as msg:
            err(filename + ': cannot open: ' + str(msg) + '\n')
            return 1
        head, tail = os.path.split(filename)
        tempname = os.path.join(head, '@' + tail)
        g = None
    # If we find a match, we rewind the file and start over but
    # now copy everything to a temp file.
    lineno = 0
    initfixline()
    while 1:
        line = f.readline()
        if not line: break
        lineno = lineno + 1
        while line[-2:] == '\\\n':
            nextline = f.readline()
            if not nextline: break
            line = line + nextline
            lineno = lineno + 1
        newline = fixline(line)
        if newline != line:
            if g is None:
                try:
                    g = open(tempname, 'w')
                except IOError as msg:
                    f.close()
                    err(tempname+': cannot create: '+
                        str(msg)+'\n')
                    return 1
                f.seek(0)
                lineno = 0
                initfixline()
                rep(filename + ':\n')
                continue # restart from the beginning
            rep(repr(lineno) + '\n')
            rep('< ' + line)
            rep('> ' + newline)
        if g is not None:
            g.write(newline)

    # End of file
    if filename == '-': return 0 # Done in filter mode
    f.close()
    if not g: return 0 # No changes

    # Finishing touch -- move files

    # First copy the file's mode to the temp file
    try:
        statbuf = os.stat(filename)
        os.chmod(tempname, statbuf[ST_MODE] & 0o7777)
    except OSError as msg:
        err(tempname + ': warning: chmod failed (' + str(msg) + ')\n')
    # Then make a backup of the original file as filename~
    try:
        os.rename(filename, filename + '~')
    except OSError as msg:
        err(filename + ': warning: backup failed (' + str(msg) + ')\n')
    # Now move the temp file to the original file
    try:
        os.rename(tempname, filename)
    except OSError as msg:
        err(filename + ': rename failed (' + str(msg) + ')\n')
        return 1
    # Return success
    return 0

# Tokenizing ANSI C (partly)

Identifier = '\(struct \)?[a-zA-Z_][a-zA-Z0-9_]+'
String = '"\([^\n\\"]\|\\\\.\)*"'
Char = '\'\([^\n\\\']\|\\\\.\)*\''
CommentStart = '/\*'
CommentEnd = '\*/'

Hexnumber = '0[xX][0-9a-fA-F]*[uUlL]*'
Octnumber = '0[0-7]*[uUlL]*'
Decnumber = '[1-9][0-9]*[uUlL]*'
Intnumber = Hexnumber + '\|' + Octnumber + '\|' + Decnumber
Exponent = '[eE][-+]?[0-9]+'
Pointfloat = '\([0-9]+\.[0-9]*\|\.[0-9]+\)\(' + Exponent + '\)?'
Expfloat = '[0-9]+' + Exponent
Floatnumber = Pointfloat + '\|' + Expfloat
Number = Floatnumber + '\|' + Intnumber

# Anything else is an operator -- don't list this explicitly because of '/*'

OutsideComment = (Identifier, Number, String, Char, CommentStart)
OutsideCommentPattern = '(' + '|'.join(OutsideComment) + ')'
OutsideCommentProgram = re.compile(OutsideCommentPattern)

InsideComment = (Identifier, Number, CommentEnd)
InsideCommentPattern = '(' + '|'.join(InsideComment) + ')'
InsideCommentProgram = re.compile(InsideCommentPattern)

def initfixline():
    global Program
    Program = OutsideCommentProgram

def fixline(line):
    global Program
##  print '-->', repr(line)
    i = 0
    while i < len(line):
        i = Program.search(line, i)
        if i < 0: break
        found = Program.group(0)
##      if Program is InsideCommentProgram: print '...',
##      else: print '   ',
##      print found
        if len(found) == 2:
            if found == '/*':
                Program = InsideCommentProgram
            elif found == '*/':
                Program = OutsideCommentProgram
        n = len(found)
        if found in Dict:
            subst = Dict[found]
            if Program is InsideCommentProgram:
                if not Docomments:
                    print('Found in comment:', found)
                    i = i + n
                    continue
                if NotInComment.has_key(found):
##                  print 'Ignored in comment:',
##                  print found, '-->', subst
##                  print 'Line:', line,
                    subst = found
##              else:
##                  print 'Substituting in comment:',
##                  print found, '-->', subst
##                  print 'Line:', line,
            line = line[:i] + subst + line[i+n:]
            n = len(subst)
        i = i + n
    return line

Docomments = 0
def setdocomments():
    global Docomments
    Docomments = 1

Reverse = 0
def setreverse():
    global Reverse
    Reverse = (not Reverse)

Dict = {}
NotInComment = {}
def addsubst(substfile):
    try:
        fp = open(substfile, 'r')
    except IOError as msg:
        err(substfile + ': cannot read substfile: ' + str(msg) + '\n')
        sys.exit(1)
    lineno = 0
    while 1:
        line = fp.readline()
        if not line: break
        lineno = lineno + 1
        try:
            i = line.index('#')
        except ValueError:
            i = -1          # Happens to delete trailing \n
        words = line[:i].split()
        if not words: continue
        if len(words) == 3 and words[0] == 'struct':
            words[:2] = [words[0] + ' ' + words[1]]
        elif len(words) != 2:
            err(substfile + '%s:%r: warning: bad line: %r' % (substfile, lineno, line))
            continue
        if Reverse:
            [value, key] = words
        else:
            [key, value] = words
        if value[0] == '*':
            value = value[1:]
        if key[0] == '*':
            key = key[1:]
            NotInComment[key] = value
        if key in Dict:
            err('%s:%r: warning: overriding: %r %r\n' % (substfile, lineno, key, value))
            err('%s:%r: warning: previous: %r\n' % (substfile, lineno, Dict[key]))
        Dict[key] = value
    fp.close()

if __name__ == '__main__':
    main()
