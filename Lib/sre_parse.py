#
# Secret Labs' Regular Expression Engine
# $Id$
#
# convert re-style regular expression to sre pattern
#
# Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

import string, sys

import _sre

from sre_constants import *

# FIXME: should be 65535, but the arraymodule is still broken
MAXREPEAT = 32767

SPECIAL_CHARS = ".\\[{()*+?^$|"
REPEAT_CHARS  = "*+?{"

DIGITS = tuple(string.digits)

OCTDIGITS = tuple("01234567")
HEXDIGITS = tuple("0123456789abcdefABCDEF")

WHITESPACE = string.whitespace

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

FLAGS = {
    # standard flags
    "i": SRE_FLAG_IGNORECASE,
    "L": SRE_FLAG_LOCALE,
    "m": SRE_FLAG_MULTILINE,
    "s": SRE_FLAG_DOTALL,
    "x": SRE_FLAG_VERBOSE,
    # extensions
    "t": SRE_FLAG_TEMPLATE,
    "u": SRE_FLAG_UNICODE,
}

class State:
    def __init__(self):
	self.flags = 0
	self.groups = 1
	self.groupdict = {}
    def getgroup(self, name=None):
	gid = self.groups
	self.groups = gid + 1
	if name:
	    self.groupdict[name] = gid
	return gid

class SubPattern:
    # a subpattern, in intermediate form
    def __init__(self, pattern, data=None):
	self.pattern = pattern
	if not data:
	    data = []
	self.data = data
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
		lo = lo + long(i) * av[0]
		hi = hi + long(j) * av[1]
	    elif op in (ANY, RANGE, IN, LITERAL, NOT_LITERAL, CATEGORY):
		lo = lo + 1
		hi = hi + 1
	    elif op == SUCCESS:
		break
	self.width = int(min(lo, sys.maxint)), int(min(hi, sys.maxint))
	return self.width

class Tokenizer:
    def __init__(self, string):
	self.index = 0
	self.string = string
	self.next = self.__next()
    def __next(self):
	if self.index >= len(self.string):
	    return None
	char = self.string[self.index]
	if char[0] == "\\":
	    try:
		c = self.string[self.index + 1]
	    except IndexError:
		raise error, "bogus escape"
	    char = char + c
	self.index = self.index + len(char)
	return char
    def match(self, char):
	if char == self.next:
	    self.next = self.__next()
	    return 1
	return 0
    def match_set(self, set):
	if self.next and self.next in set:
	    self.next = self.__next()
	    return 1
	return 0
    def get(self):
	this = self.next
	self.next = self.__next()
	return this

def _group(escape, state):
    # check if the escape string represents a valid group
    try:
	group = int(escape[1:])
	if group and group < state.groups:
	    return group
    except ValueError:
	pass
    return None # not a valid group

def _class_escape(source, escape):
    # handle escape code inside character class
    code = ESCAPES.get(escape)
    if code:
	return code
    code = CATEGORIES.get(escape)
    if code:
	return code
    try:
	if escape[1:2] == "x":
	    while source.next in HEXDIGITS:
		escape = escape + source.get()
	    escape = escape[2:]
	    # FIXME: support unicode characters!
	    return LITERAL, chr(int(escape[-4:], 16) & 0xff)
	elif str(escape[1:2]) in OCTDIGITS:
	    while source.next in OCTDIGITS:
		escape = escape + source.get()
	    escape = escape[1:]
	    # FIXME: support unicode characters!
	    return LITERAL, chr(int(escape[-6:], 8) & 0xff)
	if len(escape) == 2:
	    return LITERAL, escape[1]
    except ValueError:
	pass
    raise error, "bogus escape: %s" % repr(escape)

def _escape(source, escape, state):
    # handle escape code in expression
    code = CATEGORIES.get(escape)
    if code:
	return code
    code = ESCAPES.get(escape)
    if code:
	return code
    try:
	if escape[1:2] == "x":
	    while source.next in HEXDIGITS:
		escape = escape + source.get()
	    escape = escape[2:]
	    # FIXME: support unicode characters!
	    return LITERAL, chr(int(escape[-4:], 16) & 0xff)
	elif escape[1:2] in DIGITS:
	    while 1:
		group = _group(escape, state)
		if group:
		    if (not source.next or
			not _group(escape + source.next, state)):
		        return GROUP, group
		    escape = escape + source.get()
		elif source.next in OCTDIGITS:
		    escape = escape + source.get()
		else:
		    break
	    escape = escape[1:]
	    # FIXME: support unicode characters!
	    return LITERAL, chr(int(escape[-6:], 8) & 0xff)
	if len(escape) == 2:
	    return LITERAL, escape[1]
    except ValueError:
	pass
    raise error, "bogus escape: %s" % repr(escape)


def _branch(pattern, items):

    # form a branch operator from a set of items

    subpattern = SubPattern(pattern)

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
	return subpattern

    subpattern.append((BRANCH, (None, items)))
    return subpattern

