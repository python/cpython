#! /usr/local/bin/python
#! /usr/local/bin/python

# This file contains a class and a main program that perform two
# related (though complimentary) formatting operations on Python
# programs.  When called as "pindend -c", it takes a valid Python
# program as input and outputs a version augmented with block-closing
# comments.  When called as "pindent -r" it assumes its input is a
# Python program with block-closing comments but with its indentation
# messed up, and outputs a properly indented version.

# A "block-closing comment" is a comment of the form '# end <keyword>'
# where <keyword> is the keyword that opened the block.  If the
# opening keyword is 'def' or 'class', the function or class name may
# be repeated in the block-closing comment as well.  Here is an
# example of a program fully augmented with block-closing comments:

# def foobar(a, b):
#    if a == b:
#        a = a+1
#    elif a < b:
#        b = b-1
#        if b > a: a = a-1
#        # end if
#    else:
#        print 'oops!'
#    # end if
# # end def foobar

# Note that only the last part of an if...elif...else... block needs a
# block-closing comment; the same is true for other compound
# statements (e.g. try...except).  Also note that "short-form" blocks
# like the second 'if' in the example must be closed as well;
# otherwise the 'else' in the example would be ambiguous (remember
# that indentation is not significant when interpreting block-closing
# comments).

# Both operations are idempotent (i.e. applied to their own output
# they yield an identical result).  Running first "pindent -c" and
# then "pindent -r" on a valid Python program produces a program that
# is semantically identical to the input (though its indentation may
# be different).

# Other options:
# -s stepsize: set the indentation step size (default 8)
# -t tabsize : set the number of spaces a tab character is worth (default 8)
# file ...   : input file(s) (default standard input)
# The results always go to standard output

# Caveats:
# - comments ending in a backslash will be mistaken for continued lines
# - continuations using backslash are always left unchanged
# - continuations inside parentheses are not extra indented by -r
#   but must be indented for -c to work correctly (this breaks
#   idempotency!)
# - continued lines inside triple-quoted strings are totally garbled

# Secret feature:
# - On input, a block may also be closed with an "end statement" --
#   this is a block-closing comment without the '#' sign.

# Possible improvements:
# - check syntax based on transitions in 'next' table
# - better error reporting
# - better error recovery
# - check identifier after class/def

# The following wishes need a more complete tokenization of the source:
# - Don't get fooled by comments ending in backslash
# - reindent continuation lines indicated by backslash
# - handle continuation lines inside parentheses/braces/brackets
# - handle triple quoted strings spanning lines
# - realign comments
# - optionally do much more thorough reformatting, a la C indent

# Defaults
STEPSIZE = 8
TABSIZE = 8

import os
import regex
import string
import sys

next = {}
next['if'] = next['elif'] = 'elif', 'else', 'end'
next['while'] = next['for'] = 'else', 'end'
next['try'] = 'except', 'finally'
next['except'] = 'except', 'else', 'end'
next['else'] = next['finally'] = next['def'] = next['class'] = 'end'
next['end'] = ()
start = 'if', 'while', 'for', 'try', 'def', 'class'

