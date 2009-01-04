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

# This file demonstrates how to use the TixTree widget to display
# dynamic hierachical data (the files in the Unix file system)
#

import tkinter.tix, os

def RunSample(w):
    top = tkinter.tix.Frame(w, relief=tkinter.tix.RAISED, bd=1)
    tree = tkinter.tix.Tree(top, options='separator "/"')
    tree.pack(expand=1, fill=tkinter.tix.BOTH, padx=10, pady=10, side=tkinter.tix.LEFT)
    tree['opencmd'] = lambda dir=None, w=tree: opendir(w, dir)

    # The / directory is added in the "open" mode. The user can open it
    # and then browse its subdirectories ...
    adddir(tree, "/")

    box = tkinter.tix.ButtonBox(w, orientation=tkinter.tix.HORIZONTAL)
    box.add('ok', text='Ok', underline=0, command=w.destroy, width=6)
    box.add('cancel', text='Cancel', underline=0, command=w.destroy, width=6)
    box.pack(side=tkinter.tix.BOTTOM, fill=tkinter.tix.X)
    top.pack(side=tkinter.tix.TOP, fill=tkinter.tix.BOTH, expand=1)

def adddir(tree, dir):
    if dir == '/':
        text = '/'
    else:
        text = os.path.basename(dir)
    tree.hlist.add(dir, itemtype=tkinter.tix.IMAGETEXT, text=text,
                   image=tree.tk.call('tix', 'getimage', 'folder'))
    try:
        os.listdir(dir)
        tree.setmode(dir, 'open')
    except os.error:
        # No read permission ?
        pass

# This function is called whenever the user presses the (+) indicator or
# double clicks on a directory whose mode is "open". It loads the files
# inside that directory into the Tree widget.
#
# Note we didn't specify the closecmd option for the Tree widget, so it
# performs the default action when the user presses the (-) indicator or
# double clicks on a directory whose mode is "close": hide all of its child
# entries
def opendir(tree, dir):
    entries = tree.hlist.info_children(dir)
    if entries:
        # We have already loaded this directory. Let's just
        # show all the child entries
        #
        # Note: since we load the directory only once, it will not be
        #       refreshed if the you add or remove files from this
        #       directory.
        #
        for entry in entries:
            tree.hlist.show_entry(entry)
    files = os.listdir(dir)
    for file in files:
        if os.path.isdir(dir + '/' + file):
            adddir(tree, dir + '/' + file)
        else:
            tree.hlist.add(dir + '/' + file, itemtype=tkinter.tix.IMAGETEXT, text=file,
                           image=tree.tk.call('tix', 'getimage', 'file'))

if __name__ == '__main__':
    root = tkinter.tix.Tk()
    RunSample(root)
    root.mainloop()
