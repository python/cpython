from Tkinter import *
import Pmw

class ChipWidget(Pmw.MegaWidget):
    _WIDTH = 80
    _HEIGHT = 100

    def __init__(self, parent=None, **kw):
	optionsdefs = (('chipcolor', 'blue', self.__set_color),
		       ('width', self._WIDTH, self.__set_dims),
		       ('height', self._HEIGHT, self.__set_dims),
		       ('text', 'Color', self.__set_label),
		       )
	self.defineoptions(kw, optionsdefs)

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

    # called whenever `chipcolor' option is set
    def __set_color(self):
	color = self['chipcolor']
	self.__chip['background'] = color
	self.__name['text'] = color

    def __set_dims(self):
	width = self['width']
	height = self['height']
	self.__chip.configure(width=width, height=height)

    def __set_label(self):
	self.__label['text'] = self['text']

Pmw.forwardmethods(ChipWidget, Frame, '__chip')



if __name__ == '__main__':
    root = Pmw.initialise(fontScheme='pmw1')
    root.title('ChipWidget demonstration')

    exitbtn = Button(root, text='Exit', command=root.destroy)
    exitbtn.pack(side=BOTTOM)
    widget = ChipWidget(root, chipcolor='red', width=200,
			text='Selected Color')
    widget.pack()
    root.mainloop()
