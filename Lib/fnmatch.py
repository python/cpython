# module 'fnmatch' -- filename matching with shell patterns

def fnmatch(name, pat):
	#
	# Check for simple case: no special characters
	#
	if not ('*' in pat or '?' in pat or '[' in pat):
		return name = pat
	#
	# Check for common cases: *suffix and prefix*
	#
	if pat[0] = '*':
		p1 = pat[1:]
		if not ('*' in p1 or '?' in p1 or '[' in p1):
			start = len(name) - len(p1)
			return start >= 0 and name[start:] = p1
	elif pat[-1:] = '*':
		p1 = pat[:-1]
		if not ('*' in p1 or '?' in p1 or '[' in p1):
			return name[:len(p1)] = p1
	#
	# General case
	#
	return fnmatch1(name, pat)

def fnmatch1(name, pat):
	for i in range(len(pat)):
		c = pat[i]
		if c = '*':
			p1 = pat[i+1:]
			if not ('*' in p1 or '?' in p1 or '[' in p1):
				start = len(name) - len(p1)
				return start >= 0 and name[start:] = p1
			for i in range(i, len(name) + 1):
				if fnmatch1(name[i:], p1):
					return 1
			return 0
		elif c = '?':
			if len(name) <= i : return 0
		elif c = '[':
			c, rest = name[i], name[i+1:]
			i, n = i+1, len(pat) - 1
			match = 0
			exclude = 0
			if i < n and pat[i] = '!':
				exclude = 1
				i = i+1
			while i < n:
				if pat[i] = c: match = 1
				i = i+1
				if i >= n or pat[i] = ']':
					break
				if pat[i] = '-':
					i = i+1
					if i >= n or pat[i] = ']':
						break
					match = (pat[i-2] <= c <= pat[i])
					i = i+1
			if match = exclude:
				return 0
			return fnmatch1(rest, pat[i+1:])
		else:
			if name[i:i+1] <> c:
				return 0
	return 1

def fnmatchlist(names, pat):
	res = []
	for name in names:
		if fnmatch(name, pat): res.append(name)
	return res
