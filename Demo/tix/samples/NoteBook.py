# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# Tix Demostration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets.py": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program.

# This file demonstrates the use of the tixNoteBook widget, which allows
# you to lay out your interface using a "notebook" metaphore
#
import tkinter.tix

def RunSample(w):
    global root
    root = w

    # We use these options to set the sizes of the subwidgets inside the
    # notebook, so that they are well-aligned on the screen.
    prefix = tkinter.tix.OptionName(w)
    if prefix:
        prefix = '*'+prefix
    else:
        prefix = ''
    w.option_add(prefix+'*TixControl*entry.width', 10)
    w.option_add(prefix+'*TixControl*label.width', 18)
    w.option_add(prefix+'*TixControl*label.anchor', tkinter.tix.E)
    w.option_add(prefix+'*TixNoteBook*tagPadX', 8)

    # Create the notebook widget and set its backpagecolor to gray.
    # Note that the -backpagecolor option belongs to the "nbframe"
    # subwidget.
    nb = tkinter.tix.NoteBook(w, name='nb', ipadx=6, ipady=6)
    nb['bg'] = 'gray'
    nb.nbframe['backpagecolor'] = 'gray'

    # Create the two tabs on the notebook. The -underline option
    # puts a underline on the first character of the labels of the tabs.
    # Keyboard accelerators will be defined automatically according
    # to the underlined character.
    nb.add('hard_disk', label="Hard Disk", underline=0)
    nb.add('network', label="Network", underline=0)

    nb.pack(expand=1, fill=tkinter.tix.BOTH, padx=5, pady=5 ,side=tkinter.tix.TOP)

    #----------------------------------------
    # Create the first page
    #----------------------------------------
    # Create two frames: one for the common buttons, one for the
    # other widgets
    #
    tab=nb.hard_disk
    f = tkinter.tix.Frame(tab)
    common = tkinter.tix.Frame(tab)

    f.pack(side=tkinter.tix.LEFT, padx=2, pady=2, fill=tkinter.tix.BOTH, expand=1)
    common.pack(side=tkinter.tix.RIGHT, padx=2, fill=tkinter.tix.Y)

    a = tkinter.tix.Control(f, value=12,   label='Access time: ')
    w = tkinter.tix.Control(f, value=400,  label='Write Throughput: ')
    r = tkinter.tix.Control(f, value=400,  label='Read Throughput: ')
    c = tkinter.tix.Control(f, value=1021, label='Capacity: ')

    a.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    w.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    r.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    c.pack(side=tkinter.tix.TOP, padx=20, pady=2)

    # Create the common buttons
    createCommonButtons(common)

    #----------------------------------------
    # Create the second page
    #----------------------------------------

    tab = nb.network

    f = tkinter.tix.Frame(tab)
    common = tkinter.tix.Frame(tab)

    f.pack(side=tkinter.tix.LEFT, padx=2, pady=2, fill=tkinter.tix.BOTH, expand=1)
    common.pack(side=tkinter.tix.RIGHT, padx=2, fill=tkinter.tix.Y)

    a = tkinter.tix.Control(f, value=12,   label='Access time: ')
    w = tkinter.tix.Control(f, value=400,  label='Write Throughput: ')
    r = tkinter.tix.Control(f, value=400,  label='Read Throughput: ')
    c = tkinter.tix.Control(f, value=1021, label='Capacity: ')
    u = tkinter.tix.Control(f, value=10,   label='Users: ')

    a.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    w.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    r.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    c.pack(side=tkinter.tix.TOP, padx=20, pady=2)
    u.pack(side=tkinter.tix.TOP, padx=20, pady=2)

    createCommonButtons(common)

def doDestroy():
    global root
    root.destroy()

def createCommonButtons(master):
    ok = tkinter.tix.Button(master, name='ok', text='OK', width=6,
                command=doDestroy)
    cancel = tkinter.tix.Button(master, name='cancel',
                    text='Cancel', width=6,
                    command=doDestroy)

    ok.pack(side=tkinter.tix.TOP, padx=2, pady=2)
    cancel.pack(side=tkinter.tix.TOP, padx=2, pady=2)

if __name__ == '__main__':
    root = tkinter.tix.Tk()
    RunSample(root)
    root.mainloop()
