# Window-interface-independent part of twit
import sys
import types
import bdb
import types
import os

SIMPLE_TYPES=(
	types.NoneType,
	types.IntType,
	types.LongType,
	types.FloatType,
	types.ComplexType,
	types.StringType
)

# XXXX Mac-specific
ICON_NORMAL=500
ICON_RETURN=503
ICON_CALL=504
ICON_ZERO=505
ICON_DEAD=506

class DebuggerStuff(bdb.Bdb):

	def __init__(self, parent):
		bdb.Bdb.__init__(self)
		self.parent = parent
		self.exception_info = (None, None)
		self.reason = 'Not running'
		self.icon = ICON_NORMAL
		self.reset()
		
	def reset(self):
		bdb.Bdb.reset(self)
		self.forget()
	
	def forget(self):
		self.lineno = None
		self.stack = []
		self.curindex = 0
		self.curframe = None
		
	def run(self, cmd, locals, globals):
		self.reason = 'Running'
		bdb.Bdb.run(self, cmd, locals, globals)
		print 'RETURN from run'
		self.reason = 'Not running'
	
	def setup(self, f, t):
		self.forget()
		self.stack, self.curindex = self.get_stack(f, t)
		self.curframe = self.stack[self.curindex][0]
		
	def interaction(self, frame, traceback):
		self.setup(frame, traceback)
		self.parent.interact()
		self.exception_info = (None, None)

#	def user_call(self, frame, argument_list):
#		self.reason = 'Calling'
#		self.icon = ICON_CALL
#		self.interaction(frame, None)
			
	def user_line(self, frame):
		self.reason = 'Stopped'
		self.icon = ICON_NORMAL
		self.interaction(frame, None)
		
	def user_return(self, frame, return_value):
		self.reason = 'Returning'
		self.icon = ICON_RETURN
		self.interaction(frame, None)
				
	def user_exception(self, frame, (exc_type, exc_value, exc_traceback)):
		self.reason = 'Exception occurred'
		self.icon = ICON_DEAD
		self.parent.setstate('tb')
		self.exception_info = (exc_type, exc_value)
		self.interaction(frame, exc_traceback)

	def getexception(self):
		tp, value = self.exception_info
		if tp <> None and type(tp) <> type(''):
			tp = tp.__name__
		if value <> None and type(value) <> type(''):
			value = `value`
		return tp, value
		
	def getstacktrace(self):
		names, locations = [], []
		for frame, lineno in self.stack:
			name = frame.f_code.co_name
			if not name:
				name = "<lambda>"
			elif name == '?': 
				name = "<not a function>"
			else:
				name = name + '()'
			names.append(name)
			
			if lineno == -1:
				lineno = getframelineno(frame)
				
			modname = getframemodname(frame)
			if not modname: modname = "<unknown>"	

			locations.append("%s:%d" % (modname, lineno))
		return names, locations
		
	def getframe(self, number):
		if number < 0 or number >= len(self.stack):
			return None
		return self.stack[number][0]

	def getframevars(self, number, show_complex=1, show_system=1):
		frame = self.getframe(number)
		if not frame:
			return [], []
		return getvarsfromdict(frame.f_locals, show_complex, show_system)
		
	def getframevar(self, number, var):
		frame = self.getframe(number)
		return frame.f_locals[var]

	def getframefilepos(self, frameno):
		if frameno == None or frameno < 0 or frameno >= len(self.stack):
			return None, None, None
		frame, line = self.stack[frameno]
		if line == -1:
			line = getframelineno(frame)
		modname = getframemodname(frame)
		filename = frame.f_code.co_filename
		return filename, modname, line

	def getprogramstate(self):
		return self.reason
	
