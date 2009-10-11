#! /usr/bin/env python

# script.py -- Make typescript of terminal session.
# Usage:
#       -a      Append to typescript.
#       -p      Use Python as shell.
# Author: Steen Lumholt.


import os, time, sys, getopt
import pty

def read(fd):
    data = os.read(fd, 1024)
    script.write(data)
    return data

shell = 'sh'
filename = 'typescript'
mode = 'w'
if os.environ.has_key('SHELL'):
    shell = os.environ['SHELL']

try:
    opts, args = getopt.getopt(sys.argv[1:], 'ap')
except getopt.error, msg:
    print '%s: %s' % (sys.argv[0], msg)
    sys.exit(2)

for o, a in opts:
    if o == '-a':
        mode = 'a'
    elif o == '-p':
        shell = 'python'

script = open(filename, mode)

sys.stdout.write('Script started, file is %s\n' % filename)
script.write('Script started on %s\n' % time.ctime(time.time()))
pty.spawn(shell, read)
script.write('Script done on %s\n' % time.ctime(time.time()))
sys.stdout.write('Script done, file is %s\n' % filename)
