# Module 'glob' -- filename globbing.

import posix
import path
import fnmatch

def glob(pathname):
	if not has_magic(pathname): return [pathname]
	dirname, basename = path.split(pathname)
	if dirname[-1:] = '/' and dirname <> '/':
		dirname = dirname[:-1]
	if has_magic(dirname):
		list = glob(dirname)
	else:
		list = [dirname]
	if not has_magic(basename):
		result = []
		for dirname in list:
			if basename or path.isdir(dirname):
				name = path.cat(dirname, basename)
				if path.exists(name):
					result.append(name)
	else:
		result = []
		for dirname in list:
			sublist = glob1(dirname, basename)
			for name in sublist:
				result.append(path.cat(dirname, name))
	return result

def glob1(dirname, pattern):
	if not dirname: dirname = '.'
	try:
		names = posix.listdir(dirname)
	except posix.error:
		return []
	result = []
	for name in names:
		if name[0] <> '.' or pattern[0] = '.':
			if fnmatch.fnmatch(name, pattern): result.append(name)
	return result

def has_magic(s):
	return '*' in s or '?' in s or '[' in s
