#! /usr/local/python

# linktree
#
# Make a copy of a directory tree with symbolic links to all files in the
# original tree.
# All symbolic links go to a special symbolic link at the top, so you
# can easily fix things if the original source tree moves.
# See also "mkreal".
#
# usage: mklinks oldtree newtree

import sys, posix, path

LINK = '.LINK' # Name of special symlink at the top.

debug = 0

def main():
	if not 3 <= len(sys.argv) <= 4:
		print 'usage:', sys.argv[0], 'oldtree newtree [linkto]'
		return 2
	oldtree, newtree = sys.argv[1], sys.argv[2]
	if len(sys.argv) > 3:
		link = sys.argv[3]
		link_may_fail = 1
	else:
		link = LINK
		link_may_fail = 0
	if not path.isdir(oldtree):
		print oldtree + ': not a directory'
		return 1
	try:
		posix.mkdir(newtree, 0777)
	except posix.error, msg:
		print newtree + ': cannot mkdir:', msg
		return 1
	linkname = path.cat(newtree, link)
	try:
		posix.symlink(path.cat('..', oldtree), linkname)
	except posix.error, msg:
		if not link_may_fail:
			print linkname + ': cannot symlink:', msg
			return 1
		else:
			print linkname + ': warning: cannot symlink:', msg
	linknames(oldtree, newtree, link)
	return 0

def linknames(old, new, link):
	if debug: print 'linknames', (old, new, link)
	try:
		names = posix.listdir(old)
	except posix.error, msg:
		print old + ': warning: cannot listdir:', msg
		return
	for name in names:
	    if name not in ('.', '..'):
		oldname = path.cat(old, name)
		linkname = path.cat(link, name)
		newname = path.cat(new, name)
		if debug > 1: print oldname, newname, linkname
		if path.isdir(oldname) and not path.islink(oldname):
			try:
				posix.mkdir(newname, 0777)
				ok = 1
			except:
				print newname + ': warning: cannot mkdir:', msg
				ok = 0
			if ok:
				linkname = path.cat('..', linkname)
				linknames(oldname, newname, linkname)
		else:
			posix.symlink(linkname, newname)

sys.exit(main())
