# Print tracebacks, with a dump of local variables.
# Also an interactive stack trace browser.
# Note -- this module is obsolete -- use pdb.pm() instead.

import sys
import os
from stat import *
import linecache

def br(): browser(sys.last_traceback)

def tb(): printtb(sys.last_traceback)

def browser(tb):
    if not tb:
        print 'No traceback.'
        return
    tblist = []
    while tb:
        tblist.append(tb)
        tb = tb.tb_next
    ptr = len(tblist)-1
    tb = tblist[ptr]
    while 1:
        if tb != tblist[ptr]:
            tb = tblist[ptr]
            print `ptr` + ':',
            printtbheader(tb)
        try:
            line = raw_input('TB: ')
        except KeyboardInterrupt:
            print '\n[Interrupted]'
            break
        except EOFError:
            print '\n[EOF]'
            break
        cmd = line.strip()
        if cmd:
            if cmd == 'quit':
                break
            elif cmd == 'list':
                browserlist(tb)
            elif cmd == 'up':
                if ptr-1 >= 0: ptr = ptr-1
                else: print 'Bottom of stack.'
            elif cmd == 'down':
                if ptr+1 < len(tblist): ptr = ptr+1
                else: print 'Top of stack.'
            elif cmd == 'locals':
                printsymbols(tb.tb_frame.f_locals)
            elif cmd == 'globals':
                printsymbols(tb.tb_frame.f_globals)
            elif cmd in ('?', 'help'):
                browserhelp()
            else:
                browserexec(tb, cmd)

def browserlist(tb):
    filename = tb.tb_frame.f_code.co_filename
    lineno = tb.tb_lineno
    last = lineno
    first = max(1, last-10)
    for i in range(first, last+1):
        if i == lineno: prefix = '***' + `i`.rjust(4) + ':'
        else: prefix = `i`.rjust(7) + ':'
        line = linecache.getline(filename, i)
        if line[-1:] == '\n': line = line[:-1]
        print prefix + line

def browserexec(tb, cmd):
    locals = tb.tb_frame.f_locals
    globals = tb.tb_frame.f_globals
    try:
        exec cmd+'\n' in globals, locals
    except:
        t, v = sys.exc_info()[:2]
        print '*** Exception:',
        if type(t) is type(''):
            print t,
        else:
            print t.__name__,
        if v is not None:
            print ':', v,
        print
        print 'Type help to get help.'

def browserhelp():
    print
    print '    This is the traceback browser.  Commands are:'
    print '        up      : move one level up in the call stack'
    print '        down    : move one level down in the call stack'
    print '        locals  : print all local variables at this level'
    print '        globals : print all global variables at this level'
    print '        list    : list source code around the failure'
    print '        help    : print help (what you are reading now)'
    print '        quit    : back to command interpreter'
    print '    Typing any other 1-line statement will execute it'
    print '    using the current level\'s symbol tables'
    print

def printtb(tb):
    while tb:
        print1tb(tb)
        tb = tb.tb_next

def print1tb(tb):
    printtbheader(tb)
    if tb.tb_frame.f_locals is not tb.tb_frame.f_globals:
        printsymbols(tb.tb_frame.f_locals)

def printtbheader(tb):
    filename = tb.tb_frame.f_code.co_filename
    lineno = tb.tb_lineno
    info = '"' + filename + '"(' + `lineno` + ')'
    line = linecache.getline(filename, lineno)
    if line:
        info = info + ': ' + line.strip()
    print info

def printsymbols(d):
    keys = d.keys()
    keys.sort()
    for name in keys:
        print '  ' + name.ljust(12) + ':',
        printobject(d[name], 4)
        print

def printobject(v, maxlevel):
    if v is None:
        print 'None',
    elif type(v) in (type(0), type(0.0)):
        print v,
    elif type(v) is type(''):
        if len(v) > 20:
            print `v[:17] + '...'`,
        else:
            print `v`,
    elif type(v) is type(()):
        print '(',
        printlist(v, maxlevel)
        print ')',
    elif type(v) is type([]):
        print '[',
        printlist(v, maxlevel)
        print ']',
    elif type(v) is type({}):
        print '{',
        printdict(v, maxlevel)
        print '}',
    else:
        print v,

def printlist(v, maxlevel):
    n = len(v)
    if n == 0: return
    if maxlevel <= 0:
        print '...',
        return
    for i in range(min(6, n)):
        printobject(v[i], maxlevel-1)
        if i+1 < n: print ',',
    if n > 6: print '...',

def printdict(v, maxlevel):
    keys = v.keys()
    n = len(keys)
    if n == 0: return
    if maxlevel <= 0:
        print '...',
        return
    keys.sort()
    for i in range(min(6, n)):
        key = keys[i]
        print `key` + ':',
        printobject(v[key], maxlevel-1)
        if i+1 < n: print ',',
    if n > 6: print '...',
