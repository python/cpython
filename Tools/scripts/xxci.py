#! /usr/bin/env python

# xxci
#
# check in files for which rcsdiff returns nonzero exit status

import sys
import os
from stat import *
import fnmatch

EXECMAGIC = '\001\140\000\010'

MAXSIZE = 200*1024 # Files this big must be binaries and are skipped.

def getargs():
    args = sys.argv[1:]
    if args:
        return args
    print 'No arguments, checking almost *, in "ls -t" order'
    list = []
    for file in os.listdir(os.curdir):
        if not skipfile(file):
            list.append((getmtime(file), file))
    list.sort()
    if not list:
        print 'Nothing to do -- exit 1'
        sys.exit(1)
    list.sort()
    list.reverse()
    for mtime, file in list: args.append(file)
    return args

def getmtime(file):
    try:
        st = os.stat(file)
        return st[ST_MTIME]
    except os.error:
        return -1

badnames = ['tags', 'TAGS', 'xyzzy', 'nohup.out', 'core']
badprefixes = ['.', ',', '@', '#', 'o.']
badsuffixes = \
        ['~', '.a', '.o', '.old', '.bak', '.orig', '.new', '.prev', '.not', \
         '.pyc', '.fdc', '.rgb', '.elc', ',v']
ignore = []

def setup():
    ignore[:] = badnames
    for p in badprefixes:
        ignore.append(p + '*')
    for p in badsuffixes:
        ignore.append('*' + p)
    try:
        f = open('.xxcign', 'r')
    except IOError:
        return
    ignore[:] = ignore + f.read().split()

def skipfile(file):
    for p in ignore:
        if fnmatch.fnmatch(file, p): return 1
    try:
        st = os.lstat(file)
    except os.error:
        return 1 # Doesn't exist -- skip it
    # Skip non-plain files.
    if not S_ISREG(st[ST_MODE]): return 1
    # Skip huge files -- probably binaries.
    if st[ST_SIZE] >= MAXSIZE: return 1
    # Skip executables
    try:
        data = open(file, 'r').read(len(EXECMAGIC))
        if data == EXECMAGIC: return 1
    except:
        pass
    return 0

def badprefix(file):
    for bad in badprefixes:
        if file[:len(bad)] == bad: return 1
    return 0

def badsuffix(file):
    for bad in badsuffixes:
        if file[-len(bad):] == bad: return 1
    return 0

def go(args):
    for file in args:
        print file + ':'
        if differing(file):
            showdiffs(file)
            if askyesno('Check in ' + file + ' ? '):
                sts = os.system('rcs -l ' + file) # ignored
                sts = os.system('ci -l ' + file)

def differing(file):
    cmd = 'co -p ' + file + ' 2>/dev/null | cmp -s - ' + file
    sts = os.system(cmd)
    return sts != 0

def showdiffs(file):
    cmd = 'rcsdiff ' + file + ' 2>&1 | ${PAGER-more}'
    sts = os.system(cmd)

def askyesno(prompt):
    s = raw_input(prompt)
    return s in ['y', 'yes']

if __name__ == '__main__':
    try:
        setup()
        go(getargs())
    except KeyboardInterrupt:
        print '[Intr]'
