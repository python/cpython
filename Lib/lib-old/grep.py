# 'grep'

import regex
from regex_syntax import *

opt_show_where = 0
opt_show_filename = 0
opt_show_lineno = 1

def grep(pat, *files):
    return ggrep(RE_SYNTAX_GREP, pat, files)

def egrep(pat, *files):
    return ggrep(RE_SYNTAX_EGREP, pat, files)

def emgrep(pat, *files):
    return ggrep(RE_SYNTAX_EMACS, pat, files)

def ggrep(syntax, pat, files):
    if len(files) == 1 and type(files[0]) == type([]):
        files = files[0]
    global opt_show_filename
    opt_show_filename = (len(files) != 1)
    syntax = regex.set_syntax(syntax)
    try:
        prog = regex.compile(pat)
    finally:
        syntax = regex.set_syntax(syntax)
    for filename in files:
        fp = open(filename, 'r')
        lineno = 0
        while 1:
            line = fp.readline()
            if not line: break
            lineno = lineno + 1
            if prog.search(line) >= 0:
                showline(filename, lineno, line, prog)
        fp.close()

def pgrep(pat, *files):
    if len(files) == 1 and type(files[0]) == type([]):
        files = files[0]
    global opt_show_filename
    opt_show_filename = (len(files) != 1)
    import re
    prog = re.compile(pat)
    for filename in files:
        fp = open(filename, 'r')
        lineno = 0
        while 1:
            line = fp.readline()
            if not line: break
            lineno = lineno + 1
            if prog.search(line):
                showline(filename, lineno, line, prog)
        fp.close()

def showline(filename, lineno, line, prog):
    if line[-1:] == '\n': line = line[:-1]
    if opt_show_lineno:
        prefix = `lineno`.rjust(3) + ': '
    else:
        prefix = ''
    if opt_show_filename:
        prefix = filename + ': ' + prefix
    print prefix + line
    if opt_show_where:
        start, end = prog.regs()[0]
        line = line[:start]
        if '\t' not in line:
            prefix = ' ' * (len(prefix) + start)
        else:
            prefix = ' ' * len(prefix)
            for c in line:
                if c != '\t': c = ' '
                prefix = prefix + c
        if start == end: prefix = prefix + '\\'
        else: prefix = prefix + '^'*(end-start)
        print prefix
