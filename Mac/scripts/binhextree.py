#
# hexbintree - Recursively descend a directory and
# pack all resource files.
#
# Jack Jansen, CWI, August 1995.
#
# To do:
# - Also do project files (.µ and .¹), after using AppleEvents to the
#   various builders to clean the projects
# - Don't hexbin (and clean) if there exists a .hqx file that is newer.
#

import os
import binhex
import sys

extensions = ['.rsrc']

def walker(arg, top, names):
	for n in names:
		for ext in extensions:
			if n[-len(ext):] == ext:
				name = os.path.join(top, n)
				print 'Binhexing', name
				binhex.binhex(name, name + '.hqx')
				
def dodir(name):
	os.path.walk(name, walker, None)
				
def main():
	if len(sys.argv) > 1:
		for dir in sys.argv[1:]:
			dodir(dir)
	elif os.name == 'mac':
		import macfs
		dir, ok = macfs.GetDirectory('Folder to search:')
		if not ok:
			sys.exit(0)
		dodir(dir.as_pathname())
	else:
		print 'Usage: hexbintree dir ...'
		sys.exit(1)
	if os.name == 'mac':
		sys.exit(1)   # Keep window
	else:
		sys.exit(0)
		
if __name__ == '__main__':
	main()
	
