
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

	def __init__(self, packdialog, option, varclass):
		self.packdialog = packdialog
		self.option = option
		self.master = packdialog.top
		self.default, self.klass = packdialog.options[option]
		self.var = varclass(self.master)
		self.frame = Frame(self.master,
				   {Pack: {'expand': 0, 'fill': 'x'}})
		self.label = Label(self.frame,
				   {'text': option + ':',
				    Pack: {'side': 'left'},
				    })
		self.update()

	def refresh(self):
		self.packdialog.refresh()
		self.update()

	def update(self):
		try:
			self.current = self.packdialog.current[self.option]
		except KeyError:
			self.current = self.default
		self.var.set(self.current)

	def set(self, e=None):
		pass

class BooleanOption(Option):

	def __init__(self, packdialog, option):
		Option.__init__(self, packdialog, option, BooleanVar)
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

	def __init__(self, packdialog, option):
		Option.__init__(self, packdialog, option, StringVar)
		self.button = Menubutton(self.frame,
					 {'textvariable': self.var,
					  'relief': 'raised',
					  'borderwidth': 2,
					  Pack: {'side': 'right'},
					  })
		self.menu = Menu(self.button)
		self.button['menu'] = self.menu
		for v in self.packdialog.classes[self.klass]:
			label = v
			if v == self.default: label = label + ' (default)'
			self.menu.add_radiobutton(
				{'label': label,
				 'variable': self.var,
				 'value': v,
				 'command': self.set,
				 })

class StringOption(Option):

	def __init__(self, packdialog, option):
		Option.__init__(self, packdialog, option, StringVar)
		self.entry = Entry(self.frame,
				   {'textvariable': self.var,
				    'width': 10,
				    'relief': 'sunken',
				    'borderwidth': 2,
				    Pack: {'side': 'right',
					   'fill': 'x', 'expand': 1},
				    })
		self.entry.bind('<Return>', self.set)

class PackOption: # Mix-in class

	def set(self, e=None):
		self.current = self.var.get()
		try:
			Pack.config(self.packdialog.widget,
				    {self.option: self.current})
		except TclError:
			self.refresh()

class BooleanPackOption(PackOption, BooleanOption): pass
class EnumPackOption(PackOption, EnumOption): pass
class StringPackOption(PackOption, StringOption): pass

class PackDialog:

	options = {
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
		'Fill': ('none', 'x', 'y', 'both'),
		'Side': ('top', 'right', 'bottom', 'left'),
		'Expand': 'boolean',
		'Pad': 'pixel',
		'Widget': 'widget',
		}

	def __init__(self, widget):
		self.widget = widget
		self.refresh()
		self.top = Toplevel(self.widget)
		self.top.title('Pack: %s' % widget.widgetName)
		self.top.minsize(1, 1) # XXX
		self.anchor = EnumPackOption(self, 'anchor')
		self.side = EnumPackOption(self, 'side')
		self.fill = EnumPackOption(self, 'fill')
		self.expand = BooleanPackOption(self, 'expand')
		self.ipadx = StringPackOption(self, 'ipadx')
		self.ipady = StringPackOption(self, 'ipady')
		self.padx = StringPackOption(self, 'padx')
		self.pady = StringPackOption(self, 'pady')
		# XXX after, before, in

	def refresh(self):
		self.current = self.widget.newinfo()

class WidgetOption: # Mix-in class

	def set(self, e=None):
		self.current = self.var.get()
		try:
			self.packdialog.widget[self.option] = self.current
		except TclError:
			self.refresh()

class BooleanWidgetOption(WidgetOption, BooleanOption): pass
class EnumWidgetOption(WidgetOption, EnumOption): pass
class StringWidgetOption(WidgetOption, StringOption): pass

class WidgetDialog:

	# Universal classes
	classes = {
		'Anchor': ('n','ne', 'e','se', 's','sw', 'w','nw', 'center'),
		'Aspect': 'integer',
		'Background': 'color',
		'Bitmap': 'bitmap',
		'BorderWidth': 'pixel',
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
		

	def __init__(self, widget):
		self.widget = widget
		if self.addclasses.has_key(self.widget.widgetName):
			classes = {}
			for c in (self.classes,
				  self.addclasses[self.widget.widgetName]):
				for k in c.keys():
					classes[k] = c[k]
			self.classes = classes
		self.refresh()
		self.top = Toplevel(self.widget)
		self.top.title('Widget: %s' % widget.widgetName)
		self.top.minsize(1, 1)
		self.choices = {}
		for k, (d, c) in self.options.items():
			try:
				cl = self.classes[c]
			except KeyError:
				cl = 'unknown'
			if type(cl) == TupleType:
				cl = EnumWidgetOption
			elif cl == 'boolean':
				cl = BooleanWidgetOption
			else:
				cl = StringWidgetOption
			self.choices[k] = cl(self, k)

	def refresh(self):
		self.configuration = self.widget.config()
		self.current = {}
		self.options = {}
		for k, v in self.configuration.items():
			if len(v) > 4:
				self.current[k] = v[4]
				self.options[k] = v[3], v[2] # default, klass

def test():
	root = Tk()
	root.minsize(1, 1)
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
