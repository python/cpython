"""Main Pynche (Pythonically Natural Color and Hue Editor) widget.

This window provides the basic decorations, primarily including the menubar.
It is used to bring up other windows.
"""

import sys
import os
import string
from Tkinter import *
import tkMessageBox

# Milliseconds between interrupt checks
KEEPALIVE_TIMER = 500



class PyncheWidget:
    def __init__(self, version, switchboard, master=None):
        self.__sb = switchboard
        self.__version = version
        self.__textwin = None
        self.__listwin = None
        self.__detailswin = None
        self.__helpwin = None
        modal = self.__modal = not not master
        # If a master was given, we are running as a modal dialog servant to
        # some other application.  We rearrange our UI in this case (there's
        # no File menu and we get `Okay' and `Cancel' buttons), and we do a
        # grab_set() to make ourselves modal
        if modal:
            self.__tkroot = tkroot = Toplevel(master, class_='Pynche')
            tkroot.grab_set()
            tkroot.withdraw()
        else:
            # Is there already a default root for Tk, say because we're
            # running under Guido's IDE? :-) Two conditions say no, either the
            # import fails or _default_root is None.
            tkroot = None
            try:
                from Tkinter import _default_root
                tkroot = self.__tkroot = _default_root
            except ImportError:
                pass
            if not tkroot:
                tkroot = self.__tkroot = Tk(className='Pynche')
            # but this isn't our top level widget, so make it invisible
            tkroot.withdraw()
        # create the menubar
        menubar = self.__menubar = Menu(tkroot)
        #
        # File menu
        #
        if not modal:
            filemenu = self.__filemenu = Menu(menubar, tearoff=0)
            filemenu.add_command(label='Quit',
                                 command=self.__quit,
                                 accelerator='Alt-Q',
                                 underline=0)
        #
        # View menu
        #
        viewmenu = Menu(menubar, tearoff=0)
        viewmenu.add_command(label='Text Window...',
                             command=self.__popup_text,
                             underline=0)
        viewmenu.add_command(label='Color List Window...',
                             command=self.__popup_listwin,
                             underline=0)
        viewmenu.add_command(label='Details Window...',
                             command=self.__popup_details,
                             underline=0)
        #
        # Help menu
        #
        helpmenu = Menu(menubar, name='help', tearoff=0)
	helpmenu.add_command(label='About Pynche...',
                             command=self.__popup_about,
                             underline=0)
        helpmenu.add_command(label='Help...',
                             command=self.__popup_usage,
                             underline=0)
        #
        # Tie them all together
        #
        if not modal:
            menubar.add_cascade(label='File',
                                menu=filemenu,
                                underline=0)
        menubar.add_cascade(label='View',
                            menu=viewmenu,
                            underline=0)
        menubar.add_cascade(label='Help',
                            menu=helpmenu,
                            underline=0)

        # now create the top level window
        root = self.__root = Toplevel(tkroot, class_='Pynche', menu=menubar)
        root.protocol('WM_DELETE_WINDOW',
                      modal and self.__bell or self.__quit)
        root.title('Pynche %s' % version)
        root.iconname('Pynche')
        # Only bind accelerators for the File->Quit menu item if running as a
        # standalone app
        if not modal:
            root.bind('<Alt-q>', self.__quit)
            root.bind('<Alt-Q>', self.__quit)
        else:
            # We're a modal dialog so we have a new row of buttons
            bframe = Frame(root, borderwidth=1, relief=RAISED)
            bframe.grid(row=4, column=0, columnspan=2,
                        sticky='EW',
                        ipady=5)
            okay = Button(bframe,
                          text='Okay',
                          command=self.__okay)
            okay.pack(side=LEFT, expand=1)
            cancel = Button(bframe,
                            text='Cancel',
                            command=self.__cancel)
            cancel.pack(side=LEFT, expand=1)

    def __quit(self, event=None):
        self.__tkroot.quit()

    def __bell(self, event=None):
        self.__tkroot.bell()

    def __okay(self, event=None):
        self.__sb.withdraw_views()
        self.__tkroot.grab_release()
        self.__quit()

    def __cancel(self, event=None):
        self.__sb.canceled()
        self.__okay()

    def __keepalive(self):
        # Exercise the Python interpreter regularly so keyboard interrupts get
        # through.
        self.__tkroot.tk.createtimerhandler(KEEPALIVE_TIMER, self.__keepalive)

    def start(self):
        if not self.__modal:
            self.__keepalive()
        self.__tkroot.mainloop()

    def window(self):
        return self.__root

    def __popup_about(self, event=None):
        from Main import __version__
        tkMessageBox.showinfo('About Pynche ' + __version__,
                              '''\
Pynche %s
The PYthonically Natural
Color and Hue Editor

For information
contact: Barry A. Warsaw
email:   bwarsaw@python.org''' % __version__)

    def __popup_usage(self, event=None):
        if not self.__helpwin:
            self.__helpwin = Helpwin(self.__root, self.__quit)
        self.__helpwin.deiconify()

    def __popup_text(self, event=None):
        if not self.__textwin:
            from TextViewer import TextViewer
            self.__textwin = TextViewer(self.__sb, self.__root)
            self.__sb.add_view(self.__textwin)
        self.__textwin.deiconify()

    def __popup_listwin(self, event=None):
        if not self.__listwin:
            from ListViewer import ListViewer
            self.__listwin = ListViewer(self.__sb, self.__root)
            self.__sb.add_view(self.__listwin)
        self.__listwin.deiconify()

    def __popup_details(self, event=None):
        if not self.__detailswin:
            from DetailsViewer import DetailsViewer
            self.__detailswin = DetailsViewer(self.__sb, self.__root)
            self.__sb.add_view(self.__detailswin)
        self.__detailswin.deiconify()

    def withdraw(self):
        self.__root.withdraw()

    def deiconify(self):
        self.__root.deiconify()



class Helpwin:
    def __init__(self, master, quitfunc):
        from Main import __version__, docstring
        self.__root = root = Toplevel(master, class_='Pynche')
        root.protocol('WM_DELETE_WINDOW', self.__withdraw)
        root.title('Pynche Help Window')
        root.iconname('Pynche Help Window')
        root.bind('<Alt-q>', quitfunc)
        root.bind('<Alt-Q>', quitfunc)
        root.bind('<Alt-w>', self.__withdraw)
        root.bind('<Alt-W>', self.__withdraw)

        # more elaborate help is available in the README file
        readmefile = os.path.join(sys.path[0], 'README')
        try:
            fp = None
            try:
                fp = open(readmefile)
                contents = fp.read()
                # wax the last page, it contains Emacs cruft
                i = string.rfind(contents, '\f')
                if i > 0:
                    contents = string.rstrip(contents[:i])
            finally:
                if fp:
                    fp.close()
        except IOError:
            sys.stderr.write("Couldn't open Pynche's README, "
                             'using docstring instead.\n')
            contents = docstring()

        self.__text = text = Text(root, relief=SUNKEN,
                                  width=80, height=24)
        self.__text.focus_set()
        text.insert(0.0, contents)
        scrollbar = Scrollbar(root)
        scrollbar.pack(fill=Y, side=RIGHT)
        text.pack(fill=BOTH, expand=YES)
        text.configure(yscrollcommand=(scrollbar, 'set'))
        scrollbar.configure(command=(text, 'yview'))

    def __withdraw(self, event=None):
        self.__root.withdraw()

    def deiconify(self):
        self.__root.deiconify()
