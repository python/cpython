# A class that makes each part of a multipart message "feel" like an
# ordinary file, as long as you use fp.readline().  Allows recursive
# use, for nested multipart messages.  Probably best used together
# with module mimetools.
#
# Suggested use:
#
# real_fp = open(...)
# fp = MultiFile().init(real_fp)
#
# "read some lines from fp"
# fp.push(separator)
# while 1:
#	"read lines from fp until it returns an empty string" (A)
#	if not fp.next(): break
# fp.pop()
# "read remaining lines from fp until it returns an empty string"
#
# The latter sequence may be used recursively at (A).
# It is also allowed to use multiple push()...pop() sequences.
# Note that if a nested multipart message is terminated by a separator
# for an outer message, this is not reported, even though it is really
# illegal input.

import sys
import string

err = sys.stderr.write

Error = 'multifile.Error'

class MultiFile:
	#
	def init(self, fp):
		self.fp = fp
		self.stack = [] # Grows down
		self.level = 0
		self.last = 0
		self.start = self.fp.tell()
		self.posstack = [] # Grows down
		return self
	#
	def tell(self):
		if self.level > 0:
			return self.lastpos
		return self.fp.tell() - self.start
	#
	def seek(self, pos):
		if not 0 <= pos <= self.tell() or \
				self.level > 0 and pos > self.lastpos:
			raise Error, 'bad MultiFile.seek() call'
		self.fp.seek(pos + self.start)
		self.level = 0
		self.last = 0
	#
	def readline(self):
		if self.level > 0: return ''
		line = self.fp.readline()
		if not line:
			self.level = len(self.stack)
			self.last = (self.level > 0)
			if self.last:
				err('*** Sudden EOF in MultiFile.readline()\n')
			return ''
		if line[:2] <> '--': return line
		n = len(line)
		k = n
		while k > 0 and line[k-1] in string.whitespace: k = k-1
		mark = line[2:k]
		if mark[-2:] == '--': mark1 = mark[:-2]
		else: mark1 = None
		for i in range(len(self.stack)):
			sep = self.stack[i]
			if sep == mark:
				self.last = 0
				break
			elif mark1 <> None and sep == mark1:
				self.last = 1
				break
		else:
			return line
		# Get here after break out of loop
		self.lastpos = self.tell() - len(line)
		self.level = i+1
		if self.level > 1:
			err('*** Missing endmarker in MultiFile.readline()\n')
		return ''
	#
	def next(self):
		while self.readline(): pass
		if self.level > 1 or self.last:
			return 0
		self.level = 0
		self.last = 0
		self.start = self.fp.tell()
		return 1
	#
	def push(self, sep):
		if self.level > 0:
			raise Error, 'bad MultiFile.push() call'
		self.stack.insert(0, sep)
		self.posstack.insert(0, self.start)
		self.start = self.fp.tell()
	#
	def pop(self):
		if self.stack == []:
			raise Error, 'bad MultiFile.pop() call'
		if self.level <= 1:
			self.last = 0
		else:
			abslastpos = self.lastpos + self.start
		self.level = max(0, self.level - 1)
		del self.stack[0]
		self.start = self.posstack[0]
		del self.posstack[0]
		if self.level > 0:
			self.lastpos = abslastpos - self.start
	#
