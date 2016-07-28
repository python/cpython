# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# Tix Demonstration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets.py": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program.

# This file demonstrates the use of the tixPanedWindow widget. This program
# is a dummy news reader: the user can adjust the sizes of the list
# of artical names and the size of the text widget that shows the body
# of the article.

import Tix

TCL_ALL_EVENTS          = 0

def RunSample (root):
    panedwin = DemoPanedwin(root)
    panedwin.mainloop()
    panedwin.destroy()

class DemoPanedwin:
    def __init__(self, w):
        self.root = w
        self.exit = -1

        z = w.winfo_toplevel()
        z.wm_protocol("WM_DELETE_WINDOW", lambda self=self: self.quitcmd())

        group = Tix.LabelEntry(w, label='Newsgroup:', options='entry.width 25')
        group.entry.insert(0,'comp.lang.python')
        pane = Tix.PanedWindow(w, orientation='vertical')

        p1 = pane.add('list', min=70, size=100)
        p2 = pane.add('text', min=70)
        list = Tix.ScrolledListBox(p1)
        list.listbox['width'] = 80
        list.listbox['height'] = 5
        text = Tix.ScrolledText(p2)
        text.text['width'] = 80
        text.text['height'] = 20

        list.listbox.insert(Tix.END, "  12324 Re: Tkinter is good for your health")
        list.listbox.insert(Tix.END, "+ 12325 Re: Tkinter is good for your health")
        list.listbox.insert(Tix.END, "+ 12326 Re: Tix is even better for your health (Was: Tkinter is good...)")
        list.listbox.insert(Tix.END, "  12327 Re: Tix is even better for your health (Was: Tkinter is good...)")
        list.listbox.insert(Tix.END, "+ 12328 Re: Tix is even better for your health (Was: Tkinter is good...)")
        list.listbox.insert(Tix.END, "  12329 Re: Tix is even better for your health (Was: Tkinter is good...)")
        list.listbox.insert(Tix.END, "+ 12330 Re: Tix is even better for your health (Was: Tkinter is good...)")

        text.text['bg'] = list.listbox['bg']
        text.text['wrap'] = 'none'
        text.text.insert(Tix.END, """
    Mon, 19 Jun 1995 11:39:52        comp.lang.python              Thread   34 of  220
    Lines 353       A new way to put text and bitmaps together iNo responses
    ioi@blue.seas.upenn.edu                Ioi K. Lam at University of Pennsylvania

    Hi,

    I have implemented a new image type called "compound". It allows you
    to glue together a bunch of bitmaps, images and text strings together
    to form a bigger image. Then you can use this image with widgets that
    support the -image option. For example, you can display a text string string
    together with a bitmap, at the same time, inside a TK button widget.
    """)
        text.text['state'] = 'disabled'

        list.pack(expand=1, fill=Tix.BOTH, padx=4, pady=6)
        text.pack(expand=1, fill=Tix.BOTH, padx=4, pady=6)

        group.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH)
        pane.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH, expand=1)

        box = Tix.ButtonBox(w, orientation=Tix.HORIZONTAL)
        box.add('ok', text='Ok', underline=0, width=6,
                command=self.quitcmd)
        box.add('cancel', text='Cancel', underline=0, width=6,
                command=self.quitcmd)
        box.pack(side=Tix.BOTTOM, fill=Tix.X)

    def quitcmd (self):
        self.exit = 0

    def mainloop(self):
        while self.exit < 0:
            self.root.tk.dooneevent(TCL_ALL_EVENTS)

    def destroy (self):
        self.root.destroy()

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
