# Module 'packmail' -- create a self-unpacking shell archive.

# This module works on UNIX and on the Mac; the archives can unpack
# themselves only on UNIX.

import os
from stat import ST_MTIME
import string

# Print help
def help():
	print 'All fns have a file open for writing as first parameter'
	print 'pack(f, fullname, name): pack fullname as name'
	print 'packsome(f, directory, namelist): selected files from directory'
	print 'packall(f, directory): pack all files from directory'
	print 'packnotolder(f, directory, name): pack all files from directory'
	print '                        that are not older than a file there'
	print 'packtree(f, directory): pack entire directory tree'

# Pack one file
def pack(outfp, file, name):
	fp = open(file, 'r')
	outfp.write('echo ' + name + '\n')
	outfp.write('sed "s/^X//" >"' + name + '" <<"!"\n')
	while 1:
		line = fp.readline()
		if not line: break
		if line[-1:] <> '\n':
			line = line + '\n'
		outfp.write('X' + line)
	outfp.write('!\n')
	fp.close()

# Pack some files from a directory
def packsome(outfp, dirname, names):
	for name in names:
		print name
		file = os.path.join(dirname, name)
		pack(outfp, file, name)

# Pack all files from a directory
def packall(outfp, dirname):
	names = os.listdir(dirname)
	try:
	    names.remove('.')
	except:
	    pass
	try:
	    names.remove('..')
	except:
	    pass
	names.sort()
	packsome(outfp, dirname, names)

# Pack all files from a directory that are not older than a give one
def packnotolder(outfp, dirname, oldest):
	names = os.listdir(dirname)
	try:
	    names.remove('.')
	except:
	    pass
	try:
	    names.remove('..')
	except:
	    pass
	oldest = os.path.join(dirname, oldest)
	st = os.stat(oldest)
	mtime = st[ST_MTIME]
	todo = []
	for name in names:
		print name, '...',
		st = os.stat(os.path.join(dirname, name))
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
	names = os.listdir(dirname)
	try:
	    names.remove('.')
	except:
	    pass
	try:
	    names.remove('..')
	except:
	    pass
	subdirs = []
	for name in names:
		fullname = os.path.join(dirname, name)
		if os.path.isdir(fullname):
			subdirs.append(fullname)
		else:
			print 'pack', fullname
			pack(outfp, fullname, unixfix(fullname))
	for subdirname in subdirs:
		packtree(outfp, subdirname)

def unixfix(name):
	comps = string.splitfields(name, os.sep)
	res = ''
	for comp in comps:
		if comp:
			if res: res = res + '/'
			res = res + comp
	return res