class Application:
	"""Base code for the application"""
	
	def mi_init(self, sessiontype, arg):
		self.dbg = DebuggerStuff(self)
		self.run_dialog = self.new_stack_browser(self)
		self.run_dialog.open()
		self.module_dialog = None
		self.initial_cmd = None
		self.cur_string_name = None
		if sessiontype == 'tb':
			while arg.tb_next <> None:
				arg = arg.tb_next
			self.dbg.setup(arg.tb_frame, arg)
			self.run_dialog.setup()
		elif sessiontype == 'run':
			self.initial_cmd = arg
			
	def breaks_changed(self, filename):
		self.run_dialog.breaks_changed(filename)
		if self.module_dialog:
			self.module_dialog.breaks_changed(filename)
	
	def to_debugger(self):
		cmd = self.initial_cmd
		self.initial_cmd = None
		self.setstate('run')
		self.switch_to_app()
		apply(self.dbg.run, cmd)
		self.setstate('none')
		self.switch_to_dbg()
		self.run_dialog.update_views()
		if self.module_dialog:
			self.module_dialog.update_views()
		
	def interact(self):
		# Interact with user. First, display correct info
		self.switch_to_dbg()
		self.run_dialog.update_views()
		if self.module_dialog:
			self.module_dialog.update_views()
		
		# Next, go into mainloop
		self.one_mainloop()
		
		# Finally (before we start the debuggee again) show state
		self.switch_to_app()
		self.run_dialog.show_it_running()
		
	def quit_bdb(self):
		self.dbg.set_quit()
		
	def run(self):
		cmd = self.AskString('Statement to execute:')
		self.runstring(cmd)
		
	def runfile(self, path):
		dir, file = os.path.split(path)
		try:
			os.chdir(dir)
		except os.error, arg:
			self.Message("%s: %s"%(dir, arg))
			return
		ns = {'__name__':'__main__', '__file__':path}
		cmd = "execfile('%s')"%file
		self.runstring(cmd, ns, ns)
		
	def runstring(self, cmd, globals={}, locals={}):
		self.cur_string_name = '<string: "%s">'%cmd
		try:
			cmd = compile(cmd, self.cur_string_name, 'exec')
		except SyntaxError, arg:
			self.Message('Syntax error: %s'%`arg`)
			return
		self.initial_cmd = (cmd, globals, locals)
		self.exit_mainloop()

	def cont(self):
		self.dbg.set_continue()
		self.exit_mainloop()
				
	def step(self, frame):
		self.dbg.set_next(frame)
		self.exit_mainloop()
		
	def step_in(self):
		self.dbg.set_step()
		self.exit_mainloop()
		
	def step_out(self, frame):
		self.dbg.set_return(frame)
		self.exit_mainloop()
		
	def kill(self):
		self.dbg.set_quit()
		self.exit_mainloop()
		
	def quit(self):
		self.do_quit()
		
	def browse(self, module):
		if not self.module_dialog:
			self.module_dialog = self.new_module_browser(self)
			self.module_dialog.open(module)
		else:
			self.module_dialog.focus(module)
	
	def browse_var(self, var):
		b = self.new_var_browser(self, var)
		
