# Module 'listwin'
# List windows, a subclass of gwin

import gwin
import stdwin

def maxlinewidth(a): # Compute maximum textwidth of lines in a sequence
	max = 0
	for line in a:
		width = stdwin.textwidth(line)
		if width > max: max = width
	return max

def action(w, string, i, detail): # Default item selection method
	pass

def mup(w, detail): # Mouse up method
	(h, v), clicks, button, mask = detail
	i = divmod(v, w.lineheight)[0]
	if 0 <= i < len(w.data):
		w.action(w, w.data[i], i, detail)

def draw(w, ((left, top), (right, bottom))): # Text window draw method
	data = w.data
	d = w.begindrawing()
	lh = w.lineheight
	itop = top/lh
	ibot = (bottom-1)/lh + 1
	if itop < 0: itop = 0
	if ibot > len(data): ibot = len(data)
	for i in range(itop, ibot): d.text((0, i*lh), data[i])

def open(title, data): # Display a list of texts in a window
	lineheight = stdwin.lineheight()
	h, v = maxlinewidth(data), len(data)*lineheight
	h0, v0 = h + stdwin.textwidth(' '), v + lineheight
	if h0 > stdwin.textwidth(' ')*80: h0 = 0
	if v0 > stdwin.lineheight()*24: v0 = 0
	stdwin.setdefwinsize(h0, v0)
	w = gwin.open(title)
	w.setdocsize(h, v)
	w.lineheight = lineheight
	w.data = data
	w.draw = draw
	w.action = action
	w.mup = mup
	return w
