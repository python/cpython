# module 'string' -- A collection of string operations

# XXX Some of these operations are incredibly slow and should be built in

# Some strings for ctype-style character classification
whitespace = ' \t\n'
lowercase = 'abcdefghijklmnopqrstuvwxyz'
uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
letters = lowercase + uppercase
digits = '0123456789'
hexdigits = digits + 'abcdef' + 'ABCDEF'
octdigits = '01234567'

# Case conversion helpers
_caseswap = {}
for i in range(26):
	_caseswap[lowercase[i]] = uppercase[i]
	_caseswap[uppercase[i]] = lowercase[i]
del i

# convert UPPER CASE letters to lower case
def lower(s):
	res = ''
	for c in s:
		if 'A' <= c <= 'Z': c = _caseswap[c]
		res = res + c
	return res

# Convert lower case letters to UPPER CASE
def upper(s):
	res = ''
	for c in s:
		if 'a' <= c <= 'z': c = _caseswap[c]
		res = res + c
	return res

# Swap lower case letters and UPPER CASE
def swapcase(s):
	res = ''
	for c in s:
		if 'a' <= c <= 'z' or 'A' <= c <= 'Z': c = _caseswap[c]
		res = res + c
	return res

# Strip leading and trailing tabs and spaces
def strip(s):
	i, j = 0, len(s)
	while i < j and s[i] in whitespace: i = i+1
	while i < j and s[j-1] in whitespace: j = j-1
	return s[i:j]

# Split a string into a list of space/tab-separated words
# NB: split(s) is NOT the same as splitfields(s, ' ')!
def split(s):
	res = []
	i, n = 0, len(s)
	while i < n:
		while i < n and s[i] in whitespace: i = i+1
		if i = n: break
		j = i
		while j < n and s[j] not in whitespace: j = j+1
		res.append(s[i:j])
		i = j
	return res

# Split a list into fields separated by a given string
# NB: splitfields(s, ' ') is NOT the same as split(s)!
def splitfields(s, sep):
	res = []
	ns = len(s)
	nsep = len(sep)
	i = j = 0
	while j+nsep <= ns:
		if s[j:j+nsep] = sep:
			res.append(s[i:j])
			i = j = j + nsep
		else:
			j = j + 1
	res.append(s[i:])
	return res

# Find substring
index_error = 'substring not found in string.index'
def index(s, sub):
	n = len(sub)
	for i in range(len(s) + 1 - n):
		if sub = s[i:i+n]: return i
	raise index_error, (s, sub)

# Convert string to integer
atoi_error = 'non-numeric argument to string.atoi'
def atoi(str):
	s = str
	if s[:1] in '+-': s = s[1:]
	if not s: raise atoi_error, str
	for c in s:
		if c not in digits: raise atoi_error, str
	return eval(str)

# Left-justify a string
def ljust(s, width):
	n = len(s)
	if n >= width: return s
	return s + ' '*(width-n)

# Right-justify a string
def rjust(s, width):
	n = len(s)
	if n >= width: return s
	return ' '*(width-n) + s

# Center a string
def center(s, width):
	n = len(s)
	if n >= width: return s
	return ' '*((width-n)/2) +  s + ' '*(width -(width-n)/2)

# Zero-fill a number, e.g., (12, 3) --> '012' and (-3, 3) --> '-03'
# Decadent feature: the argument may be a string or a number
# (Use of this is deprecated; it should be a string as with ljust c.s.)
def zfill(x, width):
	if type(x) = type(''): s = x
	else: s = `x`
	n = len(s)
	if n >= width: return s
	sign = ''
	if s[0] = '-':
		sign, s = '-', s[1:]
	return sign + '0'*(width-n) + s
