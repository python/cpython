#! /usr/bin/env python

# pdb.py -- finally, a Python debugger!

# (See pdb.doc for documentation.)

import string
import sys
import linecache
import cmd
import bdb
import repr
import os


# Interaction prompt line will separate file and call info from code
# text using value of line_prefix string.  A newline and arrow may
# be to your liking.  You can set it once pdb is imported using the
# command "pdb.line_prefix = '\n% '".
# line_prefix = ': '	# Use this to get the old situation back
line_prefix = '\n-> '	# Probably a better default

class Pdb(bdb.Bdb, cmd.Cmd):
	
	def __init__(self):
		bdb.Bdb.__init__(self)
		cmd.Cmd.__init__(self)
		self.prompt = '(Pdb) '
		# Try to load readline if it exists
		try:
			import readline
		except ImportError:
			pass
	
	def reset(self):
		bdb.Bdb.reset(self)
		self.forget()
	
	def forget(self):
		self.lineno = None
		self.stack = []
		self.curindex = 0
		self.curframe = None
	
	def setup(self, f, t):
		self.forget()
		self.stack, self.curindex = self.get_stack(f, t)
		self.curframe = self.stack[self.curindex][0]
	
	# Override Bdb methods (except user_call, for now)
	
	def user_line(self, frame):
		# This function is called when we stop or break at this line
		self.interaction(frame, None)
	
	def user_return(self, frame, return_value):
		# This function is called when a return trap is set here
		frame.f_locals['__return__'] = return_value
		print '--Return--'
		self.interaction(frame, None)
	
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		# This function is called if an exception occurs,
		# but only if we are to stop at or just below this level
		frame.f_locals['__exception__'] = exc_type, exc_value
		if type(exc_type) == type(''):
			exc_type_name = exc_type
		else: exc_type_name = exc_type.__name__
		print exc_type_name + ':', repr.repr(exc_value)
		self.interaction(frame, exc_traceback)
	
	# General interaction function
	
	def interaction(self, frame, traceback):
		self.setup(frame, traceback)
		self.print_stack_entry(self.stack[self.curindex])
		self.cmdloop()
		self.forget()

	def default(self, line):
		if line[:1] == '!': line = line[1:]
		locals = self.curframe.f_locals
		globals = self.curframe.f_globals
		try:
			code = compile(line + '\n', '<stdin>', 'single')
			exec code in globals, locals
		except:
			t, v = sys.exc_info()[:2]
			if type(t) == type(''):
				exc_type_name = t
			else: exc_type_name = t.__name__
			print '***', exc_type_name + ':', v

	# Command definitions, called by cmdloop()
	# The argument is the remaining string on the command line
	# Return true to exit from the command loop 
	
	do_h = cmd.Cmd.do_help

	def do_break(self, arg):
		# break [ ([filename:]lineno | function) [, "condition"] ]
		if not arg:
			print self.get_all_breaks() # XXX
			return
		# parse arguments; comma has lowest precendence
		# and cannot occur in filename
		filename = None
		lineno = None
		cond = None
		comma = string.find(arg, ',')
		if comma > 0:
			# parse stuff after comma: "condition"
			cond = string.lstrip(arg[comma+1:])
			arg = string.rstrip(arg[:comma])
			try:
				cond = eval(
					cond,
					self.curframe.f_globals,
					self.curframe.f_locals)
			except:
				print '*** Could not eval condition:', cond
				return
		# parse stuff before comma: [filename:]lineno | function
		colon = string.rfind(arg, ':')
		if colon >= 0:
			filename = string.rstrip(arg[:colon])
			filename = self.lookupmodule(filename)
			arg = string.lstrip(arg[colon+1:])
			try:
				lineno = int(arg)
			except ValueError, msg:
				print '*** Bad lineno:', arg
				return
		else:
			# no colon; can be lineno or function
			try:
				lineno = int(arg)
			except ValueError:
				try:
					func = eval(arg,
						    self.curframe.f_globals,
						    self.curframe.f_locals)
				except:
					print '*** Could not eval argument:',
					print arg
					return
				try:
					if hasattr(func, 'im_func'):
						func = func.im_func
					code = func.func_code
				except:
					print '*** The specified object',
					print 'is not a function', arg
					return
				lineno = code.co_firstlineno
				if not filename:
					filename = code.co_filename
		# supply default filename if necessary
		if not filename:
			filename = self.curframe.f_code.co_filename
		# now set the break point
		err = self.set_break(filename, lineno, cond)
		if err: print '***', err

	do_b = do_break
	
	def do_clear(self, arg):
		if not arg:
			try:
				reply = raw_input('Clear all breaks? ')
			except EOFError:
				reply = 'no'
			reply = string.lower(string.strip(reply))
			if reply in ('y', 'yes'):
				self.clear_all_breaks()
			return
		filename = None
		colon = string.rfind(arg, ':')
		if colon >= 0:
			filename = string.rstrip(arg[:colon])
			filename = self.lookupmodule(filename)
			arg = string.lstrip(arg[colon+1:])
		try:
			lineno = int(arg)
		except:
			print '*** Bad lineno:', `arg`
			return
		if not filename:
			filename = self.curframe.f_code.co_filename
		err = self.clear_break(filename, lineno)
		if err: print '***', err
	do_cl = do_clear # 'c' is already an abbreviation for 'continue'
	
	def do_where(self, arg):
		self.print_stack_trace()
	do_w = do_where
	
	def do_up(self, arg):
		if self.curindex == 0:
			print '*** Oldest frame'
		else:
			self.curindex = self.curindex - 1
			self.curframe = self.stack[self.curindex][0]
			self.print_stack_entry(self.stack[self.curindex])
			self.lineno = None
	do_u = do_up
	
	def do_down(self, arg):
		if self.curindex + 1 == len(self.stack):
			print '*** Newest frame'
		else:
			self.curindex = self.curindex + 1
			self.curframe = self.stack[self.curindex][0]
			self.print_stack_entry(self.stack[self.curindex])
			self.lineno = None
	do_d = do_down
	
	def do_step(self, arg):
		self.set_step()
		return 1
	do_s = do_step
	
	def do_next(self, arg):
		self.set_next(self.curframe)
		return 1
	do_n = do_next
	
	def do_return(self, arg):
		self.set_return(self.curframe)
		return 1
	do_r = do_return
	
	def do_continue(self, arg):
		self.set_continue()
		return 1
	do_c = do_cont = do_continue
	
	def do_quit(self, arg):
		self.set_quit()
		return 1
	do_q = do_quit
	
	def do_args(self, arg):
		f = self.curframe
		co = f.f_code
		dict = f.f_locals
		n = co.co_argcount
		if co.co_flags & 4: n = n+1
		if co.co_flags & 8: n = n+1
		for i in range(n):
			name = co.co_varnames[i]
			print name, '=',
			if dict.has_key(name): print dict[name]
			else: print "*** undefined ***"
	do_a = do_args
	
	def do_retval(self, arg):
		if self.curframe.f_locals.has_key('__return__'):
			print self.curframe.f_locals['__return__']
		else:
			print '*** Not yet returned!'
	do_rv = do_retval
	
	def do_p(self, arg):
		try:
			value = eval(arg, self.curframe.f_globals, \
					self.curframe.f_locals)
		except:
			t, v = sys.exc_info()[:2]
			if type(t) == type(''):
				exc_type_name = t
			else: exc_type_name = t.__name__
			print '***', exc_type_name + ':', `v`
			return

		print `value`

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
					first = max(1, int(x) - 5)
			except:
				print '*** Error in argument:', `arg`
				return
		elif self.lineno is None:
			first = max(1, self.curframe.f_lineno - 5)
		else:
			first = self.lineno + 1
		if last == None:
			last = first + 10
		filename = self.curframe.f_code.co_filename
		breaklist = self.get_file_breaks(filename)
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

	def do_whatis(self, arg):
		try:
			value = eval(arg, self.curframe.f_globals, \
					self.curframe.f_locals)
		except:
			t, v = sys.exc_info()[:2]
			if type(t) == type(''):
				exc_type_name = t
			else: exc_type_name = t.__name__
			print '***', exc_type_name + ':', `v`
			return
		code = None
		# Is it a function?
		try: code = value.func_code
		except: pass
		if code:
			print 'Function', code.co_name
			return
		# Is it an instance method?
		try: code = value.im_func.func_code
		except: pass
		if code:
			print 'Method', code.co_name
			return
		# None of the above...
		print type(value)
	
	# Print a traceback starting at the top stack frame.
	# The most recently entered frame is printed last;
	# this is different from dbx and gdb, but consistent with
	# the Python interpreter's stack trace.
	# It is also consistent with the up/down commands (which are
	# compatible with dbx and gdb: up moves towards 'main()'
	# and down moves towards the most recent stack frame).
	
	def print_stack_trace(self):
		try:
			for frame_lineno in self.stack:
				self.print_stack_entry(frame_lineno)
		except KeyboardInterrupt:
			pass
	
	def print_stack_entry(self, frame_lineno, prompt_prefix=line_prefix):
		frame, lineno = frame_lineno
		if frame is self.curframe:
			print '>',
		else:
			print ' ',
		print self.format_stack_entry(frame_lineno, prompt_prefix)


	# Help methods (derived from pdb.doc)

	def help_help(self):
		self.help_h()

	def help_h(self):
		print """h(elp)
	Without argument, print the list of available commands.
	With a command name as argument, print help about that command
	"help pdb" pipes the full documentation file to the $PAGER
	"help exec" gives help on the ! command"""

	def help_where(self):
		self.help_w()

	def help_w(self):
		print """w(here)
	Print a stack trace, with the most recent frame at the bottom.
	An arrow indicates the "current frame", which determines the
	context of most commands."""

	def help_down(self):
		self.help_d()

	def help_d(self):
		print """d(own)
	Move the current frame one level down in the stack trace
	(to an older frame)."""

	def help_up(self):
		self.help_u()

	def help_u(self):
		print """u(p)
	Move the current frame one level up in the stack trace
	(to a newer frame)."""

	def help_break(self):
		self.help_b()

	def help_b(self):
		print """b(reak) ([file:]lineno | function) [, "condition"]
	With a line number argument, set a break there in the current
	file.  With a function name, set a break at the entry of that
	function.  Without argument, list all breaks.  If a second
	argument is present, it is a string specifying an expression
	which must evaluate to true before the breakpoint is honored.

	The line number may be prefixed with a filename and a colon,
	to specify a breakpoint in another file (probably one that
	hasn't been loaded yet).  The file is searched on sys.path."""

	def help_clear(self):
		self.help_cl()

	def help_cl(self):
		print """cl(ear) [lineno]
	With a line number argument, clear that break in the current file.
	Without argument, clear all breaks (but first ask confirmation).

	The line number may be prefixed with a filename and a colon,
	to specify a breakpoint in another file (probably one that
	hasn't been loaded yet).  The file is searched on sys.path."""

	def help_step(self):
		self.help_s()

	def help_s(self):
		print """s(tep)
	Execute the current line, stop at the first possible occasion
	(either in a function that is called or in the current function)."""

	def help_next(self):
		self.help_n()

	def help_n(self):
		print """n(ext)
	Continue execution until the next line in the current function
	is reached or it returns."""

	def help_return(self):
		self.help_r()

	def help_r(self):
		print """r(eturn)
	Continue execution until the current function returns."""

	def help_continue(self):
		self.help_c()

	def help_cont(self):
		self.help_c()

	def help_c(self):
		print """c(ont(inue))
	Continue execution, only stop when a breakpoint is encountered."""

	def help_list(self):
		self.help_l()

	def help_l(self):
		print """l(ist) [first [,last]]
	List source code for the current file.
	Without arguments, list 11 lines around the current line
	or continue the previous listing.
	With one argument, list 11 lines starting at that line.
	With two arguments, list the given range;
	if the second argument is less than the first, it is a count."""

	def help_args(self):
		self.help_a()

	def help_a(self):
		print """a(rgs)
	Print the arguments of the current function."""

	def help_p(self):
		print """p expression
	Print the value of the expression."""

	def help_exec(self):
		print """(!) statement
	Execute the (one-line) statement in the context of
	the current stack frame.
	The exclamation point can be omitted unless the first word
	of the statement resembles a debugger command.
	To assign to a global variable you must always prefix the
	command with a 'global' command, e.g.:
	(Pdb) global list_options; list_options = ['-l']
	(Pdb)"""

	def help_quit(self):
		self.help_q()

	def help_q(self):
		print """q(uit)	Quit from the debugger.
	The program being executed is aborted."""

	def help_pdb(self):
		help()

	# Helper function for break/clear parsing -- may be overridden

	def lookupmodule(self, filename):
		if filename == mainmodule:
			return mainpyfile
		for dirname in sys.path:
			fullname = os.path.join(dirname, filename)
			if os.path.exists(fullname):
				return fullname
		print 'Warning:', `filename`, 'not found from sys.path'
		return filename


