#! /usr/bin/env python

# Update a bunch of files according to a script.
# The input file contains lines of the form <filename>:<lineno>:<text>,
# meaning that the given line of the given file is to be replaced
# by the given text.  This is useful for performing global substitutions
# on grep output: 

import os
import sys
import regex

pat = '^\([^: \t\n]+\):\([1-9][0-9]*\):'
prog = regex.compile(pat)

class FileObj:
	def __init__(self, filename):
		self.filename = filename
		self.changed = 0
		try:
			self.lines = open(filename, 'r').readlines()
		except IOError, msg:
			print '*** Can\'t open "%s":' % filename, msg
			self.lines = None
			return
		print 'diffing', self.filename

	def finish(self):
		if not self.changed:
			print 'no changes to', self.filename
			return
		try:
			os.rename(self.filename, self.filename + '~')
			fp = open(self.filename, 'w')
		except (os.error, IOError), msg:
			print '*** Can\'t rewrite "%s":' % self.filename, msg
			return
		print 'writing', self.filename
		for line in self.lines:
			fp.write(line)
		fp.close()
		self.changed = 0

	def process(self, lineno, rest):
		if self.lines is None:
			print '(not processed): %s:%s:%s' % (
				  self.filename, lineno, rest),
			return
		i = eval(lineno) - 1
		if not 0 <= i < len(self.lines):
			print '*** Line number out of range: %s:%s:%s' % (
				  self.filename, lineno, rest),
			return
		if self.lines[i] == rest:
			print '(no change): %s:%s:%s' % (
				  self.filename, lineno, rest),
			return
		if not self.changed:
			self.changed = 1
		print '%sc%s' % (lineno, lineno)
		print '<', self.lines[i],
		print '---'
		self.lines[i] = rest
		print '>', self.lines[i],

def main():
	if sys.argv[1:]:
		try:
			fp = open(sys.argv[1], 'r')
		except IOError, msg:
			print 'Can\'t open "%s":' % sys.argv[1], msg
			sys.exit(1)
	else:
		fp = sys.stdin
	curfile = None
	while 1:
		line = fp.readline()
		if not line:
			if curfile: curfile.finish()
			break
		n = prog.match(line)
		if n < 0:
			print 'Funny line:', line,
			continue
		filename, lineno = prog.group(1, 2)
		if not curfile or filename <> curfile.filename:
			if curfile: curfile.finish()
			curfile = FileObj(filename)
		curfile.process(lineno, line[n:])

main()
