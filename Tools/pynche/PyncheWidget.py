"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.
"""

import sys
from Tkinter import *
import tkMessageBox

# Milliseconds between interrupt checks
KEEPALIVE_TIMER = 500



class PyncheWidget:
    def __init__(self, version, switchboard):
        self.__sb = switchboard
        self.__version = version
        self.__textwin = None
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
        menubar.grid(row=0, column=0, columnspan=2, sticky='EW')
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
        # View menu
        #
        viewbtn = Menubutton(menubar, text='View',
                             underline=0)
        viewbtn.pack(side=LEFT)
        viewmenu = Menu(viewbtn, tearoff=0)
        viewbtn['menu'] = viewmenu
        viewmenu.add_command(label='Text Window...',
                             command=self.__popup_text,
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

    def __popup_about(self, event=None):
        tkMessageBox.showinfo('About Pynche 1.0',
                              '''\
Pynche -- the PYthonically
Natural Color and Hue Editor

Copyright (C) 1998
Barry A. Warsaw
All rights reserved

For information about Pynche
contact: Barry A. Warsaw
email:   bwarsaw@python.org''')

    def __popup_text(self, event=None):
        if not self.__textwin:
            from TextViewer import TextViewer
            self.__textwin = TextViewer(self.__sb, self.__root)
            self.__sb.add_view(self.__textwin)
        self.__textwin.deiconify()
