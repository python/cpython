#! /usr/bin/env python
"""Test script for the dumbdbm module
   Original by Roger E. Masse
"""

# XXX This test is a disgrace.  It doesn't test that it works.

import dumbdbm as dbm
from dumbdbm import error
from test_support import verbose, TESTFN as filename

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
d = dbm.open(filename, 'w')
d.close()
d = dbm.open(filename, 'n')
d.close()

import os
def rm(fn):
    try:
        os.unlink(fn)
    except os.error:
        pass

rm(filename + '.dir')
rm(filename + '.dat')
rm(filename + '.bak')
