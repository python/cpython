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
	if cache.has_key(path):
		return cache[path]
	cache[path] = ret = os.stat(path)
	return ret


def reset():
	"""Reset the cache completely."""
	global cache
	cache = {}


def forget(path):
	"""Remove a given item from the cache, if it exists."""
	if cache.has_key(path):
		del cache[path]


def forget_prefix(prefix):
	"""Remove all pathnames with a given prefix."""
	n = len(prefix)
	for path in cache.keys():
		if path[:n] == prefix:
			del cache[path]


def forget_dir(prefix):
	"""Forget about a directory and all entries in it, but not about
	entries in subdirectories."""
	if prefix[-1:] == '/' and prefix <> '/':
		prefix = prefix[:-1]
	forget(prefix)
	if prefix[-1:] <> '/':
		prefix = prefix + '/'
	n = len(prefix)
	for path in cache.keys():
		if path[:n] == prefix:
			rest = path[n:]
			if rest[-1:] == '/': rest = rest[:-1]
			if '/' not in rest:
				del cache[path]


def forget_except_prefix(prefix):
	"""Remove all pathnames except with a given prefix.
	Normally used with prefix = '/' after a chdir()."""
	n = len(prefix)
	for path in cache.keys():
		if path[:n] <> prefix:
			del cache[path]


def isdir(path):
	"""Check for directory."""
	try:
		st = stat(path)
	except os.error:
		return 0
	return S_ISDIR(st[ST_MODE])