class PythonIndenter:

	def __init__(self, fpi = sys.stdin, fpo = sys.stdout,
		     indentsize = STEPSIZE, tabsize = TABSIZE):
		self.fpi = fpi
		self.fpo = fpo
		self.indentsize = indentsize
		self.tabsize = tabsize
		self.lineno = 0
		self.write = fpo.write
		self.kwprog = regex.symcomp(
			'^[ \t]*\(<kw>[a-z]+\)'
			'\([ \t]+\(<id>[a-zA-Z_][a-zA-Z0-9_]*\)\)?'
			'[^a-zA-Z0-9_]')
		self.endprog = regex.symcomp(
			'^[ \t]*#?[ \t]*end[ \t]+\(<kw>[a-z]+\)'
			'\([ \t]+\(<id>[a-zA-Z_][a-zA-Z0-9_]*\)\)?'
			'[^a-zA-Z0-9_]')
		self.wsprog = regex.compile('^[ \t]*')
	# end def __init__

	def readline(self):
		line = self.fpi.readline()
		if line: self.lineno = self.lineno + 1
		# end if
		return line
	# end def readline

	def error(self, fmt, *args):
		if args: fmt = fmt % args
		# end if
		sys.stderr.write('Error at line %d: %s\n' % (self.lineno, fmt))
		self.write('### %s ###\n' % fmt)
	# end def error

	def getline(self):
		line = self.readline()
		while line[-2:] == '\\\n':
			line2 = self.readline()
			if not line2: break
			# end if
			line = line + line2
		# end while
		return line
	# end def getline

	def putline(self, line, indent = None):
		if indent is None:
			self.write(line)
			return
		# end if
		tabs, spaces = divmod(indent*self.indentsize, self.tabsize)
		i = max(0, self.wsprog.match(line))
		self.write('\t'*tabs + ' '*spaces + line[i:])
	# end def putline

	def reformat(self):
		stack = []
		while 1:
			line = self.getline()
			if not line: break	# EOF
			# end if
			if self.endprog.match(line) >= 0:
				kw = 'end'
				kw2 = self.endprog.group('kw')
				if not stack:
					self.error('unexpected end')
				elif stack[-1][0] != kw2:
					self.error('unmatched end')
				# end if
				del stack[-1:]
				self.putline(line, len(stack))
				continue
			# end if
			if self.kwprog.match(line) >= 0:
				kw = self.kwprog.group('kw')
				if kw in start:
					self.putline(line, len(stack))
					stack.append((kw, kw))
					continue
				# end if
				if next.has_key(kw) and stack:
					self.putline(line, len(stack)-1)
					kwa, kwb = stack[-1]
					stack[-1] = kwa, kw
					continue
				# end if
			# end if
			self.putline(line, len(stack))
		# end while
		if stack:
			self.error('unterminated keywords')
			for kwa, kwb in stack:
				self.write('\t%s\n' % kwa)
			# end for
		# end if
	# end def reformat

	def complete(self):
		self.indentsize = 1
		stack = []
		todo = []
		current, firstkw, lastkw, topid = 0, '', '', ''
		while 1:
			line = self.getline()
			i = max(0, self.wsprog.match(line))
			if self.endprog.match(line) >= 0:
				thiskw = 'end'
				endkw = self.endprog.group('kw')
				thisid = self.endprog.group('id')
			elif self.kwprog.match(line) >= 0:
				thiskw = self.kwprog.group('kw')
				if not next.has_key(thiskw):
					thiskw = ''
				# end if
				if thiskw in ('def', 'class'):
					thisid = self.kwprog.group('id')
				else:
					thisid = ''
				# end if
			elif line[i:i+1] in ('\n', '#'):
				todo.append(line)
				continue
			else:
				thiskw = ''
			# end if
			indent = len(string.expandtabs(line[:i], self.tabsize))
			while indent < current:
				if firstkw:
					if topid:
						s = '# end %s %s\n' % (
							firstkw, topid)
					else:
						s = '# end %s\n' % firstkw
					# end if
					self.putline(s, current)
					firstkw = lastkw = ''
				# end if
				current, firstkw, lastkw, topid = stack[-1]
				del stack[-1]
			# end while
			if indent == current and firstkw:
				if thiskw == 'end':
					if endkw != firstkw:
						self.error('mismatched end')
					# end if
					firstkw = lastkw = ''
				elif not thiskw or thiskw in start:
					if topid:
						s = '# end %s %s\n' % (
							firstkw, topid)
					else:
						s = '# end %s\n' % firstkw
					# end if
					self.putline(s, current)
					firstkw = lastkw = topid = ''
				# end if
			# end if
			if indent > current:
				stack.append(current, firstkw, lastkw, topid)
				if thiskw and thiskw not in start:
					# error
					thiskw = ''
				# end if
				current, firstkw, lastkw, topid = \
					 indent, thiskw, thiskw, thisid
			# end if
			if thiskw:
				if thiskw in start:
					firstkw = lastkw = thiskw
					topid = thisid
				else:
					lastkw = thiskw
				# end if
			# end if
			for l in todo: self.write(l)
			# end for
			todo = []
			if not line: break
			# end if
			self.write(line)
		# end while
	# end def complete

# end class PythonIndenter

# Simplified user interface
# - xxx_filter(input, output): read and write file objects
# - xxx_string(s): take and return string object
# - xxx_file(filename): process file in place, return true iff changed

def complete_filter(input= sys.stdin, output = sys.stdout,
		    stepsize = STEPSIZE, tabsize = TABSIZE):
	pi = PythonIndenter(input, output, stepsize, tabsize)
	pi.complete()
