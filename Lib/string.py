"""A collection of string operations (most are no longer used in Python 1.6).

Warning: most of the code you see here isn't normally used nowadays.  With
Python 1.6, many of these functions are implemented as methods on the
standard string object. They used to be implemented by a built-in module
called strop, but strop is now obsolete itself.

Public module variables:

whitespace -- a string containing all characters considered whitespace
lowercase -- a string containing all characters considered lowercase letters
uppercase -- a string containing all characters considered uppercase letters
letters -- a string containing all characters considered letters
digits -- a string containing all characters considered decimal digits
hexdigits -- a string containing all characters considered hexadecimal digits
octdigits -- a string containing all characters considered octal digits
punctuation -- a string containing all characters considered punctuation
printable -- a string containing all characters considered printable

"""

# Some strings for ctype-style character classification
whitespace = ' \t\n\r\v\f'
lowercase = 'abcdefghijklmnopqrstuvwxyz'
uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
letters = lowercase + uppercase
digits = '0123456789'
hexdigits = digits + 'abcdef' + 'ABCDEF'
octdigits = '01234567'
punctuation = """!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~""" 
printable = digits + letters + punctuation + whitespace

# Case conversion helpers
_idmap = ''
for i in range(256): _idmap = _idmap + chr(i)
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
    return s.lower()

# Convert lower case letters to UPPER CASE
def upper(s):
    """upper(s) -> string

    Return a copy of the string s converted to uppercase.

    """
    return s.upper()

# Swap lower case letters and UPPER CASE
def swapcase(s):
    """swapcase(s) -> string

    Return a copy of the string s with upper case characters
    converted to lowercase and vice versa.

    """
    return s.swapcase()

# Strip leading and trailing tabs and spaces
def strip(s):
    """strip(s) -> string

    Return a copy of the string s with leading and trailing
    whitespace removed.

    """
    return s.strip()

# Strip leading tabs and spaces
def lstrip(s):
    """lstrip(s) -> string

    Return a copy of the string s with leading whitespace removed.

    """
    return s.lstrip()

# Strip trailing tabs and spaces
def rstrip(s):
    """rstrip(s) -> string

    Return a copy of the string s with trailing whitespace
    removed.

    """
    return s.rstrip()


# Split a string into a list of space/tab-separated words
# NB: split(s) is NOT the same as splitfields(s, ' ')!
def split(s, sep=None, maxsplit=-1):
    """split(s [,sep [,maxsplit]]) -> list of strings

    Return a list of the words in the string s, using sep as the
    delimiter string.  If maxsplit is given, splits into at most
    maxsplit words.  If sep is not specified, any whitespace string
    is a separator.

    (split and splitfields are synonymous)

    """
    return s.split(sep, maxsplit)
splitfields = split

# Join fields with optional separator
def join(words, sep = ' '):
    """join(list [,sep]) -> string

    Return a string composed of the words in list, with
    intervening occurrences of sep.  The default separator is a
    single space.

    (joinfields and join are synonymous)

    """
    return sep.join(words)
joinfields = join

# Find substring, raise exception if not found
def index(s, *args):
    """index(s, sub [,start [,end]]) -> int

    Like find but raises ValueError when the substring is not found.

    """
    return s.index(*args)

# Find last substring, raise exception if not found
def rindex(s, *args):
    """rindex(s, sub [,start [,end]]) -> int

    Like rfind but raises ValueError when the substring is not found.

    """
    return s.rindex(*args)

# Count non-overlapping occurrences of substring
def count(s, *args):
    """count(s, sub[, start[,end]]) -> int

    Return the number of occurrences of substring sub in string
    s[start:end].  Optional arguments start and end are
    interpreted as in slice notation.

    """
    return s.count(*args)

# Find substring, return -1 if not found
def find(s, *args):
    """find(s, sub [,start [,end]]) -> in

    Return the lowest index in s where substring sub is found,
    such that sub is contained within s[start,end].  Optional
    arguments start and end are interpreted as in slice notation.

    Return -1 on failure.

    """
    return s.find(*args)

