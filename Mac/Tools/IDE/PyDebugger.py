import sys
import bdb
import types
import os

import W
import WASTEconst
import PyBrowser
import Qd
import Evt
import Lists
import MacOS
_filenames = {}

SIMPLE_TYPES = (
	types.NoneType,
	types.IntType,
	types.LongType,
	types.FloatType,
	types.ComplexType,
	types.StringType
)


class Debugger(bdb.Bdb):
	
	def __init__(self, title = 'Debugger'):
		bdb.Bdb.__init__(self)
		self.closed = 1
		self.title = title
		self.breaksviewer = None
		self.reset()
		self.tracing = 0
		self.tracingmonitortime = Evt.TickCount()
		self.editors = {}
		
		prefs = W.getapplication().getprefs()
		if prefs.debugger:
			for file, breaks in prefs.debugger.breaks.items():
				for b in breaks:
					self.set_break(file, b)
			self.bounds, self.horpanes, self.verpanes = prefs.debugger.windowsettings
			self.tracemagic = prefs.debugger.tracemagic
		else:
			self.breaks = {}
			self.horpanes = (0.4, 0.6)
			self.verpanes = (0.3, 0.35, 0.35)
			self.bounds = (600, 400)
			self.tracemagic = 0
		self.laststacksel = None
	
	def canonic(self, filename):
		# override: the provided canonic() method breaks our
		# file-less Untitled windows
		return filename
	
	def reset(self):
		self.currentframe = None
		self.file = None
		self.laststack = None
		self.reason = 'Not running'
		self.continuewithoutdebugger = 0
		bdb.Bdb.reset(self)
		self.forget()
	
	def start(self, bottomframe = None, running = 0):
		W.getapplication().DebuggerQuit = bdb.BdbQuit
		import Menu
		Menu.HiliteMenu(0)
		if self.closed:
			self.setupwidgets(self.title)
			self.closed = 0
		if not self.w.parent.debugger_quitting:
			self.w.select()
			raise W.AlertError, 'There is another debugger session busy.'
		self.reset()
		self.botframe = bottomframe
		if running:
			self.set_continue()
			self.reason = 'Running\xc9'
			self.setstate('running')
		else:
			self.set_step()
			self.reason = 'stopped'
			self.setstate('stopped')
		sys.settrace(self.trace_dispatch)
	
	def stop(self):
		self.set_quit()
		if self.w.parent:
			self.exit_mainloop()
			self.resetwidgets()
	
	def set_continue_without_debugger(self):
		sys.settrace(None)
		self.set_quit()
		self.clear_tracefuncs()
		self.continuewithoutdebugger = 1
		if hasattr(self, "w") and self.w.parent:
			self.exit_mainloop()
			self.resetwidgets()
	
	def clear_tracefuncs(self):
		try:
			raise 'spam'
		except:
			pass
		frame = sys.exc_traceback.tb_frame
		while frame is not None:
			del frame.f_trace
			frame = frame.f_back
	
	def postmortem(self, exc_type, exc_value, traceback):
		if self.closed:
			self.setupwidgets(self.title)
			self.closed = 0
		if not self.w.parent.debugger_quitting:
			raise W.AlertError, 'There is another debugger session busy.'
		self.reset()
		if traceback:
			self.botframe = traceback.tb_frame
			while traceback.tb_next <> None:
				traceback = traceback.tb_next
			frame = traceback.tb_frame
		else:
			self.botframe = None
			frame = None
		self.w.panes.bottom.buttons.killbutton.enable(1)
		self.reason = '(dead) ' + self.formatexception(exc_type, exc_value)
		self.w.select()
		self.setup(frame, traceback)
		self.setstate('dead')
		self.showstack(self.curindex)
		self.showframe(self.curindex)
	
	def setupwidgets(self, title):
		self.w = w = W.Window(self.bounds, title, minsize = (500, 300))
		
		w.panes = W.HorizontalPanes((8, 4, -8, -8), self.horpanes)
		
		w.panes.browserpanes = browserpanes = W.VerticalPanes(None, self.verpanes)
		
		browserpanes.stacklist = W.Group(None)
		browserpanes.stacklist.title = W.TextBox((4, 0, 0, 12), 'Stack')
		browserpanes.stacklist.stack = W.List((0, 16, 0, 0), callback = self.do_stack, flags = Lists.lOnlyOne)
		
		browserpanes.locals = W.Group(None)
		browserpanes.locals.title = W.TextBox((4, 0, 0, 12), 'Local variables')
		browserpanes.locals.browser = PyBrowser.BrowserWidget((0, 16, 0, 0))
		
		browserpanes.globals = W.Group(None)
		browserpanes.globals.title = W.TextBox((4, 0, 0, 12), 'Global variables')
		browserpanes.globals.browser = PyBrowser.BrowserWidget((0, 16, 0, 0))
		
		w.panes.bottom = bottom = W.Group(None)
		bottom.src = src = W.Group((0, 52, 0, 0))
		source = SourceViewer((1, 1, -15, -15), readonly = 1, debugger = self)
		src.optionsmenu = W.PopupMenu((-16, 0, 16, 16), [])
		src.optionsmenu.bind('<click>', self.makeoptionsmenu)
		
		src._barx = W.Scrollbar((0, -16, -15, 16), source.hscroll, max = 32767)
		src._bary = W.Scrollbar((-16, 15, 16, -15), source.vscroll, max = 32767)
		src.source = source
		src.frame = W.Frame((0, 0, -15, -15))
		
		bottom.tracingmonitor = TracingMonitor((0, 23, 6, 6))
		bottom.state = W.TextBox((12, 20, 0, 16), self.reason)
		
		bottom.srctitle = W.TextBox((12, 36, 0, 14))
		bottom.buttons = buttons = W.Group((12, 0, 0, 16))
		
		buttons.runbutton = W.Button((0, 0, 50, 16), "Run", self.do_run)
		buttons.stopbutton = W.Button((58, 0, 50, 16), "Stop", self.do_stop)
		buttons.killbutton = W.Button((116, 0, 50, 16), "Kill", self.do_kill)
		buttons.line = W.VerticalLine((173, 0, 0, 0))
		buttons.stepbutton = W.Button((181, 0, 50, 16), "Step", self.do_step)
		buttons.stepinbutton = W.Button((239, 0, 50, 16), "Step in", self.do_stepin)
		buttons.stepoutbutton = W.Button((297, 0, 50, 16), "Step out", self.do_stepout)
		
		w.bind('cmdr', buttons.runbutton.push)
		w.bind('cmd.', buttons.stopbutton.push)
		w.bind('cmdk', buttons.killbutton.push)
		w.bind('cmds', buttons.stepbutton.push)
		w.bind('cmdt', buttons.stepinbutton.push)
		w.bind('cmdu', buttons.stepoutbutton.push)
		
		w.bind('<close>', self.close)
		
		w.open()
		w.xxx___select(w.panes.bottom.src.source)
	
	def makeoptionsmenu(self):
		options = [('Clear breakpoints', self.w.panes.bottom.src.source.clearbreakpoints), 
				('Clear all breakpoints', self.clear_all_breaks),
				('Edit breakpoints\xc9', self.edit_breaks), '-',
				(self.tracemagic and 
					'Disable __magic__ tracing' or 'Enable __magic__ tracing', self.togglemagic)]
		self.w.panes.bottom.src.optionsmenu.set(options)
	
	def edit_breaks(self):
		if self.breaksviewer:
			self.breaksviewer.select()
		else:
			self.breaksviewer = BreakpointsViewer(self)
	
	def togglemagic(self):
		self.tracemagic = not self.tracemagic
	
	def setstate(self, state):
		self.w.panes.bottom.tracingmonitor.reset()
		self.w.panes.bottom.state.set(self.reason)
		buttons = self.w.panes.bottom.buttons
		if state == 'stopped':
			buttons.runbutton.enable(1)
			buttons.stopbutton.enable(0)
			buttons.killbutton.enable(1)
			buttons.stepbutton.enable(1)
			buttons.stepinbutton.enable(1)
			buttons.stepoutbutton.enable(1)
		elif state == 'running':
			buttons.runbutton.enable(0)
			buttons.stopbutton.enable(1)
			buttons.killbutton.enable(1)
			buttons.stepbutton.enable(0)
			buttons.stepinbutton.enable(0)
			buttons.stepoutbutton.enable(0)
		elif state == 'idle':
			buttons.runbutton.enable(0)
			buttons.stopbutton.enable(0)
			buttons.killbutton.enable(0)
			buttons.stepbutton.enable(0)
			buttons.stepinbutton.enable(0)
			buttons.stepoutbutton.enable(0)
		elif state == 'dead':
			buttons.runbutton.enable(0)
			buttons.stopbutton.enable(0)
			buttons.killbutton.enable(1)
			buttons.stepbutton.enable(0)
			buttons.stepinbutton.enable(0)
			buttons.stepoutbutton.enable(0)
		else:
			print 'unknown state:', state
	
	def resetwidgets(self):
		self.reason = ''
		self.w.panes.bottom.srctitle.set('')
		self.w.panes.bottom.src.source.set('')
		self.w.panes.browserpanes.stacklist.stack.set([])
		self.w.panes.browserpanes.locals.browser.set({})
		self.w.panes.browserpanes.globals.browser.set({})
		self.setstate('idle')
	
	# W callbacks
	
	def close(self):
		self.set_quit()
		self.exit_mainloop()
		self.closed = 1
		
		self.unregister_editor(self.w.panes.bottom.src.source, 
				self.w.panes.bottom.src.source.file)
		self.horpanes = self.w.panes.getpanesizes()
		self.verpanes = self.w.panes.browserpanes.getpanesizes()
		self.bounds = self.w.getbounds()
		prefs = W.getapplication().getprefs()
		prefs.debugger.breaks = self.breaks
		prefs.debugger.windowsettings = self.bounds, self.horpanes, self.verpanes
		prefs.debugger.tracemagic = self.tracemagic
		prefs.save()
	
	# stack list callback
	
	def do_stack(self, isdbl):
		sel = self.w.panes.browserpanes.stacklist.stack.getselection()
		if isdbl:
			if sel:
				frame, lineno = self.stack[sel[0] + 1]
				filename = frame.f_code.co_filename
				editor = self.w._parentwindow.parent.openscript(filename, lineno)
				if self.breaks.has_key(filename):
					editor.showbreakpoints(1)
		else:
			if sel and sel <> self.laststacksel:
				self.showframe(sel[0] + 1)
			self.laststacksel = sel
	
	def geteditor(self, filename):
		if filename[:1] == '<' and filename[-1:] == '>':
			editor = W.getapplication().getscript(filename[1:-1])
		else:
			editor = W.getapplication().getscript(filename)
		return editor
	
	# button callbacks
	
	def do_run(self):
		self.running()
		self.set_continue()
		self.exit_mainloop()
	
	def do_stop(self):
		self.set_step()
	
	def do_kill(self):
		self.set_quit()
		self.exit_mainloop()
		self.resetwidgets()
	
	def do_step(self):
		self.running()
		self.set_next(self.curframe)
		self.exit_mainloop()
	
	def do_stepin(self):
		self.running()
		self.set_step()
		self.exit_mainloop()
	
	def do_stepout(self):
		self.running()
		self.set_return(self.curframe)
		self.exit_mainloop()
	
	def running(self):
		W.SetCursor('watch')
		self.reason = 'Running\xc9'
		self.setstate('running')
		#self.w.panes.bottom.src.source.set('')
		#self.w.panes.browserpanes.stacklist.stack.set([])
		#self.w.panes.browserpanes.locals.browser.set({})
		#self.w.panes.browserpanes.globals.browser.set({})
	
	def exit_mainloop(self):
		self.w.parent.debugger_quitting = 1
	
	#
	
	def showframe(self, stackindex):
		(frame, lineno) = self.stack[stackindex]
		W.SetCursor('watch')
		filename = frame.f_code.co_filename
		if filename <> self.file:
			editor = self.geteditor(filename)
			if editor:
				self.w.panes.bottom.src.source.set(editor.get(), filename)
			else:
				try:
					f = open(filename, 'rb')
					data = f.read()
					f.close()
				except IOError:
					if filename[-3:] == '.py':
						import imp
						modname = os.path.basename(filename)[:-3]
						try:
							f, filename, (suff, mode, dummy) = imp.find_module(modname)
						except ImportError:
							self.w.panes.bottom.src.source.set("can't find file")
						else:
							if f:
								f.close()
							if f and suff == '.py':
								f = open(filename, 'rb')
								data = f.read()
								f.close()
								self.w.panes.bottom.src.source.set(data, filename)
							else:
								self.w.panes.bottom.src.source.set("can't find file")
					else:
						self.w.panes.bottom.src.source.set("can't find file")
				else:
					self.w.panes.bottom.src.source.set(data, filename)
			self.file = filename
		self.w.panes.bottom.srctitle.set('Source: ' + filename + ((lineno > 0) and (' (line %d)' % lineno) or ' '))
		self.goto_line(lineno)
		self.lineno = lineno
		self.showvars((frame, lineno))
	
	def showvars(self, (frame, lineno)):
		if frame.f_locals is not frame.f_globals:
			locals = frame.f_locals
		else:
			locals = {'Same as Globals':''}
		filteredlocals = {}
		for key, value in locals.items():
			# empty key is magic for Python 1.4; '.' is magic for 1.5...
			if not key or key[0] <> '.':
				filteredlocals[key] = value
		self.w.panes.browserpanes.locals.browser.set(filteredlocals)
		self.w.panes.browserpanes.globals.browser.set(frame.f_globals)
	
	def showstack(self, stackindex):
		stack = []
		for frame, lineno in self.stack[1:]:
			filename = frame.f_code.co_filename
			try:
				filename = _filenames[filename]
			except KeyError:
				if filename[:1] + filename[-1:] <> '<>':
					filename = os.path.basename(filename)
				_filenames[frame.f_code.co_filename] = filename
			funcname = frame.f_code.co_name
			if funcname == '?':
				funcname = '<toplevel>'
			stack.append(filename + ': ' + funcname)
		if stack <> self.laststack:
			self.w.panes.browserpanes.stacklist.stack.set(stack)
			self.laststack = stack
		sel = [stackindex - 1]
		self.w.panes.browserpanes.stacklist.stack.setselection(sel)
		self.laststacksel = sel
	
	def goto_line(self, lineno):
		if lineno > 0:
			self.w.panes.bottom.src.source.selectline(lineno - 1)
		else:
			self.w.panes.bottom.src.source.setselection(0, 0)
	
	# bdb entry points
	
