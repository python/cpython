# pdb.py -- finally, a Python debugger!
# See file pdb.doc for instructions.

# To do:
# - It should be possible to intercept KeyboardInterrupt
# - Handle return events differently -- always printing the r.v. can be bad!
# - Merge with tb, to get a single debugger for active and post-mortem usage
# - Solve bugs in termination (e.g., 'continue' after the program
#   is done proceeds to debug the debugger; 'quit' sometimes complains
#   about the PdbQuit exception...)


import string
import sys
import linecache
from cmd import Cmd


# A specialization of Cmd for use by the debugger

PdbQuit = 'pdb.PdbQuit' # Exception to give up

class Pdb(Cmd):
	
	def init(self):
		self = Cmd.init(self)
		self.prompt = '(Pdb) '
		self.reset()
		return self
	
	def reset(self):
		self.quitting = 0
		self.breaks = {}
		self.botframe = None
		self.stopframe = None
		self.forget()
	
	def forget(self):
		self.setup(None, None)
	
	def setup(self, f, t):
		self.lineno = None
		self.stack = []
		if t and t.tb_frame is f:
			t = t.tb_next
		while f and f is not self.botframe:
			self.stack.append((f, f.f_lineno))
			f = f.f_back
		self.stack.reverse()
		self.curindex = max(0, len(self.stack) - 1)
		while t:
			self.stack.append((t.tb_frame, t.tb_lineno))
			t = t.tb_next
		if 0 <= self.curindex < len(self.stack):
			self.curframe = self.stack[self.curindex][0]
		else:
			self.curframe = None
	
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
		except:
			print '***', sys.exc_type + ':', `sys.exc_value`
			print '*** Post Mortem Debugging:'
			sys.trace = None
			del sys.trace
			try:
				self.ask_user(None, sys.exc_traceback)
			except PdbQuit:
				pass
		finally:
			self.reset()
	
	def dispatch(self, frame, event, arg):
		if self.quitting:
			return None
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
			self.ask_user(frame, None)
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
		if self.stop_here(frame):
			print '!!! exception', arg[0] + ':', `arg[1]`
			self.ask_user(frame, arg[2])
		return self.dispatch
	
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
	
	def ask_user(self, frame, traceback):
		self.setup(frame, traceback)
		self.printframelineno(self.stack[self.curindex])
		self.cmdloop()
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
		self.printstacktrace()
	do_w = do_where
	
	def do_up(self, arg):
		if self.curindex == 0:
			print '*** Oldest frame'
		else:
			self.curindex = self.curindex - 1
			self.curframe = self.stack[self.curindex][0]
			self.printframelineno(self.stack[self.curindex])
	do_u = do_up
	
	def do_down(self, arg):
		if self.curindex + 1 == len(self.stack):
			print '*** Newest frame'
		else:
			self.curindex = self.curindex + 1
			self.curframe = self.stack[self.curindex][0]
			self.printframelineno(self.stack[self.curindex])
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
		self.quitting = 1
		sys.trace = None; del sys.trace
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

	# Print a traceback starting at the top stack frame.
	# Note that the most recently entered frame is printed last;
	# this is different from dbx and gdb, but consistent with
	# the Python interpreter's stack trace.
	# It is also consistent with the up/down commands (which are
	# compatible with dbx and gdb: up moves towards 'main()'
	# and down moves towards the most recent stack frame).
	
	def printstacktrace(self):
		for x in self.stack:
			self.printframelineno(x)
	
	def printframelineno(self, (frame, lineno)):
		if frame is self.curframe: print '->',
		code = frame.f_code
		filename = code.co_filename
		print filename + '(' + `lineno` + ')',
		line = linecache.getline(filename, lineno)
		print string.strip(line),
		print


def run(statement):
	Pdb().init().run(statement)

def runctx(statement, globals, locals):
	Pdb().init().runctx(statement, globals, locals)


# --------------------- testing ---------------------

# The Ackermann function -- a highly recursive beast
cheat = 2
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
	y = ack(4, 3)
	return y

def bar(a):
	print 'bar', a
	return a*10

def melt(n):
	print 1.0/n
	melt(n-1)

def test():
	linecache.checkcache()
	runctx('from pdb import foo; foo(12)', {}, {})
	runctx('from pdb import melt; melt(5)', {}, {})


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
		run(raw_input('Python statement to debug: '))
