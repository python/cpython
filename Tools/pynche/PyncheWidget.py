"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

from Tkinter import *
import Pmw
import ColorDB
from ChipWidget import ChipWidget
from TypeinWidget import TypeinWidget
from StripWidget import StripWidget


def constant(numchips):
    step = 255.0 / (numchips - 1)
    start = 0.0
    seq = []
    while numchips > 0:
	seq.append(int(start))
	start = start + step
	numchips = numchips - 1
    return seq

# red variations, green+blue = cyan constant
def constant_cyan_generator(numchips, rgbtuple):
    red, green, blue = rgbtuple
    seq = constant(numchips)
    return map(None, seq, [green] * numchips, [blue] * numchips)

# green variations, red+blue = magenta constant
def constant_magenta_generator(numchips, rgbtuple):
    red, green, blue = rgbtuple
    seq = constant(numchips)
    return map(None, [red] * numchips, seq, [blue] * numchips)

# blue variations, red+green = yellow constant
def constant_yellow_generator(numchips, rgbtuple):
    red, green, blue = rgbtuple
    seq = constant(numchips)
    return map(None, [red] * numchips, [green] * numchips, seq)



class PyncheWidget(Pmw.MegaWidget):
    def __init__(self, colordb, parent=None, **kw):
	self.__colordb = colordb

	options = (('color', (128, 128, 128), self.__set_color),
		   ('delegate', None, None),
		   )
	self.defineoptions(kw, options)

	# initialize base class -- after defining options
	Pmw.MegaWidget.__init__(self, parent)
	interiorarg = (self.interior(),)

	# create color selectors
	group = Pmw.Group(parent, tag_text='Variations')
	group.pack(side=TOP, expand=YES, fill=BOTH)
	self.__reds = StripWidget(group.interior(),
				  generator=constant_cyan_generator,
				  axis=0, label='Red Variations')
	self.__reds.pack()
	self.__blues = StripWidget(group.interior(),
				   generator=constant_magenta_generator,
				   axis=1, label='Green Variations')
	self.__blues.pack()
	self.__greens = StripWidget(group.interior(),
				    generator=constant_yellow_generator,
				    axis=2, label='Blue Variations')
	self.__greens.pack()

	# create chip window
	group = Pmw.Group(parent, tag_text='Current Color')
	group.pack(side=LEFT, expand=YES, fill=BOTH)
	self.__selected = ChipWidget(group.interior(),
				     label_text='Selected')
	self.__selected.grid()
	self.__nearest = ChipWidget(group.interior(),
				    label_text='Nearest')
	self.__nearest.grid(row=0, column=1)
	chip = self.__nearest.component('chip')
	chip.bind('<ButtonRelease-1>', self.__set_color_to_chip)

	# create the options window
	self.__typein = TypeinWidget(group.interior())
	self.__typein.grid(row=0, column=2)

	# Check keywords and initialize options
	self.initialiseoptions(PyncheWidget)

	self.__typein.configure(delegate=self)
	self.__reds.configure(delegate=self)
	self.__greens.configure(delegate=self)
	self.__blues.configure(delegate=self)

    #
    # PUBLIC INTERFACE
    #


    def set_color(self, obj, rgbtuple):
	nearest = self.__colordb.nearest(rgbtuple)
	red, green, blue = self.__colordb.find_byname(nearest)
	# for an exact match, use the color name
	if (red, green, blue) == rgbtuple:
	    self.__selected.configure(color=nearest)
	# otherwise, use the #rrggbb name
	else:
	    rrggbb = ColorDB.triplet_to_rrggbb(rgbtuple)
	    self.__selected.configure(color=rrggbb)

	# configure all subwidgets
	self.__nearest.configure(color=nearest)
	self.__typein.configure(color=rgbtuple)
	self.__reds.configure(color=rgbtuple)
	self.__greens.configure(color=rgbtuple)
	self.__blues.configure(color=rgbtuple)
	delegate = self['delegate']
	if delegate:
	    delegate.set_color(self, rgbtuple)

    #
    # PRIVATE INTERFACE
    #

    def __set_color(self):
	self.set_color(self, self['color'])

    def __set_color_to_chip(self, event=None):
	color = self.__nearest['color']
	rgbtuple = self.__colordb.find_byname(color)
	self.set_color(self, rgbtuple)
