"""quietconsole - A module to keep console I/O quiet but dump it when needed"""
import types
import sys

class _PseudoStdin:
	def __init__(self, stdouterr):
		self.keep_stdin = sys.stdin
		sys.stdin = self
		self.keep_stdouterr = stdouterr
		
	def __del__(self):
		self.keep_stdin = self.keep_stdouterr = None
		
	def _revert(self):
		"""Return to old state, with true stdio"""
		if self.keep_stdin == None:
			return
		sys.stdin = self.keep_stdin
		self.keep_stdin = None
		self.keep_stdouterr._revert(1)
		self.keep_stdouterr = None
		
	def read(self, *args):
		self._revert()
		return apply(sys.stdin.read, args)
		
	def readlines(self, *args):
		self._revert()
		return apply(sys.stdin.readlines, args)
	
	def readline(self, *args):
		self._revert()
		return apply(sys.stdin.readline, args)
		
	def close(self):
		self._revert()
		sys.stdin.close()
		
class _PseudoStdouterr:
	def __init__(self):
		self.keep_stdout = sys.stdout
		self.keep_stderr = sys.stderr
		sys.stdout = sys.stderr = self
		self.data = []
		
	def __del__(self):
		self.keep_stdout = self.keep_stderr = None
		
	def _revert(self, dumpdata=0):
		if self.keep_stdout == None:
			return
		sys.stdout = self.keep_stdout
		sys.stderr = self.keep_stderr
		sys.keep_stdout = self.keep_stderr = None
		if dumpdata and self.data:
			for d in self.data:
				sys.stdout.write(d)
		self.data = None
		
	def write(self, arg):
		self.data.append(arg)
		
	def writelines(self, arg):
		for d in arg:
			self.data.append(arg)
			
	def close(self):
		self.keep_stdout = self.keep_stderr = self.data = None
		
beenhere = 0
		
def install():
	global beenhere
	if beenhere:
		return
	beenhere = 1
	# There's no point in re-installing if the console has been active
	obj = _PseudoStdouterr()
	_PseudoStdin(obj)
	# No need to keep the objects, they're saved in sys.std{in,out,err}
	
def revert():
	if type(sys.stdin) == types.FileType:
		return	# Not installed
	sys.stdin._revert()
	
def _test():
	import time
	install()
	print "You will not see this yet"
	time.sleep(1)
	print "You will not see this yet"
	time.sleep(1)
	print "You will not see this yet"
	time.sleep(1)
	print "You will not see this yet"
	time.sleep(1)
	print "You will not see this yet"
	time.sleep(1)
	print "5 seconds have passed, now you may type something"
	rv = sys.stdin.readline()
	print "You typed", rv
	
if __name__ == '__main__':
	_test()
	
	
		
		
		
