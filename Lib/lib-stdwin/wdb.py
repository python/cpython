# wdb.py -- a window-based Python debugger

# XXX To do:
# - don't fall out of bottom frame


import stdwin
from stdwinevents import *
import sys
import basewin
import bdb
import repr

WIDTH = 40
HEIGHT = 8

WdbDone = 'wdb.WdbDone' # Exception to continue execution


class Wdb(bdb.Bdb, basewin.BaseWindow): # Window debugger
	
	def init(self):
		self.sourcewindows = {}
		self.framewindows = {}
		self = bdb.Bdb.init(self)
		width = WIDTH*stdwin.textwidth('0')
		height = HEIGHT*stdwin.lineheight()
		stdwin.setdefwinsize(width, height)
		self = basewin.BaseWindow.init(self, '--Stack--')
		self.closed = 0
		return self
	
	def reset(self):
		if self.closed: raise RuntimeError, 'already closed'
		bdb.Bdb.reset(self)
		self.forget()
	
	def forget(self):
		self.lineno = None
		self.stack = []
		self.curindex = 0
		self.curframe = None
		for fn in self.sourcewindows.keys():
			self.sourcewindows[fn].resetlineno()
	
	def setup(self, f, t):
		self.forget()
		self.stack, self.curindex = self.get_stack(f, t)
		self.curframe = self.stack[self.curindex][0]
		# Build a list of current frames
		cfl = []
		for f, i in self.stack: cfl.append(f)
		# Remove deactivated frame windows
		for name in self.framewindows.keys():
			fw = self.framewindows[name]
			if fw.frame not in cfl: fw.close()
			else: fw.refreshframe()
		# Refresh the stack window
		self.refreshstack()
	
	# Override Bdb methods (except user_call, for now)
	
	def user_line(self, frame):
		# This function is called when we stop or break at this line
		self.interaction(frame, None)
	
	def user_return(self, frame, return_value):
		# This function is called when a return trap is set here
		frame.f_locals['__return__'] = return_value
		self.settitle('--Return--')
		self.interaction(frame, None)
		if not self.closed:
			self.settitle('--Stack--')
	
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		# This function is called if an exception occurs,
		# but only if we are to stop at or just below this level
		frame.f_locals['__exception__'] = exc_type, exc_value
		self.settitle(exc_type + ': ' + repr.repr(exc_value))
		stdwin.fleep()
		self.interaction(frame, exc_traceback)
		if not self.closed:
			self.settitle('--Stack--')
	
	# Change the title
	
	def settitle(self, title):
		self.savetitle = self.win.gettitle()
		self.win.settitle(title)
	
	# General interaction function
	
	def interaction(self, frame, traceback):
		import mainloop
		self.popup()
		self.setup(frame, traceback)
		try:
			mainloop.mainloop()
		except WdbDone:
			pass
		self.forget()
	
	# Functions whose name is do_X for some character X
	# are callable directly from the keyboard.
	
	def do_up(self):
		if self.curindex == 0:
			stdwin.fleep()
		else:
			self.curindex = self.curindex - 1
			self.curframe = self.stack[self.curindex][0]
			self.refreshstack()
	do_u = do_up
	
	def do_down(self):
		if self.curindex + 1 == len(self.stack):
			stdwin.fleep()
		else:
			self.curindex = self.curindex + 1
			self.curframe = self.stack[self.curindex][0]
			self.refreshstack()
	do_d = do_down
	
	def do_step(self):
		self.set_step()
		raise WdbDone
	do_s = do_step
	
	def do_next(self):
		self.set_next(self.curframe)
		raise WdbDone
	do_n = do_next
	
	def do_return(self):
		self.set_return(self.curframe)
		raise WdbDone
	do_r = do_return
	
	def do_continue(self):
		self.set_continue()
		raise WdbDone
	do_c = do_cont = do_continue
	
	def do_quit(self):
		self.close()
		raise WdbDone
	do_q = do_quit
	
	def do_list(self):
		fn = self.curframe.f_code.co_filename
		if not self.sourcewindows.has_key(fn):
			import wdbsrcwin
			try:
				self.sourcewindows[fn] = \
					wdbsrcwin.DebuggerSourceWindow(). \
					init(self, fn)
			except IOError:
				stdwin.fleep()
				return
		w = self.sourcewindows[fn]
		lineno = self.stack[self.curindex][1]
		w.setlineno(lineno)
		w.popup()
	do_l = do_list
	
	def do_frame(self):
		name = 'locals' + `self.curframe`[16:-1]
		if self.framewindows.has_key(name):
			self.framewindows[name].popup()
		else:
			import wdbframewin
			self.framewindows[name] = \
				wdbframewin.FrameWindow().init(self, \
					self.curframe, \
					self.curframe.f_locals, name)
	do_f = do_frame
	
	def do_globalframe(self):
		name = 'globals' + `self.curframe`[16:-1]
		if self.framewindows.has_key(name):
			self.framewindows[name].popup()
		else:
			import wdbframewin
			self.framewindows[name] = \
				wdbframewin.FrameWindow().init(self, \
					self.curframe, \
					self.curframe.f_globals, name)
	do_g = do_globalframe
	
	# Link between the debugger and the window
	
	def refreshstack(self):
		height = stdwin.lineheight() * (1 + len(self.stack))
		self.win.setdocsize((0, height))
		self.refreshall() # XXX be more subtle later
		# Also pass the information on to the source windows
		filename = self.curframe.f_code.co_filename
		lineno = self.curframe.f_lineno
		for fn in self.sourcewindows.keys():
			w = self.sourcewindows[fn]
			if fn == filename:
				w.setlineno(lineno)
			else:
				w.resetlineno()
	
	# The remaining methods override BaseWindow methods
	
	def close(self):
		if not self.closed:
			basewin.BaseWindow.close(self)
		self.closed = 1
		for key in self.sourcewindows.keys():
			self.sourcewindows[key].close()
		for key in self.framewindows.keys():
			self.framewindows[key].close()
		self.set_quit()
	
	def char(self, detail):
		try:
			func = eval('self.do_' + detail)
		except (AttributeError, SyntaxError):
			stdwin.fleep()
			return
		func()
	
	def command(self, detail):
		if detail == WC_UP:
			self.do_up()
		elif detail == WC_DOWN:
			self.do_down()
	
	def mouse_down(self, detail):
		(h, v), clicks, button, mask = detail
		i = v / stdwin.lineheight()
		if 0 <= i < len(self.stack):
			if i != self.curindex:
				self.curindex = i
				self.curframe = self.stack[self.curindex][0]
				self.refreshstack()
			elif clicks == 2:
				self.do_frame()
		else:
			stdwin.fleep()
	
	def draw(self, detail):
		import linecache, codehack, string
		d = self.win.begindrawing()
		try:
			h, v = 0, 0
			for f, lineno in self.stack:
				fn = f.f_code.co_filename
				if f is self.curframe:
					s = '> '
				else:
					s = '  '
				s = s + fn + '(' + `lineno` + ')'
				s = s + codehack.getcodename(f.f_code)
				if f.f_locals.has_key('__args__'):
					args = f.f_locals['__args__']
					if args is not None:
						s = s + repr.repr(args)
				if f.f_locals.has_key('__return__'):
					rv = f.f_locals['__return__']
					s = s + '->'
					s = s + repr.repr(rv)
				line = linecache.getline(fn, lineno)
				if line: s = s + ': ' + string.strip(line)
				d.text((h, v), s)
				v = v + d.lineheight()
		finally:
			d.close()


# Simplified interface

def run(statement):
	x = Wdb().init()
	try: x.run(statement)
	finally: x.close()

def runctx(statement, globals, locals):
	x = Wdb().init()
	try: x.runctx(statement, globals, locals)
	finally: x.close()

def runcall(*args):
	x = Wdb().init()
	try: apply(Pdb().init().runcall, args)
	finally: x.close()


# Post-Mortem interface

def post_mortem(traceback):
	p = Pdb().init()
	p.reset()
	p.interaction(None, traceback)

def pm():
	import sys
	post_mortem(sys.last_traceback)


# Main program for testing

TESTCMD = 'import x; x.main()'

def test():
	import linecache
	linecache.checkcache()
	run(TESTCMD)
