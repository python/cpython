#! /usr/bin/env python

# A Python function that generates dialog boxes with a text message,
# optional bitmap, and any number of buttons.
# Cf. Ousterhout, Tcl and the Tk Toolkit, Figs. 27.2-3, pp. 269-270.

from Tkinter import *
import sys


def dialog(master, title, text, bitmap, default, *args):

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    w = Toplevel(master, class_='Dialog')
    w.title(title)
    w.iconname('Dialog')

    top = Frame(w, relief=RAISED, borderwidth=1)
    top.pack(side=TOP, fill=BOTH)
    bot = Frame(w, relief=RAISED, borderwidth=1)
    bot.pack(side=BOTTOM, fill=BOTH)

    # 2. Fill the top part with the bitmap and message.

    msg = Message(top, width='3i', text=text,
                  font='-Adobe-Times-Medium-R-Normal-*-180-*')
    msg.pack(side=RIGHT, expand=1, fill=BOTH, padx='3m', pady='3m')
    if bitmap:
        bm = Label(top, bitmap=bitmap)
        bm.pack(side=LEFT, padx='3m', pady='3m')

    # 3. Create a row of buttons at the bottom of the dialog.

    var = IntVar()
    buttons = []
    i = 0
    for but in args:
        b = Button(bot, text=but, command=lambda v=var,i=i: v.set(i))
        buttons.append(b)
        if i == default:
            bd = Frame(bot, relief=SUNKEN, borderwidth=1)
            bd.pack(side=LEFT, expand=1, padx='3m', pady='2m')
            b.lift()
            b.pack (in_=bd, side=LEFT,
                    padx='2m', pady='2m', ipadx='2m', ipady='1m')
        else:
            b.pack (side=LEFT, expand=1,
                    padx='3m', pady='3m', ipadx='2m', ipady='1m')
        i = i+1

    # 4. Set up a binding for <Return>, if there's a default,
    # set a grab, and claim the focus too.

    if default >= 0:
        w.bind('<Return>',
               lambda e, b=buttons[default], v=var, i=default:
               (b.flash(),
                v.set(i)))

    oldFocus = w.focus_get()
    w.grab_set()
    w.focus_set()

    # 5. Wait for the user to respond, then restore the focus
    # and return the index of the selected button.

    w.waitvar(var)
    w.destroy()
    if oldFocus: oldFocus.focus_set()
    return var.get()

# The rest is the test program.

def go():
    i = dialog(mainWidget,
               'Not Responding',
               "The file server isn't responding right now; "
               "I'll keep trying.",
               '',
               -1,
               'OK')
    print 'pressed button', i
    i = dialog(mainWidget,
               'File Modified',
               'File "tcl.h" has been modified since '
               'the last time it was saved. '
               'Do you want to save it before exiting the application?',
               'warning',
               0,
               'Save File',
               'Discard Changes',
               'Return To Editor')
    print 'pressed button', i

def test():
    import sys
    global mainWidget
    mainWidget = Frame()
    Pack.config(mainWidget)
    start = Button(mainWidget, text='Press Here To Start', command=go)
    start.pack()
    endit = Button(mainWidget, text="Exit", command=sys.exit)
    endit.pack(fill=BOTH)
    mainWidget.mainloop()

if __name__ == '__main__':
    test()
