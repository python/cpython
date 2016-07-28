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

# This file demonstrates how to use the TixTree widget to display
# dynamic hierachical data (the files in the Unix file system)
#

import Tix, os

def RunSample(w):
    top = Tix.Frame(w, relief=Tix.RAISED, bd=1)
    tree = Tix.Tree(top, options='separator "/"')
    tree.pack(expand=1, fill=Tix.BOTH, padx=10, pady=10, side=Tix.LEFT)
    tree['opencmd'] = lambda dir=None, w=tree: opendir(w, dir)

    # The / directory is added in the "open" mode. The user can open it
    # and then browse its subdirectories ...
    adddir(tree, "/")

    box = Tix.ButtonBox(w, orientation=Tix.HORIZONTAL)
    box.add('ok', text='Ok', underline=0, command=w.destroy, width=6)
    box.add('cancel', text='Cancel', underline=0, command=w.destroy, width=6)
    box.pack(side=Tix.BOTTOM, fill=Tix.X)
    top.pack(side=Tix.TOP, fill=Tix.BOTH, expand=1)

def adddir(tree, dir):
    if dir == '/':
        text = '/'
    else:
        text = os.path.basename(dir)
    tree.hlist.add(dir, itemtype=Tix.IMAGETEXT, text=text,
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
            tree.hlist.add(dir + '/' + file, itemtype=Tix.IMAGETEXT, text=file,
                           image=tree.tk.call('tix', 'getimage', 'file'))

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
    root.mainloop()
