#! /usr/bin/env python

"""Create a list of files that are mentioned in CVS directories."""

import os
import sys
import string

def main():
    args = sys.argv[1:]
    if args:
	for arg in args:
	    process(arg)
    else:
	process(".")

def process(dir):
    cvsdir = 0
    subdirs = []
    names = os.listdir(dir)
    for name in names:
	fullname = os.path.join(dir, name)
	if name == "CVS":
	    cvsdir = fullname
	else:
	    if os.path.isdir(fullname):
		if not os.path.islink(fullname):
		    subdirs.append(fullname)
    if cvsdir:
	entries = os.path.join(cvsdir, "Entries")
	for e in open(entries).readlines():
	    words = string.split(e, '/')
	    if words[0] == '' and words[1:]:
		name = words[1]
		print os.path.join(dir, name)
    for sub in subdirs:
	process(sub)

main()
