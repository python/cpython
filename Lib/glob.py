# Module 'glob' -- filename globbing.

import os
import fnmatch


def glob(pathname):
	if not has_magic(pathname): return [pathname]
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
		if name[0] <> '.' or pattern[0] == '.':
			if fnmatch.fnmatch(name, pattern): result.append(name)
	return result

def has_magic(s):
	return '*' in s or '?' in s or '[' in s
