"""Color chip megawidget.
This widget is used for displaying a color.  It consists of three components:

    label -- a Tkinter.Label, this is the chip's label which is displayed
             about the color chip 
    chip  -- A Tkinter.Frame, the frame displaying the color
    name  -- a Tkinter.Label, the name of the color

In addition, the megawidget understands the following options:

    color -- the color displayed in the chip and name widgets

When run as a script, this program displays a sample chip.
"""


from Tkinter import *
import Pmw

class ChipWidget(Pmw.MegaWidget):
    _WIDTH = 150
    _HEIGHT = 80

    def __init__(self, parent=None, **kw):
	options = (('chip_borderwidth', 2,            None),
		   ('chip_width',       self._WIDTH,  None),
		   ('chip_height',      self._HEIGHT, None),
		   ('label_text',       'Color',      None),
		   ('color',            'blue',       self.__set_color),
		   )
	self.defineoptions(kw, options)

	# initialize base class -- after defining options
	Pmw.MegaWidget.__init__(self, parent)
	interiorarg = (self.interior(),)

	# create the label
	self.__label = self.createcomponent(
	    # component name, aliases, group
	    'label', (), None,
	    # widget class, widget args
	    Label, interiorarg)
	self.__label.grid(row=0, column=0)

	# create the color chip
	self.__chip = self.createcomponent(
	    'chip', (), None,
	    Frame, interiorarg,
	    relief=RAISED, borderwidth=2)
	self.__chip.grid(row=1, column=0)

	# create the color name
	self.__name = self.createcomponent(
	    'name', (), None,
	    Label, interiorarg,)
	self.__name.grid(row=2, column=0)

	# Check keywords and initialize options
	self.initialiseoptions(ChipWidget)

    # called whenever `color' option is set
    def __set_color(self):
	color = self['color']
	self.__chip['background'] = color
	self.__name['text'] = color



if __name__ == '__main__':
    root = Pmw.initialise(fontScheme='pmw1')
    root.title('ChipWidget demonstration')

    exitbtn = Button(root, text='Exit', command=root.destroy)
    exitbtn.pack(side=BOTTOM)
    widget = ChipWidget(root, color='red',
			chip_width=200,
			label_text='Selected Color')
    widget.pack()
    root.mainloop()
