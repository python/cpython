#! /usr/bin/env python

# Selectively preprocess #ifdef / #ifndef statements.
# Usage:
# ifdef [-Dname] ... [-Uname] ... [file] ...
#
# This scans the file(s), looking for #ifdef and #ifndef preprocessor
# commands that test for one of the names mentioned in the -D and -U
# options.  On standard output it writes a copy of the input file(s)
# minus those code sections that are suppressed by the selected
# combination of defined/undefined symbols.  The #if(n)def/#else/#else
# lines themselfs (if the #if(n)def tests for one of the mentioned
# names) are removed as well.

# Features: Arbitrary nesting of recognized and unrecognized
# preprocesor statements works correctly.  Unrecognized #if* commands
# are left in place, so it will never remove too much, only too
# little.  It does accept whitespace around the '#' character.

# Restrictions: There should be no comments or other symbols on the
# #if(n)def lines.  The effect of #define/#undef commands in the input
# file or in included files is not taken into account.  Tests using
# #if and the defined() pseudo function are not recognized.  The #elif
# command is not recognized.  Improperly nesting is not detected.
# Lines that look like preprocessor commands but which are actually
# part of comments or string literals will be mistaken for
# preprocessor commands.

import sys
import regex
import getopt

defs = []
undefs = []

def main():
    opts, args = getopt.getopt(sys.argv[1:], 'D:U:')
    for o, a in opts:
        if o == '-D':
            defs.append(a)
        if o == '-U':
            undefs.append(a)
    if not args:
        args = ['-']
    for filename in args:
        if filename == '-':
            process(sys.stdin, sys.stdout)
        else:
            f = open(filename, 'r')
            process(f, sys.stdout)
            f.close()

def process(fpi, fpo):
    keywords = ('if', 'ifdef', 'ifndef', 'else', 'endif')
    ok = 1
    stack = []
    while 1:
        line = fpi.readline()
        if not line: break
        while line[-2:] == '\\\n':
            nextline = fpi.readline()
            if not nextline: break
            line = line + nextline
        tmp = line.strip()
        if tmp[:1] != '#':
            if ok: fpo.write(line)
            continue
        tmp = tmp[1:].strip()
        words = tmp.split()
        keyword = words[0]
        if keyword not in keywords:
            if ok: fpo.write(line)
            continue
        if keyword in ('ifdef', 'ifndef') and len(words) == 2:
            if keyword == 'ifdef':
                ko = 1
            else:
                ko = 0
            word = words[1]
            if word in defs:
                stack.append((ok, ko, word))
                if not ko: ok = 0
            elif word in undefs:
                stack.append((ok, not ko, word))
                if ko: ok = 0
            else:
                stack.append((ok, -1, word))
                if ok: fpo.write(line)
        elif keyword == 'if':
            stack.append((ok, -1, ''))
            if ok: fpo.write(line)
        elif keyword == 'else' and stack:
            s_ok, s_ko, s_word = stack[-1]
            if s_ko < 0:
                if ok: fpo.write(line)
            else:
                s_ko = not s_ko
                ok = s_ok
                if not s_ko: ok = 0
                stack[-1] = s_ok, s_ko, s_word
        elif keyword == 'endif' and stack:
            s_ok, s_ko, s_word = stack[-1]
            if s_ko < 0:
                if ok: fpo.write(line)
            del stack[-1]
            ok = s_ok
        else:
            sys.stderr.write('Unknown keyword %s\n' % keyword)
    if stack:
        sys.stderr.write('stack: %s\n' % stack)

if __name__ == '__main__':
    main()
