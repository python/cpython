#! /usr/bin/env python
"""Simple test script for cryptmodule.c
   Roger E. Masse
"""

from test.test_support import verify, verbose
import crypt

c = crypt.crypt('mypassword', 'ab')
if verbose:
    print 'Test encryption: ', c
