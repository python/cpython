from Tkinter import *
import Pmw
import ColorDB

class LeftArrow:
    _ARROWWIDTH = 30
    _ARROWHEIGHT = 15
    _YOFFSET = 13
    _TEXTYOFFSET = 1
    _TAG = ('leftarrow',)

    def __init__(self, canvas, x):
	self._canvas = canvas
	self.__arrow, self.__text = self._create(x)
	self.move_to(x)

    def _create(self, x):
	arrow = self._canvas.create_line(
	    x, self._ARROWHEIGHT + self._YOFFSET,
	    x, self._YOFFSET,
	    x + self._ARROWWIDTH, self._YOFFSET,
	    arrow='first',
	    width=3.0,
	    tags=self._TAG)
	text = self._canvas.create_text(
	    x + self._ARROWWIDTH + 13,
	    self._ARROWHEIGHT - self._TEXTYOFFSET,
	    text='128')
	return arrow, text

    def _x(self):
	coords = self._canvas.coords(self._TAG)
	assert coords
	return coords[0]

    def move_to(self, x):
	deltax = x - self._x()
	self._canvas.move(self._TAG, deltax, 0)


class RightArrow(LeftArrow):
    _TAG = ('rightarrow',)

    def _create(self, x):
	arrow = self._canvas.create_line(
	    x, self._YOFFSET,
	    x + self._ARROWWIDTH, self._YOFFSET,
	    x + self._ARROWWIDTH, self._ARROWHEIGHT + self._YOFFSET,
	    arrow='last',
	    width=3.0,
	    tags=self._TAG)
	text = self._canvas.create_text(
	    x - self._ARROWWIDTH - 20,		  # TBD: kludge
	    self._ARROWHEIGHT - self._TEXTYOFFSET,
	    text='128')
	return arrow, text

    def _x(self):
	coords = self._canvas.bbox(self._TAG)
	assert coords
	return coords[2] - 6			  # TBD: kludge



class StripWidget(Pmw.MegaWidget):
    _CHIPHEIGHT = 50
    _CHIPWIDTH = 10
    _NUMCHIPS = 40

    def __init__(self, parent=None, **kw):
	options = (('color', (128, 128, 128), self.__set_color),
		   ('delegate', None, None),
		   ('chipwidth', self._CHIPWIDTH, Pmw.INITOPT),
		   ('chipheight', self._CHIPHEIGHT, Pmw.INITOPT),
		   ('numchips', self._NUMCHIPS, Pmw.INITOPT),
		   ('generator', None, Pmw.INITOPT),
		   )
	self.defineoptions(kw, options)

	Pmw.MegaWidget.__init__(self, parent)
	interiorarg = (self.interior(),)

	# group component contains a convas containing a bunch of objects
	# (moveable arrow + text label, relief'd rectangle color chips)
	chipwidth = self.__chipwidth = self['chipwidth']
	chipheight = self.__chipheight = self['chipheight']
	numchips = self.__numchips = self['numchips']

	canvaswidth = numchips * (chipwidth + 1)
	canvasheight = chipheight + 35

	self.__canvas = Canvas(
	    parent,
	    width=canvaswidth,
	    height=canvasheight)
	self.__canvas.pack()

	# create the color strip
	chips = self.__chips = []
	x = 1
	y = 30
	for c in range(self.__numchips):
	    rect = self.__canvas.create_rectangle(
		x, y, x+chipwidth, y+chipheight,
		fill='grey', outline='grey')

	    x = x + chipwidth + 1		  # for outline
	    chips.append(rect)

	# create the arrow and text item
	chipx = self.__arrow_x(0)
	self.__leftarrow = LeftArrow(self.__canvas, chipx)

	chipx = self.__arrow_x(len(chips) - 1)
	self.__rightarrow = RightArrow(self.__canvas, chipx)

	self.__generator = self['generator']
	self.initialiseoptions(StripWidget)

    def __set_color(self):
	rgbtuple = self['color']
	if self.__generator:
	    i = 0
	    for t in self.__generator(self.__numchips, rgbtuple):
		rrggbb = ColorDB.triplet_to_rrggbb(t)
		self.__canvas.itemconfigure(self.__chips[i],
					    fill=rrggbb,
					    outline=rrggbb)
		i = i + 1
	
    def __arrow_x(self, chipnum):
	coords = self.__canvas.coords(self.__chips[chipnum])
	assert coords
	x0, y0, x1, y1 = coords
	return (x1 + x0) / 2.0
