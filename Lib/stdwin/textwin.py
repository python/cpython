# Module 'textwin'

# Text windows, a subclass of gwin

import stdwin
import stdwinsupport
import gwin

S = stdwinsupport			# Shorthand


def fixsize(w):
	docwidth, docheight = w.text.getrect()[1]
	winheight = w.getwinsize()[1]
	if winheight > docheight: docheight = winheight
	w.setdocsize(0, docheight)
	fixeditmenu(w)

def cut(w, m, id):
	s = w.text.getfocustext()
	if s:
		stdwin.setcutbuffer(0, s)
		w.text.replace('')
		fixsize(w)

def copy(w, m, id):
	s = w.text.getfocustext()
	if s:
		stdwin.setcutbuffer(0, s)
		fixeditmenu(w)

def paste(w, m, id):
	w.text.replace(stdwin.getcutbuffer(0))
	fixsize(w)

def addeditmenu(w):
	m = w.editmenu = w.menucreate('Edit')
	m.action = []
	m.additem('Cut', 'X')
	m.action.append(cut)
	m.additem('Copy', 'C')
	m.action.append(copy)
	m.additem('Paste', 'V')
	m.action.append(paste)

def fixeditmenu(w):
	m = w.editmenu
	f = w.text.getfocus()
	can_copy = (f[0] < f[1])
	m.enable(1, can_copy)
	if not w.readonly:
		m.enable(0, can_copy)
		m.enable(2, (stdwin.getcutbuffer(0) <> ''))

def draw(w, area):			# Draw method
	w.text.draw(area)

def size(w, newsize):			# Size method
	w.text.move((0, 0), newsize)
	fixsize(w)

def close(w):				# Close method
	del w.text  # Break circular ref
	gwin.close(w)

def char(w, c):				# Char method
	w.text.replace(c)
	fixsize(w)

def backspace(w):			# Backspace method
	void = w.text.event(S.we_command, w, S.wc_backspace)
	fixsize(w)

def arrow(w, detail):			# Arrow method
	w.text.arrow(detail)
	fixeditmenu(w)

def mdown(w, detail):			# Mouse down method
	void = w.text.event(S.we_mouse_down, w, detail)
	fixeditmenu(w)

def mmove(w, detail):			# Mouse move method
	void = w.text.event(S.we_mouse_move, w, detail)

def mup(w, detail):			# Mouse up method
	void = w.text.event(S.we_mouse_up, w, detail)
	fixeditmenu(w)

def activate(w):			# Activate method
	fixeditmenu(w)

def open(title, str):			# Display a string in a window
	w = gwin.open(title)
	w.readonly = 0
	w.text = w.textcreate((0, 0), w.getwinsize())
	w.text.replace(str)
	w.text.setfocus(0, 0)
	addeditmenu(w)
	fixsize(w)
	w.draw = draw
	w.size = size
	w.close = close
	w.mdown = mdown
	w.mmove = mmove
	w.mup = mup
	w.char = char
	w.backspace = backspace
	w.arrow = arrow
	w.activate = activate
	return w

def open_readonly(title, str):		# Same with char input disabled
	w = open(title, str)
	w.readonly = 1
	w.char = w.backspace = gwin.nop
	# Disable Cut and Paste menu item; leave Copy alone
	w.editmenu.enable(0, 0)
	w.editmenu.enable(2, 0)
	return w
