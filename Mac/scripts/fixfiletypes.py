#
# Fixfiletypes - Set mac filetypes to something sensible,
#  recursively down a directory tree.
#
# It will only touch extensions it feels pretty sure about.
# This script is useful after copying files from unix.
#
# Jack Jansen, CWI, 1995.
#
import os
import macfs
import sys

list = [
	('.py', 'PYTH', 'TEXT'),
	('.pyc', 'PYTH', 'PYC '),
	('.c', 'MPCC', 'TEXT'),
	('.h', 'MPCC', 'TEXT'),
	('.as', 'TEXT', 'ToyS'),
	('.hqx', 'TEXT', 'BnHq')
]

def walktree(name, change):
	if os.path.isfile(name):
		for ext, cr, tp in list:
			if name[-len(ext):] == ext:
				fs = macfs.FSSpec(name)
				curcrtp = fs.GetCreatorType()
				if curcrtp <> (cr, tp):
					if change:
						fs.SetCreatorType(cr, tp)
						print 'Fixed ', name
					else:
						print 'Wrong', curcrtp, name
	elif os.path.isdir(name):
		print '->', name
		files = os.listdir(name)
		for f in files:
			walktree(os.path.join(name, f), change)
			
def run(change):
	fss, ok = macfs.GetDirectory()
	if not ok:
		sys.exit(0)
	walktree(fss.as_pathname(), change)
	
if __name__ == '__main__':
	run(1)
	
	
