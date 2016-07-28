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

# This file demonstrates the use of the tixNoteBook widget, which allows
# you to lay out your interface using a "notebook" metaphore
#
import Tix

def RunSample(w):
    global root
    root = w

    # We use these options to set the sizes of the subwidgets inside the
    # notebook, so that they are well-aligned on the screen.
    prefix = Tix.OptionName(w)
    if prefix:
        prefix = '*'+prefix
    else:
        prefix = ''
    w.option_add(prefix+'*TixControl*entry.width', 10)
    w.option_add(prefix+'*TixControl*label.width', 18)
    w.option_add(prefix+'*TixControl*label.anchor', Tix.E)
    w.option_add(prefix+'*TixNoteBook*tagPadX', 8)

    # Create the notebook widget and set its backpagecolor to gray.
    # Note that the -backpagecolor option belongs to the "nbframe"
    # subwidget.
    nb = Tix.NoteBook(w, name='nb', ipadx=6, ipady=6)
    nb['bg'] = 'gray'
    nb.nbframe['backpagecolor'] = 'gray'

    # Create the two tabs on the notebook. The -underline option
    # puts a underline on the first character of the labels of the tabs.
    # Keyboard accelerators will be defined automatically according
    # to the underlined character.
    nb.add('hard_disk', label="Hard Disk", underline=0)
    nb.add('network', label="Network", underline=0)

    nb.pack(expand=1, fill=Tix.BOTH, padx=5, pady=5 ,side=Tix.TOP)

    #----------------------------------------
    # Create the first page
    #----------------------------------------
    # Create two frames: one for the common buttons, one for the
    # other widgets
    #
    tab=nb.hard_disk
    f = Tix.Frame(tab)
    common = Tix.Frame(tab)

    f.pack(side=Tix.LEFT, padx=2, pady=2, fill=Tix.BOTH, expand=1)
    common.pack(side=Tix.RIGHT, padx=2, fill=Tix.Y)

    a = Tix.Control(f, value=12,   label='Access time: ')
    w = Tix.Control(f, value=400,  label='Write Throughput: ')
    r = Tix.Control(f, value=400,  label='Read Throughput: ')
    c = Tix.Control(f, value=1021, label='Capacity: ')

    a.pack(side=Tix.TOP, padx=20, pady=2)
    w.pack(side=Tix.TOP, padx=20, pady=2)
    r.pack(side=Tix.TOP, padx=20, pady=2)
    c.pack(side=Tix.TOP, padx=20, pady=2)

    # Create the common buttons
    createCommonButtons(common)

    #----------------------------------------
    # Create the second page
    #----------------------------------------

    tab = nb.network

    f = Tix.Frame(tab)
    common = Tix.Frame(tab)

    f.pack(side=Tix.LEFT, padx=2, pady=2, fill=Tix.BOTH, expand=1)
    common.pack(side=Tix.RIGHT, padx=2, fill=Tix.Y)

    a = Tix.Control(f, value=12,   label='Access time: ')
    w = Tix.Control(f, value=400,  label='Write Throughput: ')
    r = Tix.Control(f, value=400,  label='Read Throughput: ')
    c = Tix.Control(f, value=1021, label='Capacity: ')
    u = Tix.Control(f, value=10,   label='Users: ')

    a.pack(side=Tix.TOP, padx=20, pady=2)
    w.pack(side=Tix.TOP, padx=20, pady=2)
    r.pack(side=Tix.TOP, padx=20, pady=2)
    c.pack(side=Tix.TOP, padx=20, pady=2)
    u.pack(side=Tix.TOP, padx=20, pady=2)

    createCommonButtons(common)

def doDestroy():
    global root
    root.destroy()

def createCommonButtons(master):
    ok = Tix.Button(master, name='ok', text='OK', width=6,
                command=doDestroy)
    cancel = Tix.Button(master, name='cancel',
                    text='Cancel', width=6,
                    command=doDestroy)

    ok.pack(side=Tix.TOP, padx=2, pady=2)
    cancel.pack(side=Tix.TOP, padx=2, pady=2)

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
    root.mainloop()
