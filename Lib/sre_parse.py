#
# Secret Labs' Regular Expression Engine
# $Id$
#
# convert re-style regular expression to SRE template.  the current
# implementation is somewhat incomplete, and not very fast.  should
# definitely be rewritten before Python 1.6 goes beta.
#
# Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
#
# This code can only be used for 1.6 alpha testing.  All other use
# require explicit permission from Secret Labs AB.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

# FIXME: comments marked with the FIXME tag are open issues.  all such
# issues should be closed before the final beta.

import string, sys

from sre_constants import *

SPECIAL_CHARS = ".\\[{()*+?^$|"
REPEAT_CHARS  = "*+?{"

OCTDIGITS = "01234567"
HEXDIGITS = "0123456789abcdefABCDEF"

ESCAPES = {
    "\\a": (LITERAL, chr(7)),
    "\\b": (LITERAL, chr(8)),
    "\\f": (LITERAL, chr(12)),
    "\\n": (LITERAL, chr(10)),
    "\\r": (LITERAL, chr(13)),
    "\\t": (LITERAL, chr(9)),
    "\\v": (LITERAL, chr(11))
}

CATEGORIES = {
    "\\A": (AT, AT_BEGINNING), # start of string
    "\\b": (AT, AT_BOUNDARY),
    "\\B": (AT, AT_NON_BOUNDARY),
    "\\d": (IN, [(CATEGORY, CATEGORY_DIGIT)]),
    "\\D": (IN, [(CATEGORY, CATEGORY_NOT_DIGIT)]),
    "\\s": (IN, [(CATEGORY, CATEGORY_SPACE)]),
    "\\S": (IN, [(CATEGORY, CATEGORY_NOT_SPACE)]),
    "\\w": (IN, [(CATEGORY, CATEGORY_WORD)]),
    "\\W": (IN, [(CATEGORY, CATEGORY_NOT_WORD)]),
    "\\Z": (AT, AT_END), # end of string
}

class Pattern:
    # FIXME: <fl> rename class, and store flags in here too!
    def __init__(self):
	self.flags = []
	self.groups = 1
	self.groupdict = {}
    def getgroup(self, name=None):
	gid = self.groups
	self.groups = gid + 1
	if name:
	    self.groupdict[name] = gid
	return gid
    def setflag(self, flag):
	if flag not in self.flags:
	    self.flags.append(flag)

class SubPattern:
    # a subpattern, in intermediate form
    def __init__(self, pattern, data=None):
	self.pattern = pattern
	if not data:
	    data = []
	self.data = data
	self.flags = []
	self.width = None
    def __repr__(self):
	return repr(self.data)
    def __len__(self):
	return len(self.data)
    def __delitem__(self, index):
	del self.data[index]
    def __getitem__(self, index):
	return self.data[index]
    def __setitem__(self, index, code):
	self.data[index] = code
    def __getslice__(self, start, stop):
	return SubPattern(self.pattern, self.data[start:stop])
    def insert(self, index, code):
	self.data.insert(index, code)
    def append(self, code):
	self.data.append(code)
    def getwidth(self):
	# determine the width (min, max) for this subpattern
	if self.width:
	    return self.width
	lo = hi = 0L
	for op, av in self.data:
	    if op is BRANCH:
		l = sys.maxint
		h = 0
		for av in av[1]:
		    i, j = av.getwidth()
		    l = min(l, i)
		    h = min(h, j)
		lo = lo + i
		hi = hi + j
	    elif op is CALL:
		i, j = av.getwidth()
		lo = lo + i
		hi = hi + j
	    elif op is SUBPATTERN:
		i, j = av[1].getwidth()
		lo = lo + i
		hi = hi + j
	    elif op in (MIN_REPEAT, MAX_REPEAT):
		i, j = av[2].getwidth()
		lo = lo + i * av[0]
		hi = hi + j * av[1]
	    elif op in (ANY, RANGE, IN, LITERAL, NOT_LITERAL, CATEGORY):
		lo = lo + 1
		hi = hi + 1
	    elif op == SUCCESS:
		break
	self.width = int(min(lo, sys.maxint)), int(min(hi, sys.maxint))
	return self.width
    def set(self, flag):
	if not flag in self.flags:
	    self.flags.append(flag)
    def reset(self, flag):
	if flag in self.flags:
	    self.flags.remove(flag)

