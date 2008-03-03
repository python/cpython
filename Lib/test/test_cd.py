#! /usr/bin/env python
"""Whimpy test script for the cd module
   Roger E. Masse
"""
import cd
from test.test_support import verbose

cdattrs = ['BLOCKSIZE', 'CDROM', 'DATASIZE', 'ERROR', 'NODISC', 'PAUSED', 'PLAYING', 'READY',
           'STILL', '__doc__', '__name__', 'atime', 'audio', 'catalog', 'control', 'createparser', 'error',
           'ident', 'index', 'msftoframe', 'open', 'pnum', 'ptime']


# This is a very inobtrusive test for the existence of the cd module and all its
# attributes.  More comprehensive examples can be found in Demo/cd and
# require that you have a CD and a CD ROM drive

def test_main():
    # touch all the attributes of cd without doing anything
    if verbose:
        print 'Touching cd module attributes...'
    for attr in cdattrs:
        if verbose:
            print 'touching: ', attr
        getattr(cd, attr)



if __name__ == '__main__':
    test_main()