class StackBrowser:
	"""Base code for stack browser"""
	def mi_open(self):
		"""Setup initial data structures"""
		self.cur_stackitem = None
		self.cur_source = None
		self.cur_modname = None
		self.cur_line = None
		self.show_complex = 1
		self.show_system = 0
		self.setup()

	# create_items(self) should create self.modules, self.vars and self.source
	
	def setup(self):
		self.parent.SetWatch()
		"""Fill the various widgets with values"""
		name, value = self.parent.dbg.getexception()
		self.setexception(name, value)
		self.setprogramstate(self.parent.dbg.getprogramstate())
		
		names, locations = self.parent.dbg.getstacktrace()
		self.stack_setcontent(names, locations)
		self.cur_stackitem = len(names)-1
		self.stack_select(self.cur_stackitem)
		self.setup_frame()
		
	def setup_frame(self):
		"""Setup frame-dependent widget data"""
		self.parent.SetWatch()
		self.cont_varnames, self.cont_varvalues = \
			self.parent.dbg.getframevars(self.cur_stackitem, 
			self.show_complex, self.show_system)
		self.setvars()
		self.set_var_buttons()
	
		msg = ""
		if self.cur_stackitem == None:
			self.cur_source = None
			self.cur_modname = None
			self.cur_line = None
			msg = "No stackframe selected"
		else:
			self.cur_source, self.cur_modname, optnextline = \
				self.parent.dbg.getframefilepos(self.cur_stackitem)
			if optnextline >= 0:
				self.cur_line = optnextline
			if self.cur_source == '<string>':
				self.cur_source = None
				msg = "Executing from unknown <string>"
			elif type(self.cur_source) == types.StringType and \
						self.cur_source[:8] == '<string:':
				msg = "Executing from "+self.cur_source
				self.cur_source = None
				
		self.setsource(msg)
		if not self.cur_line:
			self.source_setline(1, ICON_ZERO)
		else:
			self.source_setline(self.cur_line, self.parent.dbg.icon)
		self.breaks_changed(self.cur_source)
		
		
		self.parent.SetCursor()
		
	# setsource(msg) should display cur_source+content, or msg if None
	
	def show_it_running(self):
		self.setprogramstate("Running")

	def update_views(self):
		self.setup()

	def click_stack(self, number, *dummy):
		if number == self.cur_stackitem: return
		self.cur_stackitem = number
		self.stack_select(self.cur_stackitem)
		self.setup_frame()
				
	def click_var(self, var, *dummy):
		v = self.parent.dbg.getframevar(self.cur_stackitem, var)
		self.parent.browse_var(v)
		
	def click_source(self, lineno, inborder):
		if not inborder:
			self.source_select(lineno)
			self.cur_line = lineno
		if lineno == None or not self.cur_source or not inborder:
			return
		if self.parent.dbg.get_break(self.cur_source, lineno):
			self.parent.dbg.clear_break(self.cur_source, lineno)
		else:
			self.parent.dbg.set_break(self.cur_source, lineno)
		self.parent.breaks_changed(self.cur_source)
		
	def breaks_changed(self, filename):
		if filename == self.cur_source:
			list = self.parent.dbg.get_file_breaks(filename)
			self.source_setbreaks(list)
		
	def click_quit(self):
		self.parent.quit()
		
	def click_run(self):
		self.parent.run()
		
	def click_continue(self):
		self.parent.cont()
		
	def click_step(self):
		if self.cur_stackitem <> None:
			frame = self.parent.dbg.getframe(self.cur_stackitem)
			self.parent.step(frame)
		else:
			self.parent.step_in()
		
	def click_step_in(self):
		self.parent.step_in()
		
	def click_step_out(self):
		if self.cur_stackitem <> None:
			frame = self.parent.dbg.getframe(self.cur_stackitem)
			self.parent.step_out(frame)
		else:
			self.parent.step_in()
			
	def click_kill(self):
		self.parent.kill()
		
	def click_browse(self):
		self.parent.browse(self.cur_modname)
		
	def click_edit(self):
		lino = self.cur_line
		if not lino:
			lino = 1
		if self.cur_source:
			self.parent.edit(self.cur_source, lino)

