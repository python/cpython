#! /usr/bin/env python
"""Test script for the grp module
   Roger E. Masse
"""
verbose = 0
if __name__ == '__main__':
    verbose = 1
    
import grp

groups = grp.getgrall()
if verbose:
    print 'Groups:'
    for group in groups:
	print group


group = grp.getgrgid(groups[0][2])
if verbose:
    print 'Group Entry for GID %d: %s' % (groups[0][2], group)

group = grp.getgrnam(groups[0][0])
if verbose:
    print 'Group Entry for group %s: %s' % (groups[0][0], group)