#	def user_call(self, frame, argument_list):
#		self.reason = 'Calling'
#		self.interaction(frame, None)
	
	def user_line(self, frame):
		# This function is called when we stop or break at this line
		self.reason = 'Stopped'
		self.interaction(frame, None)
	
	def user_return(self, frame, return_value):
		# This function is called when a return trap is set here
		fname = frame.f_code.co_name
		if fname <> '?':
			self.reason = 'Returning from %s()' % frame.f_code.co_name
			frame.f_locals['__return__'] = return_value
		elif frame.f_back is self.botframe:
			self.reason = 'Done'
		else:
			self.reason = 'Returning'
		self.interaction(frame, None, 1)
	
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		# This function is called when we stop or break at this line
		self.reason = self.formatexception(exc_type, exc_value)
		self.interaction(frame, exc_traceback)
	
	def formatexception(self, exc_type, exc_value):
		if exc_type == SyntaxError:
			try:
				value, (filename, lineno, charno, line) = exc_value
			except:
				pass
			else:
				return str(exc_type) + ': ' + str(value)
		if type(exc_type) == types.ClassType:
			nice = exc_type.__name__
		else:
			nice = str(exc_type)
		value = str(exc_value)
		if exc_value and value:
			nice = nice + ": " + value
		return nice
	
	def forget(self):
		self.stack = []
		self.curindex = 0
		self.curframe = None
	
	def setup(self, f, t, isreturning = 0):
		self.forget()
		self.stack, self.curindex = self.get_stack(f, t)
		self.curframe = self.stack[self.curindex - isreturning][0]
	
	def interaction(self, frame, traceback, isreturning = 0):
		saveport = Qd.GetPort()
		self.w.select()
		try:
			self.setup(frame, traceback, isreturning)
			self.setstate('stopped')
			stackindex = self.curindex
			if isreturning:
				if frame.f_back is not self.botframe:
					stackindex = stackindex - 1
			self.showstack(stackindex)
			self.showframe(stackindex)
			self.w.parent.debugger_mainloop()
			self.forget()
		finally:
			Qd.SetPort(saveport)
	
	# bdb customization
	
	def trace_dispatch(self, frame, event, arg, TickCount = Evt.TickCount):
		if TickCount() - self.tracingmonitortime > 15:
			self.tracingmonitortime = TickCount()
			self.w.panes.bottom.tracingmonitor.toggle()
		try:
			try:
				MacOS.EnableAppswitch(0)
				if self.quitting:
					# returning None is not enough, a former BdbQuit exception
					# might have been eaten by the print statement
					raise bdb.BdbQuit
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
			finally:
				MacOS.EnableAppswitch(-1)
		except KeyboardInterrupt:
			self.set_step()
			return self.trace_dispatch
		except bdb.BdbQuit:
			if self.continuewithoutdebugger:
				self.clear_tracefuncs()
				return
			else:
				raise bdb.BdbQuit
		except:
			print 'XXX Exception during debugger interaction.', \
					self.formatexception(sys.exc_type, sys.exc_value)
			import traceback
			traceback.print_exc()
			return self.trace_dispatch
	
	def dispatch_call(self, frame, arg):
		if not self.tracemagic and \
				frame.f_code.co_name[:2] == '__' == frame.f_code.co_name[-2:] and \
				frame.f_code.co_name <> '__init__':
			return
		if self.botframe is None:
			# First call of dispatch since reset()
			self.botframe = frame.f_back	# xxx !!! added f_back
			return self.trace_dispatch
		if not (self.stop_here(frame) or self.break_anywhere(frame)):
			# No need to trace this function
			return # None
		self.user_call(frame, arg)
		if self.quitting:
			raise bdb.BdbQuit
		return self.trace_dispatch
	
	def set_continue(self):
		# Don't stop except at breakpoints or when finished
		self.stopframe = self.botframe
		self.returnframe = None
		self.quitting = 0
		# unlike in bdb/pdb, there's a chance that breakpoints change 
		# *while* a program (this program ;-) is running. It's actually quite likely.
		# So we don't delete frame.f_trace until the bottom frame if there are no breakpoints.
	
	def set_break(self, filename, lineno):
		if not self.breaks.has_key(filename):
			self.breaks[filename] = []
		list = self.breaks[filename]
		if lineno in list:
			return 'There is already a breakpoint there!'
		list.append(lineno)
		list.sort()	# I want to keep them neatly sorted; easier for drawing
		if hasattr(bdb, "Breakpoint"):
			# 1.5.2b1 specific
			bp = bdb.Breakpoint(filename, lineno, 0, None)
		self.update_breaks(filename)
	
	def clear_break(self, filename, lineno):
		bdb.Bdb.clear_break(self, filename, lineno)
		self.update_breaks(filename)
	
	def clear_all_file_breaks(self, filename):
		bdb.Bdb.clear_all_file_breaks(self, filename)
		self.update_breaks(filename)
	
	def clear_all_breaks(self):
		bdb.Bdb.clear_all_breaks(self)
		for editors in self.editors.values():
			for editor in editors:
				editor.drawbreakpoints()
	
	# special
	
	def toggle_break(self, filename, lineno):
		if self.get_break(filename, lineno):
			self.clear_break(filename, lineno)
		else:
			self.set_break(filename, lineno)
	
	def clear_breaks_above(self, filename, above):
		if not self.breaks.has_key(filename):
			return 'There are no breakpoints in that file!'
		for lineno in self.breaks[filename][:]:
			if lineno > above:
				self.breaks[filename].remove(lineno)
		if not self.breaks[filename]:
			del self.breaks[filename]
	
	# editor stuff
	
	def update_breaks(self, filename):
		if self.breaksviewer:
			self.breaksviewer.update()
		if self.editors.has_key(filename):
			for editor in self.editors[filename]:
				if editor._debugger:	# XXX
					editor.drawbreakpoints()
				else:
					print 'xxx dead editor!'
	
	def update_allbreaks(self):
		if self.breaksviewer:
			self.breaksviewer.update()
		for filename in self.breaks.keys():
			if self.editors.has_key(filename):
				for editor in self.editors[filename]:
					if editor._debugger:	# XXX
						editor.drawbreakpoints()
					else:
						print 'xxx dead editor!'
	
	def register_editor(self, editor, filename):
		if not filename:
			return
		if not self.editors.has_key(filename):
			self.editors[filename] = [editor]
		elif editor not in self.editors[filename]:
			self.editors[filename].append(editor)
	
	def unregister_editor(self, editor, filename):
		if not filename:
			return
		try:
			self.editors[filename].remove(editor)
			if not self.editors[filename]:
				del self.editors[filename]
				# if this was an untitled window, clear the breaks.
				if filename[:1] == '<' and filename[-1:] == '>' and \
						self.breaks.has_key(filename):
					self.clear_all_file_breaks(filename)
		except (KeyError, ValueError):
			pass
		

