#! /usr/bin/env python3

"""Python utility to print MD5 checksums of argument files.
"""


bufsize = 8096
fnfilter = None
rmode = 'rb'

usage = """
usage: sum5 [-b] [-t] [-l] [-s bufsize] [file ...]
-b        : read files in binary mode (default)
-t        : read files in text mode (you almost certainly don't want this!)
-l        : print last pathname component only
-s bufsize: read buffer size (default %d)
file ...  : files to sum; '-' or no files means stdin
""" % bufsize

import sys
import os
import getopt
from hashlib import md5

def sum(*files):
    sts = 0
    if files and isinstance(files[-1], file):
        out, files = files[-1], files[:-1]
    else:
        out = sys.stdout
    if len(files) == 1 and not isinstance(files[0], str):
        files = files[0]
    for f in files:
        if isinstance(f, str):
            if f == '-':
                sts = printsumfp(sys.stdin, '<stdin>', out) or sts
            else:
                sts = printsum(f, out) or sts
        else:
            sts = sum(f, out) or sts
    return sts

def printsum(filename, out=sys.stdout):
    try:
        fp = open(filename, rmode)
    except IOError as msg:
        sys.stderr.write('%s: Can\'t open: %s\n' % (filename, msg))
        return 1
    if fnfilter:
        filename = fnfilter(filename)
    sts = printsumfp(fp, filename, out)
    fp.close()
    return sts

def printsumfp(fp, filename, out=sys.stdout):
    m = md5.new()
    try:
        while 1:
            data = fp.read(bufsize)
            if not data:
                break
            m.update(data)
    except IOError as msg:
        sys.stderr.write('%s: I/O error: %s\n' % (filename, msg))
        return 1
    out.write('%s %s\n' % (m.hexdigest(), filename))
    return 0

def main(args = sys.argv[1:], out=sys.stdout):
    global fnfilter, rmode, bufsize
    try:
        opts, args = getopt.getopt(args, 'blts:')
    except getopt.error as msg:
        sys.stderr.write('%s: %s\n%s' % (sys.argv[0], msg, usage))
        return 2
    for o, a in opts:
        if o == '-l':
            fnfilter = os.path.basename
        elif o == '-b':
            rmode = 'rb'
        elif o == '-t':
            rmode = 'r'
        elif o == '-s':
            bufsize = int(a)
    if not args:
        args = ['-']
    return sum(args, out)

if __name__ == '__main__' or __name__ == sys.argv[0]:
    sys.exit(main(sys.argv[1:], sys.stdout))