class ModuleBrowser:
	"""Base code for a module-browser"""

	def mi_open(self, module):
		"""Setup initial data structures"""
		self.cur_module = module
		self.cur_source = None
		self.cur_line = None
		self.cont_modules = []
		self.value_windows = []
		self.setup()

	# create_items(self) should create self.modules, self.vars and self.source
	
	def setup(self):
		"""Fill the various widgets with values"""
		self.parent.SetWatch()
		modnames = getmodulenames()
		if not self.cur_module in modnames:
			self.cur_module = None
		if modnames <> self.cont_modules:
			self.cont_modules = modnames
			self.setmodulenames()
		if self.cur_module:
			self.module_select(self.cont_modules.index(self.cur_module))
		else:
			self.module_select(None)
		self.setup_module()
		
	def setup_module(self):
		"""Setup module-dependent widget data"""
		self.parent.SetWatch()
		if not self.cur_module:
			self.cont_varnames = []
			self.cont_varvalues = []
		else:
			self.cont_varnames, self.cont_varvalues = getmodulevars(self.cur_module)
		self.setvars()
			
		msg = ""
		if not self.cur_module:
			self.cur_source = None
			msg = "No module selected"
		else:
			m = sys.modules[self.cur_module]
			try:
				self.cur_source = m.__file__
			except AttributeError:
				self.cur_source = None
				msg = "Not a python module"
		self.cur_lineno = 0	
		self.setsource(msg)
		self.source_select(self.cur_line)
		self.breaks_changed(self.cur_source)
		
		self.parent.SetCursor()

	# setsource(msg) should display cur_source+content, or msg if None
	
	def update_views(self):
		self.setup_module()
	
	def click_module(self, module, *dummy):
		if not module or module == self.cur_module: return
		self.focus(module)
		
	def focus(self, module):
		self.cur_module = module
		self.setup()
		
	def click_var(self, var, *dummy):
		if not var: return
		m = sys.modules[self.cur_module]
		dict = m.__dict__
		self.parent.browse_var(dict[var])
				
	def click_source(self, lineno, inborder):
		if not inborder:
			self.source_select(lineno)
			self.cur_lineno = lineno
		if lineno == None or not self.cur_source or not inborder:
			return
		if self.parent.dbg.get_break(self.cur_source, lineno):
			self.parent.dbg.clear_break(self.cur_source, lineno)
		else:
			self.parent.dbg.set_break(self.cur_source, lineno)
		self.parent.breaks_changed(self.cur_source)
		
	def breaks_changed(self, filename):
		if filename == self.cur_source:
			list = self.parent.dbg.get_file_breaks(filename)
			self.source_setbreaks(list)
		
	def click_edit(self):
		lino = self.cur_lineno
		if not lino:
			lino = 1
		if self.cur_source:
			self.parent.edit(self.cur_source, lino)
		
			
def getmodulenames():
	"""Return a list of all current modules, sorted"""
	list = sys.modules.keys()[:]
	list.sort()
	return list
	
def getmodulevars(name):
	"""For given module return lists with names and values"""
	m = sys.modules[name]
	try:
		dict = m.__dict__
	except AttributeError:
		dict = {}
	return getvarsfromdict(dict)
	
def getvarsfromdict(dict, show_complex=1, show_system=1):
	allnames = dict.keys()[:]
	allnames.sort()
	names = []
	for n in allnames:
		if not show_complex:
			if not type(dict[n]) in SIMPLE_TYPES:
				continue
		if not show_system:
			if n[:2] == '__' and n[-2:] == '__':
				continue
		names.append(n)
	values = []
	for n in names:
		v = pretty(dict[n])
		values.append(v)
	return names, values
	
def pretty(var):
	t = type(var)
	if t == types.FunctionType: return '<function>'
	if t == types.ClassType: return '<class>'
	return `var`
	
def getframelineno(frame):
	"""Given a frame return the line number"""
	return getcodelineno(frame.f_code)
	
def getfunclineno(func):
	"""Given a function return the line number"""
	return getcodelineno(func.func_code)
	
def getcodelineno(cobj):
	"""Given a code object return the line number"""
	code = cobj.co_code
	lineno = -1
	if ord(code[0]) == 127: # SET_LINENO instruction
		lineno = ord(code[1]) | (ord(code[2]) << 8)
	return lineno

def getframemodname(frame):
	"""Given a frame return the module name"""
	globals = frame.f_globals
	if globals.has_key('__name__'):
		return globals['__name__']
	return None
