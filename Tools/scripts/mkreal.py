#! /usr/bin/env python

# mkreal
#
# turn a symlink to a directory into a real directory

import sys
import os
from stat import *

join = os.path.join

error = 'mkreal error'

BUFSIZE = 32*1024

def mkrealfile(name):
    st = os.stat(name) # Get the mode
    mode = S_IMODE(st[ST_MODE])
    linkto = os.readlink(name) # Make sure again it's a symlink
    f_in = open(name, 'r') # This ensures it's a file
    os.unlink(name)
    f_out = open(name, 'w')
    while 1:
        buf = f_in.read(BUFSIZE)
        if not buf: break
        f_out.write(buf)
    del f_out # Flush data to disk before changing mode
    os.chmod(name, mode)

def mkrealdir(name):
    st = os.stat(name) # Get the mode
    mode = S_IMODE(st[ST_MODE])
    linkto = os.readlink(name)
    files = os.listdir(name)
    os.unlink(name)
    os.mkdir(name, mode)
    os.chmod(name, mode)
    linkto = join(os.pardir, linkto)
    #
    for filename in files:
        if filename not in (os.curdir, os.pardir):
            os.symlink(join(linkto, filename), join(name, filename))

def main():
    sys.stdout = sys.stderr
    progname = os.path.basename(sys.argv[0])
    if progname == '-c': progname = 'mkreal'
    args = sys.argv[1:]
    if not args:
        print('usage:', progname, 'path ...')
        sys.exit(2)
    status = 0
    for name in args:
        if not os.path.islink(name):
            print(progname+':', name+':', 'not a symlink')
            status = 1
        else:
            if os.path.isdir(name):
                mkrealdir(name)
            else:
                mkrealfile(name)
    sys.exit(status)

if __name__ == '__main__':
    main()
