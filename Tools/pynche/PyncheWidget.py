"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

from Tkinter import *
import Pmw
import ColorDB
from ChipWidget import ChipWidget
from TypeinWidget import TypeinWidget
from StripWidget import StripWidget



ABOUTTEXT = '''Pynche 1.0 -- The Pythonically Natural Color and Hue Editor
Copyright (C) 1998 Barry A. Warsaw

Pynche is based on ICE 1.2 (Interactive Color Editor), written by
Barry A. Warsaw for the SunView window system in 1987.'''



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
	self.__parent = parent
	self.__about = None

	options = (('color', (128, 128, 128), self.__set_color),
		   ('delegate', None, None),
		   )
	self.defineoptions(kw, options)

	# initialize base class -- after defining options
	Pmw.MegaWidget.__init__(self, parent)

	# create menubar
	self.__menubar = Pmw.MenuBar(parent,
				     hull_relief=RAISED,
				     hull_borderwidth=1)
	self.__menubar.pack(side=TOP, expand=YES, fill=BOTH)
	self.__menubar.addmenu('File', None)
	self.__menubar.addmenuitem('File',
				   type=COMMAND,
				   label='Quit',
				   command=self.__quit,
				   accelerator='Alt-Q')
	self.__menubar.addmenu('Help', None, side=RIGHT)
	self.__menubar.addmenuitem('Help',
				   type=COMMAND,
				   label='About...',
				   command=self.__popup_about,
				   accelerator='Alt-A')
	parent.bind('<Alt-q>', self.__quit)
	parent.bind('<Alt-Q>', self.__quit)
	parent.bind('<Alt-a>', self.__popup_about)
	parent.bind('<Alt-A>', self.__popup_about)

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

	# TBD: this is somewhat bogus, as the code should be in a derived
	# class of ChipWidget.
	self.__chip = self.__nearest.component('chip')
	self.__chip.bind('<ButtonPress-1>', self.__buttonpress)
	self.__chip.bind('<ButtonRelease-1>', self.__buttonrelease)

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

    def __buttonpress(self, event=None):
	self.__chip.configure(relief=SUNKEN)

    def __buttonrelease(self, event=None):
	self.__chip.configure(relief=RAISED)
	color = self.__nearest['color']
	rgbtuple = self.__colordb.find_byname(color)
	self.set_color(self, rgbtuple)

    def __quit(self, event=None):
	self.__parent.quit()

    def __popup_about(self, event=None):
	if not self.__about:
	    Pmw.aboutversion('1.0')
	    Pmw.aboutcopyright('Copyright (C) 1998 Barry A. Warsaw\n'
			       'All rights reserved')
	    Pmw.aboutcontact('For information about Pynche contact:\n'
			     'Barry A. Warsaw\n'
			     'email: bwarsaw@python.org')
	    self.__about = Pmw.AboutDialog(
		applicationname='Pynche -- the PYthonically Natural\n'
		'Color and Hue Editor')
	self.__about.show()
