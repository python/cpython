# Module 'packmail' -- create a shell script out of some files.

import mac
import macpath
from stat import ST_MTIME
import string

# Pack one file
def pack(outfp, file, name):
	fp = open(file, 'r')
	outfp.write('sed "s/^X//" >' + name + ' <<"!"\n')
	while 1:
		line = fp.readline()
		if not line: break
		if line[-1:] <> '\n':
			line = line + '\n'
		outfp.write('X' + line)
	outfp.write('!\n')

# Pack some files from a directory
def packsome(outfp, dirname, names):
	for name in names:
		print name
		file = macpath.join(dirname, name)
		pack(outfp, file, name)

# Pack all files from a directory
def packall(outfp, dirname):
	names = mac.listdir(dirname)
	names.sort()
	packsome(outfp, dirname, names)

# Pack all files from a directory that are not older than a give one
def packnotolder(outfp, dirname, oldest):
	names = mac.listdir(dirname)
	oldest = macpath.join(dirname, oldest)
	st = mac.stat(oldest)
	mtime = st[ST_MTIME]
	todo = []
	for name in names:
		print name, '...',
		st = mac.stat(macpath.join(dirname, name))
		if st[ST_MTIME] >= mtime:
			print 'Yes.'
			todo.append(name)
		else:
			print 'No.'
	todo.sort()
	packsome(outfp, dirname, todo)

# Pack a whole tree (no exceptions)
def packtree(outfp, dirname):
	print 'packtree', dirname
	outfp.write('mkdir ' + unixfix(dirname) + '\n')
	names = mac.listdir(dirname)
	subdirs = []
	for name in names:
		fullname = macpath.join(dirname, name)
		if macpath.isdir(fullname):
			subdirs.append(fullname)
		else:
			print 'pack', fullname
			pack(outfp, fullname, unixfix(fullname))
	for subdirname in subdirs:
		packtree(outfp, subdirname)

def unixfix(name):
	comps = string.splitfields(name, ':')
	res = ''
	for comp in comps:
		if comp:
			if res: res = res + '/'
			res = res + comp
	return res
