"""Checkversions - recursively search a directory (default: sys.prefix) 
for _checkversion.py files, and run each of them. This will tell you of
new versions available for any packages you have installed."""

import os
import getopt
import sys
import string
import pyversioncheck

CHECKNAME="_checkversion.py"

VERBOSE=1

USAGE="""Usage: checkversions [-v verboselevel] [dir ...]
Recursively examine a tree (default: sys.prefix) and for each package
with a _checkversion.py file compare the installed version against the current
version.

Values for verboselevel:
0 - Minimal output, one line per package
1 - Also print descriptions for outdated packages (default)
2 - Print information on each URL checked
3 - Check every URL for packages with multiple locations"""

def check1dir(dummy, dir, files):
	if CHECKNAME in files:
		fullname = os.path.join(dir, CHECKNAME)
		try:
			execfile(fullname)
		except:
			print '** Exception in', fullname
			
def walk1tree(tree):
	os.path.walk(tree, check1dir, None)
	
def main():
	global VERBOSE
	try:
		options, arguments = getopt.getopt(sys.argv[1:], 'v:')
	except getopt.error:
		print USAGE
		sys.exit(1)
	for o, a in options:
		if o == '-v':
			VERBOSE = string.atoi(a)
	if not arguments:
		arguments = [sys.prefix]
	for dir in arguments:
		walk1tree(dir)
		
if __name__ == '__main__':
	main()
		
	
