"""Make the finder open an application, file or folder"""

import Finder_7_0_Suite
import aetools
import MacOS
import sys
import macfs

SIGNATURE='MACS'

class Finder(aetools.TalkTo, Finder_7_0_Suite.Finder_7_0_Suite):
	pass
	
def open_in_finder(file):
	"""Open a file thru the finder. Specify file by name or fsspec"""
	finder = Finder(SIGNATURE)
	fss = macfs.FSSpec(file)
	vRefNum, parID, name = fss.as_tuple()
	dir_fss = macfs.FSSpec((vRefNum, parID, ''))
	file_alias = fss.NewAlias()
	dir_alias = dir_fss.NewAlias()
	return finder.open(file_alias, items=[file_alias])

def main():
	fss, ok = macfs.PromptGetFile('File to launch:')
	if not ok: sys.exit(0)
	result = open_in_finder(fss)
	if result:
		print 'Result: ', result
		
if __name__ == '__main__':
	main()
	
