#! /usr/bin/env python

# This file contains a class and a main program that perform three
# related (though complimentary) formatting operations on Python
# programs.  When called as "pindent -c", it takes a valid Python
# program as input and outputs a version augmented with block-closing
# comments.  When called as "pindent -d", it assumes its input is a
# Python program with block-closing comments and outputs a commentless
# version.   When called as "pindent -r" it assumes its input is a
# Python program with block-closing comments but with its indentation
# messed up, and outputs a properly indented version.

# A "block-closing comment" is a comment of the form '# end <keyword>'
# where <keyword> is the keyword that opened the block.  If the
# opening keyword is 'def' or 'class', the function or class name may
# be repeated in the block-closing comment as well.  Here is an
# example of a program fully augmented with block-closing comments:

# def foobar(a, b):
#    if a == b:
#        a = a+1
#    elif a < b:
#        b = b-1
#        if b > a: a = a-1
#        # end if
#    else:
#        print 'oops!'
#    # end if
# # end def foobar

# Note that only the last part of an if...elif...else... block needs a
# block-closing comment; the same is true for other compound
# statements (e.g. try...except).  Also note that "short-form" blocks
# like the second 'if' in the example must be closed as well;
# otherwise the 'else' in the example would be ambiguous (remember
# that indentation is not significant when interpreting block-closing
# comments).

# The operations are idempotent (i.e. applied to their own output
# they yield an identical result).  Running first "pindent -c" and
# then "pindent -r" on a valid Python program produces a program that
# is semantically identical to the input (though its indentation may
# be different). Running "pindent -e" on that output produces a
# program that only differs from the original in indentation.

# Other options:
# -s stepsize: set the indentation step size (default 8)
# -t tabsize : set the number of spaces a tab character is worth (default 8)
# -e         : expand TABs into spaces
# file ...   : input file(s) (default standard input)
# The results always go to standard output

# Caveats:
# - comments ending in a backslash will be mistaken for continued lines
# - continuations using backslash are always left unchanged
# - continuations inside parentheses are not extra indented by -r
#   but must be indented for -c to work correctly (this breaks
#   idempotency!)
# - continued lines inside triple-quoted strings are totally garbled

# Secret feature:
# - On input, a block may also be closed with an "end statement" --
#   this is a block-closing comment without the '#' sign.

# Possible improvements:
# - check syntax based on transitions in 'next' table
# - better error reporting
# - better error recovery
# - check identifier after class/def

# The following wishes need a more complete tokenization of the source:
# - Don't get fooled by comments ending in backslash
# - reindent continuation lines indicated by backslash
# - handle continuation lines inside parentheses/braces/brackets
# - handle triple quoted strings spanning lines
# - realign comments
# - optionally do much more thorough reformatting, a la C indent

from __future__ import print_function

# Defaults
STEPSIZE = 8
TABSIZE = 8
EXPANDTABS = False

import io
import re
import sys

next = {}
next['if'] = next['elif'] = 'elif', 'else', 'end'
next['while'] = next['for'] = 'else', 'end'
next['try'] = 'except', 'finally'
next['except'] = 'except', 'else', 'finally', 'end'
next['else'] = next['finally'] = next['with'] = \
    next['def'] = next['class'] = 'end'
next['end'] = ()
start = 'if', 'while', 'for', 'try', 'with', 'def', 'class'