# end def complete_filter

def reformat_filter(input = sys.stdin, output = sys.stdout,
		    stepsize = STEPSIZE, tabsize = TABSIZE):
	pi = PythonIndenter(input, output, stepsize, tabsize)
	pi.reformat()
# end def reformat

class StringReader:
	def __init__(self, buf):
		self.buf = buf
		self.pos = 0
		self.len = len(self.buf)
	# end def __init__
	def read(self, n = 0):
		if n <= 0:
			n = self.len - self.pos
		else:
			n = min(n, self.len - self.pos)
		# end if
		r = self.buf[self.pos : self.pos + n]
		self.pos = self.pos + n
		return r
	# end def read
	def readline(self):
		i = string.find(self.buf, '\n', self.pos)
		return self.read(i + 1 - self.pos)
	# end def readline
	def readlines(self):
		lines = []
		line = self.readline()
		while line:
			lines.append(line)
			line = self.readline()
		# end while
		return lines
	# end def readlines
	# seek/tell etc. are left as an exercise for the reader
# end class StringReader

class StringWriter:
	def __init__(self):
		self.buf = ''
	# end def __init__
	def write(self, s):
		self.buf = self.buf + s
	# end def write
	def getvalue(self):
		return self.buf
	# end def getvalue
# end class StringWriter

def complete_string(source, stepsize = STEPSIZE, tabsize = TABSIZE):
	input = StringReader(source)
	output = StringWriter()
	pi = PythonIndenter(input, output, stepsize, tabsize)
	pi.complete()
	return output.getvalue()
# end def complete_string

def reformat_string(source, stepsize = STEPSIZE, tabsize = TABSIZE):
	input = StringReader(source)
	output = StringWriter()
	pi = PythonIndenter(input, output, stepsize, tabsize)
	pi.reformat()
	return output.getvalue()
# end def reformat_string

def complete_file(filename, stepsize = STEPSIZE, tabsize = TABSIZE):
	source = open(filename, 'r').read()
	result = complete_string(source, stepsize, tabsize)
	if source == result: return 0
	# end if
	import os
	try: os.rename(filename, filename + '~')
	except os.error: pass
	# end try
	f = open(filename, 'w')
	f.write(result)
	f.close()
	return 1
# end def complete_file

def reformat_file(filename, stepsize = STEPSIZE, tabsize = TABSIZE):
	source = open(filename, 'r').read()
	result = reformat_string(source, stepsize, tabsize)
	if source == result: return 0
	# end if
	import os
	os.rename(filename, filename + '~')
	f = open(filename, 'w')
	f.write(result)
	f.close()
	return 1
# end def reformat_file

# Test program when called as a script

usage = """
usage: pindent (-c|-r) [-s stepsize] [-t tabsize] [file] ...
-c         : complete a correctly indented program (add #end directives)
-r         : reformat a completed program (use #end directives)
-s stepsize: indentation step (default %(STEPSIZE)d)
-t tabsize : the worth in spaces of a tab (default %(TABSIZE)d)
[file] ... : files are changed in place, with backups in file~
If no files are specified or a single - is given,
the program acts as a filter (reads stdin, writes stdout).
""" % vars()

def test():
	import getopt
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'crs:t:')
	except getopt.error, msg:
		sys.stderr.write('Error: %s\n' % msg)
		sys.stderr.write(usage)
		sys.exit(2)
	# end try
	action = None
	stepsize = STEPSIZE
	tabsize = TABSIZE
	for o, a in opts:
		if o == '-c':
			action = 'complete'
		elif o == '-r':
			action = 'reformat'
		elif o == '-s':
			stepsize = string.atoi(a)
		elif o == '-t':
			tabsize = string.atoi(a)
		# end if
	# end for
	if not action:
		sys.stderr.write(
			'You must specify -c(omplete) or -r(eformat)\n')
		sys.stderr.write(usage)
		sys.exit(2)
	# end if
	if not args or args == ['-']:
		action = eval(action + '_filter')
		action(sys.stdin, sys.stdout, stepsize, tabsize)
	else:
		action = eval(action + '_file')
		for file in args:
			action(file, stepsize, tabsize)
		# end for
	# end if
# end def test

if __name__ == '__main__':
	test()
# end if
