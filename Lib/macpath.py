# module 'macpath' -- pathname (or -related) operations for the Macintosh

import mac

from stat import *


# Return true if a path is absolute.
# On the Mac, relative paths begin with a colon,
# but as a special case, paths with no colons at all are also relative.
# Anything else is absolute (the string up to the first colon is the
# volume name).

def isabs(s):
	return ':' in s and s[0] <> ':'


# Concatenate two pathnames.
# The result is equivalent to what the second pathname would refer to
# if the first pathname were the current directory.

def cat(s, t):
	if (not s) or isabs(t): return t
	if t[:1] = ':': t = t[1:]
	if ':' not in s:
		s = ':' + s
	if s[-1:] <> ':':
		s = s + ':'
	return s + t


# Split a pathname in two parts: the directory leading up to the final bit,
# and the basename (the filename, without colons, in that directory).
# The result (s, t) is such that cat(s, t) yields the original argument.

def split(s):
	if ':' not in s: return '', s
	colon = 0
	for i in range(len(s)):
		if s[i] = ':': colon = i+1
	return s[:colon], s[colon:]


# Normalize a pathname: get rid of '::' sequences by backing up,
# e.g., 'foo:bar::bletch' becomes 'foo:bletch'.
# Raise the exception norm_error below if backing up is impossible,
# e.g., for '::foo'.

norm_error = 'macpath.norm_error: path cannot be normalized'

def norm(s):
	import string
	if ':' not in s:
		return ':' + s
	f = string.splitfields(s, ':')
	pre = []
	post = []
	if not f[0]:
		pre = f[:1]
		f = f[1:]
	if not f[len(f)-1]:
		post = f[-1:]
		f = f[:-1]
	res = []
	for seg in f:
		if seg:
			res.append(seg)
		else:
			if not res: raise norm_error, 'path starts with ::'
			del res[len(res)-1]
			if not (pre or res):
				raise norm_error, 'path starts with volume::'
	if pre: res = pre + res
	if post: res = res + post
	s = res[0]
	for seg in res[1:]:
		s = s + ':' + seg
	return s


# Return true if the pathname refers to an existing directory.

def isdir(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return S_ISDIR(st[ST_MODE])


# Return true if the pathname refers to an existing regular file.

def isfile(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return S_ISREG(st[ST_MODE])


# Return true if the pathname refers to an existing file or directory.

def exists(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return 1
