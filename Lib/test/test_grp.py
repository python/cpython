#! /usr/bin/env python
"""Test script for the grp module
   Roger E. Masse
"""
  
import grp
from test_support import verbose

groups = grp.getgrall()
if verbose:
    print 'Groups:'
    for group in groups:
        print group

if not groups:
    if verbose:
        print "Empty Group Database -- no further tests of grp module possible"
else:
    group = grp.getgrgid(groups[0][2])
    if verbose:
        print 'Group Entry for GID %d: %s' % (groups[0][2], group)

    group = grp.getgrnam(groups[0][0])
    if verbose:
        print 'Group Entry for group %s: %s' % (groups[0][0], group)
