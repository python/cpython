#! /usr/bin/env python
"""Test script for the binhex C module

   Uses the mechanism of the python binhex module
   Roger E. Masse
"""
import binhex
import tempfile
from test_support import verbose, TestSkipped

def test():

    try:
        fname1 = tempfile.mktemp()
        fname2 = tempfile.mktemp()
        f = open(fname1, 'w')
    except:
        raise TestSkipped, "Cannot test binhex without a temp file"

    start = 'Jack is my hero'
    f.write(start)
    f.close()
    
    binhex.binhex(fname1, fname2)
    if verbose:
        print 'binhex'

    binhex.hexbin(fname2, fname1)
    if verbose:
        print 'hexbin'

    f = open(fname1, 'r')
    finish = f.readline()

    if start <> finish:
        print 'Error: binhex <> hexbin'
    elif verbose:
        print 'binhex == hexbin'

    try:
        import os
        os.unlink(fname1)
        os.unlink(fname2)
    except:
        pass
test()
