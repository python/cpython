#! /usr/bin/env python

"""The Tab Police watches for possibly inconsistent indentation."""

import os
import sys
import getopt
import tokenize

verbose = 0
quiet = 0

def main():
	global verbose, quiet
	try:
		opts, args = getopt.getopt(sys.argv[1:], "qv")
	except getopt.error, msg:
		print msg
	for o, a in opts:
		if o == '-v':
			verbose = verbose + 1
			quiet = 0
		if o == '-q':
			quiet = 1
			verbose = 0
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
		print "checking", `file`, "with tabsize 1..."
	f.seek(0)
	alttokens = []
	tokenize.tabsize = 1
	try:
		tokenize.tokenize(f.readline, alttokens.append)
	except tokenize.TokenError, msg:
		print "%s: Token Error: %s" % (`file`, str(msg))
	f.close()

	if tokens == alttokens:
		if verbose:
			print "%s: Clean bill of health." % `file`
	elif quiet:
		print file
	else:
		badline = 0
		n, altn = len(tokens), len(alttokens)
		for i in range(max(n, altn)):
			if i >= n:
				x = None
			else:
				x = tokens[i]
			if i >= altn:
				y = None
			else:
				y = alttokens[i]
			if x != y:
				if x:
					kind, token, start, end, line = x
				else:
					kind, token, start, end, line = y
				badline = start[0]
				break
		if verbose:
			print "%s: *** Line %d: trouble in tab city! ***" % (
				`file`, badline)
			print "offending line:", `line`
		else:
			print file, badline, `line`

if __name__ == '__main__':
	main()
