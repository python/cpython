"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

import sys
from Tkinter import *

# Milliseconds between interrupt checks
KEEPALIVE_TIMER = 500



class PyncheWidget:
    def __init__(self, version):
        # create the first and top window
        root = self.__root = Tk(className='Pynche')
        root.protocol('WM_DELETE_WINDOW', self.__quit)
        root.title('Pynche %s' % version)
        root.iconname('Pynche')
        root.tk.createtimerhandler(KEEPALIVE_TIMER, self.__keepalive)
        #
        # create the menubar
        #
        menubar = self.__menubar = Frame(root, relief=RAISED, borderwidth=2)
        menubar.pack(fill=X)
        #
        # File menu
        #
        filebtn = Menubutton(menubar, text='File',
                             underline=0)
        filebtn.pack(side=LEFT)
        filemenu = Menu(filebtn, tearoff=0)
        filebtn['menu'] = filemenu
        filemenu.add_command(label='Quit',
                             command=self.__quit,
                             accelerator='Alt-Q',
                             underline=0)
        root.bind('<Alt-q>', self.__quit)
        root.bind('<Alt-Q>', self.__quit)
        #
        # Edit Menu
        #
	editbtn = Menubutton(menubar, text='Edit',
                             underline=0)
        editbtn.pack(side=LEFT)
        editmenu = Menu(editbtn, tearoff=0)
        editbtn['menu'] = editmenu
	editmenu.add_command(label='Options...',
                             command=self.__popup_options,
                             underline=0)
        #
        # Help menu
        #
        helpbtn = Menubutton(menubar, text='Help',
                             underline=0)
        helpbtn.pack(side=RIGHT)
        helpmenu = Menu(helpbtn, tearoff=0)
        helpbtn['menu'] = helpmenu
	helpmenu.add_command(label='About...',
                             command=self.__popup_about,
                             underline=0)

    def __keepalive(self):
        # Exercise the Python interpreter regularly so keyboard interrupts get
        # through.
        self.__root.tk.createtimerhandler(KEEPALIVE_TIMER, self.__keepalive)

    def __quit(self, event=None):
        sys.exit(0)

    def start(self):
        self.__keepalive()
        self.__root.mainloop()

    def parent(self):
        return self.__root

    def __popup_options(self, event=None):
        print 'Options...'

    def __popup_about(self, event=None):
        print 'About...'

##	# create chip window
##	group = Pmw.Group(parent, tag_text='Current Color')
##        interior = group.interior()
##	group.pack(side=LEFT, expand=YES, fill=BOTH)
##	self.__selected = ChipWidget(interior,
##				     label_text='Selected')
##	self.__selected.grid()
##	self.__nearest = ChipWidget(interior,
##				    label_text='Nearest')
##	self.__nearest.grid(row=0, column=1)

##	# TBD: this is somewhat bogus, as the code should be in a derived
##	# class of ChipWidget.
##	self.__chip = self.__nearest.component('chip')
##	self.__chip.bind('<ButtonPress-1>', self.__buttonpress)
##	self.__chip.bind('<ButtonRelease-1>', self.__buttonrelease)

##	# create the type-in window
##	self.__typein = TypeinWidget(interior)
##	self.__typein.grid(row=0, column=2)

##	# Check keywords and initialize options
##	self.initialiseoptions(PyncheWidget)

##	self.__typein.configure(delegate=self)
##	self.__reds.configure(delegate=self)
##	self.__greens.configure(delegate=self)
##	self.__blues.configure(delegate=self)

##    def __popup_about(self, event=None):
##	if not self.__about_dialog:
##	    Pmw.aboutversion('1.0')
##	    Pmw.aboutcopyright('Copyright (C) 1998 Barry A. Warsaw\n'
##			       'All rights reserved')
##	    Pmw.aboutcontact('For information about Pynche contact:\n'
##			     'Barry A. Warsaw\n'
##			     'email: bwarsaw@python.org')
##	    self.__about_dialog = Pmw.AboutDialog(
##		applicationname='Pynche -- the PYthonically Natural\n'
##		'Color and Hue Editor')
##	self.__about_dialog.show()

