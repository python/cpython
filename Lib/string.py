# module 'string' -- A collection of string operations

# Warning: most of the code you see here isn't normally used nowadays.
# At the end of this file most functions are replaced by built-in
# functions imported from built-in module "strop".

"""Common string manipulations.

Public module variables:

whitespace -- a string containing all characters considered whitespace
lowercase -- a string containing all characters considered lowercase letters
uppercase -- a string containing all characters considered uppercase letters
letters -- a string containing all characters considered letters
digits -- a string containing all characters considered decimal digits
hexdigits -- a string containing all characters considered hexadecimal digits
octdigits -- a string containing all characters considered octal digits

"""

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

# Backward compatible names for exceptions
index_error = ValueError
atoi_error = ValueError
atof_error = ValueError
atol_error = ValueError

# convert UPPER CASE letters to lower case
def lower(s):
	"""lower(s) -> string

	Return a copy of the string s converted to lowercase.

	"""
	res = ''
	for c in s:
		res = res + _lower[ord(c)]
	return res

# Convert lower case letters to UPPER CASE
def upper(s):
	"""upper(s) -> string

	Return a copy of the string s converted to uppercase.

	"""
	res = ''
	for c in s:
		res = res + _upper[ord(c)]
	return res

# Swap lower case letters and UPPER CASE
def swapcase(s):
	"""swapcase(s) -> string

	Return a copy of the string s with upper case characters
	converted to lowercase and vice versa.

	"""
	res = ''
	for c in s:
		res = res + _swapcase[ord(c)]
	return res

# Strip leading and trailing tabs and spaces
def strip(s):
	"""strip(s) -> string

	Return a copy of the string s with leading and trailing
	whitespace removed.

	"""
	i, j = 0, len(s)
	while i < j and s[i] in whitespace: i = i+1
	while i < j and s[j-1] in whitespace: j = j-1
	return s[i:j]

# Strip leading tabs and spaces
def lstrip(s):
	"""lstrip(s) -> string

	Return a copy of the string s with leading whitespace removed.

	"""
	i, j = 0, len(s)
	while i < j and s[i] in whitespace: i = i+1
	return s[i:j]

# Strip trailing tabs and spaces
def rstrip(s):
	"""rstrip(s) -> string

	Return a copy of the string s with trailing whitespace
	removed.

	"""
	i, j = 0, len(s)
	while i < j and s[j-1] in whitespace: j = j-1
	return s[i:j]


# Split a string into a list of space/tab-separated words
# NB: split(s) is NOT the same as splitfields(s, ' ')!
def split(s, sep=None, maxsplit=0):
	"""split(str [,sep [,maxsplit]]) -> list of strings

	Return a list of the words in the string s, using sep as the
	delimiter string.  If maxsplit is nonzero, splits into at most
	maxsplit words If sep is not specified, any whitespace string
	is a separator.  Maxsplit defaults to 0.

	(split and splitfields are synonymous)

	"""
	if sep is not None: return splitfields(s, sep, maxsplit)
	res = []
	i, n = 0, len(s)
	if maxsplit <= 0: maxsplit = n
	count = 0
	while i < n:
		while i < n and s[i] in whitespace: i = i+1
		if i == n: break
		if count >= maxsplit:
		    res.append(s[i:])
		    break
		j = i
		while j < n and s[j] not in whitespace: j = j+1
		count = count + 1
		res.append(s[i:j])
		i = j
	return res

# Split a list into fields separated by a given string
# NB: splitfields(s, ' ') is NOT the same as split(s)!
# splitfields(s, '') returns [s] (in analogy with split() in nawk)
def splitfields(s, sep=None, maxsplit=0):
	"""splitfields(str [,sep [,maxsplit]]) -> list of strings

	Return a list of the words in the string s, using sep as the
	delimiter string.  If maxsplit is nonzero, splits into at most
	maxsplit words If sep is not specified, any whitespace string
	is a separator.  Maxsplit defaults to 0.

	(split and splitfields are synonymous)

	"""
	if sep is None: return split(s, None, maxsplit)
	res = []
	nsep = len(sep)
	if nsep == 0:
		return [s]
	ns = len(s)
	if maxsplit <= 0: maxsplit = ns
	i = j = 0
	count = 0
	while j+nsep <= ns:
		if s[j:j+nsep] == sep:
			count = count + 1
			res.append(s[i:j])
			i = j = j + nsep
			if count >= maxsplit: break
		else:
			j = j + 1
	res.append(s[i:])
	return res

# Join words with spaces between them
def join(words, sep = ' '):
	"""join(list [,sep]) -> string

	Return a string composed of the words in list, with
	intervening occurences of sep.  Sep defaults to a single
	space.

	(joinfields and join are synonymous)

	"""
	return joinfields(words, sep)

# Join fields with optional separator
def joinfields(words, sep = ' '):
	"""joinfields(list [,sep]) -> string

	Return a string composed of the words in list, with
	intervening occurences of sep.  The default separator is a
	single space.

	(joinfields and join are synonymous)

	"""    
	res = ''
	for w in words:
		res = res + (sep + w)
	return res[len(sep):]

