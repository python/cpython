from Tkinter import *
import Pmw

class TypeinWidget(Pmw.MegaWidget):
    def __init__(self, parent=None, **kw):
	options = (('delegate', None, None),
		   ('color', (128, 128, 128), self.__set_color),
		   )
	self.defineoptions(kw, options)

	Pmw.MegaWidget.__init__(self,parent)
	interiorarg = (self.interior(),)

	# create the x, y, and z label
	self.__x = self.createcomponent(
	    'x', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Red',
	    labelpos=E,
	    maxwidth=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__x.grid(row=0, column=0)

	self.__y = self.createcomponent(
	    'y', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Green',
	    labelpos=E,
	    maxwidth=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__y.grid(row=1, column=0)

	self.__z = self.createcomponent(
	    'z', (), None,
	    Pmw.EntryField, interiorarg,
	    label_text='Blue',
	    labelpos=E,
	    maxwidth=4,
	    validate=self.__validate,
	    modifiedcommand=self.__modified)
	self.__z.grid(row=2, column=0)

	# TBD: gross hack, fix later
	self.__initializing = 1
	# Check keywords and initialize options
	self.initialiseoptions(TypeinWidget)
	# TBD: gross hack, fix later
	self.__initializing = 0

    # public set color interface
    def set_color(self, red, green, blue):
	self.__x.configure(label_text=`red`)
	self.__y.configure(label_text=`green`)
	self.__z.configure(label_text=`blue`)
	# dispatch to the delegate
	delegate = self['delegate']
	if delegate:
	    delegate.set_color(red, green, blue)
	    
    # called to validate the entry text
    SAFE_EVAL = {'__builtins__': {}}
    def __str_to_int(self, text):
	try:
	    val = eval(text, self.SAFE_EVAL, {})
	    return val
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
	red = self.__str_to_int(self.__x['value'])
	green = self.__str_to_int(self.__y['value'])
	blue = self.__str_to_int(self.__z['value'])
	self.set_color(red, green, blue)

    # called whenever the color option is changed
    def __set_color(self):
	red, green, blue = self['color']
	if not self.__initializing:
	    self.set_color(red, green, blue)
