#! /usr/bin/env python

"""The Tab Police watches for possibly inconsistent indentation."""

import os
import sys
import getopt
import string
import tokenize

verbose = 0

def main():
	global verbose
	try:
		opts, args = getopt.getopt(sys.argv[1:], "v")
	except getopt.error, msg:
		print msg
	for o, a in opts:
		if o == '-v':
			verbose = verbose + 1
	for arg in args:
		check(arg)

def check(file):
	if os.path.isdir(file) and not os.path.islink(file):
		if verbose:
			print "%s: listing directory" % `file`
		names = os.listdir(file)
		for name in names:
			fullname = os.path.join(file, name)
			if (os.path.isdir(fullname) and
			    not os.path.islink(fullname) or
			    os.path.normcase(name[-3:]) == ".py"):
				check(fullname)
		return

	try:
		f = open(file)
	except IOError, msg:
		print "%s: I/O Error: %s" % (`file`, str(msg))
		return

	if verbose > 1:
		print "checking", `file`, "with tabsize 8..."
	tokens = []
	tokenize.tabsize = 8
	try:
		tokenize.tokenize(f.readline, tokens.append)
	except tokenize.TokenError, msg:
		print "%s: Token Error: %s" % (`file`, str(msg))

	if verbose > 1:
		print "checking", `file`, "with tabsize 4..."
	f.seek(0)
	alttokens = []
	tokenize.tabsize = 4
	try:
		tokenize.tokenize(f.readline, alttokens.append)
	except tokenize.TokenError, msg:
		print "%s: Token Error: %s" % (`file`, str(msg))
	f.close()

	if tokens != alttokens:
		if verbose:
			print "%s: *** Trouble in tab city! ***" % `file`
		else:
			print file
	else:
		if verbose:
			print "%s: Clean bill of health." % `file`

if __name__ == '__main__':
	main()
