# A generic Python debugger base class.
# This class takes care of details of the trace facility;
# a derived class should implement user interaction.
# There are two debuggers based upon this:
# 'pdb', a text-oriented debugger not unlike dbx or gdb;
# and 'wdb', a window-oriented debugger.
# And of course... you can roll your own!

import sys

BdbQuit = 'bdb.BdbQuit' # Exception to give up completely


class Bdb: # Basic Debugger
	
	def init(self):
		self.breaks = {}
		return self
	
	def reset(self):
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
	# functions, but they may if they want to redefine the
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
		if not self.breaks.has_key(frame.f_code.co_filename):
			return 0
		if not frame.f_lineno in \
				self.breaks[frame.f_code.co_filename]:
			return 0
		return 1
	
	def break_anywhere(self, frame):
		return self.breaks.has_key(frame.f_code.co_filename)
	
	# Derived classes should override the user_* functions
	# to gain control.
	
	def user_call(self, frame, argument_list):
		# This function is called when there is the remote possibility
		# that we ever need to stop in this function
		pass
	
	def user_line(self, frame):
		# This function is called when we stop or break at this line
		pass
	
	def user_return(self, frame, return_value):
		# This function is called when a return trap is set here
		pass
	
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		# This function is called if an exception occurs,
		# but only if we are to stop at or just below this level
		pass
	
	# Derived classes and clients can call the following functions
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
	
	def set_continue(self):
		# Don't stop except at breakpoints or when finished
		self.stopframe = self.botframe
		self.returnframe = None
		self.quitting = 0
	
	def set_quit(self):
		self.stopframe = self.botframe
		self.returnframe = None
		self.quitting = 1
		sys.settrace(None)
	
	# Derived classes and clients can call the following functions
	# to manipulate breakpoints.  These functions return an
	# error message is something went wrong, None if all is well.
	# Call self.get_*break*() to see the breakpoints.
	
	def set_break(self, filename, lineno):
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
	
	def clear_break(self, filename, lineno):
		if not self.breaks.has_key(filename):
			return 'There are no breakpoints in that file!'
		if lineno not in self.breaks[filename]:
			return 'There is no breakpoint there!'
		self.breaks[filename].remove(lineno)
		if not self.breaks[filename]:
			del self.breaks[filename]
	
	def clear_all_file_breaks(self, filename):
		if not self.breaks.has_key(filename):
			return 'There are no breakpoints in that file!'
		del self.breaks[filename]
	
	def clear_all_breaks(self, filename, lineno):
		if not self.breaks:
			return 'There are no breakpoints!'
		self.breaks = {}
	
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
	
	# Derived classes and clients can call the following function
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
	
	def format_stack_entry(self, (frame, lineno)):
		import codehack, linecache, repr, string
		filename = frame.f_code.co_filename
		s = filename + '(' + `lineno` + ')'
		s = s + codehack.getcodename(frame.f_code)
		if frame.f_locals.has_key('__args__'):
			args = frame.f_locals['__args__']
			if args is not None:
				s = s + repr.repr(args)
		if frame.f_locals.has_key('__return__'):
			rv = frame.f_locals['__return__']
			s = s + '->'
			s = s + repr.repr(rv)
		line = linecache.getline(filename, lineno)
		if line: s = s + ': ' + string.strip(line)
		return s
	
	# The following two functions can be called by clients to use
	# a debugger to debug a statement, given as a string.
	
	def run(self, cmd):
		import __main__
		dict = __main__.__dict__
		self.runctx(cmd, dict, dict)
	
	def runctx(self, cmd, globals, locals):
		self.reset()
		sys.settrace(self.trace_dispatch)
		try:
			exec(cmd + '\n', globals, locals)
		except BdbQuit:
			pass
		finally:
			self.quitting = 1
			sys.settrace(None)
		# XXX What to do if the command finishes normally?


# -------------------- testing --------------------

class Tdb(Bdb):
	def user_call(self, frame, args):
		import codehack
		name = codehack.getcodename(frame.f_code)
		if not name: name = '???'
		print '+++ call', name, args
	def user_line(self, frame):
		import linecache, string, codehack
		name = codehack.getcodename(frame.f_code)
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
	import linecache
	linecache.checkcache()
	t = Tdb().init()
	t.run('import bdb; bdb.foo(10)')
