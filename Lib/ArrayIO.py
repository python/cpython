"""File-like objects that read/write an array buffer.

This implements (nearly) all stdio methods.

f = ArrayIO()       # ready for writing
f = ArrayIO(buf)    # ready for reading
f.close()           # explicitly release resources held
flag = f.isatty()   # always false
pos = f.tell()      # get current position
f.seek(pos)         # set current position
f.seek(pos, mode)   # mode 0: absolute; 1: relative; 2: relative to EOF
buf = f.read()      # read until EOF
buf = f.read(n)     # read up to n bytes
buf = f.readline()  # read until end of line ('\n') or EOF
list = f.readlines()# list of f.readline() results until EOF
f.write(buf)        # write at current position
f.writelines(list)  # for line in list: f.write(line)
f.getvalue()        # return whole file's contents as a string

Notes:
- This is very similar to StringIO.  StringIO is faster for reading,
  but ArrayIO is faster for writing.
- ArrayIO uses an array object internally, but all its interfaces
  accept and return strings.
- Using a real file is often faster (but less convenient).
- fileno() is left unimplemented so that code which uses it triggers
  an exception early.
- Seeking far beyond EOF and then writing will insert real null
  bytes that occupy space in the buffer.
- There's a simple test set (see end of this file).
"""

import string
from array import array

class ArrayIO:
	def __init__(self, buf = ''):
		self.buf = array('c', buf)
		self.pos = 0
		self.closed = 0
		self.softspace = 0
	def close(self):
		if not self.closed:
			self.closed = 1
			del self.buf, self.pos
	def isatty(self):
		return 0
	def seek(self, pos, mode = 0):
		if mode == 1:
			pos = pos + self.pos
		elif mode == 2:
			pos = pos + len(self.buf)
		self.pos = max(0, pos)
	def tell(self):
		return self.pos
	def read(self, n = -1):
		if n < 0:
			newpos = len(self.buf)
		else:
			newpos = min(self.pos+n, len(self.buf))
		r = self.buf[self.pos:newpos].tostring()
		self.pos = newpos
		return r
	def readline(self):
		i = string.find(self.buf[self.pos:].tostring(), '\n')
		if i < 0:
			newpos = len(self.buf)
		else:
			newpos = self.pos+i+1
		r = self.buf[self.pos:newpos].tostring()
		self.pos = newpos
		return r
	def readlines(self):
		lines = string.splitfields(self.read(), '\n')
		if not lines:
			return lines
		for i in range(len(lines)-1):
			lines[i] = lines[i] + '\n'
		if not lines[-1]:
			del lines[-1]
		return lines
	def write(self, s):
		if not s: return
		a = array('c', s)
		n = self.pos - len(self.buf)
		if n > 0:
			self.buf[len(self.buf):] = array('c', '\0')*n
		newpos = self.pos + len(a)
		self.buf[self.pos:newpos] = a
		self.pos = newpos
	def writelines(self, list):
		self.write(string.joinfields(list, ''))
	def flush(self):
		pass
	def getvalue(self):
		return self.buf.tostring()


# A little test suite

def test():
	import sys
	if sys.argv[1:]:
		file = sys.argv[1]
	else:
		file = '/etc/passwd'
	lines = open(file, 'r').readlines()
	text = open(file, 'r').read()
	f = ArrayIO()
	for line in lines[:-2]:
		f.write(line)
	f.writelines(lines[-2:])
	if f.getvalue() != text:
		raise RuntimeError, 'write failed'
	length = f.tell()
	print 'File length =', length
	f.seek(len(lines[0]))
	f.write(lines[1])
	f.seek(0)
	print 'First line =', `f.readline()`
	here = f.tell()
	line = f.readline()
	print 'Second line =', `line`
	f.seek(-len(line), 1)
	line2 = f.read(len(line))
	if line != line2:
		raise RuntimeError, 'bad result after seek back'
	f.seek(len(line2), 1)
	list = f.readlines()
	line = list[-1]
	f.seek(f.tell() - len(line))
	line2 = f.read()
	if line != line2:
		raise RuntimeError, 'bad result after seek back from EOF'
	print 'Read', len(list), 'more lines'
	print 'File length =', f.tell()
	if f.tell() != length:
		raise RuntimeError, 'bad length'
	f.close()

if __name__ == '__main__':
	test()
