#! /usr/bin/env python
"""Test program for the fcntl C module.
   Roger E. Masse
"""
import struct
import fcntl
import FCNTL
import os, sys
from test_support import verbose

filename = '/tmp/delete-me'

# the example from the library docs
f = open(filename,'w')
rv = fcntl.fcntl(f.fileno(), FCNTL.F_SETFL, os.O_NONBLOCK)
if verbose:
    print 'Status from fnctl with O_NONBLOCK: ', rv
    
if sys.platform in ('netbsd1', 'Darwin1.2',
                    'freebsd2', 'freebsd3', 'freebsd4', 'freebsd5',
                    'bsdos2', 'bsdos3', 'bsdos4',
                    'openbsd', 'openbsd2'):
    lockdata = struct.pack('lxxxxlxxxxlhh', 0, 0, 0, FCNTL.F_WRLCK, 0)
elif sys.platform in ['aix3', 'aix4', 'hp-uxB']:
    lockdata = struct.pack('hhlllii', FCNTL.F_WRLCK, 0, 0, 0, 0, 0, 0)
else:
    lockdata = struct.pack('hhllhh', FCNTL.F_WRLCK, 0, 0, 0, 0, 0)
if verbose:
    print 'struct.pack: ', `lockdata`
    
rv = fcntl.fcntl(f.fileno(), FCNTL.F_SETLKW, lockdata)
if verbose:
    print 'String from fcntl with F_SETLKW: ', `rv`

f.close()
os.unlink(filename)
