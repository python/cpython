# module 'macpath'

import mac

import string

from stat import *

def isabs(s):
	return ':' in s and s[0] <> ':'

def cat(s, t):
	if (not s) or isabs(t): return t
	if t[:1] = ':': t = t[1:]
	if ':' not in s:
		s = ':' + s
	if s[-1:] <> ':':
		s = s + ':'
	return s + t

norm_error = 'path cannot be normalized'

def norm(s):
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
			if not res: raise norm_error # starts with '::'
			del res[len(res)-1]
			if not (pre or res):
				raise norm_error # starts with 'vol::'
	if pre: res = pre + res
	if post: res = res + post
	s = res[0]
	for seg in res[1:]:
		s = s + ':' + seg
	return s

def isdir(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return S_ISDIR(st[ST_MODE])

def isfile(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return S_ISREG(st[ST_MODE])

def exists(s):
	try:
		st = mac.stat(s)
	except mac.error:
		return 0
	return 1
