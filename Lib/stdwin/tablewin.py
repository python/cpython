# Module 'tablewin'

# Display a table, with per-item actions:

#	   A1   |   A2   |   A3   |  ....  |   AN
#	   B1   |   B2   |   B3   |  ....  |   BN
#	   C1   |   C2   |   C3   |  ....  |   CN
#	   ..   |   ..   |   ..   |  ....  |   ..
#	   Z1   |   Z2   |   Z3   |  ....  |   ZN

# Not all columns need to have the same length.
# The data structure is a list of columns;
# each column is a list of items.
# Each item is a pair of a string and an action procedure.
# The first item may be a column title.

import stdwin
import gwin

def open(title, data): # Public function to open a table window
	#
	# Set geometry parameters (one day, these may be changeable)
	#
	margin = stdwin.textwidth('  ')
	lineheight = stdwin.lineheight()
	#
	# Geometry calculations
	#
	colstarts = [0]
	totwidth = 0
	maxrows = 0
	for coldata in data:
		# Height calculations
		rows = len(coldata)
		if rows > maxrows: maxrows = rows
		# Width calculations
		width = colwidth(coldata) + margin
		totwidth = totwidth + width
		colstarts.append(totwidth)
	#
	# Calculate document and window height
	#
	docwidth, docheight = totwidth, maxrows*lineheight
	winwidth, winheight = docwidth, docheight
	if winwidth > stdwin.textwidth('n')*100: winwidth = 0
	if winheight > stdwin.lineheight()*30: winheight = 0
	#
	# Create the window
	#
	stdwin.setdefwinsize(winwidth, winheight)
	w = gwin.open(title)
	#
	# Set properties and override methods
	#
	w.data = data
	w.margin = margin
	w.lineheight = lineheight
	w.colstarts = colstarts
	w.totwidth = totwidth
	w.maxrows = maxrows
	w.selection = (-1, -1)
	w.lastselection = (-1, -1)
	w.selshown = 0
	w.setdocsize(docwidth, docheight)
	w.draw = draw
	w.mup = mup
	w.arrow = arrow
	#
	# Return
	#
	return w

def update(w, data): # Change the data
	#
	# Hide selection
	#
	hidesel(w, w.begindrawing())
	#
	# Get old geometry parameters
	#
	margin = w.margin
	lineheight = w.lineheight
	#
	# Geometry calculations
	#
	colstarts = [0]
	totwidth = 0
	maxrows = 0
	for coldata in data:
		# Height calculations
		rows = len(coldata)
		if rows > maxrows: maxrows = rows
		# Width calculations
		width = colwidth(coldata) + margin
		totwidth = totwidth + width
		colstarts.append(totwidth)
	#
	# Calculate document and window height
	#
	docwidth, docheight = totwidth, maxrows*lineheight
	#
	# Set changed properties and change window size
	#
	w.data = data
	w.colstarts = colstarts
	w.totwidth = totwidth
	w.maxrows = maxrows
	w.change((0, 0), (10000, 10000))
	w.setdocsize(docwidth, docheight)
	w.change((0, 0), (docwidth, docheight))
	#
	# Show selection, or forget it if out of range
	#
	showsel(w, w.begindrawing())
	if not w.selshown: w.selection = (-1, -1)

def colwidth(coldata): # Subroutine to calculate column width
	maxwidth = 0
	for string, action in coldata:
		width = stdwin.textwidth(string)
		if width > maxwidth: maxwidth = width
	return maxwidth

def draw(w, ((left, top), (right, bottom))): # Draw method
	ileft = whichcol(w, left)
	iright = whichcol(w, right-1) + 1
	if iright > len(w.data): iright = len(w.data)
	itop = divmod(top, w.lineheight)[0]
	if itop < 0: itop = 0
	ibottom, remainder = divmod(bottom, w.lineheight)
	if remainder: ibottom = ibottom + 1
	d = w.begindrawing()
	if ileft <= w.selection[0] < iright:
		if itop <= w.selection[1] < ibottom:
			hidesel(w, d)
	d.erase((left, top), (right, bottom))
	for i in range(ileft, iright):
		col = w.data[i]
		jbottom = len(col)
		if ibottom < jbottom: jbottom = ibottom
		h = w.colstarts[i]
		v = itop * w.lineheight
		for j in range(itop, jbottom):
			string, action = col[j]
			d.text((h, v), string)
			v = v + w.lineheight
	showsel(w, d)

def mup(w, detail): # Mouse up method
	(h, v), nclicks, button, mask = detail
	icol = whichcol(w, h)
	if 0 <= icol < len(w.data):
		irow = divmod(v, w.lineheight)[0]
		col = w.data[icol]
		if 0 <= irow < len(col):
			string, action = col[irow]
			action(w, string, (icol, irow), detail)

def whichcol(w, h): # Return column number (may be >= len(w.data))
	for icol in range(0, len(w.data)):
		if h < w.colstarts[icol+1]:
			return icol
	return len(w.data)

def arrow(w, type):
	import stdwinsupport
	S = stdwinsupport
	if type = S.wc_left:
		incr = -1, 0
	elif type = S.wc_up:
		incr = 0, -1
	elif type = S.wc_right:
		incr = 1, 0
	elif type = S.wc_down:
		incr = 0, 1
	else:
		return
	icol, irow = w.lastselection
	icol = icol + incr[0]
	if icol < 0: icol = len(w.data)-1
	if icol >= len(w.data): icol = 0
	if 0 <= icol < len(w.data):
		irow = irow + incr[1]
		if irow < 0: irow = len(w.data[icol]) - 1
		if irow >= len(w.data[icol]): irow = 0
	else:
		irow = 0
	if 0 <= icol < len(w.data) and 0 <= irow < len(w.data[icol]):
		w.lastselection = icol, irow
		string, action = w.data[icol][irow]
		detail = (0, 0), 1, 1, 1
		action(w, string, (icol, irow), detail)


# Selection management
# TO DO: allow multiple selected entries

def select(w, selection): # Public function to set the item selection
	d = w.begindrawing()
	hidesel(w, d)
	w.selection = selection
	showsel(w, d)
	if w.selshown: lastselection = selection

def hidesel(w, d): # Hide the selection, if shown
	if w.selshown: invertsel(w, d)

def showsel(w, d): # Show the selection, if hidden
	if not w.selshown: invertsel(w, d)

def invertsel(w, d): # Invert the selection, if valid
	icol, irow = w.selection
	if 0 <= icol < len(w.data) and 0 <= irow < len(w.data[icol]):
		left = w.colstarts[icol]
		right = w.colstarts[icol+1]
		top = irow * w.lineheight
		bottom = (irow+1) * w.lineheight
		d.invert((left, top), (right, bottom))
		w.selshown = (not w.selshown)


# Demonstration

def demo_action(w, string, (icol, irow), detail): # Action function for demo
	select(w, (irow, icol))

def demo(): # Demonstration
	da = demo_action # shorthand
	col0 = [('a1', da), ('bbb1', da), ('c1', da)]
	col1 = [('a2', da), ('bbb2', da)]
	col2 = [('a3', da), ('b3', da), ('c3', da), ('d4', da), ('d5', da)]
	col3 = []
	for i in range(1, 31): col3.append('xxx' + `i`, da)
	data = [col0, col1, col2, col3]
	w = open('tablewin.demo', data)
	gwin.mainloop()
	return w
