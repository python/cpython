# pdb.py -- finally, a Python debugger!

# To use the debugger in its simplest form:
#	>>> import pdb
#	>>> pdb.run('<a statement>')
# The debugger's prompt is '(Pdb) '.
# This will stop in the first function call in <a statement>.

# The commands recognized by the debugger are listed below.
# Most can be abbreviated as indicated; e.g., h(elp) means that
# 'help' can be typed as 'h' or 'help'
# (but not as 'he' or 'hel', nor as 'H' or 'Help' or 'HELP').
# Optional arguments are enclosed in square brackets.

# A blank line repeats the previous command literally.
# (Except for 'list', where it lists the next 11 lines.)

# Commands that the debugger does not recognized are assumed to
# be Python statements and are executed in the context of the
# program being debugged.
# Python statements can also be prefixed with an exclamation point ('!').
# This is a powerful way to inspect the program being debugged;
# it is even possible to change variables.
# When an exception occurs in such a statement, the exception name
# is printed but the debugger's state is not changed.

# The debugger is not directly programmable; but it is implemented
# as a class from which you can derive your own debugger class,
# so you can make as fancy as you like.

# The debugger's commands are:

# h(elp)
#	Without argument, print the list of available commands.
#	With a command name as argument, print help about that command
#	(this is currently not implemented).

# w(here)
#	Print a stack trace, with the most recent frame at the bottom.
#	An arrow indicates the "current frame", which determines the
#	context of most commands.

# d(own)
#	Move the current frame one level down in the stack trace
#	(to an older frame).

# u(p)
#	Move the current frame one level up in the stack trace
#	(to a newer frame).

# b(reak) [lineno]
#	With a line number argument, set a break there in the current file.
#	Without argument, list all breaks.

# cl(ear) [lineno]
#	With a line number argument, clear that break in the current file.
#	Without argument, clear all breaks (but first ask confirmation).

# s(tep)
#	Execute the current line, stop at the first possible occasion
#	(either in a function that is called or in the current function).

# n(ext)
#	Continue execution until the next line in the current function
#	is reached or it returns.

# r(eturn)
#	Continue execution until the current function returns.

# c(ont(inue))
#	Continue execution, only stop when a breakpoint is encountered.

# l(ist) [first [,last]]
#	List source code for the current file.
#	Without arguments, list 11 lines around the current line
#	or continue the previous listing.
#	With one argument, list 11 lines starting at that line.
#	With two arguments, list the given range;
#	if the second argument is less than the first, it is a count.

# a(rgs)
#	Print the argument list of the current function.

# p expression
#	Print the value of the expression.

# (!) statement
#	Execute the (one-line) statement in the context of
#	the current stack frame.
#	The exclamation point can be omitted unless the first word
#	of the statement resembles a debugger command.
#	To assign to a global variable you must always prefix the
#	command with a 'global' command, e.g.:
#	(Pdb) global list_options; list_options = ['-l']
#	(Pdb)

# q(uit)
#	Quit from the debugger.
#	The program being executed is aborted.


# Here's how it works.

# Some changes were made to the interpreter:
# - if sys.trace is defined (by the user), it should be a function
# - sys.trace is called the global trace function
# - there can also a local trace function (see later)

# Trace functions have three arguments: (frame, event, arg)
#   - frame is the current stack frame
#   - event is a string: 'call', 'line', 'return' or 'exception'
#   - arg is dependent on the event type
# A trace function should return a new trace function or None.
# Class methods are accepted (and most useful!) as trace methods.

# The events have the following meaning:
#
#   'call':      A function is called (or some other code block entered).
#                The global trace function is called;
#                arg is the argument list to the function;
#                the return value specifies the local trace function.
#
#   'line':      The interpreter is about to execute a new line of code
#                (sometimes multiple line events on one line exist).
#                The local trace function is called; arg in None;
#                the return value specifies the new local trace function.
#
#   'return':    A function (or other code block) is about to return.
#                The local trace function is called;
#                arg is the value that will be returned.
#                The trace function's return value is ignored.
#
#   'exception': An exception has occurred.
#                The local trace function is called if there is one,
#                else the global trace function is called;
#                arg is a triple (exception, value, traceback);
#                the return value specifies the new local trace function
#
# Note that as an exception is propagated down the chain of callers,
# an 'exception' event is generated at each level.

# A stack frame object has the following read-only attributes:
#   f_code:      the code object being executed
#   f_lineno:    the current line number (-1 for 'call' events)
#   f_back:      the stack frame of the caller, or None
#   f_locals:    dictionary containing local name bindings
#   f_globals:   dictionary containing global name bindings

# A code object has the following read-only attributes:
#   co_code:     the code string
#   co_names:    the list of names used by the code
#   co_consts:   the list of (literal) constants used by the code
#   co_filename: the filename from which the code was compiled


import string
import sys
import linecache


# A generic class to build command interpreters

PROMPT = '(Cmd) '
IDENTCHARS = string.letters + string.digits + '_'

