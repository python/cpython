#! /usr/bin/env python
"""Test script for the dbm module
   Roger E. Masse
"""
import dbm
from dbm import error
from test_support import verbose

filename = '/tmp/delete_me'

d = dbm.open(filename, 'c')
d['a'] = 'b'
d['12345678910'] = '019237410982340912840198242'
d.keys()
if d.has_key('a'):
    if verbose:
        print 'Test dbm keys: ', d.keys()

d.close()
d = dbm.open(filename, 'r')
d.close()
d = dbm.open(filename, 'rw')
d.close()
d = dbm.open(filename, 'w')
d.close()
d = dbm.open(filename, 'n')
d.close()

try:
    import os
    if dbm.library == "ndbm":
        # classic dbm
        os.unlink(filename + '.dir')
        os.unlink(filename + '.pag')
    elif dbm.library == "BSD db":
        # BSD DB's compatibility layer
        os.unlink(filename + '.db')
    else:
        # GNU gdbm compatibility layer
        os.unlink(filename)
except:
    pass
