"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

from Tkinter import *
import Pmw
import ColorDB
from ChipWidget import ChipWidget
from TypeinWidget import TypeinWidget

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
	self.__selectors = Pmw.Group(parent, tag_text='Color Selectors')
	self.__selectors.pack(side=TOP, expand=YES, fill=BOTH)

	# create chip window
	group = Pmw.Group(parent, tag_text='Current Color')
	group.pack(side=LEFT, fill=Y)
	self.__selected = ChipWidget(group.interior(),
				     label_text='Selected')
	self.__selected.grid()
	self.__nearest = ChipWidget(group.interior(),
				    label_text='Nearest')
	self.__nearest.grid(row=0, column=1)

	# create the options window
	group = Pmw.Group(parent, tag_text='Options')
	group.pack(expand=YES, fill=BOTH)
	self.__typein = TypeinWidget(group.interior())
	self.__typein.grid()

	# Check keywords and initialize options
	self.initialiseoptions(PyncheWidget)

	self.__typein.configure(delegate=self)

    #
    # PUBLIC INTERFACE
    #


    def set_color(self, obj, rgbtuple):
	print 'setting color to:', rgbtuple
	nearest = self.__colordb.nearest(rgbtuple)
	red, green, blue = self.__colordb.find_byname(nearest)
	# for an exact match, use the color name
	if (red, green, blue) == rgbtuple:
	    self.__selected.configure(color=nearest)
	# otherwise, use the #rrggbb name
	else:
	    rrggbb = ColorDB.triplet_to_rrggbb(rgbtuple)
	    self.__selected.configure(color=rrggbb)
	self.__nearest.configure(color=nearest)
	self.__typein.configure(color=rgbtuple)
	delegate = self['delegate']
	if delegate:
	    delegate.set_color(self, rgbtuple)

    #
    # PRIVATE INTERFACE
    #

    def __set_color(self):
	self.set_color(self, self['color'])
