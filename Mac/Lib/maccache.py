# Module 'maccache'
#
# Maintain a cache of listdir(), isdir(), isfile() or exists() outcomes.
# XXX Should merge with module statcache

import os


# The cache.
# Keys are absolute pathnames;
# values are 0 (nothing), 1 (file) or [...] (dir).
#
cache = {}


# Current working directory.
#
cwd = os.getcwd()


# Constants.
#
NONE = 0
FILE = 1
LISTTYPE = type([])

def _stat(name):
	name = os.path.join(cwd, name)
	if cache.has_key(name):
		return cache[name]
	if os.path.isfile(name):
		cache[name] = FILE
		return FILE
	try:
		list = os.listdir(name)
	except:
		cache[name] = NONE
		return NONE
	cache[name] = list
	if name[-1:] == ':': cache[name[:-1]] = list
	else: cache[name+':'] = list
	return list

def isdir(name):
	st = _stat(name)
	return type(st) == LISTTYPE

def isfile(name):
	st = _stat(name)
	return st == FILE

def exists(name):
	st = _stat(name)
	return st <> NONE

def listdir(name):
	st = _stat(name)
	if type(st) == LISTTYPE:
		return st
	else:
		raise RuntimeError, 'list non-directory'