class Cmd:
	def init(self):
		self.prompt = PROMPT
		self.identchars = IDENTCHARS
		self.lastcmd = ''
		return self
	def cmdloop(self):
		stop = None
		while not stop:
			try:
				line = raw_input(self.prompt)
			except EOFError:
				line = 'EOF'
			stop = self.onecmd(line)
		return stop
	def onecmd(self, line):
		line = string.strip(line)
		if not line:
			line = self.lastcmd
			print line
		else:
			self.lastcmd = line
		i, n = 0, len(line)
		while i < n and line[i] in self.identchars: i = i+1
		cmd, arg = line[:i], string.strip(line[i:])
		if cmd == '':
			return self.default(line)
		else:
			try:
				func = eval('self.do_' + cmd)
			except AttributeError:
				return self.default(line)
			return func(arg)
	def default(self, line):
		print '*** Unknown syntax:', line
	def do_help(self, arg):
		if arg:
			# XXX check arg syntax
			try:
				func = eval('self.help_' + arg)
			except:
				print '*** No help on', `arg`
				return
			func()
		else:
			import getattr
			names = getattr.dir(self)
			cmds = []
			for name in names:
				if name[:3] == 'do_':
					cmds.append(name[3:])
			print cmds


# A specialization of Cmd for use by the debugger

PdbQuit = 'pdb.PdbQuit' # Exception to give up

class Pdb(Cmd):
	
	def init(self):
		self = Cmd.init(self)
		self.prompt = '(Pdb) '
		self.reset()
		return self
	
	def reset(self):
		self.breaks = {}
		self.botframe = None
		self.stopframe = None
		self.forget()
	
	def forget(self):
		self.setup(None)
	
	def setup(self, frame):
		self.curframe = self.topframe = frame
		self.stack = []
		self.lineno = None
	
	def run(self, cmd):
		import __main__
		dict = __main__.__dict__
		self.runctx(cmd, dict, dict)
	
	def runctx(self, cmd, globals, locals):
		self.reset()
		sys.trace = self.dispatch
		try:
			exec(cmd + '\n', globals, locals)
		except PdbQuit:
			pass
		finally:
			sys.trace = None
			del sys.trace
			self.reset()
	
	def dispatch(self, frame, event, arg):
		if event == 'line':
			return self.dispatch_line(frame)
		if event == 'call':
			return self.dispatch_call(frame, arg)
		if event == 'return':
			return self.dispatch_return(frame, arg)
		if event == 'exception':
			return self.dispatch_exception(frame, arg)
		print '*** dispatch: unknown event type', `event`
		return self.dispatch
	
	def dispatch_line(self, frame):
		if self.stop_here(frame) or self.break_here(frame):
			self.ask_user(frame)
		return self.dispatch
	
	def dispatch_call(self, frame, arg):
		if self.botframe is None:
			self.botframe = frame
			return
		if not (self.stop_here(frame) or self.break_anywhere(frame)):
			return
		frame.f_locals['__args__'] = arg
		return self.dispatch
	
	def dispatch_return(self, frame, arg):
		if self.stop_here(frame):
			print '!!! return', `arg`
		return
	
	def dispatch_exception(self, frame, arg):
		if arg[0] is PdbQuit: return None
		if self.stop_here(frame):
			print '!!! exception', arg[0] + ':', `arg[1]`
			self.ask_user(frame)
		return self.dispatch
	
	def stop_here(self, frame):
		if self.stopframe is None:
			return 1
		if frame is self.stopframe:
			return 1
		while frame is not self.stopframe:
			if frame is None:
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
	
	def ask_user(self, frame):
		self.setup(frame)
		self.printwhere(self.curframe)
		dummy = self.cmdloop()
		self.forget()
	
	def default(self, line):
		if not line:
			return self.do_next('')
		else:
			if line[0] == '!': line = line[1:]
			try:
				exec(line + '\n', \
					self.curframe.f_globals, \
					self.curframe.f_locals)
			except:
				print '***', sys.exc_type				 + ':',
				print `sys.exc_value`
	
	do_h = Cmd.do_help
	
	def do_break(self, arg):
		if not arg:
			print self.breaks # XXX
			return
		try:
			lineno = int(eval(arg))
		except:
			print '*** Error in argument:', `arg`
			return
		filename = self.curframe.f_code.co_filename
		line = linecache.getline(filename, lineno)
		if not line:
			print '*** That line does not exist!'
			return
		if not self.breaks.has_key(filename):
			self.breaks[filename] = []
		list = self.breaks[filename]
		if lineno in list:
			print '*** There is already a break there!'
			return
		list.append(lineno)
	do_b = do_break
	
	def do_clear(self, arg):
		if not arg:
			try:
				reply = raw_input('Clear all breaks? ')
			except EOFError:
				reply = 'no'
			reply = string.lower(string.strip(reply))
			if reply in ('y', 'yes'):
				self.breaks = {}
			return
		try:
			lineno = int(eval(arg))
		except:
			print '*** Error in argument:', `arg`
			return
		filename = self.curframe.f_code.co_filename
		try:
			self.breaks[filename].remove(lineno)
		except (ValueError, KeyError):
			print '*** There is no break there!'
			return
		if not self.breaks[filename]:
			del self.breaks[filename]
	do_cl = do_clear # 'c' is already an abbreviation for 'continue'
	
	def do_where(self, arg):
		self.printtb()
	do_w = do_where
	
	def do_up(self, arg):
		if self.curframe == self.botframe or \
			not self.curframe.f_back: print '*** Top'
		else:
			self.stack.append(self.curframe)
			self.curframe = self.curframe.f_back
			self.lineno = None
			self.printwhere(self.curframe)
	do_u = do_up
	
	def do_down(self, arg):
		if not self.stack: print '*** Bottom'
		else:
			self.curframe = self.stack[-1]
			self.lineno = None
			del self.stack[-1]
			self.printwhere(self.curframe)
	do_d = do_down
	
	def do_step(self, arg):
		self.stopframe = None
		return 1
	do_s = do_step
	
	def do_next(self, arg):
		self.stopframe = self.curframe
		return 1
	do_n = do_next
	
	def do_return(self, arg):
		self.stopframe = self.curframe.f_back
		return 1
	do_r = do_return
	
	def do_continue(self, arg):
		self.stopframe = self.botframe
		return 1
	do_c = do_cont = do_continue
	
	def do_quit(self, arg):
		self.stopframe = self.botframe
		raise PdbQuit
	do_q = do_quit
	
	def do_list(self, arg):
		self.lastcmd = 'list'
		last = None
		if arg:
			try:
				x = eval(arg, {}, {})
				if type(x) == type(()):
					first, last = x
					first = int(first)
					last = int(last)
					if last < first:
						# Assume it's a count
						last = first + last
				else:
					first = int(x)
			except:
				print '*** Error in argument:', `arg`
				return
		elif self.lineno is None:
			first = max(1, self.curframe.f_lineno - 5)
		else:
			first = self.lineno + 1
		if last is None:
			last = first + 10
		filename = self.curframe.f_code.co_filename
		if self.breaks.has_key(filename):
			breaklist = self.breaks[filename]
		else:
			breaklist = []
		try:
			for lineno in range(first, last+1):
				line = linecache.getline(filename, lineno)
				if not line:
					print '[EOF]'
					break
				else:
					s = string.rjust(`lineno`, 3)
					if len(s) < 4: s = s + ' '
					if lineno in breaklist: s = s + 'B'
					else: s = s + ' '
					if lineno == self.curframe.f_lineno:
						s = s + '->'
					print s + '\t' + line,
					self.lineno = lineno
		except KeyboardInterrupt:
			pass
	do_l = do_list
	
	def do_args(self, arg):
		try:
			value = eval('__args__', self.curframe.f_globals, \
					self.curframe.f_locals)
		except:
			print '***', sys.exc_type + ':', `sys.exc_value`
			return
		print `value`
	do_a = do_args
	
	def do_p(self, arg):
		try:
			value = eval(arg, self.curframe.f_globals, \
					self.curframe.f_locals)
		except:
			print '***', sys.exc_type + ':', `sys.exc_value`
			return
		print `value`

	# Print a traceback starting at a given stack frame
	# Note that it is printed upside-down with respect
	# to the orientation suggested by the up/down commands.
	# This is consistent with gdb.
	def printtb(self):
		list = []
		frame = self.topframe
		while frame:
			list.append(frame)
			if frame is self.botframe: break
			frame = frame.f_back
		list.reverse()
		for frame in list:
			self.printwhere(frame)
	
	def printwhere(self, frame):
		if frame is self.curframe: print '->',
		code = frame.f_code
		filename = code.co_filename
		lineno = frame.f_lineno
		print filename + '(' + `lineno` + ')',
		line = linecache.getline(filename, lineno)
		if line: print string.strip(line),
		print


