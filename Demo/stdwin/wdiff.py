#! /usr/local/bin/python

# A window-oriented recursive diff utility.
# NB: This uses undocumented window classing modules.

# TO DO:
#	- faster update after moving/copying one file
#	- diff flags (-b, etc.) should be global or maintained per window
#	- use a few fixed windows instead of creating new ones all the time
#	- ways to specify patterns to skip
#	  (best by pointing at a file and clicking a special menu entry!)
#	- add rcsdiff menu commands
#	- add a way to view status of selected files without opening them
#	- add a way to diff two files with different names
#	- add a way to rename files
#	- keep backups of overwritten/deleted files
#	- a way to mark specified files as uninteresting for dircmp

import sys
import os
import rand
import commands
import dircache
import statcache
import cmp
import cmpcache
import stdwin
import gwin
import textwin
import filewin
import tablewin
import anywin

mkarg = commands.mkarg
mk2arg = commands.mk2arg

# List of names to ignore in dircmp()
#
skiplist = ['RCS', '.Amake', 'tags', '.', '..']

# Function to determine whether a name should be ignored in dircmp().
#
def skipthis(file):
	return file[-1:] == '~' or file in skiplist


def anydiff(a, b, flags): # Display differences between any two objects
	print 'diff', flags, a, b
	if os.path.isdir(a) and os.path.isdir(b):
		w = dirdiff(a, b, flags)
	else:
		w = filediff(a, b, flags)
	addstatmenu(w, [a, b])
	w.original_close = w.close
	w.close = close_dirwin
	return w

def close_dirwin(w):
	close_subwindows(w, (), 0)
	w.original_close(w)

def filediff(a, b, flags): # Display differences between two text files
	diffcmd = 'diff'
	if flags: diffcmd = diffcmd + mkarg(flags)
	diffcmd = diffcmd + mkarg(a) + mkarg(b)
	difftext = commands.getoutput(diffcmd)
	return textwin.open_readonly(mktitle(a, b), difftext)

def dirdiff(a, b, flags): # Display differences between two directories
	data = diffdata(a, b, flags)
	w = tablewin.open(mktitle(a, b), data)
	w.flags = flags
	w.a = a
	w.b = b
	addviewmenu(w)
	addactionmenu(w)
	return w

def diffdata(a, b, flags): # Compute directory differences.
	#
	a_only = [('A only:', header_action), ('', header_action)]
	b_only = [('B only:', header_action), ('', header_action)]
	ab_diff = [('A <> B:', header_action), ('', header_action)]
	ab_same = [('A == B:', header_action), ('', header_action)]
	data = [a_only, b_only, ab_diff, ab_same]
	#
	a_list = dircache.listdir(a)[:]
	b_list = dircache.listdir(b)[:]
	dircache.annotate(a, a_list)
	dircache.annotate(b, b_list)
	a_list.sort()
	b_list.sort()
	#
	for x in a_list:
		if x in ['./', '../']:
			pass
		elif x not in b_list:
			a_only.append(x, a_only_action)
		else:
			ax = os.path.join(a, x)
			bx = os.path.join(b, x)
			if os.path.isdir(ax) and os.path.isdir(bx):
				if flags == '-r':
					same = dircmp(ax, bx)
				else:
					same = 0
			else:
				try:
					same = cmp.cmp(ax, bx)
				except (RuntimeError, os.error):
					same = 0
			if same:
				ab_same.append(x, ab_same_action)
			else:
				ab_diff.append(x, ab_diff_action)
	#
	for x in b_list:
		if x in ['./', '../']:
			pass
		elif x not in a_list:
			b_only.append(x, b_only_action)
	#
	return data

# Re-read the directory.
# Attempt to find the selected item back.

def update(w):
	setbusy(w)
	icol, irow = w.selection
	if 0 <= icol < len(w.data) and 2 <= irow < len(w.data[icol]):
		selname = w.data[icol][irow][0]
	else:
		selname = ''
	statcache.forget_dir(w.a)
	statcache.forget_dir(w.b)
	tablewin.select(w, (-1, -1))
	tablewin.update(w, diffdata(w.a, w.b, w.flags))
	if selname:
		for icol in range(len(w.data)):
			for irow in range(2, len(w.data[icol])):
				if w.data[icol][irow][0] == selname:
					tablewin.select(w, (icol, irow))
					break

# Action functions for table items in directory diff windows

def header_action(w, string, (icol, irow), (pos, clicks, button, mask)):
	tablewin.select(w, (-1, -1))

def a_only_action(w, string, (icol, irow), (pos, clicks, button, mask)):
	tablewin.select(w, (icol, irow))
	if clicks == 2:
		w2 = anyopen(os.path.join(w.a, string))
		if w2:
			w2.parent = w