class PythonIndenter:

    def __init__(self, fpi = sys.stdin, fpo = sys.stdout,
                 indentsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
        self.fpi = fpi
        self.fpo = fpo
        self.indentsize = indentsize
        self.tabsize = tabsize
        self.lineno = 0
        self.expandtabs = expandtabs
        self._write = fpo.write
        self.kwprog = re.compile(
                r'^(?:\s|\\\n)*(?P<kw>[a-z]+)'
                r'((?:\s|\\\n)+(?P<id>[a-zA-Z_]\w*))?'
                r'[^\w]')
        self.endprog = re.compile(
                r'^(?:\s|\\\n)*#?\s*end\s+(?P<kw>[a-z]+)'
                r'(\s+(?P<id>[a-zA-Z_]\w*))?'
                r'[^\w]')
        self.wsprog = re.compile(r'^[ \t]*')
    # end def __init__

    def write(self, line):
        if self.expandtabs:
            self._write(line.expandtabs(self.tabsize))
        else:
            self._write(line)
        # end if
    # end def write

    def readline(self):
        line = self.fpi.readline()
        if line: self.lineno += 1
        # end if
        return line
    # end def readline

    def error(self, fmt, *args):
        if args: fmt = fmt % args
        # end if
        sys.stderr.write('Error at line %d: %s\n' % (self.lineno, fmt))
        self.write('### %s ###\n' % fmt)
    # end def error

    def getline(self):
        line = self.readline()
        while line[-2:] == '\\\n':
            line2 = self.readline()
            if not line2: break
            # end if
            line += line2
        # end while
        return line
    # end def getline

    def putline(self, line, indent):
        tabs, spaces = divmod(indent*self.indentsize, self.tabsize)
        i = self.wsprog.match(line).end()
        line = line[i:]
        if line[:1] not in ('\n', '\r', ''):
            line = '\t'*tabs + ' '*spaces + line
        # end if
        self.write(line)
    # end def putline

    def reformat(self):
        stack = []
        while True:
            line = self.getline()
            if not line: break      # EOF
            # end if
            m = self.endprog.match(line)
            if m:
                kw = 'end'
                kw2 = m.group('kw')
                if not stack:
                    self.error('unexpected end')
                elif stack.pop()[0] != kw2:
                    self.error('unmatched end')
                # end if
                self.putline(line, len(stack))
                continue
            # end if
            m = self.kwprog.match(line)
            if m:
                kw = m.group('kw')
                if kw in start:
                    self.putline(line, len(stack))
                    stack.append((kw, kw))
                    continue
                # end if
                if next.has_key(kw) and stack:
                    self.putline(line, len(stack)-1)
                    kwa, kwb = stack[-1]
                    stack[-1] = kwa, kw
                    continue
                # end if
            # end if
            self.putline(line, len(stack))
        # end while
        if stack:
            self.error('unterminated keywords')
            for kwa, kwb in stack:
                self.write('\t%s\n' % kwa)
            # end for
        # end if
    # end def reformat

    def delete(self):
        begin_counter = 0
        end_counter = 0
        while True:
            line = self.getline()
            if not line: break      # EOF
            # end if
            m = self.endprog.match(line)
            if m:
                end_counter += 1
                continue
            # end if
            m = self.kwprog.match(line)
            if m:
                kw = m.group('kw')
                if kw in start:
                    begin_counter += 1
                # end if
            # end if
            self.write(line)
        # end while
        if begin_counter - end_counter < 0:
            sys.stderr.write('Warning: input contained more end tags than expected\n')
        elif begin_counter - end_counter > 0:
            sys.stderr.write('Warning: input contained less end tags than expected\n')
        # end if
    # end def delete

    def complete(self):
        stack = []
        todo = []
        currentws = thisid = firstkw = lastkw = topid = ''
        while True:
            line = self.getline()
            i = self.wsprog.match(line).end()
            m = self.endprog.match(line)
            if m:
                thiskw = 'end'
                endkw = m.group('kw')
                thisid = m.group('id')
            else:
                m = self.kwprog.match(line)
                if m:
                    thiskw = m.group('kw')
                    if not next.has_key(thiskw):
                        thiskw = ''
                    # end if
                    if thiskw in ('def', 'class'):
                        thisid = m.group('id')
                    else:
                        thisid = ''
                    # end if
                elif line[i:i+1] in ('\n', '#'):
                    todo.append(line)
                    continue
                else:
                    thiskw = ''
                # end if
            # end if
            indentws = line[:i]
            indent = len(indentws.expandtabs(self.tabsize))
            current = len(currentws.expandtabs(self.tabsize))
            while indent < current:
                if firstkw:
                    if topid:
                        s = '# end %s %s\n' % (
                                firstkw, topid)
                    else:
                        s = '# end %s\n' % firstkw
                    # end if
                    self.write(currentws + s)
                    firstkw = lastkw = ''
                # end if
                currentws, firstkw, lastkw, topid = stack.pop()
                current = len(currentws.expandtabs(self.tabsize))
            # end while
            if indent == current and firstkw:
                if thiskw == 'end':
                    if endkw != firstkw:
                        self.error('mismatched end')
                    # end if
                    firstkw = lastkw = ''
                elif not thiskw or thiskw in start:
                    if topid:
                        s = '# end %s %s\n' % (
                                firstkw, topid)
                    else:
                        s = '# end %s\n' % firstkw
                    # end if
                    self.write(currentws + s)
                    firstkw = lastkw = topid = ''
                # end if
            # end if
            if indent > current:
                stack.append((currentws, firstkw, lastkw, topid))
                if thiskw and thiskw not in start:
                    # error
                    thiskw = ''
                # end if
                currentws, firstkw, lastkw, topid = \
                          indentws, thiskw, thiskw, thisid
            # end if
            if thiskw:
                if thiskw in start:
                    firstkw = lastkw = thiskw
                    topid = thisid
                else:
                    lastkw = thiskw
                # end if
            # end if
            for l in todo: self.write(l)
            # end for
            todo = []
            if not line: break
            # end if
            self.write(line)
        # end while
    # end def complete
# end class PythonIndenter

# Simplified user interface
# - xxx_filter(input, output): read and write file objects
# - xxx_string(s): take and return string object
# - xxx_file(filename): process file in place, return true iff changed

def complete_filter(input = sys.stdin, output = sys.stdout,
                    stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.complete()
# end def complete_filter

def delete_filter(input= sys.stdin, output = sys.stdout,
                        stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.delete()
# end def delete_filter

def reformat_filter(input = sys.stdin, output = sys.stdout,
                    stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.reformat()
# end def reformat_filter

def complete_string(source, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    input = io.BytesIO(source)
    output = io.BytesIO()
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.complete()
    return output.getvalue()
# end def complete_string

def delete_string(source, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    input = io.BytesIO(source)
    output = io.BytesIO()
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.delete()
    return output.getvalue()
# end def delete_string

def reformat_string(source, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    input = io.BytesIO(source)
    output = io.BytesIO()
    pi = PythonIndenter(input, output, stepsize, tabsize, expandtabs)
    pi.reformat()
    return output.getvalue()
# end def reformat_string

def make_backup(filename):
    import os, os.path
    backup = filename + '~'
    if os.path.lexists(backup):
        try:
            os.remove(backup)
        except os.error:
            print("Can't remove backup %r" % (backup,), file=sys.stderr)
        # end try
    # end if
    try:
        os.rename(filename, backup)
    except os.error:
        print("Can't rename %r to %r" % (filename, backup), file=sys.stderr)
    # end try
# end def make_backup

def complete_file(filename, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    with open(filename, 'r') as f:
        source = f.read()
    # end with
    result = complete_string(source, stepsize, tabsize, expandtabs)
    if source == result: return 0
    # end if
    make_backup(filename)
    with open(filename, 'w') as f:
        f.write(result)
    # end with
    return 1
# end def complete_file

def delete_file(filename, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    with open(filename, 'r') as f:
        source = f.read()
    # end with
    result = delete_string(source, stepsize, tabsize, expandtabs)
    if source == result: return 0
    # end if
    make_backup(filename)
    with open(filename, 'w') as f:
        f.write(result)
    # end with
    return 1
# end def delete_file

def reformat_file(filename, stepsize = STEPSIZE, tabsize = TABSIZE, expandtabs = EXPANDTABS):
    with open(filename, 'r') as f:
        source = f.read()
    # end with
    result = reformat_string(source, stepsize, tabsize, expandtabs)
    if source == result: return 0
    # end if
    make_backup(filename)
    with open(filename, 'w') as f:
        f.write(result)
    # end with
    return 1
# end def reformat_file

# Test program when called as a script

usage = """
usage: pindent (-c|-d|-r) [-s stepsize] [-t tabsize] [-e] [file] ...
-c         : complete a correctly indented program (add #end directives)
-d         : delete #end directives
-r         : reformat a completed program (use #end directives)
-s stepsize: indentation step (default %(STEPSIZE)d)
-t tabsize : the worth in spaces of a tab (default %(TABSIZE)d)
-e         : expand TABs into spaces (default OFF)
[file] ... : files are changed in place, with backups in file~
If no files are specified or a single - is given,
the program acts as a filter (reads stdin, writes stdout).
""" % vars()

def error_both(op1, op2):
    sys.stderr.write('Error: You can not specify both '+op1+' and -'+op2[0]+' at the same time\n')
    sys.stderr.write(usage)
    sys.exit(2)
# end def error_both

def test():
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'cdrs:t:e')
    except getopt.error, msg:
        sys.stderr.write('Error: %s\n' % msg)
        sys.stderr.write(usage)
        sys.exit(2)
    # end try
    action = None
    stepsize = STEPSIZE
    tabsize = TABSIZE
    expandtabs = EXPANDTABS
    for o, a in opts:
        if o == '-c':
            if action: error_both(o, action)
            # end if
            action = 'complete'
        elif o == '-d':
            if action: error_both(o, action)
            # end if
            action = 'delete'
        elif o == '-r':
            if action: error_both(o, action)
            # end if
            action = 'reformat'
        elif o == '-s':
            stepsize = int(a)
        elif o == '-t':
            tabsize = int(a)
        elif o == '-e':
            expandtabs = True
        # end if
    # end for
    if not action:
        sys.stderr.write(
                'You must specify -c(omplete), -d(elete) or -r(eformat)\n')
        sys.stderr.write(usage)
        sys.exit(2)
    # end if
    if not args or args == ['-']:
        action = eval(action + '_filter')
        action(sys.stdin, sys.stdout, stepsize, tabsize, expandtabs)
    else:
        action = eval(action + '_file')
        for filename in args:
            action(filename, stepsize, tabsize, expandtabs)
        # end for
    # end if
# end def test

if __name__ == '__main__':
    test()
# end if