# Find substring, raise exception if not found
def index(s, sub, i = 0, last=None):
	"""index(s, sub [,start [,end]]) -> int

	Return the lowest index in s where substring sub is found,
	such that sub is contained within s[start,end].  Optional
	arguments start and end are interpreted as in slice notation.

	Raise ValueError if not found.

	"""
	if last is None: last = len(s)
	res = find(s, sub, i, last)
	if res < 0:
		raise ValueError, 'substring not found in string.index'
	return res

# Find last substring, raise exception if not found
def rindex(s, sub, i = 0, last=None):
	"""rindex(s, sub [,start [,end]]) -> int

	Return the highest index in s where substring sub is found,
	such that sub is contained within s[start,end].  Optional
	arguments start and end are interpreted as in slice notation.

	Raise ValueError if not found.

	"""
	if last is None: last = len(s)
	res = rfind(s, sub, i, last)
	if res < 0:
		raise ValueError, 'substring not found in string.index'
	return res

# Count non-overlapping occurrences of substring
def count(s, sub, i = 0, last=None):
	"""count(s, sub[, start[,end]]) -> int

	Return the number of occurrences of substring sub in string
	s[start:end].  Optional arguments start and end are
	interpreted as in slice notation.

	"""
	Slen = len(s)  # cache this value, for speed
	if last is None:
		last = Slen
	elif last < 0:
		last = max(0, last + Slen)
	elif last > Slen:
		last = Slen
	if i < 0: i = max(0, i + Slen)
	n = len(sub)
	m = last + 1 - n
	if n == 0: return m-i
	r = 0
	while i < m:
		if sub == s[i:i+n]:
			r = r+1
			i = i+n
		else:
			i = i+1
	return r

# Find substring, return -1 if not found
def find(s, sub, i = 0, last=None):
	"""find(s, sub [,start [,end]]) -> in

	Return the lowest index in s where substring sub is found,
	such that sub is contained within s[start,end].  Optional
	arguments start and end are interpreted as in slice notation.

	Return -1 on failure.

	"""
	Slen = len(s)  # cache this value, for speed
	if last is None:
		last = Slen
	elif last < 0:
		last = max(0, last + Slen)
	elif last > Slen:
		last = Slen
	if i < 0: i = max(0, i + Slen)
	n = len(sub)
	m = last + 1 - n
	while i < m:
		if sub == s[i:i+n]: return i
		i = i+1
	return -1

# Find last substring, return -1 if not found
def rfind(s, sub, i = 0, last=None):
	"""rfind(s, sub [,start [,end]]) -> int

	Return the highest index in s where substring sub is found,
	such that sub is contained within s[start,end].  Optional
	arguments start and end are interpreted as in slice notation.

	Return -1 on failure.

	"""
	Slen = len(s)  # cache this value, for speed
	if last is None:
		last = Slen
	elif last < 0:
		last = max(0, last + Slen)
	elif last > Slen:
		last = Slen
	if i < 0: i = max(0, i + Slen)
	n = len(sub)
	m = last + 1 - n
	r = -1
	while i < m:
		if sub == s[i:i+n]: r = i
		i = i+1
	return r

# "Safe" environment for eval()
safe_env = {"__builtins__": {}}

# Convert string to float
re = None
def atof(str):
	"""atof(s) -> float

	Return the floating point number represented by the string s.

	"""
	global re
	if re is None:
		# Don't fail if re doesn't exist -- just skip the syntax check
		try:
			import re
		except ImportError:
			re = 0
	sign = ''
	s = strip(str)
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s:
		raise ValueError, 'non-float argument to string.atof'
	while s[0] == '0' and len(s) > 1 and s[1] in digits: s = s[1:]
	if re and not re.match('[0-9]*(\.[0-9]*)?([eE][-+]?[0-9]+)?$', s):
		raise ValueError, 'non-float argument to string.atof'
	try:
		return float(eval(sign + s, safe_env))
	except SyntaxError:
		raise ValueError, 'non-float argument to string.atof'

# Convert string to integer
def atoi(str, base=10):
	"""atoi(s [,base]) -> int

	Return the integer represented by the string s in the given
	base, which defaults to 10.  The string s must consist of one
	or more digits, possibly preceded by a sign.  If base is 0, it
	is chosen from the leading characters of s, 0 for octal, 0x or
	0X for hexadecimal.  If base is 16, a preceding 0x or 0X is
	accepted.

	"""
	if base != 10:
		# We only get here if strop doesn't define atoi()
		raise ValueError, "this string.atoi doesn't support base != 10"
	sign = ''
	s = str
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s:
		raise ValueError, 'non-integer argument to string.atoi'
	while s[0] == '0' and len(s) > 1: s = s[1:]
	for c in s:
		if c not in digits:
			raise ValueError, 'non-integer argument to string.atoi'
	return eval(sign + s, safe_env)