class SourceViewer(W.PyEditor):
	
	def __init__(self, *args, **kwargs):
		apply(W.PyEditor.__init__, (self,) + args, kwargs)
		self.bind('<click>', self.clickintercept)
	
	def clickintercept(self, point, modifiers):
		if self._parentwindow._currentwidget <> self and not self.pt_in_breaks(point):
			self._parentwindow.xxx___select(self)
			return 1
	
	def _getviewrect(self):
		l, t, r, b = self._bounds
		if self._debugger:
			return (l + 12, t + 2, r - 1, b - 2)
		else:
			return (l + 5, t + 2, r - 1, b - 2)
	
	def select(self, onoff, isclick = 0):
		if W.SelectableWidget.select(self, onoff):
			return
		self.SetPort()
		#if onoff:
		#	self.ted.WEActivate()
		#else:
		#	self.ted.WEDeactivate()
		self.drawselframe(onoff)
	
	def drawselframe(self, onoff):
		pass


class BreakpointsViewer:
	
	def __init__(self, debugger):
		self.debugger = debugger
		import Lists
		self.w = W.Window((300, 250), 'Breakpoints', minsize = (200, 200))
		self.w.panes = W.HorizontalPanes((8, 8, -8, -32), (0.3, 0.7))
		self.w.panes.files = W.List(None, callback = self.filehit)		#, flags = Lists.lOnlyOne)
		self.w.panes.gr = W.Group(None)
		self.w.panes.gr.breaks = W.List((0, 0, -130, 0), callback = self.linehit)	#, flags = Lists.lOnlyOne)
		self.w.panes.gr.openbutton = W.Button((-80, 4, 0, 16), 'View\xc9', self.openbuttonhit)
		self.w.panes.gr.deletebutton = W.Button((-80, 28, 0, 16), 'Delete', self.deletebuttonhit)
		
		self.w.bind('<close>', self.close)
		self.w.bind('backspace', self.w.panes.gr.deletebutton.push)
		
		self.setup()
		self.w.open()
		self.w.panes.gr.openbutton.enable(0)
		self.w.panes.gr.deletebutton.enable(0)
		self.curfile = None
	
	def deletebuttonhit(self):
		if self.w._currentwidget == self.w.panes.files:
			self.del_filename()
		else:
			self.del_number()
		self.checkbuttons()
	
	def del_number(self):
		if self.curfile is None:
			return
		sel = self.w.panes.gr.breaks.getselectedobjects()
		for lineno in sel:
			self.debugger.clear_break(self.curfile, lineno)
	
	def del_filename(self):
		sel = self.w.panes.files.getselectedobjects()
		for filename in sel:
			self.debugger.clear_all_file_breaks(filename)
		self.debugger.update_allbreaks()
	
	def setup(self):
		files = self.debugger.breaks.keys()
		files.sort()
		self.w.panes.files.set(files)
	
	def close(self):
		self.debugger.breaksviewer = None
		self.debugger = None
	
	def update(self):
		sel = self.w.panes.files.getselectedobjects()
		self.setup()
		self.w.panes.files.setselectedobjects(sel)
		sel = self.w.panes.files.getselection()
		if len(sel) == 0 and self.curfile:
			self.w.panes.files.setselectedobjects([self.curfile])
		self.filehit(0)
	
	def select(self):
		self.w.select()
	
	def selectfile(self, file):
		self.w.panes.files.setselectedobjects([file])
		self.filehit(0)			
	
	def openbuttonhit(self):
		self.filehit(1)
	
	def filehit(self, isdbl):
		sel = self.w.panes.files.getselectedobjects()
		if isdbl:
			for filename in sel:
				lineno = None
				if filename == self.curfile:
					linesel = self.w.panes.gr.breaks.getselectedobjects()
					if linesel:
						lineno = linesel[-1]
					elif self.w.panes.gr.breaks:
						lineno = self.w.panes.gr.breaks[0]
				editor = self.w._parentwindow.parent.openscript(filename, lineno)
				editor.showbreakpoints(1)
			return
		if len(sel) == 1:
			file = sel[0]
			filebreaks = self.debugger.breaks[file][:]
			if self.curfile == file:
				linesel = self.w.panes.gr.breaks.getselectedobjects()
			self.w.panes.gr.breaks.set(filebreaks)
			if self.curfile == file:
				self.w.panes.gr.breaks.setselectedobjects(linesel)
			self.curfile = file
		else:
			if len(sel) <> 0:
				self.curfile = None
			self.w.panes.gr.breaks.set([])
		self.checkbuttons()
	
	def linehit(self, isdbl):
		if isdbl:
			files = self.w.panes.files.getselectedobjects()
			if len(files) <> 1:
				return
			filename = files[0]
			linenos = self.w.panes.gr.breaks.getselectedobjects()
			if not linenos:
				return
			lineno = linenos[-1]
			editor = self.w._parentwindow.parent.openscript(filename, lineno)
			editor.showbreakpoints(1)
		self.checkbuttons()
	
	def checkbuttons(self):
		if self.w.panes.files.getselection():
			self.w.panes.gr.openbutton.enable(1)
			self.w._parentwindow.setdefaultbutton(self.w.panes.gr.openbutton)
			if self.w._currentwidget == self.w.panes.files:
				if self.w.panes.files.getselection():
					self.w.panes.gr.deletebutton.enable(1)
				else:
					self.w.panes.gr.deletebutton.enable(0)
			else:
				if self.w.panes.gr.breaks.getselection():
					self.w.panes.gr.deletebutton.enable(1)
				else:
					self.w.panes.gr.deletebutton.enable(0)
		else:
			self.w.panes.gr.openbutton.enable(0)
			self.w.panes.gr.deletebutton.enable(0)


