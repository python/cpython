# Module 'dircache'
#
# Return a sorted list of the files in a POSIX directory, using a cache
# to avoid reading the directory more often than necessary.
# Also contains a subroutine to append slashes to directories.

import posix
import path

cache = {}

def listdir(path): # List directory contents, using cache
	try:
		cached_mtime, list = cache[path]
		del cache[path]
	except RuntimeError:
		cached_mtime, list = -1, []
	try:
		mtime = posix.stat(path)[8]
	except posix.error:
		return []
	if mtime <> cached_mtime:
		try:
			list = posix.listdir(path)
		except posix.error:
			return []
		list.sort()
	cache[path] = mtime, list
	return list

opendir = listdir # XXX backward compatibility

def annotate(head, list): # Add '/' suffixes to directories
	for i in range(len(list)):
		if path.isdir(path.cat(head, list[i])):
			list[i] = list[i] + '/'
