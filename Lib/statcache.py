# Module 'statcache'
#
# Maintain a cache of file stats.
# There are functions to reset the cache or to selectively remove items.

import posix


# The cache.
# Keys are pathnames, values are `posix.stat' outcomes.
#
cache = {}


# Stat a file, possibly out of the cache.
#
def stat(path):
	try:
		return cache[path]
	except RuntimeError:
		pass
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
	try:
		del cache[path]
	except RuntimeError:
		pass


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
		# mode is st[0]; type is mode/4096; S_IFDIR is 4
		return stat(path)[0] / 4096 = 4
	except RuntimeError:
		return 0
