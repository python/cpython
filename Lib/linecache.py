# Cache lines from files.

import os
from stat import *

def getline(filename, lineno):
	lines = getlines(filename)
	if 1 <= lineno <= len(lines):
		return lines[lineno-1]
	else:
		return ''


# The cache

cache = {} # The cache


# Clear the cache entirely

def clearcache():
	global cache
	cache = {}


# Get the lines for a file from the cache.
# Update the cache if it doesn't contain an entry for this file already.

def getlines(filename):
	if cache.has_key(filename):
		return cache[filename][2]
	else:
		return updatecache(filename)


# Discard cache entries that are out of date.
# (This is not checked upon each call

def checkcache():
	for filename in cache.keys():
		size, mtime, lines = cache[filename]
		try:	stat = os.stat(filename)
		except os.error:
			del cache[filename]
			continue
		if size <> stat[ST_SIZE] or mtime <> stat[ST_MTIME]:
			del cache[filename]


# Update a cache entry and return its list of lines.
# If something's wrong, print a message, discard the cache entry,
# and return an empty list.

def updatecache(filename):
	try:	del cache[filename]
	except KeyError: pass
	try:	stat = os.stat(filename)
	except os.error, msg:
		if filename[0] + filename[-1] <> '<>':
			print '*** Cannot stat', filename, ':', msg
		return []
	try:
		fp = open(filename, 'r')
		lines = fp.readlines()
		fp.close()
	except IOError, msg:
		print '*** Cannot open', filename, ':', msg
		return []
	size, mtime = stat[ST_SIZE], stat[ST_MTIME]
	cache[filename] = size, mtime, lines
	return lines
