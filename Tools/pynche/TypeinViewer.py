from Tkinter import *
import Pmw
import string

class TypeinWidget(Pmw.MegaWidget):
    def __init__(self, parent=None, **kw):
	options = (('color', (128, 128, 128), self.__set_color),
		   ('delegate', None, None),
		   )
	self.defineoptions(kw, options)

	Pmw.MegaWidget.__init__(self,parent)
	interiorarg = (self.interior(),)

	# create the x, y, and z label
	self.__x = self.createcomponent(
	    'x', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Red',
	    label_width=5,
	    label_anchor=E,
	    labelpos=W,
	    maxwidth=4,
	    entry_width=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__x.grid(row=0, column=0)

	self.__y = self.createcomponent(
	    'y', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Green',
	    label_width=5,
	    label_anchor=E,
	    labelpos=W,
	    maxwidth=4,
	    entry_width=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__y.grid(row=1, column=0)

	self.__z = self.createcomponent(
	    'z', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Blue',
	    label_width=5,
	    label_anchor=E,
	    labelpos=W,
	    maxwidth=4,
	    entry_width=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__z.grid(row=2, column=0)

	# Check keywords and initialize options
	self.initialiseoptions(TypeinWidget)

    #
    # PUBLIC INTERFACE
    #

    def set_color(self, obj, rgbtuple):
	# break infloop
	red, green, blue = rgbtuple
	self.__x.setentry(`red`)
	self.__y.setentry(`green`)
	self.__z.setentry(`blue`)
	if obj == self:
	    return

    #
    # PRIVATE INTERFACE
    #
	 
    # called to validate the entry text
    def __str_to_int(self, text):
	try:
	    if text[:2] == '0x':
		return string.atoi(text[2:], 16)
	    else:
		return string.atoi(text)
	except:
	    return None

    def __validate(self, text):
	val = self.__str_to_int(text)
	if val and val >= 0 and val < 256:
	    return 1
	else:
	    return -1

    # called whenever a text entry is modified
    def __modified(self):
	# these are guaranteed to be valid, right?
	vals = map(lambda x: x.get(), (self.__x, self.__y, self.__z))
	rgbs = map(self.__str_to_int, vals)
	valids = map(self.__validate, vals)
	delegate = self['delegate']
	if None not in rgbs and -1 not in valids and delegate:
	    delegate.set_color(self, rgbs)

    # called whenever the color option is changed
    def __set_color(self):
	rgbtuple = self['color']
	self.set_color(self, rgbtuple)