class Tokenizer:
    def __init__(self, string):
	self.string = list(string)
	self.next = self.__next()
    def __next(self):
	if not self.string:
	    return None
	char = self.string[0]
	if char[0] == "\\":
	    try:
		c = self.string[1]
	    except IndexError:
		raise SyntaxError, "bogus escape"
	    char = char + c
	    try:
		if c == "x":
		    # hexadecimal constant
		    for i in xrange(2, sys.maxint):
			c = self.string[i]
			if c not in HEXDIGITS:
			    break
			char = char + c
		elif c in string.digits:
		    # decimal (or octal) number
		    for i in xrange(2, sys.maxint):
			c = self.string[i]
			# FIXME: if larger than current number of
			# groups, interpret as an octal number 
			if c not in string.digits:
			    break
			char = char + c
	    except IndexError:
		pass # use what we've got this far
	del self.string[0:len(char)]
	return char
    def match(self, char):
	if char == self.next:
	    self.next = self.__next()
	    return 1
	return 0
    def match_set(self, set):
	if self.next in set:
	    self.next = self.__next()
	    return 1
	return 0
    def get(self):
	this = self.next
	self.next = self.__next()
	return this

def _fixescape(escape, character_class=0):
    # convert escape to (type, value)
    if character_class:
	# inside a character class, we'll look in the character
	# escapes dictionary first
	code = ESCAPES.get(escape)
	if code:
	    return code
	code = CATEGORIES.get(escape)
    else:
	code = CATEGORIES.get(escape)
	if code:
	    return code
	code = ESCAPES.get(escape)
    if code:
	return code
    if not character_class:
	try:
	    group = int(escape[1:])
	    # FIXME: only valid if group <= current number of groups
	    return GROUP, group
	except ValueError:
	    pass
    try:
	if escape[1:2] == "x":
	    escape = escape[2:]
	    return LITERAL, chr(string.atoi(escape[-2:], 16) & 0xff)
	elif escape[1:2] in string.digits:
	    return LITERAL, chr(string.atoi(escape[1:], 8) & 0xff)
	elif len(escape) == 2:
	    return LITERAL, escape[1]
    except ValueError:
	pass
    raise SyntaxError, "bogus escape: %s" % repr(escape)

def _branch(subpattern, items):

    # form a branch operator from a set of items (FIXME: move this
    # optimization to the compiler module!)

    # check if all items share a common prefix
    while 1:
	prefix = None
	for item in items:
	    if not item:
		break
	    if prefix is None:
		prefix = item[0]
	    elif item[0] != prefix:
		break
	else:
	    # all subitems start with a common "prefix".
	    # move it out of the branch
	    for item in items:
		del item[0]
	    subpattern.append(prefix)
	    continue # check next one
	break

    # check if the branch can be replaced by a character set
    for item in items:
	if len(item) != 1 or item[0][0] != LITERAL:
	    break
    else:
	# we can store this as a character set instead of a
	# branch (FIXME: use a range if possible)
	set = []
	for item in items:
	    set.append(item[0])
	subpattern.append((IN, set))
	return

    subpattern.append((BRANCH, (None, items)))

def _parse(source, pattern, flags=()):

    # parse regular expression pattern into an operator list.

    subpattern = SubPattern(pattern)

    this = None

    while 1:

	if source.next in ("|", ")"):
	    break # end of subpattern
	this = source.get()
	if this is None:
	    break # end of pattern

	if this and this[0] not in SPECIAL_CHARS:
	    subpattern.append((LITERAL, this))

	elif this == "[":
	    # character set
	    set = []
