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
import EasyDialogs
import sys
import macostools
import MacOS

list = [
	('.py', 'Pyth', 'TEXT'),
	('.pyc', 'Pyth', 'PYC '),
	('.c', 'CWIE', 'TEXT'),
	('.h', 'CWIE', 'TEXT'),
	('.as', 'ToyS', 'TEXT'),
	('.hqx', 'BnHq', 'TEXT'),
	('.cmif', 'CMIF', 'TEXT'),
	('.cmc', 'CMIF', 'CMC '),
	('.aiff', 'SCPL', 'AIFF'),
	('.mpg', 'mMPG', 'MPEG'),
]

def walktree(name, change):
	if os.path.isfile(name):
		for ext, cr, tp in list:
			if name[-len(ext):] == ext:
				curcrtp = MacOS.GetCreatorAndType(name)
				if curcrtp <> (cr, tp):
					if change:
						MacOS.SetCreatorAndType(name, cr, tp)
						macostools.touched(fs)
						print 'Fixed ', name
					else:
						print 'Wrong', curcrtp, name
	elif os.path.isdir(name):
		print '->', name
		files = os.listdir(name)
		for f in files:
			walktree(os.path.join(name, f), change)
			
def run(change):
	pathname = EasyDialogs.AskFolder(message='Folder to search:')
	if not pathname:
		sys.exit(0)
	walktree(pathname, change)
	
if __name__ == '__main__':
	run(0)
	
	