# Simplified interface

def run(statement, globals=None, locals=None):
	Pdb().run(statement, globals, locals)

def runeval(expression, globals=None, locals=None):
	return Pdb().runeval(expression, globals, locals)

def runctx(statement, globals, locals):
	# B/W compatibility
	run(statement, globals, locals)

def runcall(*args):
	return apply(Pdb().runcall, args)

def set_trace():
	Pdb().set_trace()

# Post-Mortem interface

def post_mortem(t):
	p = Pdb()
	p.reset()
	while t.tb_next <> None: t = t.tb_next
	p.interaction(t.tb_frame, t)

def pm():
	post_mortem(sys.last_traceback)


# Main program for testing

TESTCMD = 'import x; x.main()'

def test():
	run(TESTCMD)

# print help
def help():
	for dirname in sys.path:
		fullname = os.path.join(dirname, 'pdb.doc')
		if os.path.exists(fullname):
			sts = os.system('${PAGER-more} '+fullname)
			if sts: print '*** Pager exit status:', sts
			break
	else:
		print 'Sorry, can\'t find the help file "pdb.doc"',
		print 'along the Python search path'

mainmodule = ''
mainpyfile = ''

# When invoked as main program, invoke the debugger on a script
if __name__=='__main__':
	global mainmodule, mainpyfile
	if not sys.argv[1:]:
		print "usage: pdb.py scriptfile [arg] ..."
		sys.exit(2)

	mainpyfile = filename = sys.argv[1]	# Get script filename
	if not os.path.exists(filename):
		print 'Error:', `filename`, 'does not exist'
		sys.exit(1)
	mainmodule = os.path.basename(filename)
	del sys.argv[0]		# Hide "pdb.py" from argument list

	# Insert script directory in front of module search path
	sys.path.insert(0, os.path.dirname(filename))

	run('execfile(' + `filename` + ')', {'__name__': '__main__'})