## 	    if source.match(":"):
## 		pass # handle character classes
	    if source.match("^"):
		set.append((NEGATE, None))
	    # check remaining characters
	    start = set[:]
	    while 1:
		this = source.get()
		if this == "]" and set != start:
		    break
		elif this and this[0] == "\\":
		    code1 = _fixescape(this, 1)
		elif this:
		    code1 = LITERAL, this
		else:
		    raise SyntaxError, "unexpected end of regular expression"
		if source.match("-"):
		    # potential range
		    this = source.get()
		    if this == "]":
			set.append(code1)
			set.append((LITERAL, "-"))
			break
		    else:
			if this[0] == "\\":
			    code2 = _fixescape(this, 1)
			else:
			    code2 = LITERAL, this
			if code1[0] != LITERAL or code2[0] != LITERAL:
			    raise SyntaxError, "illegal range"
			if len(code1[1]) != 1 or len(code2[1]) != 1:
			    raise SyntaxError, "illegal range"
			set.append((RANGE, (code1[1], code2[1])))
		else:
		    if code1[0] is IN:
			code1 = code1[1][0]
		    set.append(code1)

	    # FIXME: <fl> move set optimization to support function
	    if len(set)==1 and set[0][0] is LITERAL:
		subpattern.append(set[0]) # optimization
	    elif len(set)==2 and set[0][0] is NEGATE and set[1][0] is LITERAL:
		subpattern.append((NOT_LITERAL, set[1][1])) # optimization
	    else:
		# FIXME: <fl> add charmap optimization
		subpattern.append((IN, set))

	elif this and this[0] in REPEAT_CHARS:
	    # repeat previous item
	    if this == "?":
		min, max = 0, 1
	    elif this == "*":
		min, max = 0, sys.maxint
	    elif this == "+":
		min, max = 1, sys.maxint
	    elif this == "{":
		min, max = 0, sys.maxint
		lo = hi = ""
		while source.next in string.digits:
		    lo = lo + source.get()
		if source.match(","):
		    while source.next in string.digits:
			hi = hi + source.get()
		else:
		    hi = lo
		if not source.match("}"):
		    raise SyntaxError, "bogus range"
		if lo:
		    min = int(lo)
		if hi:
		    max = int(hi)
		# FIXME: <fl> check that hi >= lo!
	    else:
		raise SyntaxError, "not supported"
	    # figure out which item to repeat
	    # FIXME: should back up to the right mark, right?
	    if subpattern:
		index = len(subpattern)-1
		while subpattern[index][0] is MARK:
		    index = index - 1
		item = subpattern[index:index+1]
	    else:
		raise SyntaxError, "nothing to repeat"
	    if source.match("?"):
		subpattern[index] = (MIN_REPEAT, (min, max, item))
	    else:
		subpattern[index] = (MAX_REPEAT, (min, max, item))
	elif this == ".":
	    subpattern.append((ANY, None))
	elif this == "(":
	    group = 1
	    name = None
	    if source.match("?"):
		group = 0
		# options
		if source.match("P"):
		    # named group: skip forward to end of name
		    if source.match("<"):
			name = ""
			while 1:
			    char = source.get()
			    if char in (">", None):
				break
			    name = name + char
			group = 1
		elif source.match(":"):
		    # non-capturing group
		    group = 2
		elif source.match_set("iI"):
		    pattern.setflag("i")
		elif source.match_set("lL"):
		    pattern.setflag("l")
		elif source.match_set("mM"):
		    pattern.setflag("m")
		elif source.match_set("sS"):
		    pattern.setflag("s")
		elif source.match_set("xX"):
		    pattern.setflag("x")
	    if group:
		# parse group contents
		b = []
		if group == 2:
		    # anonymous group
		    group = None
		else:
		    group = pattern.getgroup(name)
 		if group:
 		    subpattern.append((MARK, (group-1)*2))
		while 1:
		    p = _parse(source, pattern, flags)
		    if source.match(")"):
			if b:
			    b.append(p)
			    _branch(subpattern, b)
			else:
			    subpattern.append((SUBPATTERN, (group, p)))
			break
		    elif source.match("|"):
			b.append(p)
		    else:
			raise SyntaxError, "group not properly closed"
 		if group:
 		    subpattern.append((MARK, (group-1)*2+1))
	    else:
		# FIXME: should this really be a while loop?
 		while source.get() not in (")", None):
 		    pass

	elif this == "^":
	    subpattern.append((AT, AT_BEGINNING))

	elif this == "$":
	    subpattern.append((AT, AT_END))

	elif this and this[0] == "\\":
	    code =_fixescape(this)
	    subpattern.append(code)

	else:
	    raise SyntaxError, "parser error"

    return subpattern

def parse(source, flags=()):
    s = Tokenizer(source)
    g = Pattern()
    b = []
    while 1:
	p = _parse(s, g, flags)
	tail = s.get()
	if tail == "|":
	    b.append(p)
	elif tail == ")":
	    raise SyntaxError, "unbalanced parenthesis"
	elif tail is None:
	    if b:
		b.append(p)
		p = SubPattern(g)
		_branch(p, b)
	    break
	else:
	    raise SyntaxError, "bogus characters at end of regular expression"
    return p

if __name__ == "__main__":
    from pprint import pprint
    from testpatterns import PATTERNS
    a = b = c = 0
    for pattern, flags in PATTERNS:
	if flags:
	    continue
	print "-"*68
	try:
	    p = parse(pattern)
	    print repr(pattern), "->"
	    pprint(p.data)
	    import sre_compile
	    try:
		code = sre_compile.compile(p)
		c = c + 1
	    except:
		pass
	    a = a + 1
	except SyntaxError, v:
	    print "**", repr(pattern), v
	b = b + 1
    print "-"*68
    print a, "of", b, "patterns successfully parsed"
    print c, "of", b, "patterns successfully compiled"