def b_only_action(w, string, (icol, irow), (pos, clicks, button, mask)):
	tablewin.select(w, (icol, irow))
	if clicks == 2:
		w2 = anyopen(os.path.join(w.b, string))
		if w2:
			w2.parent = w

def ab_diff_action(w, string, (icol, irow), (pos, clicks, button, mask)):
	tablewin.select(w, (icol, irow))
	if clicks == 2:
		w2 = anydiff(os.path.join(w.a, string), os.path.join(w.b, string),'')
		w2.parent = w

def ab_same_action(w, string, sel, detail):
	ax = os.path.join(w.a, string)
	if os.path.isdir(ax):
		ab_diff_action(w, string, sel, detail)
	else:
		a_only_action(w, string, sel, detail)

def anyopen(name): # Open any kind of document, ignore errors
	try:
		w = anywin.open(name)
	except (RuntimeError, os.error):
		stdwin.message('Can\'t open ' + name)
		return 0
	addstatmenu(w, [name])
	return w

def dircmp(a, b): # Compare whether two directories are the same
	# To make this as fast as possible, it uses the statcache
	print '  dircmp', a, b
	a_list = dircache.listdir(a)
	b_list = dircache.listdir(b)
	for x in a_list:
		if skipthis(x):
			pass
		elif x not in b_list:
			return 0
		else:
			ax = os.path.join(a, x)
			bx = os.path.join(b, x)
			if statcache.isdir(ax) and statcache.isdir(bx):
				if not dircmp(ax, bx): return 0
			else:
				try:
					if not cmpcache.cmp(ax, bx): return 0
				except (RuntimeError, os.error):
					return 0
	for x in b_list:
		if skipthis(x):
			pass
		elif x not in a_list:
			return 0
	return 1


# View menu (for dir diff windows only)

def addviewmenu(w):
	w.viewmenu = m = w.menucreate('View')
	m.action = []
	add(m, 'diff -r A B', diffr_ab)
	add(m, 'diff A B', diff_ab)
	add(m, 'diff -b A B', diffb_ab)
	add(m, 'diff -c A B', diffc_ab)
	add(m, 'gdiff A B', gdiff_ab)
	add(m, ('Open A   ', 'A'), open_a)
	add(m, ('Open B   ', 'B'), open_b)
	add(m, 'Rescan', rescan)
	add(m, 'Rescan -r', rescan_r)

# Action menu (for dir diff windows only)

def addactionmenu(w):
	w.actionmenu = m = w.menucreate('Action')
	m.action = []
	add(m, 'cp A B', cp_ab)
	add(m, 'rm B', rm_b)
	add(m, '', nop)
	add(m, 'cp B A', cp_ba)
	add(m, 'rm A', rm_a)

# Main menu (global):

def mainmenu():
	m = stdwin.menucreate('Wdiff')
	m.action = []
	add(m, ('Quit wdiff', 'Q'), quit_wdiff)
	add(m, 'Close subwindows', close_subwindows)
	return m

def add(m, text, action):
	m.additem(text)
	m.action.append(action)

def quit_wdiff(w, m, item):
	if askyesno('Really quit wdiff altogether?', 1):
		sys.exit(0)

def close_subwindows(w, m, item):
	while 1:
		for w2 in gwin.windows:
			if w2.parent == w:
				close_subwindows(w2, m, item)
				w2.close(w2)
				break # inner loop, continue outer loop
		else:
			break # outer loop

def diffr_ab(w, m, item):
	dodiff(w, '-r')

def diff_ab(w, m, item):
	dodiff(w, '')

def diffb_ab(w, m, item):
	dodiff(w, '-b')

def diffc_ab(w, m, item):
	dodiff(w, '-c')

def gdiff_ab(w, m, item): # Call SGI's gdiff utility
	x = getselection(w)
	if x:
		a, b = os.path.join(w.a, x), os.path.join(w.b, x)
		if os.path.isdir(a) or os.path.isdir(b):
			stdwin.fleep() # This is for files only
		else:
			diffcmd = 'gdiff'
			diffcmd = diffcmd + mkarg(a) + mkarg(b) + ' &'
			print diffcmd
			sts = os.system(diffcmd)
			if sts: print 'Exit status', sts

def dodiff(w, flags):
	x = getselection(w)
	if x:
		w2 = anydiff(os.path.join(w.a, x), os.path.join(w.b, x), flags)
		w2.parent = w

def open_a(w, m, item):
	x = getselection(w)
	if x:
		w2 = anyopen(os.path.join(w.a, x))
		if w2:
			w2.parent = w

def open_b(w, m, item):
	x = getselection(w)
	if x:
		w2 = anyopen(os.path.join(w.b, x))
		if w2:
			w2.parent = w

def rescan(w, m, item):
	w.flags = ''
	update(w)

