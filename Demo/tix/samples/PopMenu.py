# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
#       $Id$
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets.py": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program using tixwish.

# This file demonstrates the use of the tixPopupMenu widget.
#
import Tix

def RunSample(w):
    # We create the frame and the button, then we'll bind the PopupMenu
    # to both widgets. The result is, when you press the right mouse
    # button over $w.top or $w.top.but, the PopupMenu will come up.
    #
    top = Tix.Frame(w, relief=Tix.RAISED, bd=1)
    but = Tix.Button(top, text='Press the right mouse button over this button or its surrounding area')
    but.pack(expand=1, fill=Tix.BOTH, padx=50, pady=50)

    p = Tix.PopupMenu(top, title='Popup Test')
    p.bind_widget(top)
    p.bind_widget(but)

    # Set the entries inside the PopupMenu widget.
    # [Hint] You have to manipulate the "menu" subwidget.
    #        $w.top.p itself is NOT a menu widget.
    # [Hint] Watch carefully how the sub-menu is created
    #
    p.menu.add_command(label='Desktop', underline=0)
    p.menu.add_command(label='Select', underline=0)
    p.menu.add_command(label='Find', underline=0)
    p.menu.add_command(label='System', underline=1)
    p.menu.add_command(label='Help', underline=0)
    m1 = Tix.Menu(p.menu)
    m1.add_command(label='Hello')
    p.menu.add_cascade(label='More', menu=m1)

    but.pack(side=Tix.TOP, padx=40, pady=50)

    box = Tix.ButtonBox(w, orientation=Tix.HORIZONTAL)
    box.add('ok', text='Ok', underline=0, width=6,
            command=lambda w=w: w.destroy())
    box.add('cancel', text='Cancel', underline=0, width=6,
            command=lambda w=w: w.destroy())
    box.pack(side=Tix.BOTTOM, fill=Tix.X)
    top.pack(side=Tix.TOP, fill=Tix.BOTH, expand=1)

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
    root.mainloop()
