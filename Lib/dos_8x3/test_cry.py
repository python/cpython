#! /usr/bin/env python
"""Simple test script for cryptmodule.c
   Roger E. Masse
"""

from test_support import verbose    
import crypt

c = crypt.crypt('mypassword', 'ab')
if verbose:
    print 'Test encryption: ', c
