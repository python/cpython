#! /usr/bin/env python

# A Python program implementing rmt, an application for remotely
# controlling other Tk applications.
# Cf. Ousterhout, Tcl and the Tk Toolkit, Figs. 27.5-8, pp. 273-276.

# Note that because of forward references in the original, we
# sometimes delay bindings until after the corresponding procedure is
# defined.  We also introduce names for some unnamed code blocks in
# the original because of restrictions on lambda forms in Python.

# XXX This should be written in a more Python-like style!!!

from Tkinter import *
import sys

# 1. Create basic application structure: menu bar on top of
# text widget, scrollbar on right.

root = Tk()
tk = root.tk
mBar = Frame(root, relief=RAISED, borderwidth=2)
mBar.pack(fill=X)

f = Frame(root)
f.pack(expand=1, fill=BOTH)
s = Scrollbar(f, relief=FLAT)
s.pack(side=RIGHT, fill=Y)
t = Text(f, relief=RAISED, borderwidth=2, yscrollcommand=s.set, setgrid=1)
t.pack(side=LEFT, fill=BOTH, expand=1)
t.tag_config('bold', font='-Adobe-Courier-Bold-R-Normal-*-120-*')
s['command'] = t.yview

root.title('Tk Remote Controller')
root.iconname('Tk Remote')

# 2. Create menu button and menus.

file = Menubutton(mBar, text='File', underline=0)
file.pack(side=LEFT)
file_m = Menu(file)
file['menu'] = file_m
file_m_apps = Menu(file_m, tearoff=0)
file_m.add_cascade(label='Select Application', underline=0,
                   menu=file_m_apps)
file_m.add_command(label='Quit', underline=0, command=sys.exit)

# 3. Create bindings for text widget to allow commands to be
# entered and information to be selected.  New characters
# can only be added at the end of the text (can't ever move
# insertion point).

def single1(e):
    x = e.x
    y = e.y
    t.setvar('tk_priv(selectMode)', 'char')
    t.mark_set('anchor', At(x, y))
    # Should focus W
t.bind('<1>', single1)

def double1(e):
    x = e.x
    y = e.y
    t.setvar('tk_priv(selectMode)', 'word')
    t.tk_textSelectTo(At(x, y))
t.bind('<Double-1>', double1)

def triple1(e):
    x = e.x
    y = e.y
    t.setvar('tk_priv(selectMode)', 'line')
    t.tk_textSelectTo(At(x, y))
t.bind('<Triple-1>', triple1)

def returnkey(e):
    t.insert(AtInsert(), '\n')
    invoke()
t.bind('<Return>', returnkey)

def controlv(e):
    t.insert(AtInsert(), t.selection_get())
    t.yview_pickplace(AtInsert())
    if t.index(AtInsert())[-2:] == '.0':
        invoke()
t.bind('<Control-v>', controlv)

# 4. Procedure to backspace over one character, as long as
# the character isn't part of the prompt.

def backspace(e):
    if t.index('promptEnd') != t.index('insert - 1 char'):
        t.delete('insert - 1 char', AtInsert())
        t.yview_pickplace(AtInsert())
t.bind('<BackSpace>', backspace)
t.bind('<Control-h>', backspace)
t.bind('<Delete>', backspace)


# 5. Procedure that's invoked when return is typed:  if
# there's not yet a complete command (e.g. braces are open)
# then do nothing.  Otherwise, execute command (locally or
# remotely), output the result or error message, and issue
# a new prompt.

def invoke():
    cmd = t.get('promptEnd + 1 char', AtInsert())
    if t.getboolean(tk.call('info', 'complete', cmd)): # XXX
        if app == root.winfo_name():
            msg = tk.call('eval', cmd) # XXX
        else:
            msg = t.send(app, cmd)
        if msg:
            t.insert(AtInsert(), msg + '\n')
        prompt()
    t.yview_pickplace(AtInsert())

def prompt():
    t.insert(AtInsert(), app + ': ')
    t.mark_set('promptEnd', 'insert - 1 char')
    t.tag_add('bold', 'insert linestart', 'promptEnd')

# 6. Procedure to select a new application.  Also changes
# the prompt on the current command line to reflect the new
# name.

def newApp(appName):
    global app
    app = appName
    t.delete('promptEnd linestart', 'promptEnd')
    t.insert('promptEnd', appName + ':')
    t.tag_add('bold', 'promptEnd linestart', 'promptEnd')

def fillAppsMenu():
    file_m_apps.add('command')
    file_m_apps.delete(0, 'last')
    names = root.winfo_interps()
    names = map(None, names) # convert tuple to list
    names.sort()
    for name in names:
        try:
            root.send(name, 'winfo name .')
        except TclError:
            # Inoperative window -- ignore it
            pass
        else:
            file_m_apps.add_command(
                label=name,
                command=lambda name=name: newApp(name))

file_m_apps['postcommand'] = fillAppsMenu
mBar.tk_menuBar(file)

# 7. Miscellaneous initialization.

app = root.winfo_name()
prompt()
t.focus()

root.mainloop()
