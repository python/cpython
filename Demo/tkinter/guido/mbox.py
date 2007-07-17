#! /usr/bin/env python

# Scan MH folder, display results in window

import os
import sys
import re
import getopt
import string
import mhlib

from Tkinter import *

from dialog import dialog

mailbox = os.environ['HOME'] + '/Mail'

def main():
    global root, tk, top, mid, bot
    global folderbox, foldermenu, scanbox, scanmenu, viewer
    global folder, seq
    global mh, mhf

    # Parse command line options

    folder = 'inbox'
    seq = 'all'
    try:
        opts, args = getopt.getopt(sys.argv[1:], '')
    except getopt.error as msg:
        print(msg)
        sys.exit(2)
    for arg in args:
        if arg[:1] == '+':
            folder = arg[1:]
        else:
            seq = arg

    # Initialize MH

    mh = mhlib.MH()
    mhf = mh.openfolder(folder)

    # Build widget hierarchy

    root = Tk()
    tk = root.tk

    top = Frame(root)
    top.pack({'expand': 1, 'fill': 'both'})

    # Build right part: folder list

    right = Frame(top)
    right.pack({'fill': 'y', 'side': 'right'})

    folderbar = Scrollbar(right, {'relief': 'sunken', 'bd': 2})
    folderbar.pack({'fill': 'y', 'side': 'right'})

    folderbox = Listbox(right, {'exportselection': 0})
    folderbox.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

    foldermenu = Menu(root)
    foldermenu.add('command',
                   {'label': 'Open Folder',
                    'command': open_folder})
    foldermenu.add('separator')
    foldermenu.add('command',
                   {'label': 'Quit',
                    'command': 'exit'})
    foldermenu.bind('<ButtonRelease-3>', folder_unpost)

    folderbox['yscrollcommand'] = (folderbar, 'set')
    folderbar['command'] = (folderbox, 'yview')
    folderbox.bind('<Double-1>', open_folder, 1)
    folderbox.bind('<3>', folder_post)

    # Build left part: scan list

    left = Frame(top)
    left.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

    scanbar = Scrollbar(left, {'relief': 'sunken', 'bd': 2})
    scanbar.pack({'fill': 'y', 'side': 'right'})

    scanbox = Listbox(left, {'font': 'fixed'})
    scanbox.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

    scanmenu = Menu(root)
    scanmenu.add('command',
                 {'label': 'Open Message',
                  'command': open_message})
    scanmenu.add('command',
                 {'label': 'Remove Message',
                  'command': remove_message})
    scanmenu.add('command',
                 {'label': 'Refile Message',
                  'command': refile_message})
    scanmenu.add('separator')
    scanmenu.add('command',
                 {'label': 'Quit',
                  'command': 'exit'})
    scanmenu.bind('<ButtonRelease-3>', scan_unpost)

    scanbox['yscrollcommand'] = (scanbar, 'set')
    scanbar['command'] = (scanbox, 'yview')
    scanbox.bind('<Double-1>', open_message)
    scanbox.bind('<3>', scan_post)

    # Separator between middle and bottom part

    rule2 = Frame(root, {'bg': 'black'})
    rule2.pack({'fill': 'x'})

    # Build bottom part: current message

    bot = Frame(root)
    bot.pack({'expand': 1, 'fill': 'both'})
    #
    viewer = None

    # Window manager commands

    root.minsize(800, 1) # Make window resizable

    # Fill folderbox with text

    setfolders()

    # Fill scanbox with text

    rescan()

    # Enter mainloop

    root.mainloop()

def folder_post(e):
    x, y = e.x_root, e.y_root
    foldermenu.post(x - 10, y - 10)
    foldermenu.grab_set()

def folder_unpost(e):
    tk.call('update', 'idletasks')
    foldermenu.grab_release()
    foldermenu.unpost()
    foldermenu.invoke('active')

def scan_post(e):
    x, y = e.x_root, e.y_root
    scanmenu.post(x - 10, y - 10)
    scanmenu.grab_set()