# Convert string to long integer
def atol(str, base=10):
	"""atol(s [,base]) -> long

	Return the long integer represented by the string s in the
	given base, which defaults to 10.  The string s must consist
	of one or more digits, possibly preceded by a sign.  If base
	is 0, it is chosen from the leading characters of s, 0 for
	octal, 0x or 0X for hexadecimal.  If base is 16, a preceding
	0x or 0X is accepted.  A trailing L or l is not accepted,
	unless base is 0.

	"""
	if base != 10:
		# We only get here if strop doesn't define atol()
		raise ValueError, "this string.atol doesn't support base != 10"
	sign = ''
	s = str
	if s and s[0] in '+-':
		sign = s[0]
		s = s[1:]
	if not s:
		raise ValueError, 'non-integer argument to string.atol'
	while s[0] == '0' and len(s) > 1: s = s[1:]
	for c in s:
		if c not in digits:
			raise ValueError, 'non-integer argument to string.atol'
	return eval(sign + s + 'L', safe_env)

# Left-justify a string
def ljust(s, width):
	"""ljust(s, width) -> string

	Return a left-justified version of s, in a field of the
	specified width, padded with spaces as needed.  The string is
	never truncated.

	"""
	n = width - len(s)
	if n <= 0: return s
	return s + ' '*n

# Right-justify a string
def rjust(s, width):
    	"""rjust(s, width) -> string

	Return a right-justified version of s, in a field of the
	specified width, padded with spaces as needed.  The string is
	never truncated.

	"""
	n = width - len(s)
	if n <= 0: return s
	return ' '*n + s

# Center a string
def center(s, width):
   	"""center(s, width) -> string

	Return a center version of s, in a field of the specified
	width. padded with spaces as needed.  The string is never
	truncated.

	"""
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
	"""zfill(x, width) -> string

	Pad a numeric string x with zeros on the left, to fill a field
	of the specified width.  The string x is never truncated.

	"""
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
def expandtabs(s, tabsize=8):
	"""expandtabs(s [,tabsize]) -> string

	Return a copy of the string s with all tab characters replaced
	by the appropriate number of spaces, depending on the current
	column, and the tabsize (default 8).

	"""
	res = line = ''
	for c in s:
		if c == '\t':
			c = ' '*(tabsize - len(line)%tabsize)
		line = line + c
		if c == '\n':
			res = res + line
			line = ''
	return res + line

# Character translation through look-up table.
def translate(s, table, deletions=""):
	"""translate(s,table [,deletechars]) -> string

	Return a copy of the string s, where all characters occurring
	in the optional argument deletechars are removed, and the
	remaining characters have been mapped through the given
	translation table, which must be a string of length 256.

	"""
	if type(table) != type('') or len(table) != 256:
	    raise TypeError, "translation table must be 256 characters long"
	res = ""
	for c in s:
		if c not in deletions:
			res = res + table[ord(c)]
	return res

# Capitalize a string, e.g. "aBc  dEf" -> "Abc  def".
def capitalize(s):
	"""capitalize(s) -> string

	Return a copy of the string s with only its first character
	capitalized.

	"""
	return upper(s[:1]) + lower(s[1:])

# Capitalize the words in a string, e.g. " aBc  dEf " -> "Abc Def".
# See also regsub.capwords().
def capwords(s, sep=None):
	"""capwords(s, [sep]) -> string

	Split the argument into words using split, capitalize each
	word using capitalize, and join the capitalized words using
	join. Note that this replaces runs of whitespace characters by
	a single space.

	"""
	return join(map(capitalize, split(s, sep)), sep or ' ')

# Construct a translation string
_idmapL = None
def maketrans(fromstr, tostr):
	"""maketrans(frm, to) -> string

	Return a translation table (a string of 256 bytes long)
	suitable for use in string.translate.  The strings frm and to
	must be of the same length.

	"""
	if len(fromstr) != len(tostr):
		raise ValueError, "maketrans arguments must have same length"
	global _idmapL
	if not _idmapL:
		_idmapL = map(None, _idmap)
	L = _idmapL[:]
	fromstr = map(ord, fromstr)
	for i in range(len(fromstr)):
		L[fromstr[i]] = tostr[i]
	return joinfields(L, "")

# Substring replacement (global)
def replace(str, old, new, maxsplit=0):
	"""replace (str, old, new[, maxsplit]) -> string

	Return a copy of string str with all occurrences of substring
	old replaced by new. If the optional argument maxsplit is
	given, only the first maxsplit occurrences are replaced.

	"""
	return joinfields(splitfields(str, old, maxsplit), new)


# Try importing optional built-in module "strop" -- if it exists,
# it redefines some string operations that are 100-1000 times faster.
# It also defines values for whitespace, lowercase and uppercase
# that match <ctype.h>'s definitions.

try:
	from strop import *
	letters = lowercase + uppercase
except ImportError:
	pass # Use the original, slow versions
