#! /ufs/guido/bin/sgi/tkpython

# A Python program implementing rmt, an application for remotely
# controlling other Tk applications.
# Cf. Ousterhout, Tcl and the Tk Toolkit, Figs. 27.5-8, pp. 273-276.

# Note that because of forward references in the original, we
# sometimes delay bindings until after the corresponding procedure is
# defined.  We also introduce names for some unnamed code blocks in
# the original because of restrictions on lambda forms in Python.

from Tkinter import *

# 1. Create basic application structure: menu bar on top of
# text widget, scrollbar on right.

root = Tk()
tk = root.tk
mBar = Frame(root, {'relief': 'raised', 'bd': 2,
		    Pack: {'side': 'top', 'fill': 'x'}})
f = Frame(root)
f.pack({'expand': 1, 'fill': 'both'})
s = Scrollbar(f, {'relief': 'flat',
		  Pack: {'side': 'right', 'fill': 'y'}})
t = Text(f, {'relief': 'raised', 'bd': 2, 'yscrollcommand': (s, 'set'),
	     'setgrid': 1,
	     Pack: {'side': 'left', 'fill': 'both', 'expand': 1}})

t.tag_config('bold', {'font': '-Adobe-Courier-Bold-R-Normal-*-120-*'}) 
s['command'] = (t, 'yview')
root.title('Tk Remote Controller')
root.iconname('Tk Remote')

# 2. Create menu button and menus.

file = Menubutton(mBar, {'text': 'File', 'underline': 0,
			 Pack: {'side': 'left'}})
file_m = Menu(file)
file['menu'] = file_m
file_m_apps = Menu(file_m)
file_m.add('cascade', {'label': 'Select Application', 'underline': 0,
		       'menu': file_m_apps})
file_m.add('command', {'label': 'Quit', 'underline': 0, 'command': 'exit'})

# 3. Create bindings for text widget to allow commands to be
# entered and information to be selected.  New characters
# can only be added at the end of the text (can't ever move
# insertion point).

def single1(e):
	x = e.x
	y = e.y
	tk.setvar('tk_priv(selectMode)', 'char')
	t.mark_set('anchor', At(x, y))
	# Should focus W
t.bind('<1>', single1)

def double1(e):
	x = e.x
	y = e.y
	tk.setvar('tk_priv(selectMode)', 'word')
	tk.call('tk_textSelectTo', t, At(x, y))
t.bind('<Double-1>', double1)

def triple1(e):
	x = e.x
	y = e.y
	tk.setvar('tk_priv(selectMode)', 'line')
	tk.call('tk_textSelectTo', t, At(x, y))
t.bind('<Triple-1>', triple1)

def returnkey(e):
	t.insert('insert', '\n')
	invoke()
t.bind('<Return>', returnkey)

def controlv(e):
	t.insert('insert', tk.call('selection', 'get'))
	t.yview_pickplace('insert')
	if t.index('insert')[-2:] == '.0':
		invoke()
t.bind('<Control-v>', controlv)

# 4. Procedure to backspace over one character, as long as
# the character isn't part of the prompt.

def backspace(e):
	if t.index('promptEnd') != t.index('insert - 1 char'):
		t.delete('insert - 1 char', 'insert')
		t.yview_pickplace('insert')
t.bind('<BackSpace>', backspace)
t.bind('<Control-h>', backspace)
t.bind('<Delete>', backspace)


# 5. Procedure that's invoked when return is typed:  if
# there's not yet a complete command (e.g. braces are open)
# then do nothing.  Otherwise, execute command (locally or
# remotely), output the result or error message, and issue
# a new prompt.

def invoke():
	cmd = t.get('promptEnd + 1 char', 'insert')
	if tk.getboolean(tk.call('info', 'complete', cmd)):
		if app == tk.call('winfo', 'name', '.'):
			msg = tk.call('eval', cmd)
		else:
			msg = tk.call('send', app, cmd)
		if msg:
			t.insert('insert', msg + '\n')
		prompt()
	t.yview_pickplace('insert')

def prompt():
	t.insert('insert', app + ': ')
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

newApp_tcl = `id(newApp)`
tk.createcommand(newApp_tcl, newApp)

def fillAppsMenu():
	file_m_apps.add('command')
	file_m_apps.delete(0, 'last')
	names = tk.splitlist(tk.call('winfo', 'interps'))
	names = map(None, names) # convert tuple to list
	names.sort()
	for name in names:
		file_m_apps.add('command', {'label': name,
					    'command': (newApp_tcl, name)})

file_m_apps['postcommand'] = fillAppsMenu
mBar.tk_menuBar(file)

# 7. Miscellaneous initialization.

app = tk.call('winfo', 'name', '.')
prompt()
tk.call('focus', t)

root.mainloop()
