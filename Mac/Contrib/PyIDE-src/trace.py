__version__ = "$Id$"

import sys
from time import time

class Trace:
	def __init__(self):
		self.dispatch = {
			'call': self.trace_dispatch_call,
			'return': self.trace_dispatch_return,
			'exception': self.trace_dispatch_exception,
			}
		self.curframe = None
		self.depth = -1
		self.stdout = sys.stdout

	def run(self, cmd, globals = None, locals = None):
		if globals is None:
			import __main__
			globals = __main__.__dict__
		if locals is None:
			locals = globals
		sys.setprofile(self.trace_dispatch)
		try:
			exec cmd in globals, locals
		finally:
			sys.setprofile(None)

	def runcall(self, func, *args):
		sys.setprofile(self.trace_dispatch)
		try:
			apply(func, args)
		finally:
			sys.setprofile(None)

	def trace_dispatch(self, frame, event, arg):
		curstdout = sys.stdout
		sys.stdout = self.stdout
		self.dispatch[event](frame, arg)
		sys.stdout = curstdout

	def trace_dispatch_call(self, frame, arg):
		self.depth = self.depth + 1
		self.curframe = frame
		code = frame.f_code
		funcname = code.co_name
		if not funcname:
			funcname = '<lambda>'
		filename = code.co_filename
		lineno = frame.f_lineno
		if lineno == -1:
			code = code.co_code
			if ord(code[0]) == 127:	# SET_LINENO
				lineno = ord(code[1]) | ord(code[2]) << 8
		pframe = frame.f_back
		if pframe:
			plineno = ' (%d)' % pframe.f_lineno
		else:
			plineno = ''
		print '%s> %s:%d %s%s' % (' '*self.depth,filename,lineno,funcname,plineno)
		frame.f_locals['__start_time'] = time()

	def trace_dispatch_return(self, frame, arg):
		try:
			t = frame.f_locals['__start_time']
		except KeyError:
			t = ''
		else:
			t = ' [%.4f]' % (time() - t)
		funcname = frame.f_code.co_name
		self.curframe = frame.f_back
		if not funcname:
			funcname = '<lambda>'
		filename = frame.f_code.co_filename
		print '%s< %s:%d %s%s' % (' '*self.depth,filename,frame.f_lineno,funcname,t)
		self.depth = self.depth - 1

	def trace_dispatch_exception(self, frame, arg):
		t = ''
		if frame is not self.curframe:
			try:
				t = frame.f_locals['__start_time']
			except KeyError:
				pass
			else:
				t = ' [%.4f]' % (time() - t)
			self.depth = self.depth - 1
			self.curframe = frame
		funcname = frame.f_code.co_name
		if not funcname:
			funcname = '<lambda>'
		filename = frame.f_code.co_filename
		print '%sE %s:%d %s%s' % (' '*(self.depth+1),filename,frame.f_lineno,funcname,t)

	def set_trace(self):
		try:
			raise 'xyzzy'
		except:
			frame = sys.exc_traceback.tb_frame
		while frame.f_code.co_name != 'set_trace':
			frame = frame.f_back
		d = 0
		while frame:
			d = d + 1
			frame = frame.f_back
		self.depth = d
		sys.setprofile(self.trace_dispatch)

def run(cmd, globals = None, locals = None):
	Trace().run(cmd, globals, locals)

def runcall(*func_args):
	apply(Trace().runcall, funcargs)

def set_trace():
	Trace().set_trace()

def unset_trace():
	sys.setprofile(None)
