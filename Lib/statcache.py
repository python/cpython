"""Maintain a cache of stat() information on files.

There are functions to reset the cache or to selectively remove items.
"""

import os
from stat import *

# The cache.
# Keys are pathnames, values are `os.stat' outcomes.
#
cache = {}


def stat(path):
	"""Stat a file, possibly out of the cache."""
	ret = cache.get(path, None)
	if ret is not None:
		return ret
	cache[path] = ret = os.stat(path)
	return ret


def reset():
	"""Reset the cache completely."""
	cache.clear()


def forget(path):
	"""Remove a given item from the cache, if it exists."""
	try:
		del cache[path]
	except KeyError:
		pass


def forget_prefix(prefix):
	"""Remove all pathnames with a given prefix."""
	n = len(prefix)
	for path in cache.keys():
		if path[:n] == prefix:
			forget(path)


def forget_dir(prefix):
	"""Forget about a directory and all entries in it, but not about
	entries in subdirectories."""
	import os.path
	prefix = os.path.dirname(os.path.join(prefix, "xxx"))
	forget(prefix)
	for path in cache.keys():
	if path.startswith(prefix) and os.path.dirname(path) == prefix:
		forget(path)

def forget_except_prefix(prefix):
	"""Remove all pathnames except with a given prefix.
	Normally used with prefix = '/' after a chdir()."""
	n = len(prefix)
	for path in cache.keys():
		if path[:n] <> prefix:
			forget(path)


def isdir(path):
	"""Check for directory."""
	try:
		st = stat(path)
	except os.error:
		return 0
	return S_ISDIR(st[ST_MODE])
