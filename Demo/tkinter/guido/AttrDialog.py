
# The options of a widget are described by the following attributes
# of the Pack and Widget dialogs:
#
# Dialog.current: {name: value}
# -- changes during Widget's lifetime
#
# Dialog.options: {name: (default, klass)}
# -- depends on widget class only
#
# Dialog.classes: {klass: (v0, v1, v2, ...) | 'boolean' | 'other'}
# -- totally static, though different between PackDialog and WidgetDialog
#    (but even that could be unified)


from Tkinter import *

class Option:

	varclass = StringVar		# May be overridden

	def __init__(self, dialog, option):
		self.dialog = dialog
		self.option = option
		self.master = dialog.top
		self.default, self.klass = dialog.options[option]
		self.var = self.varclass(self.master)
		self.frame = Frame(self.master,
				   {Pack: {'expand': 0, 'fill': 'x'}})
		self.label = Label(self.frame,
				   {'text': option + ':',
				    Pack: {'side': 'left'},
				    })
		self.update()
		self.addoption()

	def refresh(self):
		self.dialog.refresh()
		self.update()

	def update(self):
		try:
			self.current = self.dialog.current[self.option]
		except KeyError:
			self.current = self.default
		self.var.set(self.current)

	def set(self, e=None):		# Should be overridden
		pass

class BooleanOption(Option):

	varclass = BooleanVar

	def addoption(self):
		self.button = Checkbutton(self.frame,
					 {'text': 'on/off',
					  'onvalue': '1',
					  'offvalue': '0',
					  'variable': self.var,
					  'relief': 'raised',
					  'borderwidth': 2,
					  'command': self.set,
					  Pack: {'side': 'right'},
					  })

class EnumOption(Option):

	def addoption(self):
		self.button = Menubutton(self.frame,
					 {'textvariable': self.var,
					  'relief': 'raised',
					  'borderwidth': 2,
					  Pack: {'side': 'right'},
					  })
		self.menu = Menu(self.button)
		self.button['menu'] = self.menu
		for v in self.dialog.classes[self.klass]:
			self.menu.add_radiobutton(
				{'label': v,
				 'variable': self.var,
				 'value': v,
				 'command': self.set,
				 })

class StringOption(Option):

	def addoption(self):
		self.entry = Entry(self.frame,
				   {'textvariable': self.var,
				    'width': 10,
				    'relief': 'sunken',
				    'borderwidth': 2,
				    Pack: {'side': 'right',
					   'fill': 'x', 'expand': 1},
				    })
		self.entry.bind('<Return>', self.set)

class ReadonlyOption(Option):

	def addoption(self):
		self.label = Label(self.frame,
				   {'textvariable': self.var,
				    Pack: {'side': 'right'}})

class Dialog:

	def __init__(self, master):
		self.master = master
		self.refresh()
		self.top = Toplevel(self.master)
		self.top.title(self.__class__.__name__)
		self.top.minsize(1, 1)
		self.addchoices()

	def addchoices(self):
		self.choices = {}
		list = []
		for k, dc in self.options.items():
			list.append(k, dc)
		list.sort()
		for k, (d, c) in list:
			try:
				cl = self.classes[c]
			except KeyError:
				cl = 'unknown'
			if type(cl) == TupleType:
				cl = self.enumoption
			elif cl == 'boolean':
				cl = self.booleanoption
			elif cl == 'readonly':
				cl = self.readonlyoption
			else:
				cl = self.stringoption
			self.choices[k] = cl(self, k)

	booleanoption = BooleanOption
	stringoption = StringOption
	enumoption = EnumOption
	readonlyoption = ReadonlyOption

class PackDialog(Dialog):

	def __init__(self, widget):
		self.widget = widget
		Dialog.__init__(self, widget)

	def refresh(self):
		self.current = self.widget.newinfo()
		self.current['.class'] = self.widget.winfo_class()

	class packoption: # Mix-in class
		def set(self, e=None):
			self.current = self.var.get()
			try:
				Pack.config(self.dialog.widget,
					    {self.option: self.current})
			except TclError:
				self.refresh()

	class booleanoption(packoption, BooleanOption): pass
	class enumoption(packoption, EnumOption): pass
	class stringoption(packoption, StringOption): pass
	class readonlyoption(packoption, ReadonlyOption): pass

	options = {
		'.class': (None, 'Class'),
		'after': (None, 'Widet'),
		'anchor': ('center', 'Anchor'),
		'before': (None, 'Widget'),
		'expand': ('no', 'Boolean'),
		'fill': ('none', 'Fill'),
		'in': (None, 'Widget'),
		'ipadx': (0, 'Pad'),
		'ipady': (0, 'Pad'),
		'padx': (0, 'Pad'),
		'pady': (0, 'Pad'),
		'side': ('top', 'Side'),
		}

	classes = {
		'Anchor': ('n','ne', 'e','se', 's','sw', 'w','nw', 'center'),
		'Boolean': 'boolean',
		'Class': 'readonly',
		'Expand': 'boolean',
		'Fill': ('none', 'x', 'y', 'both'),
		'Pad': 'pixel',
		'Side': ('top', 'right', 'bottom', 'left'),
		'Widget': 'readonly',
		}

