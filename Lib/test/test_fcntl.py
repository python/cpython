#! /usr/bin/env python
"""Test program for the fcntl C module.
   Roger E. Masse
"""
import struct
import fcntl
import FCNTL
import os
from test_support import verbose

filename = '/tmp/delete-me'

# the example from the library docs
f = open(filename,'w')
rv = fcntl.fcntl(f.fileno(), FCNTL.F_SETFL, FCNTL.FNDELAY)
if verbose:
    print 'Status from fnctl with O_NDELAY: ', rv
    
lockdata = struct.pack('hhllhh', FCNTL.F_WRLCK, 0, 0, 0, 0, 0)
if verbose:
    print 'struct.pack: ', `lockdata`
    
rv = fcntl.fcntl(f.fileno(), FCNTL.F_SETLKW, lockdata)
if verbose:
    print 'String from fcntl with F_SETLKW: ', `rv`

f.close()
os.unlink(filename)
