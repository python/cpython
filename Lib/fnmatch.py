# module 'fnmatch' -- filename matching with shell patterns

# XXX [] patterns are not supported (but recognized)

def fnmatch(name, pat):
	if '*' in pat or '?' in pat or '[' in pat:
		return fnmatch1(name, pat)
	return name = pat

def fnmatch1(name, pat):
	for i in range(len(pat)):
		c = pat[i]
		if c = '*':
			restpat = pat[i+1:]
			if '*' in restpat or '?' in restpat or '[' in restpat:
				for i in range(i, len(name)):
					if fnmatch1(name[i:], restpat):
						return 1
				return 0
			else:
				return name[len(name)-len(restpat):] = restpat
		elif c = '?':
			if len(name) <= i : return 0
		elif c = '[':
			return 0 # XXX
		else:
			if name[i:i+1] <> c:
				return 0
	return 1

def fnmatchlist(names, pat):
	res = []
	for name in names:
		if fnmatch(name, pat): res.append(name)
	return res
