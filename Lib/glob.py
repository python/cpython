"""Filename globbing utility."""

import os
import fnmatch
import re


def glob(pathname):
	"""Return a list of paths matching a pathname pattern.

	The pattern may contain simple shell-style wildcards a la fnmatch.

	"""
	if not has_magic(pathname):
		if os.path.exists(pathname):
			return [pathname]
		else:
			return []
	dirname, basename = os.path.split(pathname)
	if has_magic(dirname):
		list = glob(dirname)
	else:
		list = [dirname]
	if not has_magic(basename):
		result = []
		for dirname in list:
			if basename or os.path.isdir(dirname):
				name = os.path.join(dirname, basename)
				if os.path.exists(name):
					result.append(name)
	else:
		result = []
		for dirname in list:
			sublist = glob1(dirname, basename)
			for name in sublist:
				result.append(os.path.join(dirname, name))
	return result

def glob1(dirname, pattern):
	if not dirname: dirname = os.curdir
	try:
		names = os.listdir(dirname)
	except os.error:
		return []
	result = []
	for name in names:
		if name[0] != '.' or pattern[0] == '.':
			if fnmatch.fnmatch(name, pattern):
				result.append(name)
	return result


magic_check = re.compile('[*?[]')

def has_magic(s):
	return magic_check.search(s) is not None
