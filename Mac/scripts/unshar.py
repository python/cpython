# Extract files from a SHAR archive.
# Run this on the Mac.
# Usage:
# >>> import unshar
# >>> f = open('SHAR')
# >>> unshar.unshar(f)

import string

def unshar(fp, verbose=0, overwrite=0):
	ofp = None
	file = None
	while 1:
		line = fp.readline()
		if verbose > 3: print 'Got:', `line`
		if line[:1] == 'X':
			# Most common case first
			if ofp: ofp.write(line[1:])
			continue
		if not line:
			if verbose: print 'EOF'
			if ofp:
				print 'Unterminated file -- closing'
				ofp.close()
				ofp = None
			break
		if line[0] == '#':
			if verbose: print line,
			continue
		if line[:14] == 'sed "s/^X//" >':
			if verbose: print "!!!", `line`
			i = string.find(line, "'")
			j = string.find(line, "'", i+1)
			if i >= 0 and j > i:
				file = line[i+1:j]
				if '/' in file:
					words = string.splitfields(file, '/')
					for funny in '', '.':
						while funny in words: words.remove(funny)
					for i in range(len(words)):
						if words[i] == '..': words[i] = ''
					words.insert(0, '')
					file = string.joinfields(words, ':')
				try:
					ofp = open(file, 'r')
					ofp.close()
					ofp = None
					over = 1
				except IOError:
					over = 0
				if over and not overwrite:
					print 'Skipping', file, '(already exists) ...'
					continue
				ofp = open(file, 'w')
				if over:
					print 'Overwriting', file, '...'
				else:
					print 'Writing', file, '...'
			continue
		if line == 'END_OF_FILE\n':
			if not file:
				print 'Unexpected END_OF_FILE marker'
			if ofp:
				print 'done'
				ofp.close()
				ofp = None
			else:
				print 'done skipping'
			file = None
			continue
		if verbose: print "...", `line`
		
def main():
	import sys
	import os
	if len(sys.argv) > 1:
		for fname in sys.argv[1:]:
			fp = open(fname, 'r')
			dir, fn = os.path.split(fname)
			if dir:
				os.chdir(dir)
			unshar(fp)
	else:
		import macfs
		fss, ok = macfs.StandardGetFile('TEXT')
		if not ok:
			sys.exit(0)
		fname = fss.as_pathname()
		fp = open(fname, 'r')
		fss, ok = macfs.GetDirectory('Folder to save files in:')
		if not ok:
			sys.exit(0)
		os.chdir(fss.as_pathname())
		unshar(fp)
		
if __name__ == '__main__':
	main()
