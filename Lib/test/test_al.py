#! /usr/bin/env python
"""Whimpy test script for the al module
   Roger E. Masse
"""
import al
from test_support import verbose

alattrs = ['__doc__', '__name__', 'getdefault', 'getminmax', 'getname', 'getparams',
           'newconfig', 'openport', 'queryparams', 'setparams']

# This is a very unobtrusive test for the existence of the al module and all it's
# attributes.  More comprehensive examples can be found in Demo/al

def main():
    # touch all the attributes of al without doing anything
    if verbose:
        print 'Touching al module attributes...'
    for attr in alattrs:
        if verbose:
            print 'touching: ', attr
        getattr(al, attr)

main()
