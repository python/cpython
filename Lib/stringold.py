# module 'string' -- A collection of string operations

# Warning: most of the code you see here isn't normally used nowadays.
# At the end of this file most functions are replaced by built-in
# functions imported from built-in module "strop".

# Some strings for ctype-style character classification
whitespace = ' \t\n\r\v\f'
lowercase = 'abcdefghijklmnopqrstuvwxyz'
uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
letters = lowercase + uppercase
digits = '0123456789'
hexdigits = digits + 'abcdef' + 'ABCDEF'
octdigits = '01234567'

# Case conversion helpers
_idmap = ''
for i in range(256): _idmap = _idmap + chr(i)
_lower = _idmap[:ord('A')] + lowercase + _idmap[ord('Z')+1:]
_upper = _idmap[:ord('a')] + uppercase + _idmap[ord('z')+1:]
_swapcase = _upper[:ord('A')] + lowercase + _upper[ord('Z')+1:]
del i

# convert UPPER CASE letters to lower case
def lower(s):
	res = ''
	for c in s:
		res = res + _lower[ord(c)]
	return res

# Convert lower case letters to UPPER CASE
def upper(s):
	res = ''
	for c in s:
		res = res + _upper[ord(c)]
	return res

# Swap lower case letters and UPPER CASE
def swapcase(s):
	res = ''
	for c in s:
		res = res + _swapcase[ord(c)]
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
		if i == n: break
		j = i
		while j < n and s[j] not in whitespace: j = j+1
		res.append(s[i:j])
		i = j
	return res

# Split a list into fields separated by a given string
# NB: splitfields(s, ' ') is NOT the same as split(s)!
# splitfields(s, '') returns [s] (in analogy with split() in nawk)
def splitfields(s, sep):
	res = []
	nsep = len(sep)
	if nsep == 0:
		return [s]
	ns = len(s)
	i = j = 0
	while j+nsep <= ns:
		if s[j:j+nsep] == sep:
			res.append(s[i:j])
			i = j = j + nsep
		else:
			j = j + 1
	res.append(s[i:])
	return res

# Join words with spaces between them
def join(words):
	return joinfields(words, ' ')

# Join fields with separator
def joinfields(words, sep):
	res = ''
	for w in words:
		res = res + (sep + w)
	return res[len(sep):]

# Find substring, raise exception if not found
index_error = 'substring not found in string.index'
def index(s, sub, *args):
	if args:
		if len(args) > 1:
			raise TypeError, 'string.index(): too many args'
		i = args[0]
	else:
		i = 0
	n = len(sub)
	m = len(s) + 1 - n
	while i < m:
		if sub == s[i:i+n]: return i
		i = i+1
	raise index_error, (s, sub) + args

# Find substring, return -1 if not found
def find(*args):
	try:
		return apply(index, args)
	except index_error:
		return -1

# Convert string to float
atof_error = 'non-float argument to string.atof'
def atof(str):
	import regex
	sign = ''
	s = str
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s: raise atof_error, str
	while s[0] == '0' and len(s) > 1 and s[1] in digits: s = s[1:]
	if regex.match('[0-9]*\(\.[0-9]*\)?\([eE][-+]?[0-9]+\)?', s) != len(s):
		raise atof_error, str
	try:
		return eval(sign + s)
	except SyntaxError:
		raise atof_error, str

# Convert string to integer
atoi_error = 'non-integer argument to string.atoi'
def atoi(str):
	sign = ''
	s = str
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s: raise atoi_error, str
	while s[0] == '0' and len(s) > 1: s = s[1:]
	for c in s:
		if c not in digits: raise atoi_error, str
	return eval(sign + s)

# Convert string to long integer
atol_error = 'non-integer argument to string.atol'
def atol(str):
	sign = ''
	s = str
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s: raise atoi_error, str
	while s[0] == '0' and len(s) > 1: s = s[1:]
	for c in s:
		if c not in digits: raise atoi_error, str
	return eval(sign + s + 'L')

# Left-justify a string
def ljust(s, width):
	n = width - len(s)
	if n <= 0: return s
	return s + ' '*n

# Right-justify a string
def rjust(s, width):
	n = width - len(s)
	if n <= 0: return s
	return ' '*n + s

# Center a string
def center(s, width):
	n = width - len(s)
	if n <= 0: return s
	half = n/2
	if n%2 and width%2:
		# This ensures that center(center(s, i), j) = center(s, j)
		half = half+1
	return ' '*half +  s + ' '*(n-half)

# Zero-fill a number, e.g., (12, 3) --> '012' and (-3, 3) --> '-03'
# Decadent feature: the argument may be a string or a number
# (Use of this is deprecated; it should be a string as with ljust c.s.)
def zfill(x, width):
	if type(x) == type(''): s = x
	else: s = `x`
	n = len(s)
	if n >= width: return s
	sign = ''
	if s[0] in ('-', '+'):
		sign, s = s[0], s[1:]
	return sign + '0'*(width-n) + s

# Expand tabs in a string.
# Doesn't take non-printing chars into account, but does understand \n.
def expandtabs(s, tabsize):
	res = line = ''
	for c in s:
		if c == '\t':
			c = ' '*(tabsize - len(line)%tabsize)
		line = line + c
		if c == '\n':
			res = res + line
			line = ''
	return res + line


# Try importing optional built-in module "strop" -- if it exists,
# it redefines some string operations that are 100-1000 times faster.
# It also defines values for whitespace, lowercase and uppercase
# that match <ctype.h>'s definitions.
# The manipulation with index_error is needed for compatibility.

try:
	from strop import *
	letters = lowercase + uppercase
	from strop import index
	index_error = ValueError
except ImportError:
	pass # Use the original, slow versions
