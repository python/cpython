# Tkinter.py -- Tk/Tcl widget wrappers

import tkinter
from tkinter import TclError

class _Dummy:
	def meth(self):	return

def _func():
	pass

FunctionType = type(_func)
ClassType = type(_Dummy)
MethodType = type(_Dummy.meth)
StringType = type('')
TupleType = type(())
ListType = type([])
DictionaryType = type({})
NoneType = type(None)
CallableTypes = (FunctionType, MethodType)

def _flatten(tuple):
	res = ()
	for item in tuple:
		if type(item) in (TupleType, ListType):
			res = res + _flatten(item)
		else:
			res = res + (item,)
	return res

def _cnfmerge(cnfs):
	if type(cnfs) in (NoneType, DictionaryType, StringType):
		return cnfs
	else:
		cnf = {}
		for c in _flatten(cnfs):
			for k, v in c.items():
				cnf[k] = v
		return cnf

class Event:
	pass

_default_root = None

def _tkerror(err):
	pass

def _exit(code='0'):
	import sys
	sys.exit(getint(code))

_varnum = 0
class Variable:
	def __init__(self, master=None):
		global _default_root
		global _varnum
		if master:
			self._tk = master.tk
		else:
			self._tk = _default_root.tk
		self._name = 'PY_VAR' + `_varnum`
		_varnum = _varnum + 1
	def __del__(self):
		self._tk.unsetvar(self._name)
	def __str__(self):
		return self._name
	def __call__(self, value=None):
		if value == None:
			return self.get()
		else:
			self.set(value)
	def set(self, value):
		return self._tk.setvar(self._name, value)

class StringVar(Variable):
	def __init__(self, master=None):
		Variable.__init__(self, master)
	def get(self):
		return self._tk.getvar(self._name)

class IntVar(Variable):
	def __init__(self, master=None):
		Variable.__init__(self, master)
	def get(self):
		return self._tk.getint(self._tk.getvar(self._name))

class DoubleVar(Variable):
	def __init__(self, master=None):
		Variable.__init__(self, master)
	def get(self):
		return self._tk.getdouble(self._tk.getvar(self._name))

class BooleanVar(Variable):
	def __init__(self, master=None):
		Variable.__init__(self, master)
	def get(self):
		return self._tk.getboolean(self._tk.getvar(self._name))

def mainloop():
	_default_root.tk.mainloop()

def getint(s):
	return _default_root.tk.getint(s)

def getdouble(s):
	return _default_root.tk.getdouble(s)

def getboolean(s):
	return _default_root.tk.getboolean(s)

