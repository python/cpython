"""Whimpy test script for the al module
   Roger E. Masse
"""
from test.test_support import verbose, import_module
al = import_module('al', deprecated=True)

alattrs = ['__doc__', '__name__', 'getdefault', 'getminmax', 'getname', 'getparams',
           'newconfig', 'openport', 'queryparams', 'setparams']

# This is a very unobtrusive test for the existence of the al module and all its
# attributes.  More comprehensive examples can be found in Demo/al

def test_main():
    # touch all the attributes of al without doing anything
    if verbose:
        print 'Touching al module attributes...'
    for attr in alattrs:
        if verbose:
            print 'touching: ', attr
        getattr(al, attr)


if __name__ == '__main__':
    test_main()