def rescan_r(w, m, item):
	w.flags = '-r'
	update(w)

def rm_a(w, m, item):
	x = getselection(w)
	if x:
		if x[-1:] == '/': x = x[:-1]
		x = os.path.join(w.a, x)
		if os.path.isdir(x):
			if askyesno('Recursively remove A directory ' + x, 1):
				runcmd('rm -rf' + mkarg(x))
		else:
			runcmd('rm -f' + mkarg(x))
		update(w)

def rm_b(w, m, item):
	x = getselection(w)
	if x:
		if x[-1:] == '/': x = x[:-1]
		x = os.path.join(w.b, x)
		if os.path.isdir(x):
			if askyesno('Recursively remove B directory ' + x, 1):
				runcmd('rm -rf' + mkarg(x))
		else:
			runcmd('rm -f' + mkarg(x))
		update(w)

def cp_ab(w, m, item):
	x = getselection(w)
	if x:
		if x[-1:] == '/': x = x[:-1]
		ax = os.path.join(w.a, x)
		bx = os.path.join(w.b, x)
		if os.path.isdir(ax):
			if os.path.exists(bx):
				m = 'Can\'t copy directory to existing target'
				stdwin.message(m)
				return
			runcmd('cp -r' + mkarg(ax) + mkarg(w.b))
		else:
			runcmd('cp' + mkarg(ax) + mk2arg(w.b, x))
		update(w)

def cp_ba(w, m, item):
	x = getselection(w)
	if x:
		if x[-1:] == '/': x = x[:-1]
		ax = os.path.join(w.a, x)
		bx = os.path.join(w.b, x)
		if os.path.isdir(bx):
			if os.path.exists(ax):
				m = 'Can\'t copy directory to existing target'
				stdwin.message(m)
				return
			runcmd('cp -r' + mkarg(bx) + mkarg(w.a))
		else:
			runcmd('cp' + mk2arg(w.b, x) + mkarg(ax))
		update(w)

def nop(args):
	pass

def getselection(w):
	icol, irow = w.selection
	if 0 <= icol < len(w.data):
		if 0 <= irow < len(w.data[icol]):
			return w.data[icol][irow][0]
	stdwin.message('no selection')
	return ''

def runcmd(cmd):
	print cmd
	sts, output = commands.getstatusoutput(cmd)
	if sts or output:
		if not output:
			output = 'Exit status ' + `sts`
		stdwin.message(output)


# Status menu (for all kinds of windows)

def addstatmenu(w, files):
	w.statmenu = m = w.menucreate('Stat')
	m.files = files
	m.action = []
	for file in files:
		m.additem(commands.getstatus(file))
		m.action.append(stataction)

def stataction(w, m, item): # Menu item action for stat menu
	file = m.files[item]
	try:
		m.setitem(item, commands.getstatus(file))
	except os.error:
		stdwin.message('Can\'t get status for ' + file)


# Compute a suitable window title from two paths

def mktitle(a, b):
	if a == b: return a
	i = 1
	while a[-i:] == b[-i:]: i = i+1
	i = i-1
	if not i:
		return a + '  ' + b
	else:
		return '{' + a[:-i] + ',' + b[:-i] + '}' + a[-i:]


# Ask a confirmation question

def askyesno(prompt, default):
	try:
		return stdwin.askync(prompt, default)
	except KeyboardInterrupt:
		return 0


# Display a message "busy" in a window, and mark it for updating

def setbusy(w):
	left, top = w.getorigin()
	width, height = w.getwinsize()
	right, bottom = left + width, top + height
	d = w.begindrawing()
	d.erase((0, 0), (10000, 10000))
	text = 'Busy...'
	textwidth = d.textwidth(text)
	textheight = d.lineheight()
	h, v = left + (width-textwidth)/2, top + (height-textheight)/2
	d.text((h, v), text)
	del d
	w.change((0, 0), (10000, 10000))


# Main function

def main():
	print 'wdiff: warning: this program does NOT make backups'
	argv = sys.argv
	flags = ''
	if len(argv) >= 2 and argv[1][:1] == '-':
		flags = argv[1]
		del argv[1]
	m = mainmenu() # Create menu earlier than windows
	if len(argv) == 2: # 1 argument
		w = anyopen(argv[1])
		if not w: return
	elif len(argv) == 3: # 2 arguments
		w = anydiff(argv[1], argv[2], flags)
		w.parent = ()
	else:
		sys.stdout = sys.stderr
		print 'usage:', argv[0], '[diff-flags] dir-1 [dir-2]'
		sys.exit(2)
	del w # It's preserved in gwin.windows
	while 1:
		try:
			gwin.mainloop()
			break
		except KeyboardInterrupt:
			pass	# Just continue...

# Start the main function (this is a script)
main()
