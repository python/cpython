# pdb.py -- finally, a Python debugger!

# To do:
# - Keep a list of exceptions trapped (default only KeyboardInterrupt?)
# - A blank line should repeat the previous command, not execute 'next'
# - Don't line-trace functions (or at least files) without breakpoints
# - Better handling of call/return events
# - It should be possible to intercept KeyboardInterrupt completely
# - Where and when to stop exactly when 'next' encounters a return?
#   (should be level-based -- don't trace anything deeper than current)
# - Show stack traces upside-down (like dbx/gdb)
# - When stopping on an exception, show traceback stack
# - Merge with tb (for post-mortem usage)

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
		return self
	def commandloop(self):
		try:
			self.innerloop()
		except EOFError:
			pass
	def innerloop(self):
		while 1:
			line = string.strip(self.getline())
			self.execline(line)
	def execline(self, line):
		i, n = 0, len(line)
		while i < n and line[i] in self.identchars: i = i+1
		cmd, arg = line[:i], string.strip(line[i:])
		if cmd == '':
			self.default(line)
		else:
			try:
				func = eval('self.do_' + cmd)
			except AttributeError:
				self.default(line)
				return
			func(arg)
	def getline(self):
		return raw_input(self.prompt)
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

PdbDone = 'PdbDone' # Exception used internally
PdbQuit = 'PdbQuit' # Exception to just give up

class Pdb(Cmd):
	
	def init(self):
		self = Cmd.init(self)
		self.prompt = '(Pdb) '
		self.reset()
		self.breaks = {}
		return self
	
	def reset(self):
		self.whatnext = ''
		self.botframe = None
		self.forget()
	
	def forget(self):
		self.curframe = self.topframe = None
		self.stack = []
		self.lineno = None
	
	def setup(self, frame):
		self.curframe = self.topframe = frame
		self.stack = []
		self.lineno = None
		self.stopframe = None
		self.whatnext = ''
	
	def run(self, cmd):
		if cmd[-1:] != '\n': cmd = cmd + '\n'
		self.reset()
		sys.trace = self.dispatch
		try:
			exec(cmd)
		except PdbQuit:
			pass
		finally:
			sys.trace = None
			del sys.trace
			self.reset()
	
	def dispatch(self, frame, where, arg):
		if self.whatnext == 'quit':
			return
		if self.botframe is None:
			self.botframe = frame
		if where == 'exception':
			if self.whatnext == 'continue' and \
				arg[0] is not KeyboardInterrupt:
				return self.trace
			stop = 1
		elif self.whatnext == 'continue':
			stop = 0
		elif self.whatnext == 'next':
			stop = (frame == self.stopframe)
		else:
			stop = 1
		if not stop:
			# Check breakpoints only
			filename = frame.f_code.co_filename
			if not self.breaks.has_key(filename):
				return self.dispatch
			lineno = frame.f_lineno
			if lineno not in self.breaks[filename]:
				return self.dispatch
		if where == 'call':
			print 'call arguments:', arg
		elif where == 'return':
			print 'return value:', arg
		elif where == 'exception':
			print 'exception:', arg[:2]
		elif where != 'line':
			print 'unknown trace type:', `where`
		self.setup(frame)
		try:
			self.whatnext = self.commandloop()
		finally:
			self.forget()
		if self.whatnext == 'quit':
			raise PdbQuit
		if self.whatnext == 'next':
			if where != 'return':
				self.stopframe = frame
			else:
				self.whatnext = 'step'
		return self.dispatch
	
	def commandloop(self):
		self.printwhere(self.curframe)
		try:
			self.innerloop()
		except EOFError:
			self.do_next('')
		except PdbDone, msg:
			return msg
	
	def default(self, line):
		if not line:
			self.do_next('')
		else:
			if line[0] == '!': line = line[1:]
			try:
				exec(line + '\n', \
					self.curframe.f_globals, \
					self.curframe.f_locals)
			except:
				print '***', sys.exc_type + ':',
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
			self.breaks = {}
			print 'All breaks cleared!'
			return
		try:
			lineno = int(eval(arg))
		except:
			print '*** Error in argument:', `arg`
			return
		filename = self.curframe.f_code.co_filename
		try:
			self.breaks[filename].remove(lineno)
			if self.breaks[filename] == []:
				del self.breaks[filename]
		except (ValueError, KeyError):
			print '*** There is no break there!'
			return
	
	def do_where(self, arg):
		self.printtb()
	do_w = do_where
	
	def do_up(self, arg):
		if not self.stack: print '*** Top'
		else:
			self.curframe = self.stack[-1]
			self.lineno = None
			del self.stack[-1]
			self.printwhere(self.curframe)
	do_u = do_up
	
	def do_down(self, arg):
		if self.curframe == self.botframe or \
			not self.curframe.f_back: print '*** Bottom'
		else:
			self.stack.append(self.curframe)
			self.curframe = self.curframe.f_back
			self.lineno = None
			self.printwhere(self.curframe)
	do_d = do_down
	
	def do_step(self, arg):
		raise PdbDone, 'step'
	do_s = do_step
	
	def do_next(self, arg):
		raise PdbDone, 'next'
	do_n = do_next
	
	def do_continue(self, arg):
		raise PdbDone, 'continue'
	do_c = do_cont = do_continue
	
	def do_quit(self, arg):
		raise PdbDone, 'quit'
	do_q = do_quit
	
	def do_list(self, arg):
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
	Pdb().init().run('foo(12)')


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
		Pdb().init().run('import ' + tail[:-3])
	else:
		Pdb().init().run('')
