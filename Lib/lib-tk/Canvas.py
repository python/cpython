# This module exports classes for the various canvas item types

# vi:set tabsize=4:

from Tkinter import Canvas, _isfunctype

class CanvasItem:
	def __init__(self, canvas, itemType, args = (), cnf={}):
		self.canvas = canvas
		self.id = canvas._create(itemType, args + (cnf,))
	def __str__(self):
		return str(self.id)
	def __repr__(self):
		return '<%s, id=%d>' % (self.__class__.__name__, self.id)
	def __del__(self):
		self.canvas.delete(self.id)
	delete = __del__
	def __getitem__(self, key):
		v = self.canvas.tk.split(self.canvas.tk.call(self.canvas.pathName, 
							     'itemconfigure',
							     str(self.id),
							     '-' + key))
		return v[4]
	def __setitem__(self, key, value):
		self.canvas._itemconfig(self.id, {key: value})
	def keys(self):
		if not hasattr(self, '_keys'):
			self._keys = map(lambda x, tk=self.canvas.tk:
							        tk.splitlist(x)[0][1:],
							 self.canvas._splitlist(
								 self.canvas.cmd('itemconfigure', self.id)))
			return self._keys
	def has_key(self, key):
		return key in self.keys()
	def addtag(self, tag, option='withtag'):
		self.canvas.addtag(tag, option, self.id)
	def bbox(self):
		x1, y1, x2, y2 = self.canvas.bbox(self.id)
		return (x1, y1), (x2, y2)
	def bind(self, sequence=None, command=None):
		return self.canvas.bind(self.id, sequence, command)
	def coords(self, pts = ()):
		flat = ()
		for x, y in pts: flat = flat + (x, y)
		return apply(self.canvas.coords, (self.id,) + flat)
	def dchars(self, first, last=None):
		self.canvas.dchars(self.id, first, last)
	def dtag(self, ttd):
		self.canvas.dtag(self.id, ttd)
	def focus(self):
		self.canvas.focus(self.id)
	def gettags(self):
		return self.canvas.gettags(self.id)
	def icursor(self):
		self.canvas.icursor(self.id)
	def index(self):
		return self.canvas.index(self.id)
	def insert(self, beforethis, string):
		self.canvas.insert(self.id, beforethis, string)
	def lower(self, belowthis=None):
		self.canvas.lower(self.id, belowthis)
	def move(self, xamount, yamount):
		self.canvas.move(self.id, xamount, yamount)
	def raise_(self, abovethis=None):
		self.canvas.raise_(self.id, abovethis)
	def scale(self, xorigin, yorigin, xscale, yscale):
		self.canvas.scale(self.id, xorigin, yorigin, xscale, yscale)
	def type(self):
		return self.canvas.type(self.id)

class Arc(CanvasItem):
	def __init__(self, canvas, (x1, y1), (x2, y2), cnf={}):
		CanvasItem.__init__(self, canvas, 'arc',
							(str(x1), str(y1), str(x2), str(y2)), cnf)

class Bitmap(CanvasItem):
	def __init__(self, canvas, (x1, y1), cnf={}):
		CanvasItem.__init__(self, canvas, 'bitmap', (str(x1), str(y1)), cnf)

class Line(CanvasItem):
	def __init__(self, canvas, pts, cnf={}):
		pts = reduce(lambda a, b: a+b,
					 map(lambda pt: (str(pt[0]), str(pt[1])), pts))
		CanvasItem.__init__(self, canvas, 'line', pts, cnf)

class Oval(CanvasItem):
	def __init__(self, canvas, (x1, y1), (x2, y2), cnf={}):
		CanvasItem.__init__(self, canvas, 'oval',
							(str(x1), str(y1), str(x2), str(y2)), cnf)

class Polygon(CanvasItem):
	def __init__(self, canvas, pts, cnf={}):
		pts = reduce(lambda a, b: a+b,
					 map(lambda pt: (str(pt[0]), str(pt[1])), pts))
		CanvasItem.__init__(self, canvas, 'polygon', pts, cnf)

class Curve(Polygon):
	def __init__(self, canvas, pts, cnf={}):
		cnf['smooth'] = 'yes'
		Polygon.__init__(self, canvas, pts, cnf)

class Rectangle(CanvasItem):
	def __init__(self, canvas, (x1, y1), (x2, y2), cnf={}):
		CanvasItem.__init__(self, canvas, 'rectangle',
							(str(x1), str(y1), str(x2), str(y2)), cnf)

# XXX Can't use name "Text" since that is already taken by the Text widget...
class String(CanvasItem):
	def __init__(self, canvas, (x1, y1), cnf={}):
		CanvasItem.__init__(self, canvas, 'text', (str(x1), str(y1)), cnf)

class Window(CanvasItem):
	def __init__(self, canvas, where, cnf={}):
		CanvasItem.__init__(self, canvas, 'window',
							(str(where[0]), str(where[1])), cnf)
