import fnmatch
import os

_debug = 0

_prune = ['(*)']

def find(pattern, dir = os.curdir):
	list = []
	names = os.listdir(dir)
	names.sort()
	for name in names:
		if name in (os.curdir, os.pardir):
			continue
		fullname = os.path.join(dir, name)
		if fnmatch.fnmatch(name, pattern):
			list.append(fullname)
		if os.path.isdir(fullname) and not os.path.islink(fullname):
			for p in _prune:
				if fnmatch.fnmatch(name, p):
					if _debug: print "skip", `fullname`
					break
			else:
				if _debug: print "descend into", `fullname`
				list = list + find(pattern, fullname)
	return list