class Misc:
	def tk_strictMotif(self, boolean=None):
		return self.tk.getboolean(self.tk.call(
			'set', 'tk_strictMotif', boolean))
	def tk_menuBar(self, *args):
		apply(self.tk.call, ('tk_menuBar', self._w) + args)
	def wait_variable(self, name='PY_VAR'):
		self.tk.call('tkwait', 'variable', name)
	waitvar = wait_variable # XXX b/w compat
	def wait_window(self, window=None):
		if window == None:
			window = self
		self.tk.call('tkwait', 'window', window._w)
	def wait_visibility(self, window=None):
		if window == None:
			window = self
		self.tk.call('tkwait', 'visibility', window._w)
	def setvar(self, name='PY_VAR', value='1'):
		self.tk.setvar(name, value)
	def getvar(self, name='PY_VAR'):
		return self.tk.getvar(name)
	def getint(self, s):
		return self.tk.getint(s)
	def getdouble(self, s):
		return self.tk.getdouble(s)
	def getboolean(self, s):
		return self.tk.getboolean(s)
	def focus_set(self):
		self.tk.call('focus', self._w)
	focus = focus_set # XXX b/w compat?
	def focus_default_set(self):
		self.tk.call('focus', 'default', self._w)
	def focus_default_none(self):
		self.tk.call('focus', 'default', 'none')
	focus_default = focus_default_set
	def focus_none(self):
		self.tk.call('focus', 'none')
	def focus_get(self):
		name = self.tk.call('focus')
		if name == 'none': return None
		return self._nametowidget(name)
	def after(self, ms, func=None, *args):
		if not func:
			self.tk.call('after', ms)
		else:
			# XXX Disgusting hack to clean up after calling func
			tmp = []
			def callit(func=func, args=args, tk=self.tk, tmp=tmp):
				try:
					apply(func, args)
				finally:
					tk.deletecommand(tmp[0])
			name = self._register(callit)
			tmp.append(name)
			self.tk.call('after', ms, name)
	# XXX grab current w/o window argument
	def grab_current(self):
		name = self.tk.call('grab', 'current', self._w)
		if not name: return None
		return self._nametowidget(name)
	def grab_release(self):
		self.tk.call('grab', 'release', self._w)
	def grab_set(self):
		self.tk.call('grab', 'set', self._w)
	def grab_set_global(self):
		self.tk.call('grab', 'set', '-global', self._w)
	def grab_status(self):
		status = self.tk.call('grab', 'status', self._w)
		if status == 'none': status = None
		return status
	def lower(self, belowThis=None):
		self.tk.call('lower', self._w, belowThis)
	def selection_clear(self):
		self.tk.call('selection', 'clear', self._w)
	def selection_get(self, type=None):
		return self.tk.call('selection', 'get', type)
	def selection_handle(self, func, type=None, format=None):
		name = self._register(func)
		self.tk.call('selection', 'handle', self._w, 
			     name, type, format)
	def selection_own(self, func=None):
		name = self._register(func)
		self.tk.call('selection', 'own', self._w, name)
	def selection_own_get(self):
		return self._nametowidget(self.tk.call('selection', 'own'))
	def send(self, interp, cmd, *args):
		return apply(self.tk.call, ('send', interp, cmd) + args)
	def lower(self, belowThis=None):
		self.tk.call('lift', self._w, belowThis)
	def tkraise(self, aboveThis=None):
		self.tk.call('raise', self._w, aboveThis)
	lift = tkraise
	def colormodel(self, value=None):
		return self.tk.call('tk', 'colormodel', self._w, value)
	def winfo_atom(self, name):
		return self.tk.getint(self.tk.call('winfo', 'atom', name))
	def winfo_atomname(self, id):
		return self.tk.call('winfo', 'atomname', id)
	def winfo_cells(self):
		return self.tk.getint(
			self.tk.call('winfo', 'cells', self._w))
	def winfo_children(self):
		return map(self._nametowidget,
			   self.tk.splitlist(self.tk.call(
				   'winfo', 'children', self._w)))
	def winfo_class(self):
		return self.tk.call('winfo', 'class', self._w)
	def winfo_containing(self, rootX, rootY):
		return self.tk.call('winfo', 'containing', rootx, rootY)
	def winfo_depth(self):
		return self.tk.getint(self.tk.call('winfo', 'depth', self._w))
	def winfo_exists(self):
		return self.tk.getint(
			self.tk.call('winfo', 'exists', self._w))
	def winfo_fpixels(self, number):
		return self.tk.getdouble(self.tk.call(
			'winfo', 'fpixels', self._w, number))
	def winfo_geometry(self):
		return self.tk.call('winfo', 'geometry', self._w)
	def winfo_height(self):
		return self.tk.getint(
			self.tk.call('winfo', 'height', self._w))
	def winfo_id(self):
		return self.tk.getint(
			self.tk.call('winfo', 'id', self._w))
	def winfo_interps(self):
		return self.tk.splitlist(
			self.tk.call('winfo', 'interps'))
	def winfo_ismapped(self):
		return self.tk.getint(
			self.tk.call('winfo', 'ismapped', self._w))
	def winfo_name(self):
		return self.tk.call('winfo', 'name', self._w)
	def winfo_parent(self):
		return self.tk.call('winfo', 'parent', self._w)
	def winfo_pathname(self, id):
		return self.tk.call('winfo', 'pathname', id)
	def winfo_pixels(self, number):
		return self.tk.getint(
			self.tk.call('winfo', 'pixels', self._w, number))
	def winfo_reqheight(self):
		return self.tk.getint(
			self.tk.call('winfo', 'reqheight', self._w))
	def winfo_reqwidth(self):
		return self.tk.getint(
			self.tk.call('winfo', 'reqwidth', self._w))
	def winfo_rgb(self, color):
		return self._getints(
			self.tk.call('winfo', 'rgb', self._w, color))
	def winfo_rootx(self):
		return self.tk.getint(
			self.tk.call('winfo', 'rootx', self._w))
	def winfo_rooty(self):
		return self.tk.getint(
			self.tk.call('winfo', 'rooty', self._w))
	def winfo_screen(self):
		return self.tk.call('winfo', 'screen', self._w)
	def winfo_screencells(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screencells', self._w))
	def winfo_screendepth(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screendepth', self._w))
	def winfo_screenheight(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screenheight', self._w))
	def winfo_screenmmheight(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screenmmheight', self._w))
	def winfo_screenmmwidth(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screenmmwidth', self._w))
	def winfo_screenvisual(self):
		return self.tk.call('winfo', 'screenvisual', self._w)
	def winfo_screenwidth(self):
		return self.tk.getint(
			self.tk.call('winfo', 'screenwidth', self._w))
	def winfo_toplevel(self):
		return self._nametowidget(self.tk.call(
			'winfo', 'toplevel', self._w))
	def winfo_visual(self):
		return self.tk.call('winfo', 'visual', self._w)
	def winfo_vrootheight(self):
		return self.tk.getint(
			self.tk.call('winfo', 'vrootheight', self._w))
	def winfo_vrootwidth(self):
		return self.tk.getint(
			self.tk.call('winfo', 'vrootwidth', self._w))
	def winfo_vrootx(self):
		return self.tk.getint(
			self.tk.call('winfo', 'vrootx', self._w))
	def winfo_vrooty(self):
		return self.tk.getint(
			self.tk.call('winfo', 'vrooty', self._w))
	def winfo_width(self):
		return self.tk.getint(
			self.tk.call('winfo', 'width', self._w))
	def winfo_x(self):
		return self.tk.getint(
			self.tk.call('winfo', 'x', self._w))
	def winfo_y(self):
		return self.tk.getint(
			self.tk.call('winfo', 'y', self._w))
	def update(self):
		self.tk.call('update')
	def update_idletasks(self):
		self.tk.call('update', 'idletasks')
	def bind(self, sequence, func, add=''):
		if add: add = '+'
		name = self._register(func, self._substitute)
		self.tk.call('bind', self._w, sequence, 
			     (add + name,) + self._subst_format)
	def bind_all(self, sequence, func, add=''):
		if add: add = '+'
		name = self._register(func, self._substitute)
		self.tk.call('bind', 'all' , sequence, 
			     (add + name,) + self._subst_format)
	def bind_class(self, className, sequence, func, add=''):
		if add: add = '+'
		name = self._register(func, self._substitute)
		self.tk.call('bind', className , sequence, 
			     (add + name,) + self._subst_format)
	def mainloop(self):
		self.tk.mainloop()
	def quit(self):
		self.tk.quit()
	def _getints(self, string):
		if not string: return None
		res = ()
		for v in self.tk.splitlist(string):
			res = res +  (self.tk.getint(v),)
		return res
	def _getboolean(self, string):
		if string:
			return self.tk.getboolean(string)
	def _options(self, cnf):
		cnf = _cnfmerge(cnf)
		res = ()
		for k, v in cnf.items():
			if type(v) in CallableTypes:
				v = self._register(v)
			res = res + ('-'+k, v)
		return res
	def _nametowidget(self, name):
		w = self
		if name[0] == '.':
			w = w._root()
			name = name[1:]
		from string import find
		while name:
			i = find(name, '.')
			if i >= 0:
				name, tail = name[:i], name[i+1:]
			else:
				tail = ''
			w = w.children[name]
			name = tail
		return w
	def _register(self, func, subst=None):
		f = _CallSafely(func, subst).__call__
		name = `id(f)`
		if hasattr(func, 'im_func'):
			func = func.im_func
		if hasattr(func, 'func_name') and \
		   type(func.func_name) == type(''):
			name = name + func.func_name
		self.tk.createcommand(name, f)
		return name
	register = _register
	def _root(self):
		w = self
		while w.master: w = w.master
		return w
	_subst_format = ('%#', '%b', '%f', '%h', '%k', 
			 '%s', '%t', '%w', '%x', '%y',
			 '%A', '%E', '%K', '%N', '%W', '%T', '%X', '%Y')
	def _substitute(self, *args):
		tk = self.tk
		if len(args) != len(self._subst_format): return args
		nsign, b, f, h, k, s, t, w, x, y, A, E, K, N, W, T, X, Y = args
		# Missing: (a, c, d, m, o, v, B, R)
		e = Event()
		e.serial = tk.getint(nsign)
		e.num = tk.getint(b)
		try: e.focus = tk.getboolean(f)
		except TclError: pass
		e.height = tk.getint(h)
		e.keycode = tk.getint(k)
		e.state = tk.getint(s)
		e.time = tk.getint(t)
		e.width = tk.getint(w)
		e.x = tk.getint(x)
		e.y = tk.getint(y)
		e.char = A
		try: e.send_event = tk.getboolean(E)
		except TclError: pass
		e.keysym = K
		e.keysym_num = tk.getint(N)
		e.type = T
		e.widget = self._nametowidget(W)
		e.x_root = tk.getint(X)
		e.y_root = tk.getint(Y)
		return (e,)

class _CallSafely:
	def __init__(self, func, subst=None):
		self.func = func
		self.subst = subst
	def __call__(self, *args):
		if self.subst:
			args = self.apply_func(self.subst, args)
		args = self.apply_func(self.func, args)
	def apply_func(self, func, args):
		import sys
		try:
			return apply(func, args)
		except SystemExit, msg:
			raise SystemExit, msg
		except:
			try:
				try:
					t = sys.exc_traceback
					while t:
						sys.stderr.write(
							'  %s, line %s\n' %
							(t.tb_frame.f_code,
							 t.tb_lineno))
						t = t.tb_next
				finally:
					sys.stderr.write('%s: %s\n' %
							 (sys.exc_type,
							  sys.exc_value))
				(sys.last_type,
				 sys.last_value,
				 sys.last_traceback) = (sys.exc_type,
							sys.exc_value,
							sys.exc_traceback)
				import pdb
				pdb.pm()
			except:
				print '*** Error in error handling ***'
				print sys.exc_type, ':', sys.exc_value

class Wm:
	def aspect(self, 
		   minNumer=None, minDenom=None, 
		   maxNumer=None, maxDenom=None):
		return self._getints(
			self.tk.call('wm', 'aspect', self._w, 
				     minNumer, minDenom, 
				     maxNumer, maxDenom))
	def client(self, name=None):
		return self.tk.call('wm', 'client', self._w, name)
	def command(self, value=None):
		return self.tk.call('wm', 'command', self._w, value)
	def deiconify(self):
		return self.tk.call('wm', 'deiconify', self._w)
	def focusmodel(self, model=None):
		return self.tk.call('wm', 'focusmodel', self._w, model)
	def frame(self):
		return self.tk.call('wm', 'frame', self._w)
	def geometry(self, newGeometry=None):
		return self.tk.call('wm', 'geometry', self._w, newGeometry)
	def grid(self,
		 baseWidht=None, baseHeight=None, 
		 widthInc=None, heightInc=None):
		return self._getints(self.tk.call(
			'wm', 'grid', self._w,
			baseWidht, baseHeight, widthInc, heightInc))
	def group(self, pathName=None):
		return self.tk.call('wm', 'group', self._w, pathName)
	def iconbitmap(self, bitmap=None):
		return self.tk.call('wm', 'iconbitmap', self._w, bitmap)
	def iconify(self):
		return self.tk.call('wm', 'iconify', self._w)
	def iconmask(self, bitmap=None):
		return self.tk.call('wm', 'iconmask', self._w, bitmap)
	def iconname(self, newName=None):
		return self.tk.call('wm', 'iconname', self._w, newName)
	def iconposition(self, x=None, y=None):
		return self._getints(self.tk.call(
			'wm', 'iconposition', self._w, x, y))
	def iconwindow(self, pathName=None):
		return self.tk.call('wm', 'iconwindow', self._w, pathName)
	def maxsize(self, width=None, height=None):
		return self._getints(self.tk.call(
			'wm', 'maxsize', self._w, width, height))
	def minsize(self, width=None, height=None):
		return self._getints(self.tk.call(
			'wm', 'minsize', self._w, width, height))
	def overrideredirect(self, boolean=None):
		return self._getboolean(self.tk.call(
			'wm', 'overrideredirect', self._w, boolean))
	def positionfrom(self, who=None):
		return self.tk.call('wm', 'positionfrom', self._w, who)
	def protocol(self, name=None, func=None):
		if type(func) in CallableTypes:
			command = self._register(func)
		else:
			command = func
		return self.tk.call(
			'wm', 'protocol', self._w, name, command)
	def sizefrom(self, who=None):
		return self.tk.call('wm', 'sizefrom', self._w, who)
	def state(self):
		return self.tk.call('wm', 'state', self._w)
	def title(self, string=None):
		return self.tk.call('wm', 'title', self._w, string)
	def transient(self, master=None):
		return self.tk.call('wm', 'transient', self._w, master)
	def withdraw(self):
		return self.tk.call('wm', 'withdraw', self._w)

class Tk(Misc, Wm):
	_w = '.'
	def __init__(self, screenName=None, baseName=None, className='Tk'):
		self.master = None
		self.children = {}
		if baseName is None:
			import sys, os
			baseName = os.path.basename(sys.argv[0])
			if baseName[-3:] == '.py': baseName = baseName[:-3]
		self.tk = tkinter.create(screenName, baseName, className)
		self.tk.createcommand('tkerror', _tkerror)
		self.tk.createcommand('exit', _exit)
	def destroy(self):
		for c in self.children.values(): c.destroy()
##		del self.master.children[self._name]
		self.tk.call('destroy', self._w)
	def __str__(self):
		return self._w

class Pack:
	def config(self, cnf={}):
		apply(self.tk.call, 
		      ('pack', 'configure', self._w) 
		      + self._options(cnf))
	pack = config
	def __setitem__(self, key, value):
		Pack.config({key: value})
	def forget(self):
		self.tk.call('pack', 'forget', self._w)
	def newinfo(self):
		words = self.tk.splitlist(
			self.tk.call('pack', 'newinfo', self._w))
		dict = {}
		for i in range(0, len(words), 2):
			key = words[i][1:]
			value = words[i+1]
			if value[0] == '.':
				value = self._nametowidget(value)
			dict[key] = value
		return dict
	info = newinfo
	def propagate(self, boolean=None):
		if boolean:
			self.tk.call('pack', 'propagate', self._w)
		else:
			return self._getboolean(self.tk.call(
				'pack', 'propagate', self._w))
	def slaves(self):
		return map(self._nametowidget,
			   self.tk.splitlist(
				   self.tk.call('pack', 'slaves', self._w)))

class Place:
	def config(self, cnf={}):
		apply(self.tk.call, 
		      ('place', 'configure', self._w) 
		      + self._options(cnf))
	place = config
	def __setitem__(self, key, value):
		Place.config({key: value})
	def forget(self):
		self.tk.call('place', 'forget', self._w)
	def info(self):
		return self.tk.call('place', 'info', self._w)
	def slaves(self):
		return map(self._nametowidget,
			   self.tk.splitlist(
				   self.tk.call(
					   'place', 'slaves', self._w)))

class Widget(Misc, Pack, Place):
	def _setup(self, master, cnf):
		global _default_root
		if not master:
			if not _default_root:
				_default_root = Tk()
			master = _default_root
		if not _default_root:
			_default_root = master
		self.master = master
		self.tk = master.tk
		if cnf.has_key('name'):
			name = cnf['name']
			del cnf['name']
		else:
			name = `id(self)`
		self._name = name
		if master._w=='.':
			self._w = '.' + name
		else:
			self._w = master._w + '.' + name
		self.children = {}
		if self.master.children.has_key(self._name):
			self.master.children[self._name].destroy()
		self.master.children[self._name] = self
	def __init__(self, master, widgetName, cnf={}, extra=()):
		cnf = _cnfmerge(cnf)
		Widget._setup(self, master, cnf)
		self.widgetName = widgetName
		apply(self.tk.call, (widgetName, self._w) + extra)
		Widget.config(self, cnf)
	def config(self, cnf=None):
		cnf = _cnfmerge(cnf)
		if cnf is None:
			cnf = {}
			for x in self.tk.split(
				self.tk.call(self._w, 'configure')):
				cnf[x[0][1:]] = (x[0][1:],) + x[1:]
			return cnf
		if type(cnf) == StringType:
			x = self.tk.split(self.tk.call(
				self._w, 'configure', '-'+cnf))
			return (x[0][1:],) + x[1:]
		for k in cnf.keys():
			if type(k) == ClassType:
				k.config(self, cnf[k])
				del cnf[k]
		apply(self.tk.call, (self._w, 'configure')
		      + self._options(cnf))
	def __getitem__(self, key):
		v = self.tk.splitlist(self.tk.call(
			self._w, 'configure', '-' + key))
		return v[4]
	def __setitem__(self, key, value):
		Widget.config(self, {key: value})
	def keys(self):
		return map(lambda x: x[0][1:],
			   self.tk.split(self.tk.call(self._w, 'configure')))
	def __str__(self):
		return self._w
	def destroy(self):
		for c in self.children.values(): c.destroy()
		self.tk.call('destroy', self._w)
	def _do(self, name, args=()):
		return apply(self.tk.call, (self._w, name) + args) 

class Toplevel(Widget, Wm):
	def __init__(self, master=None, cnf={}):
		extra = ()
		if cnf.has_key('screen'):
			extra = ('-screen', cnf['screen'])
			del cnf['screen']
		if cnf.has_key('class'):
			extra = extra + ('-class', cnf['class'])
			del cnf['class']
		Widget.__init__(self, master, 'toplevel', cnf, extra)
		root = self._root()
		self.iconname(root.iconname())
		self.title(root.title())

class Button(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'button', cnf)
	def tk_butEnter(self, *dummy):
		self.tk.call('tk_butEnter', self._w)
	def tk_butLeave(self, *dummy):
		self.tk.call('tk_butLeave', self._w)
	def tk_butDown(self, *dummy):
		self.tk.call('tk_butDown', self._w)
	def tk_butUp(self, *dummy):
		self.tk.call('tk_butUp', self._w)
	def flash(self):
		self.tk.call(self._w, 'flash')
	def invoke(self):
		self.tk.call(self._w, 'invoke')

# Indices:
def AtEnd():
	return 'end'
def AtInsert(*args):
	s = 'insert'
	for a in args:
		if a: s = s + (' ' + a)
	return s
def AtSelFirst():
	return 'sel.first'
def AtSelLast():
	return 'sel.last'
def At(x, y=None):
	if y is None:
		return '@' + `x`		
	else:
		return '@' + `x` + ',' + `y`

class Canvas(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'canvas', cnf)
	def addtag(self, *args):
		self._do('addtag', args)
	def addtag_above(self, tagOrId):
		self.addtag('above', tagOrId)
	def addtag_all(self):
		self.addtag('all')
	def addtag_below(self, tagOrId):
		self.addtag('below', tagOrId)
	def addtag_closest(self, x, y, halo=None, start=None):
		self.addtag('closest', x, y, halo, start)
	def addtag_enclosed(self, x1, y1, x2, y2):
		self.addtag('enclosed', x1, y1, x2, y2)
	def addtag_overlapping(self, x1, y1, x2, y2):
		self.addtag('overlapping', x1, y1, x2, y2)
	def addtag_withtag(self, tagOrId):
		self.addtag('withtag', tagOrId)
	def bbox(self, *args):
		return self._getints(self._do('bbox', args)) or None
	def bind(self, tagOrId, sequence, func, add=''):
		if add: add='+'
		name = self._register(func, self._substitute)
		self.tk.call(self._w, 'bind', tagOrId, sequence, 
			     (add + name,) + self._subst_format)
	def canvasx(self, screenx, gridspacing=None):
		return self.tk.getint(self.tk.call(
			self._w, 'canvasx', screenx, gridspacing))
	def canvasy(self, screeny, gridspacing=None):
		return self.tk.getint(self.tk.call(
			self._w, 'canvasy', screeny, gridspacing))
	def coords(self, *args):
		return self._do('coords', args)
	def _create(self, itemType, args): # Args: (value, value, ..., cnf={})
		args = _flatten(args)
		cnf = args[-1]
		if type(cnf) in (DictionaryType, TupleType):
			args = args[:-1]
		else:
			cnf = {}
		return self.tk.getint(apply(
			self.tk.call,
			(self._w, 'create', itemType) 
			+ args + self._options(cnf)))
	def create_arc(self, *args):
		return Canvas._create(self, 'arc', args)
	def create_bitmap(self, *args):
		return Canvas._create(self, 'bitmap', args)
	def create_line(self, *args):
		return Canvas._create(self, 'line', args)
	def create_oval(self, *args):
		return Canvas._create(self, 'oval', args)
	def create_polygon(self, *args):
		return Canvas._create(self, 'polygon', args)
	def create_rectangle(self, *args):
		return Canvas._create(self, 'rectangle', args)
	def create_text(self, *args):
		return Canvas._create(self, 'text', args)
	def create_window(self, *args):
		return Canvas._create(self, 'window', args)
	def dchars(self, *args):
		self._do('dchars', args)
	def delete(self, *args):
		self._do('delete', args)
	def dtag(self, *args):
		self._do('dtag', args)
	def find(self, *args):
		return self._getints(self._do('find', args))
	def find_above(self, tagOrId):
		return self.find('above', tagOrId)
	def find_all(self):
		return self.find('all')
	def find_below(self, tagOrId):
		return self.find('below', tagOrId)
	def find_closest(self, x, y, halo=None, start=None):
		return self.find('closest', x, y, halo, start)
	def find_enclosed(self, x1, y1, x2, y2):
		return self.find('enclosed', x1, y1, x2, y2)
	def find_overlapping(self, x1, y1, x2, y2):
		return self.find('overlapping', x1, y1, x2, y2)
	def find_withtag(self, tagOrId):
		return self.find('withtag', tagOrId)
	def focus(self, *args):
		return self._do('focus', args)
	def gettags(self, *args):
		return self.tk.splitlist(self._do('gettags', args))
	def icursor(self, *args):
		self._do('icursor', args)
	def index(self, *args):
		return self.tk.getint(self._do('index', args))
	def insert(self, *args):
		self._do('insert', args)
	def itemconfig(self, tagOrId, cnf=None):
		if cnf is None:
			cnf = {}
			for x in self.tk.split(
				self._do('itemconfigure', (tagOrId))):
				cnf[x[0][1:]] = (x[0][1:],) + x[1:]
			return cnf
		if type(cnf) == StringType:
			x = self.tk.split(self._do('itemconfigure',
						   (tagOrId, '-'+cnf,)))
			return (x[0][1:],) + x[1:]
		self._do('itemconfigure', (tagOrId,) + self._options(cnf))
	def lower(self, *args):
		self._do('lower', args)
	def move(self, *args):
		self._do('move', args)
	def postscript(self, cnf={}):
		return self._do('postscript', self._options(cnf))
	def tkraise(self, *args):
		self._do('raise', args)
	lift = tkraise
	def scale(self, *args):
		self._do('scale', args)
	def scan_mark(self, x, y):
		self.tk.call(self._w, 'scan', 'mark', x, y)
	def scan_dragto(self, x, y):
		self.tk.call(self._w, 'scan', 'dragto', x, y)
	def select_adjust(self, tagOrId, index):
		self.tk.call(self._w, 'select', 'adjust', tagOrId, index)
	def select_clear(self):
		self.tk.call(self._w, 'select', 'clear')
	def select_from(self, tagOrId, index):
		self.tk.call(self._w, 'select', 'from', tagOrId, index)
	def select_item(self):
		self.tk.call(self._w, 'select', 'item')
	def select_to(self, tagOrId, index):
		self.tk.call(self._w, 'select', 'to', tagOrId, index)
	def type(self, tagOrId):
		return self.tk.call(self._w, 'type', tagOrId) or None
	def xview(self, index):
		self.tk.call(self._w, 'xview', index)
	def yview(self, index):
		self.tk.call(self._w, 'yview', index)

class Checkbutton(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'checkbutton', cnf)
	def deselect(self):
		self.tk.call(self._w, 'deselect')
	def flash(self):
		self.tk.call(self._w, 'flash')
	def invoke(self):
		self.tk.call(self._w, 'invoke')
	def select(self):
		self.tk.call(self._w, 'select')
	def toggle(self):
		self.tk.call(self._w, 'toggle')

class Entry(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'entry', cnf)
	def tk_entryBackspace(self):
		self.tk.call('tk_entryBackspace', self._w)
	def tk_entryBackword(self):
		self.tk.call('tk_entryBackword', self._w)
	def tk_entrySeeCaret(self):
		self.tk.call('tk_entrySeeCaret', self._w)
	def delete(self, first, last=None):
		self.tk.call(self._w, 'delete', first, last)
	def get(self):
		return self.tk.call(self._w, 'get')
	def icursor(self, index):
		self.tk.call(self._w, 'icursor', index)
	def index(self, index):
		return self.tk.getint(self.tk.call(
			self._w, 'index', index))
	def insert(self, index, string):
		self.tk.call(self._w, 'insert', index, string)
	def scan_mark(self, x):
		self.tk.call(self._w, 'scan', 'mark', x)
	def scan_dragto(self, x):
		self.tk.call(self._w, 'scan', 'dragto', x)
	def select_adjust(self, index):
		self.tk.call(self._w, 'select', 'adjust', index)
	def select_clear(self):
		self.tk.call(self._w, 'select', 'clear')
	def select_from(self, index):
		self.tk.call(self._w, 'select', 'from', index)
	def select_to(self, index):
		self.tk.call(self._w, 'select', 'to', index)
	def select_view(self, index):
		self.tk.call(self._w, 'select', 'view', index)

class Frame(Widget):
	def __init__(self, master=None, cnf={}):
		cnf = _cnfmerge(cnf)
		extra = ()
		if cnf.has_key('class'):
			extra = ('-class', cnf['class'])
			del cnf['class']
		Widget.__init__(self, master, 'frame', cnf, extra)
	def tk_menuBar(self, *args):
		apply(self.tk.call, ('tk_menuBar', self._w) + args)

class Label(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'label', cnf)

class Listbox(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'listbox', cnf)
	def tk_listboxSingleSelect(self):
		self.tk.call('tk_listboxSingleSelect', self._w) 
	def curselection(self):
		return self.tk.splitlist(self.tk.call(
			self._w, 'curselection'))
	def delete(self, first, last=None):
		self.tk.call(self._w, 'delete', first, last)
	def get(self, index):
		return self.tk.call(self._w, 'get', index)
	def insert(self, index, *elements):
		apply(self.tk.call,
		      (self._w, 'insert', index) + elements)
	def nearest(self, y):
		return self.tk.getint(self.tk.call(
			self._w, 'nearest', y))
	def scan_mark(self, x, y):
		self.tk.call(self._w, 'scan', 'mark', x, y)
	def scan_dragto(self, x, y):
		self.tk.call(self._w, 'scan', 'dragto', x, y)
	def select_adjust(self, index):
		self.tk.call(self._w, 'select', 'adjust', index)
	def select_clear(self):
		self.tk.call(self._w, 'select', 'clear')
	def select_from(self, index):
		self.tk.call(self._w, 'select', 'from', index)
	def select_to(self, index):
		self.tk.call(self._w, 'select', 'to', index)
	def size(self):
		return self.tk.getint(self.tk.call(self._w, 'size'))
	def xview(self, index):
		self.tk.call(self._w, 'xview', index)
	def yview(self, index):
		self.tk.call(self._w, 'yview', index)

class Menu(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'menu', cnf)
	def tk_bindForTraversal(self):
		self.tk.call('tk_bindForTraversal', self._w)
	def tk_mbPost(self):
		self.tk.call('tk_mbPost', self._w)
	def tk_mbUnpost(self):
		self.tk.call('tk_mbUnpost')
	def tk_traverseToMenu(self, char):
		self.tk.call('tk_traverseToMenu', self._w, char)
	def tk_traverseWithinMenu(self, char):
		self.tk.call('tk_traverseWithinMenu', self._w, char)
	def tk_getMenuButtons(self):
		return self.tk.call('tk_getMenuButtons', self._w)
	def tk_nextMenu(self, count):
		self.tk.call('tk_nextMenu', count)
	def tk_nextMenuEntry(self, count):
		self.tk.call('tk_nextMenuEntry', count)
	def tk_invokeMenu(self):
		self.tk.call('tk_invokeMenu', self._w)
	def tk_firstMenu(self):
		self.tk.call('tk_firstMenu', self._w)
	def tk_mbButtonDown(self):
		self.tk.call('tk_mbButtonDown', self._w)
	def activate(self, index):
		self.tk.call(self._w, 'activate', index)
	def add(self, itemType, cnf={}):
		apply(self.tk.call, (self._w, 'add', itemType) 
		      + self._options(cnf))
	def add_cascade(self, cnf={}):
		self.add('cascade', cnf)
	def add_checkbutton(self, cnf={}):
		self.add('checkbutton', cnf)
	def add_command(self, cnf={}):
		self.add('command', cnf)
	def add_radiobutton(self, cnf={}):
		self.add('radiobutton', cnf)
	def add_separator(self, cnf={}):
		self.add('separator', cnf)
	def delete(self, index1, index2=None):
		self.tk.call(self._w, 'delete', index1, index2)
	def entryconfig(self, index, cnf={}):
		apply(self.tk.call, (self._w, 'entryconfigure', index)
		      + self._options(cnf))
	def index(self, index):
		i = self.tk.call(self._w, 'index', index)
		if i == 'none': return None
		return self.tk.getint(i)
	def invoke(self, index):
		return self.tk.call(self._w, 'invoke', index)
	def post(self, x, y):
		self.tk.call(self._w, 'post', x, y)
	def unpost(self):
		self.tk.call(self._w, 'unpost')
	def yposition(self, index):
		return self.tk.getint(self.tk.call(
			self._w, 'yposition', index))

class Menubutton(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'menubutton', cnf)

class Message(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'message', cnf)

class Radiobutton(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'radiobutton', cnf)
	def deselect(self):
		self.tk.call(self._w, 'deselect')
	def flash(self):
		self.tk.call(self._w, 'flash')
	def invoke(self):
		self.tk.call(self._w, 'invoke')
	def select(self):
		self.tk.call(self._w, 'select')

class Scale(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'scale', cnf)
	def get(self):
		return self.tk.getint(self.tk.call(self._w, 'get'))
	def set(self, value):
		self.tk.call(self._w, 'set', value)

class Scrollbar(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'scrollbar', cnf)
	def get(self):
		return self._getints(self.tk.call(self._w, 'get'))
	def set(self, totalUnits, windowUnits, firstUnit, lastUnit):
		self.tk.call(self._w, 'set', 
			     totalUnits, windowUnits, firstUnit, lastUnit)

class Text(Widget):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'text', cnf)
	def tk_textSelectTo(self, index):
		self.tk.call('tk_textSelectTo', self._w, index)
	def tk_textBackspace(self):
		self.tk.call('tk_textBackspace', self._w)
	def tk_textIndexCloser(self, a, b, c):
		self.tk.call('tk_textIndexCloser', self._w, a, b, c)
	def tk_textResetAnchor(self, index):
		self.tk.call('tk_textResetAnchor', self._w, index)
	def compare(self, index1, op, index2):
		return self.tk.getboolean(self.tk.call(
			self._w, 'compare', index1, op, index2))
	def debug(self, boolean=None):
		return self.tk.getboolean(self.tk.call(
			self._w, 'debug', boolean))
	def delete(self, index1, index2=None):
		self.tk.call(self._w, 'delete', index1, index2)
	def get(self, index1, index2=None):
		return self.tk.call(self._w, 'get', index1, index2)
	def index(self, index):
		return self.tk.call(self._w, 'index', index)
	def insert(self, index, chars):
		self.tk.call(self._w, 'insert', index, chars)
	def mark_names(self):
		return self.tk.splitlist(self.tk.call(
			self._w, 'mark', 'names'))
	def mark_set(self, markName, index):
		self.tk.call(self._w, 'mark', 'set', markName, index)
	def mark_unset(self, markNames):
		apply(self.tk.call, (self._w, 'mark', 'unset') + markNames)
	def scan_mark(self, y):
		self.tk.call(self._w, 'scan', 'mark', y)
	def scan_dragto(self, y):
		self.tk.call(self._w, 'scan', 'dragto', y)
	def tag_add(self, tagName, index1, index2=None):
		self.tk.call(
			self._w, 'tag', 'add', tagName, index1, index2)
	def tag_bind(self, tagName, sequence, func, add=''):
		if add: add='+'
		name = self._register(func, self._substitute)
		self.tk.call(self._w, 'tag', 'bind', 
			     tagName, sequence, 
			     (add + name,) + self._subst_format)
	def tag_config(self, tagName, cnf={}):
		apply(self.tk.call, 
		      (self._w, 'tag', 'configure', tagName) 
		      + self._options(cnf))
	def tag_delete(self, *tagNames):
		apply(self.tk.call, (self._w, 'tag', 'delete') + tagNames)
	def tag_lower(self, tagName, belowThis=None):
		self.tk.call(self._w, 'tag', 'lower', tagName, belowThis)
	def tag_names(self, index=None):
		return self.tk.splitlist(
			self.tk.call(self._w, 'tag', 'names', index))
	def tag_nextrange(self, tagName, index1, index2=None):
		return self.tk.splitlist(self.tk.call(
			self._w, 'tag', 'nextrange', index1, index2))
	def tag_raise(self, tagName, aboveThis=None):
		self.tk.call(
			self._w, 'tag', 'raise', tagName, aboveThis)
	def tag_ranges(self, tagName):
		return self.tk.splitlist(self.tk.call(
			self._w, 'tag', 'ranges', tagName))
	def tag_remove(self, tagName, index1, index2=None):
		self.tk.call(
			self._w, 'tag', 'remove', tagName, index1, index2)
	def yview(self, what):
		self.tk.call(self._w, 'yview', what)
	def yview_pickplace(self, what):
		self.tk.call(self._w, 'yview', '-pickplace', what)

######################################################################
# Extensions:

class Studbutton(Button):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'studbutton', cnf)
		self.bind('<Any-Enter>',       self.tk_butEnter)
		self.bind('<Any-Leave>',       self.tk_butLeave)
		self.bind('<1>',               self.tk_butDown)
		self.bind('<ButtonRelease-1>', self.tk_butUp)

class Tributton(Button):
	def __init__(self, master=None, cnf={}):
		Widget.__init__(self, master, 'tributton', cnf)
		self.bind('<Any-Enter>',       self.tk_butEnter)
		self.bind('<Any-Leave>',       self.tk_butLeave)
		self.bind('<1>',               self.tk_butDown)
		self.bind('<ButtonRelease-1>', self.tk_butUp)