class RemotePackDialog(PackDialog):

	def __init__(self, master, app, widget):
		self.master = master
		self.app = app
		self.widget = widget
		self.refresh()
		self.top = Toplevel(self.master)
		self.top.title('Remote %s Pack: %s' % (self.app, self.widget))
		self.top.minsize(1, 1) # XXX
		self.addchoices()

	def refresh(self):
		try:
			words = self.master.tk.splitlist(
				self.master.send(self.app,
						 'pack',
						 'newinfo',
						 self.widget))
		except TclError, msg:
			print 'send pack newinfo', self.widget, ':', msg
			return
		dict = {}
		for i in range(0, len(words), 2):
			key = words[i][1:]
			value = words[i+1]
			dict[key] = value
		dict['.class'] = self.master.send(self.app,
						  'winfo',
						  'class',
						  self.widget)
		self.current = dict

	class remotepackoption: # Mix-in class
		def set(self, e=None):
			self.current = self.var.get()
			try:
				self.dialog.master.send(
					self.dialog.app,
					'pack', 'config', self.dialog.widget,
					'-'+self.option, self.current)
			except TclError, msg:
				print 'send pack config ... :', msg
				self.refresh()

	class booleanoption(remotepackoption, BooleanOption): pass
	class enumoption(remotepackoption, EnumOption): pass
	class stringoption(remotepackoption, StringOption): pass
	class readonlyoption(remotepackoption, ReadonlyOption): pass

class WidgetDialog(Dialog):

	def __init__(self, widget):
		self.widget = widget
		if self.addclasses.has_key(self.widget.widgetName):
			classes = {}
			for c in (self.classes,
				  self.addclasses[self.widget.widgetName]):
				for k in c.keys():
					classes[k] = c[k]
			self.classes = classes
		Dialog.__init__(self, widget)

	def refresh(self):
		self.configuration = self.widget.config()
		self.current = {}
		self.options = {}
		self.options['.class'] = (None, 'Class')
		self.current['.class'] = self.widget.winfo_class()
		for k, v in self.configuration.items():
			if len(v) > 4:
				self.current[k] = v[4]
				self.options[k] = v[3], v[2] # default, klass

	class widgetoption: # Mix-in class
		def set(self, e=None):
			self.current = self.var.get()
			try:
				self.dialog.widget[self.option] = self.current
			except TclError:
				self.refresh()

	class booleanoption(widgetoption, BooleanOption): pass
	class enumoption(widgetoption, EnumOption): pass
	class stringoption(widgetoption, StringOption): pass
	class readonlyoption(widgetoption, ReadonlyOption): pass

	# Universal classes
	classes = {
		'Anchor': ('n','ne', 'e','se', 's','sw', 'w','nw', 'center'),
		'Aspect': 'integer',
		'Background': 'color',
		'Bitmap': 'bitmap',
		'BorderWidth': 'pixel',
		'Class': 'readonly',
		'CloseEnough': 'double',
		'Command': 'command',
		'Confine': 'boolean',
		'Cursor': 'cursor',
		'CursorWidth': 'pixel',
		'DisabledForeground': 'color',
		'ExportSelection': 'boolean',
		'Font': 'font',
		'Foreground': 'color',
		'From': 'integer',
		'Geometry': 'geometry',
		'Height': 'pixel',
		'InsertWidth': 'time',
		'Justify': ('left', 'center', 'right'),
		'Label': 'string',
		'Length': 'pixel',
		'MenuName': 'widget',
		'OffTime': 'time',
		'OnTime': 'time',
		'Orient': ('horizontal', 'vertical'),
		'Pad': 'pixel',
		'Relief': ('raised', 'sunken', 'flat', 'ridge', 'groove'),
		'RepeatDelay': 'time',
		'RepeatInterval': 'time',
		'ScrollCommand': 'command',
		'ScrollIncrement': 'pixel',
		'ScrollRegion': 'rectangle',
		'ShowValue': 'boolean',
		'SetGrid': 'boolean',
		'Sliderforeground': 'color',
		'SliderLength': 'pixel',
		'Text': 'string',
		'TickInterval': 'integer',
		'To': 'integer',
		'Underline': 'index',
		'Variable': 'variable',
		'Value': 'string',
		'Width': 'pixel',
		'Wrap': ('none', 'char', 'word'),
		}

	# Classes that (may) differ per widget type
	_tristate = {'State': ('normal', 'active', 'disabled')}
	_bistate = {'State': ('normal', 'disabled')}
	addclasses = {
		'button': _tristate,
		'radiobutton': _tristate,
		'checkbutton': _tristate,
		'entry': _bistate,
		'text': _bistate,
		'menubutton': _tristate,
		'slider': _bistate,
		}
		

def test():
	import sys
	root = Tk()
	root.minsize(1, 1)
	if sys.argv[2:]:
		pd = RemotePackDialog(root, sys.argv[1], sys.argv[2])
	else:
		frame = Frame(root, {Pack: {'expand': 1, 'fill': 'both'}})
		button = Button(frame, {'text': 'button',
					Pack: {'expand': 1}})
		canvas = Canvas(frame, {Pack: {}})
		bpd = PackDialog(button)
		bwd = WidgetDialog(button)
		cpd = PackDialog(canvas)
		cwd = WidgetDialog(canvas)
	root.mainloop()

test()
