"""findgremlins - Search through a folder and subfolders for
text files that have characters with bit 8 set, and print
the filename and a bit of context.

By Just, with a little glue by Jack"""

import macfs
import re
import os
import string
import sys

xpat = re.compile(r"[\200-\377]")

def walk(top, recurse=1):
	if os.path.isdir(top):
		if recurse:
			for name in os.listdir(top):
				path = os.path.join(top, name)
				walk(path)
	else:
		cr, tp = macfs.FSSpec(top).GetCreatorType()
		if tp == 'TEXT' and top[-4:] <> ".hqx":
			data = open(top).read()
			badcount = 0
			for ch in data[:256]:
				if ord(ch) == 0 or ord(ch) >= 0200:
					badcount = badcount + 1
			if badcount > 16:
				print `top`, 'appears to be a binary file'
				return
			pos = 0
			gotone = 0
			while 1:
				m = xpat.search(data, pos)
				if m is None:
					break
				if not gotone:
					print `top`
					gotone = 1
				[(i, j)] = m.regs
				print "     ", string.replace(data[i-15:j+15], '\n', ' ')
				pos = j

def main():
	fss, ok = macfs.GetDirectory()
	if ok:
		walk(fss.as_pathname())
		
if __name__ == '__main__':
	main()
	sys.exit(1) # So we see the output
	