# Find last substring, return -1 if not found
def rfind(s, *args):
    """rfind(s, sub [,start [,end]]) -> int

    Return the highest index in s where substring sub is found,
    such that sub is contained within s[start,end].  Optional
    arguments start and end are interpreted as in slice notation.

    Return -1 on failure.

    """
    return s.rfind(*args)

# for a bit of speed
_float = float
_int = int
_long = long
_StringType = type('')

# Convert string to float
def atof(s):
    """atof(s) -> float

    Return the floating point number represented by the string s.

    """
    return _float(s)


# Convert string to integer
def atoi(s , base=10):
    """atoi(s [,base]) -> int

    Return the integer represented by the string s in the given
    base, which defaults to 10.  The string s must consist of one
    or more digits, possibly preceded by a sign.  If base is 0, it
    is chosen from the leading characters of s, 0 for octal, 0x or
    0X for hexadecimal.  If base is 16, a preceding 0x or 0X is
    accepted.

    """
    return _int(s, base)


# Convert string to long integer
def atol(s, base=10):
    """atol(s [,base]) -> long

    Return the long integer represented by the string s in the
    given base, which defaults to 10.  The string s must consist
    of one or more digits, possibly preceded by a sign.  If base
    is 0, it is chosen from the leading characters of s, 0 for
    octal, 0x or 0X for hexadecimal.  If base is 16, a preceding
    0x or 0X is accepted.  A trailing L or l is not accepted,
    unless base is 0.

    """
    return _long(s, base)


# Left-justify a string
def ljust(s, width):
    """ljust(s, width) -> string

    Return a left-justified version of s, in a field of the
    specified width, padded with spaces as needed.  The string is
    never truncated.

    """
    return s.ljust(width)

# Right-justify a string
def rjust(s, width):
    """rjust(s, width) -> string

    Return a right-justified version of s, in a field of the
    specified width, padded with spaces as needed.  The string is
    never truncated.

    """
    return s.rjust(width)

# Center a string
def center(s, width):
    """center(s, width) -> string

    Return a center version of s, in a field of the specified
    width. padded with spaces as needed.  The string is never
    truncated.

    """
    return s.center(width)

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
    return s.expandtabs(tabsize)

# Character translation through look-up table.
def translate(s, table, deletions=""):
    """translate(s,table [,deletechars]) -> string

    Return a copy of the string s, where all characters occurring
    in the optional argument deletechars are removed, and the
    remaining characters have been mapped through the given
    translation table, which must be a string of length 256.

    """
    return s.translate(table, deletions)

# Capitalize a string, e.g. "aBc  dEf" -> "Abc  def".
def capitalize(s):
    """capitalize(s) -> string

    Return a copy of the string s with only its first character
    capitalized.

    """
    return s.capitalize()

# Capitalize the words in a string, e.g. " aBc  dEf " -> "Abc Def".
# See also regsub.capwords().
def capwords(s, sep=None):
    """capwords(s, [sep]) -> string

    Split the argument into words using split, capitalize each
    word using capitalize, and join the capitalized words using
    join. Note that this replaces runs of whitespace characters by
    a single space.

    """
    return join(map(capitalize, s.split(sep)), sep or ' ')

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
def replace(s, old, new, maxsplit=-1):
    """replace (str, old, new[, maxsplit]) -> string

    Return a copy of string str with all occurrences of substring
    old replaced by new. If the optional argument maxsplit is
    given, only the first maxsplit occurrences are replaced.

    """
    return s.replace(old, new, maxsplit)


# Try importing optional built-in module "strop" -- if it exists,
# it redefines some string operations that are 100-1000 times faster.
# It also defines values for whitespace, lowercase and uppercase
# that match <ctype.h>'s definitions.

try:
    from strop import maketrans, lowercase, uppercase, whitespace
    letters = lowercase + uppercase
except ImportError:
    pass                                          # Use the original versions
