# port of StringIO.py using arrays
# ArrayIO.py
# jjk  02/28/96  001  direct mod of StringIO, test suite checks out
# jjk  02/28/96  002  inherit from StringIO, test suite checks out
# jjk  02/28/96  003  add __xx__() functions
#
# class ArrayIO implements  file-like objects that read/write a
# string buffer (a.k.a. "memory files"). 
#
# all methods that interface with ArrayIO pass strings. Internally, however,
# ArrayIO uses an array object.
#
# the interface is the same as StringIO.py
# also handles len(a), a[i], a[i]='x', a[i:j], a[i:j] = aString
#

import string
import array
import StringIO

class ArrayIO(StringIO.StringIO):
# jjk  02/28/96
	def __init__(self, buf = ''):
	#jjk  02/28/96
		self.buf = array.array('c', buf)
		self.pos = 0
		self.closed = 0
	def __len__(self): 
	#jjk  02/28/96
		return len(self.buf)
	def __getitem__(self, key): 
	#jjk  02/28/96
		return self.buf[key]
	def __setitem__(self, key, item): 
	#jjk  02/28/96
		self.buf[key] = item
	def __getslice__(self, i, j): 
	#jjk  02/28/96
		return self.buf[i:j].tostring()
	def __setslice__(self, i, j, aString): 
	#jjk  02/28/96
		self.buf[i:j] = array.array('c', aString)
	def read(self, n = 0):
	#jjk  02/28/96
		r = StringIO.StringIO.read(self, n)
		return (r.tostring())
	def _findCharacter(self, char, start):
        #probably very slow
	#jjk  02/28/96
		for i in range(max(start, 0), len(self.buf)):
			if (self.buf[i] == char):
				return(i)
		return(-1)
	def readline(self):
	#jjk  02/28/96
		i = self._findCharacter('\n', self.pos)
		if i < 0:
			newpos = len(self.buf)
		else:
			newpos = i+1
		r = self.buf[self.pos:newpos].tostring()
		self.pos = newpos
		return r
	def write(self, s):
	#jjk  02/28/96
		if not s: return
		if self.pos > len(self.buf):
			self.buf.fromstring('\0'*(self.pos - len(self.buf)))
		newpos = self.pos + len(s)
		self.buf[self.pos:newpos] = array.array('c', s)
		self.pos = newpos
	def getvalue(self):
	#jjk  02/28/96
		return self.buf.tostring()


# A little test suite
# identical to test suite in StringIO.py , except for "f = ArrayIO()"
# too bad I couldn't inherit this :-)

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