# --------------------- testing ---------------------

# The Ackermann function -- a highly recursive beast
cheat = 0
cache = {}
def ack(x, y):
	key = `(long(x), long(y))`
	if cache.has_key(key):
		res = cache[key]
	else:
		if x == 0:
			res = 1L
		elif y == 0:
			if x == 1:
				res = 2L
			else:
				res = 2L + x
		elif y == 1 and cheat >= 1:
			res = 2L * x
		elif y == 2 and cheat >= 2:
			res = pow(2L, x)
		else:
			res = ack(ack(x-1, y), y-1)
		cache[key] = res
	return res

def foo(n):
	print 'foo', n
	x = bar(n*2)
	print 'bar returned', x
	return

def bar(a):
	print 'bar', a
	return a*10

def test():
	linecache.checkcache()
	Pdb().init().run('foo(12)\n')


# --------------------- main ---------------------

import os

def main():
	if sys.argv[1:]:
		file = sys.argv[1]
		head, tail = os.path.split(file)
		if tail[-3:] != '.py':
			print 'Sorry, file arg must be a python module'
			print '(i.e., must end in \'.py\')'
			# XXX Or we could copy it to a temp file
			sys.exit(2)
		del sys.argv[0]
		sys.path.insert(0, head)
		run('import ' + tail[:-3])
	else:
		run('')

def run(statement):
	Pdb().init().run(statement)
