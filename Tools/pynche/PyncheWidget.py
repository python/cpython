"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

from Tkinter import *
import Pmw
import ColorDB
from ChipWidget import ChipWidget

class PyncheWidget(Pmw.MegaWidget):
    def __init__(self, colordb, parent=None, **kw):
	self.__colordb = colordb

	optionsdefs = (('color', 'grey50', self.__set_color),)
	self.defineoptions(kw, optionsdefs)

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
	options = Pmw.Group(parent, tag_text='Options')
	options.pack(expand=YES, fill=BOTH)

	# Check keywords and initialize options
	self.initialiseoptions(PyncheWidget)


    def __set_color(self):
	color = self['color']
	try:
	    red, green, blue, rrggbb = self.__colordb.find_byname(color)
	except ColorDB.BadColor:
	    red, green, blue = ColorDB.rrggbb_to_triplet(color)
	    
	nearest = self.__colordb.nearest(red, green, blue)
	self.__selected.configure(color=color)
	self.__nearest.configure(color=nearest)
