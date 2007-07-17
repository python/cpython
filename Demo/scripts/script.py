#! /usr/bin/env python
# script.py -- Make typescript of terminal session.
# Usage:
#       -a      Append to typescript.
#       -p      Use Python as shell.
# Author: Steen Lumholt.


import os, time, sys
import pty

def read(fd):
    data = os.read(fd, 1024)
    file.write(data)
    return data

shell = 'sh'
filename = 'typescript'
mode = 'w'
if 'SHELL' in os.environ:
    shell = os.environ['SHELL']
if '-a' in sys.argv:
    mode = 'a'
if '-p' in sys.argv:
    shell = 'python'

file = open(filename, mode)

sys.stdout.write('Script started, file is %s\n' % filename)
file.write('Script started on %s\n' % time.ctime(time.time()))
pty.spawn(shell, read)
file.write('Script done on %s\n' % time.ctime(time.time()))
sys.stdout.write('Script done, file is %s\n' % filename)
