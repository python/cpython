#! /usr/bin/env python
"""Test program for the fcntl C module.
   Roger E. Masse
"""
import struct
import fcntl
import os, sys
from test_support import verbose, TESTFN

filename = TESTFN

try:
    os.O_LARGEFILE
except AttributeError:
    start_len = "ll"
else:
    start_len = "qq"

if sys.platform in ('netbsd1', 'Darwin1.2', 'darwin',
                    'freebsd2', 'freebsd3', 'freebsd4', 'freebsd5',
                    'bsdos2', 'bsdos3', 'bsdos4',
                    'openbsd', 'openbsd2', 'openbsd3'):
    lockdata = struct.pack('lxxxxlxxxxlhh', 0, 0, 0, fcntl.F_WRLCK, 0)
elif sys.platform in ['aix3', 'aix4', 'hp-uxB', 'unixware7']:
    lockdata = struct.pack('hhlllii', fcntl.F_WRLCK, 0, 0, 0, 0, 0, 0)
else:
    lockdata = struct.pack('hh'+start_len+'hh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
if verbose:
    print 'struct.pack: ', `lockdata`


# the example from the library docs
f = open(filename, 'w')
rv = fcntl.fcntl(f.fileno(), fcntl.F_SETFL, os.O_NONBLOCK)
if verbose:
    print 'Status from fnctl with O_NONBLOCK: ', rv

rv = fcntl.fcntl(f.fileno(), fcntl.F_SETLKW, lockdata)
if verbose:
    print 'String from fcntl with F_SETLKW: ', `rv`

f.close()
os.unlink(filename)


# Again, but pass the file rather than numeric descriptor:
f = open(filename, 'w')
rv = fcntl.fcntl(f, fcntl.F_SETFL, os.O_NONBLOCK)

rv = fcntl.fcntl(f, fcntl.F_SETLKW, lockdata)

f.close()
os.unlink(filename)