def _parse(source, state, flags=0):

    # parse regular expression pattern into an operator list.

    subpattern = SubPattern(state)

    while 1:

	if source.next in ("|", ")"):
	    break # end of subpattern
	this = source.get()
	if this is None:
	    break # end of pattern

	if state.flags & SRE_FLAG_VERBOSE:
	    # skip whitespace and comments
	    if this in WHITESPACE:
		continue
	    if this == "#":
		while 1:
		    this = source.get()
		    if this in (None, "\n"):
			break
		continue

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
		    code1 = _class_escape(source, this)
		elif this:
		    code1 = LITERAL, this
		else:
		    raise error, "unexpected end of regular expression"
		if source.match("-"):
		    # potential range
		    this = source.get()
		    if this == "]":
			set.append(code1)
			set.append((LITERAL, "-"))
			break
		    else:
			if this[0] == "\\":
			    code2 = _class_escape(source, this)
			else:
			    code2 = LITERAL, this
			if code1[0] != LITERAL or code2[0] != LITERAL:
			    raise error, "illegal range"
			if len(code1[1]) != 1 or len(code2[1]) != 1:
			    raise error, "illegal range"
			set.append((RANGE, (code1[1], code2[1])))
		else:
		    if code1[0] is IN:
			code1 = code1[1][0]
		    set.append(code1)

	    # FIXME: <fl> move set optimization to compiler!
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
		min, max = 0, MAXREPEAT
	    elif this == "+":
		min, max = 1, MAXREPEAT
	    elif this == "{":
		min, max = 0, MAXREPEAT
		lo = hi = ""
		while source.next in DIGITS:
		    lo = lo + source.get()
		if source.match(","):
		    while source.next in DIGITS:
			hi = hi + source.get()
		else:
		    hi = lo
		if not source.match("}"):
		    raise error, "bogus range"
		if lo:
		    min = int(lo)
		if hi:
		    max = int(hi)
		# FIXME: <fl> check that hi >= lo!
	    else:
		raise error, "not supported"
	    # figure out which item to repeat
	    if subpattern:
		item = subpattern[-1:]
	    else:
		raise error, "nothing to repeat"
	    if source.match("?"):
		subpattern[-1] = (MIN_REPEAT, (min, max, item))
	    else:
		subpattern[-1] = (MAX_REPEAT, (min, max, item))

	elif this == ".":
	    subpattern.append((ANY, None))

	elif this == "(":
	    group = 1
	    name = None
	    if source.match("?"):
		group = 0
		# options
		if source.match("P"):
		    # python extensions
		    if source.match("<"):
			# named group: skip forward to end of name
			name = ""
			while 1:
			    char = source.get()
			    if char is None:
				raise error, "unterminated name"
			    if char == ">":
				break
			    # FIXME: check for valid character
			    name = name + char
			group = 1
		    elif source.match("="):
			# named backreference
			raise error, "not yet implemented"
		    else:
			char = source.get()
			if char is None:
			    raise error, "unexpected end of pattern"
			raise error, "unknown specifier: ?P%s" % char
		elif source.match(":"):
		    # non-capturing group
		    group = 2
		elif source.match("#"):
		    # comment
		    while 1:
			if source.next is None or source.next == ")":
			    break
			source.get()
		else:
		    # flags
		    while FLAGS.has_key(source.next):
			state.flags = state.flags | FLAGS[source.get()]
	    if group:
		# parse group contents
		b = []
		if group == 2:
		    # anonymous group
		    group = None
		else:
		    group = state.getgroup(name)
		while 1:
		    p = _parse(source, state, flags)
		    if source.match(")"):
			if b:
			    b.append(p)
			    p = _branch(state, b)
			subpattern.append((SUBPATTERN, (group, p)))
			break
		    elif source.match("|"):
			b.append(p)
		    else:
			raise error, "group not properly closed"
	    else:
		while 1:
		    char = source.get()
		    if char is None or char == ")":
			break
		    raise error, "unknown extension"

	elif this == "^":
	    subpattern.append((AT, AT_BEGINNING))

	elif this == "$":
	    subpattern.append((AT, AT_END))

	elif this and this[0] == "\\":
	    code = _escape(source, this, state)
	    subpattern.append(code)

	else:
	    raise error, "parser error"

    return subpattern

def parse(pattern, flags=0):
    # parse 're' pattern into list of (opcode, argument) tuples
    source = Tokenizer(pattern)
    state = State()
    b = []
    while 1:
	p = _parse(source, state, flags)
	tail = source.get()
	if tail == "|":
	    b.append(p)
	elif tail == ")":
	    raise error, "unbalanced parenthesis"
	elif tail is None:
	    if b:
		b.append(p)
		p = _branch(state, b)
	    break
	else:
	    raise error, "bogus characters at end of regular expression"
    return p

def parse_template(source, pattern):
    # parse 're' replacement string into list of literals and
    # group references
    s = Tokenizer(source)
    p = []
    a = p.append
    while 1:
	this = s.get()
	if this is None:
	    break # end of replacement string
	if this and this[0] == "\\":
	    if this == "\\g":
		name = ""
		if s.match("<"):
		    while 1:
			char = s.get()
			if char is None:
			    raise error, "unterminated index"
			if char == ">":
			    break
			# FIXME: check for valid character
			name = name + char
		if not name:
		    raise error, "bad index"
		try:
		    index = int(name)
		except ValueError:
		    try:
			index = pattern.groupindex[name]
		    except KeyError:
			raise IndexError, "unknown index"
		a((MARK, index))
	    elif len(this) > 1 and this[1] in DIGITS:
		while s.next in DIGITS:
		    this = this + s.get()
		a((MARK, int(this[1:])))
	    else:
		try:
		    a(ESCAPES[this])
		except KeyError:
		    for char in this:
			a((LITERAL, char))
	else:
	    a((LITERAL, this))
    return p

def expand_template(template, match):
    # FIXME: <fl> this is sooooo slow.  drop in the slicelist
    # code instead
    p = []
    a = p.append
    for c, s in template:
	if c is LITERAL:
	    a(s)
	elif c is MARK:
	    s = match.group(s)
	    if s is None:
		raise error, "empty group"
	    a(s)
    return match.string[:0].join(p)
