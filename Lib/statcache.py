# Module 'statcache'
#
# Maintain a cache of file stats.
# There are functions to reset the cache or to selectively remove items.

import posix
from stat import *

# The cache.
# Keys are pathnames, values are `posix.stat' outcomes.
#
cache = {}


# Stat a file, possibly out of the cache.
#
def stat(path):
	if cache.has_key(path):
		return cache[path]
	cache[path] = ret = posix.stat(path)
	return ret


# Reset the cache completely.
# Hack: to reset a global variable, we import this module.
#
def reset():
	import statcache
	# Check that we really imported the same module
	if cache is not statcache.cache:
		raise 'sorry, statcache identity crisis'
	statcache.cache = {}


# Remove a given item from the cache, if it exists.
#
def forget(path):
	if cache.has_key(path):
		del cache[path]


# Remove all pathnames with a given prefix.
#
def forget_prefix(prefix):
	n = len(prefix)
	for path in cache.keys():
		if path[:n] = prefix:
			del cache[path]


# Forget about a directory and all entries in it, but not about
# entries in subdirectories.
#
def forget_dir(prefix):
	if prefix[-1:] = '/' and prefix <> '/':
		prefix = prefix[:-1]
	forget(prefix)
	if prefix[-1:] <> '/':
		prefix = prefix + '/'
	n = len(prefix)
	for path in cache.keys():
		if path[:n] = prefix:
			rest = path[n:]
			if rest[-1:] = '/': rest = rest[:-1]
			if '/' not in rest:
				del cache[path]


# Remove all pathnames except with a given prefix.
# Normally used with prefix = '/' after a chdir().
#
def forget_except_prefix(prefix):
	n = len(prefix)
	for path in cache.keys():
		if path[:n] <> prefix:
			del cache[path]


# Check for directory.
#
def isdir(path):
	try:
		st = stat(path)
	except posix.error:
		return 0
	return S_ISDIR(st[ST_MODE])
