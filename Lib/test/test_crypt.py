#! /usr/bin/env python
"""Simple test script for cryptmodule.c
   Roger E. Masse
"""
verbose = 0
if __name__ == '__main__':
    verbose = 1
    
import crypt
c = crypt.crypt('mypassword', 'ab')
if verbose:
    print 'Test encryption: ', c