def scan_unpost(e):
    tk.call('update', 'idletasks')
    scanmenu.grab_release()
    scanmenu.unpost()
    scanmenu.invoke('active')

scanparser = re.compile('^ *([0-9]+)')

def open_folder(e=None):
    global folder, mhf
    sel = folderbox.curselection()
    if len(sel) != 1:
        if len(sel) > 1:
            msg = "Please open one folder at a time"
        else:
            msg = "Please select a folder to open"
        dialog(root, "Can't Open Folder", msg, "", 0, "OK")
        return
    i = sel[0]
    folder = folderbox.get(i)
    mhf = mh.openfolder(folder)
    rescan()

def open_message(e=None):
    global viewer
    sel = scanbox.curselection()
    if len(sel) != 1:
        if len(sel) > 1:
            msg = "Please open one message at a time"
        else:
            msg = "Please select a message to open"
        dialog(root, "Can't Open Message", msg, "", 0, "OK")
        return
    cursor = scanbox['cursor']
    scanbox['cursor'] = 'watch'
    tk.call('update', 'idletasks')
    i = sel[0]
    line = scanbox.get(i)
    if scanparser.match(line) >= 0:
        num = string.atoi(scanparser.group(1))
        m = mhf.openmessage(num)
        if viewer: viewer.destroy()
        from MimeViewer import MimeViewer
        viewer = MimeViewer(bot, '+%s/%d' % (folder, num), m)
        viewer.pack()
        viewer.show()
    scanbox['cursor'] = cursor

def interestingheader(header):
    return header != 'received'

def remove_message(e=None):
    itop = scanbox.nearest(0)
    sel = scanbox.curselection()
    if not sel:
        dialog(root, "No Message To Remove",
               "Please select a message to remove", "", 0, "OK")
        return
    todo = []
    for i in sel:
        line = scanbox.get(i)
        if scanparser.match(line) >= 0:
            todo.append(string.atoi(scanparser.group(1)))
    mhf.removemessages(todo)
    rescan()
    fixfocus(min(todo), itop)

lastrefile = ''
tofolder = None
def refile_message(e=None):
    global lastrefile, tofolder
    itop = scanbox.nearest(0)
    sel = scanbox.curselection()
    if not sel:
        dialog(root, "No Message To Refile",
               "Please select a message to refile", "", 0, "OK")
        return
    foldersel = folderbox.curselection()
    if len(foldersel) != 1:
        if not foldersel:
            msg = "Please select a folder to refile to"
        else:
            msg = "Please select exactly one folder to refile to"
        dialog(root, "No Folder To Refile", msg, "", 0, "OK")
        return
    refileto = folderbox.get(foldersel[0])
    todo = []
    for i in sel:
        line = scanbox.get(i)
        if scanparser.match(line) >= 0:
            todo.append(string.atoi(scanparser.group(1)))
    if lastrefile != refileto or not tofolder:
        lastrefile = refileto
        tofolder = None
        tofolder = mh.openfolder(lastrefile)
    mhf.refilemessages(todo, tofolder)
    rescan()
    fixfocus(min(todo), itop)

def fixfocus(near, itop):
    n = scanbox.size()
    for i in range(n):
        line = scanbox.get(repr(i))
        if scanparser.match(line) >= 0:
            num = string.atoi(scanparser.group(1))
            if num >= near:
                break
    else:
        i = 'end'
    scanbox.select_from(i)
    scanbox.yview(itop)

def setfolders():
    folderbox.delete(0, 'end')
    for fn in mh.listallfolders():
        folderbox.insert('end', fn)

def rescan():
    global viewer
    if viewer:
        viewer.destroy()
        viewer = None
    scanbox.delete(0, 'end')
    for line in scanfolder(folder, seq):
        scanbox.insert('end', line)

def scanfolder(folder = 'inbox', sequence = 'all'):
    return [line[:-1] for line in os.popen('scan +%s %s' % (folder, sequence), 'r').readlines()]

main()
