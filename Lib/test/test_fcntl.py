#! /usr/bin/env python
"""Test program for the fcntl C module.
   OS/2+EMX doesn't support the file locking operations.
   Roger E. Masse
"""
import struct
import fcntl
import os, sys
from test.test_support import verbose, TESTFN

filename = TESTFN

try:
    os.O_LARGEFILE
except AttributeError:
    start_len = "ll"
else:
    start_len = "qq"

if sys.platform.startswith('atheos'):
    start_len = "qq"

if sys.platform in ('netbsd1', 'Darwin1.2', 'darwin',
                    'freebsd2', 'freebsd3', 'freebsd4', 'freebsd5', 'freebsd6',
                    'bsdos2', 'bsdos3', 'bsdos4',
                    'openbsd', 'openbsd2', 'openbsd3'):
    lockdata = struct.pack('lxxxxlxxxxlhh', 0, 0, 0, fcntl.F_WRLCK, 0)
elif sys.platform in ['aix3', 'aix4', 'hp-uxB', 'unixware7']:
    lockdata = struct.pack('hhlllii', fcntl.F_WRLCK, 0, 0, 0, 0, 0, 0)
elif sys.platform in ['os2emx']:
    lockdata = None
else:
    lockdata = struct.pack('hh'+start_len+'hh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
if lockdata:
    if verbose:
        print 'struct.pack: ', repr(lockdata)

# the example from the library docs
f = open(filename, 'w')
rv = fcntl.fcntl(f.fileno(), fcntl.F_SETFL, os.O_NONBLOCK)
if verbose:
    print 'Status from fnctl with O_NONBLOCK: ', rv

if sys.platform not in ['os2emx']:
    rv = fcntl.fcntl(f.fileno(), fcntl.F_SETLKW, lockdata)
    if verbose:
        print 'String from fcntl with F_SETLKW: ', repr(rv)

f.close()
os.unlink(filename)


# Again, but pass the file rather than numeric descriptor:
f = open(filename, 'w')
rv = fcntl.fcntl(f, fcntl.F_SETFL, os.O_NONBLOCK)

if sys.platform not in ['os2emx']:
    rv = fcntl.fcntl(f, fcntl.F_SETLKW, lockdata)

f.close()
os.unlink(filename)
