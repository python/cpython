# A generic Python debugger base class.
# This class takes care of details of the trace facility;
# a derived class should implement user interaction.
# There are two debuggers based upon this:
# 'pdb', a text-oriented debugger not unlike dbx or gdb;
# and 'wdb', a window-oriented debugger.
# And of course... you can roll your own!

import sys
import types

BdbQuit = 'bdb.BdbQuit' # Exception to give up completely


class Bdb: # Basic Debugger
	
	def __init__(self):
		self.breaks = {}
		self.cbreaks = {}
	
	def reset(self):
		import linecache
		linecache.checkcache()
		self.botframe = None
		self.stopframe = None
		self.returnframe = None
		self.quitting = 0
	
	def trace_dispatch(self, frame, event, arg):
		if self.quitting:
			return # None
		if event == 'line':
			return self.dispatch_line(frame)
		if event == 'call':
			return self.dispatch_call(frame, arg)
		if event == 'return':
			return self.dispatch_return(frame, arg)
		if event == 'exception':
			return self.dispatch_exception(frame, arg)
		print 'bdb.Bdb.dispatch: unknown debugging event:', `event`
		return self.trace_dispatch
	
	def dispatch_line(self, frame):
		if self.stop_here(frame) or self.break_here(frame):
			self.user_line(frame)
			if self.quitting: raise BdbQuit
		return self.trace_dispatch
	
	def dispatch_call(self, frame, arg):
		frame.f_locals['__args__'] = arg
		if self.botframe is None:
			# First call of dispatch since reset()
			self.botframe = frame
			return self.trace_dispatch
		if not (self.stop_here(frame) or self.break_anywhere(frame)):
			# No need to trace this function
			return # None
		self.user_call(frame, arg)
		if self.quitting: raise BdbQuit
		return self.trace_dispatch
	
	def dispatch_return(self, frame, arg):
		if self.stop_here(frame) or frame == self.returnframe:
			self.user_return(frame, arg)
			if self.quitting: raise BdbQuit
	
	def dispatch_exception(self, frame, arg):
		if self.stop_here(frame):
			self.user_exception(frame, arg)
			if self.quitting: raise BdbQuit
		return self.trace_dispatch
	
	# Normally derived classes don't override the following
	# methods, but they may if they want to redefine the
	# definition of stopping and breakpoints.
	
	def stop_here(self, frame):
		if self.stopframe is None:
			return 1
		if frame is self.stopframe:
			return 1
		while frame is not None and frame is not self.stopframe:
			if frame is self.botframe:
				return 1
			frame = frame.f_back
		return 0
	
	def break_here(self, frame):
		filename=frame.f_code.co_filename
		if not self.breaks.has_key(filename):
			return 0
		lineno=frame.f_lineno
		if not lineno in self.breaks[filename]:
			return 0
		if self.cbreaks.has_key((filename, lineno)):
			cond=self.cbreaks[filename, lineno]
			return eval(cond, frame.f_globals,
				    frame.f_locals)
		return 1
	
	def break_anywhere(self, frame):
		return self.breaks.has_key(frame.f_code.co_filename)
	
	# Derived classes should override the user_* methods
	# to gain control.
	
	def user_call(self, frame, argument_list):
		# This method is called when there is the remote possibility
		# that we ever need to stop in this function
		pass
	
	def user_line(self, frame):
		# This method is called when we stop or break at this line
		pass
	
	def user_return(self, frame, return_value):
		# This method is called when a return trap is set here
		pass
	
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		# This method is called if an exception occurs,
		# but only if we are to stop at or just below this level
		pass
	
	# Derived classes and clients can call the following methods
	# to affect the stepping state.
	
	def set_step(self):
		# Stop after one line of code
		self.stopframe = None
		self.returnframe = None
		self.quitting = 0
	
	def set_next(self, frame):
		# Stop on the next line in or below the given frame
		self.stopframe = frame
		self.returnframe = None
		self.quitting = 0
	
	def set_return(self, frame):
		# Stop when returning from the given frame
		self.stopframe = frame.f_back
		self.returnframe = frame
		self.quitting = 0
	
	def set_trace(self):
		# Start debugging from here
		try:
			1 + ''
		except:
			frame = sys.exc_traceback.tb_frame.f_back
		self.reset()
		while frame:
			frame.f_trace = self.trace_dispatch
			self.botframe = frame
			frame = frame.f_back
		self.set_step()
		sys.settrace(self.trace_dispatch)

	def set_continue(self):
		# Don't stop except at breakpoints or when finished
		self.stopframe = self.botframe
		self.returnframe = None
		self.quitting = 0
		if not self.breaks:
			# no breakpoints; run without debugger overhead
			sys.settrace(None)
			try:
				1 + ''	# raise an exception
			except:
				frame = sys.exc_traceback.tb_frame.f_back
			while frame and frame is not self.botframe:
				del frame.f_trace
				frame = frame.f_back
	
	def set_quit(self):
		self.stopframe = self.botframe
		self.returnframe = None
		self.quitting = 1
		sys.settrace(None)
	
	# Derived classes and clients can call the following methods
	# to manipulate breakpoints.  These methods return an
	# error message is something went wrong, None if all is well.
	# Call self.get_*break*() to see the breakpoints.
	
	def set_break(self, filename, lineno, cond=None):
		import linecache # Import as late as possible
		line = linecache.getline(filename, lineno)
		if not line:
			return 'That line does not exist!'
		if not self.breaks.has_key(filename):
			self.breaks[filename] = []
		list = self.breaks[filename]
		if lineno in list:
			return 'There is already a breakpoint there!'
		list.append(lineno)
		if cond is not None: self.cbreaks[filename, lineno]=cond
	
	def clear_break(self, filename, lineno):
		if not self.breaks.has_key(filename):
			return 'There are no breakpoints in that file!'
		if lineno not in self.breaks[filename]:
			return 'There is no breakpoint there!'
		self.breaks[filename].remove(lineno)
		if not self.breaks[filename]:
			del self.breaks[filename]
		try: del self.cbreaks[filename, lineno]
		except: pass
	
	def clear_all_file_breaks(self, filename):
		if not self.breaks.has_key(filename):
			return 'There are no breakpoints in that file!'
		del self.breaks[filename]
		for f,l in self.cbreaks.keys():
		    if f==filename: del self.cbreaks[f,l]
	
	def clear_all_breaks(self):
		if not self.breaks:
			return 'There are no breakpoints!'
		self.breaks = {}
		self.cbreaks = {}
	
	def get_break(self, filename, lineno):
		return self.breaks.has_key(filename) and \
			lineno in self.breaks[filename]
	
	def get_file_breaks(self, filename):
		if self.breaks.has_key(filename):
			return self.breaks[filename]
		else:
			return []
	
	def get_all_breaks(self):
		return self.breaks
	
	# Derived classes and clients can call the following method
	# to get a data structure representing a stack trace.
	
	def get_stack(self, f, t):
		stack = []
		if t and t.tb_frame is f:
			t = t.tb_next
		while f is not None:
			stack.append((f, f.f_lineno))
			if f is self.botframe:
				break
			f = f.f_back
		stack.reverse()
		i = max(0, len(stack) - 1)
		while t is not None:
			stack.append((t.tb_frame, t.tb_lineno))
			t = t.tb_next
		return stack, i
	
	# 
	
	def format_stack_entry(self, frame_lineno, lprefix=': '):
		import linecache, repr, string
		frame, lineno = frame_lineno
		filename = frame.f_code.co_filename
		s = filename + '(' + `lineno` + ')'
		if frame.f_code.co_name:
		    s = s + frame.f_code.co_name
		else:
		    s = s + "<lambda>"
		if frame.f_locals.has_key('__args__'):
			args = frame.f_locals['__args__']
		else:
			args = None
		if args:
			s = s + repr.repr(args)
		else:
			s = s + '()'
		if frame.f_locals.has_key('__return__'):
			rv = frame.f_locals['__return__']
			s = s + '->'
			s = s + repr.repr(rv)
		line = linecache.getline(filename, lineno)
		if line: s = s + lprefix + string.strip(line)
		return s
	
	# The following two methods can be called by clients to use
	# a debugger to debug a statement, given as a string.
	
	def run(self, cmd, globals=None, locals=None):
		if globals is None:
			import __main__
			globals = __main__.__dict__
		if locals is None:
			locals = globals
		self.reset()
		sys.settrace(self.trace_dispatch)
		if type(cmd) <> types.CodeType:
			cmd = cmd+'\n'
		try:
			try:
				exec cmd in globals, locals
			except BdbQuit:
				pass
		finally:
			self.quitting = 1
			sys.settrace(None)
	
	def runeval(self, expr, globals=None, locals=None):
		if globals is None:
			import __main__
			globals = __main__.__dict__
		if locals is None:
			locals = globals
		self.reset()
		sys.settrace(self.trace_dispatch)
		if type(expr) <> types.CodeType:
			expr = expr+'\n'
		try:
			try:
				return eval(expr, globals, locals)
			except BdbQuit:
				pass
		finally:
			self.quitting = 1
			sys.settrace(None)

	def runctx(self, cmd, globals, locals):
		# B/W compatibility
		self.run(cmd, globals, locals)

	# This method is more useful to debug a single function call.

	def runcall(self, func, *args):
		self.reset()
		sys.settrace(self.trace_dispatch)
		res = None
		try:
			try:
				res = apply(func, args)
			except BdbQuit:
				pass
		finally:
			self.quitting = 1
			sys.settrace(None)
		return res


def set_trace():
	Bdb().set_trace()

# -------------------- testing --------------------

class Tdb(Bdb):
	def user_call(self, frame, args):
		name = frame.f_code.co_name
		if not name: name = '???'
		print '+++ call', name, args
	def user_line(self, frame):
		import linecache, string
		name = frame.f_code.co_name
		if not name: name = '???'
		fn = frame.f_code.co_filename
		line = linecache.getline(fn, frame.f_lineno)
		print '+++', fn, frame.f_lineno, name, ':', string.strip(line)
	def user_return(self, frame, retval):
		print '+++ return', retval
	def user_exception(self, frame, exc_stuff):
		print '+++ exception', exc_stuff
		self.set_continue()

def foo(n):
	print 'foo(', n, ')'
	x = bar(n*10)
	print 'bar returned', x

def bar(a):
	print 'bar(', a, ')'
	return a/2

def test():
	t = Tdb()
	t.run('import bdb; bdb.foo(10)')
