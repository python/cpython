#
# FixCreator - Search for files with PYTH creator
# and set it to Pyth.
#
import os
import macfs
import sys
import macostools

OLD='PYTH'
NEW='Pyth'

def walktree(name, change):
	if os.path.isfile(name):
		fs = macfs.FSSpec(name)
		cur_cr, cur_tp = fs.GetCreatorType()
		if cur_cr == OLD:
			fs.SetCreatorType(NEW, cur_tp)
			macostools.touched(fs)
			print 'Fixed ', name
	elif os.path.isdir(name):
		print '->', name
		files = os.listdir(name)
		for f in files:
			walktree(os.path.join(name, f), change)
			
def run(change):
	fss, ok = macfs.GetDirectory('Folder to search:')
	if not ok:
		sys.exit(0)
	walktree(fss.as_pathname(), change)
	
if __name__ == '__main__':
	run(1)
	
	