class TracingMonitor(W.Widget):
	
	def __init__(self, *args, **kwargs):
		apply(W.Widget.__init__, (self,) + args, kwargs)
		self.state = 0
	
	def toggle(self):
		if hasattr(self, "_parentwindow") and self._parentwindow is not None:
			self.state = self.state % 2 + 1
			port = Qd.GetPort()
			self.SetPort()
			self.draw()
			Qd.SetPort(port)
	
	def reset(self):
		if self._parentwindow:
			self.state = 0
			port = Qd.GetPort()
			self.SetPort()
			self.draw()
			Qd.SetPort(port)
	
	def draw(self, visRgn = None):
		if self.state == 2:
			Qd.PaintOval(self._bounds)
		else:
			Qd.EraseOval(self._bounds)


# convenience funcs

def postmortem(exc_type, exc_value, tb):
	d = getdebugger()
	d.postmortem(exc_type, exc_value, tb)

def start(bottomframe = None):
	d = getdebugger()
	d.start(bottomframe)

def startfromhere():
	d = getdebugger()
	try:
		raise 'spam'
	except:
		frame = sys.exc_traceback.tb_frame.f_back
	d.start(frame)

def startfrombottom():
	d = getdebugger()
	d.start(_getbottomframe(), 1)

def stop():
	d = getdebugger()
	d.stop()

def cont():
	sys.settrace(None)
	d = getdebugger()
	d.set_continue_without_debugger()

def _getbottomframe():
	try:
		raise 'spam'
	except:
		pass
	frame = sys.exc_traceback.tb_frame
	while 1:
		if frame.f_code.co_name == 'mainloop' or frame.f_back is None:
			break
		frame = frame.f_back
	return frame

_debugger = None

def getdebugger():
	if not __debug__:
		raise W.AlertError, "Can't debug in \"Optimize bytecode\" mode.\r(see \"Default startup options\" in EditPythonPreferences)"
	global _debugger
	if _debugger is None:
		_debugger = Debugger()
	return _debugger
