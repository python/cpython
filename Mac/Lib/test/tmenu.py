# Create hierarchical menus for some volumes.

import os
from Menu import *
import macfs
import sys

def main():
	global oldbar
	my_volumes = []
	while 1:
		fss, ok = macfs.GetDirectory()
		if not ok:
			break
		my_volumes.append(fss.as_pathname())
	if not my_volumes:
		return
	oldbar = GetMenuBar()
	ClearMenuBar()
	makevolmenus(my_volumes)
	DrawMenuBar()

def reset():
	oldbar.SetMenuBar()
	DrawMenuBar()

id = 1
def nextid():
	global id
	nid = id
	id = id+1
	return nid

def makevolmenus(volumes):
	for vol in volumes:
		makevolmenu(vol)

def makevolmenu(vol):
	menu = NewMenu(nextid(), vol)
	adddirectory(menu, vol)
	menu.InsertMenu(0)

def adddirectory(menu, dir, maxdepth = 1):
	print "adddirectory:", `dir`, maxdepth
	files = os.listdir(dir)
	item = 0
	for file in files:
		item = item+1
		menu.AppendMenu('x')		# add a dummy string
		menu.SetMenuItemText(item, file)	# set the actual text
		fullname = os.path.join(dir, file)
		if os.path.isdir(fullname):
			menu.SetMenuItemText(item, ':' + file + ':')	# append colons
			if maxdepth > 0:
				id = nextid()
				submenu = NewMenu(id, fullname)
				adddirectory(submenu, fullname, maxdepth-1)
				submenu.InsertMenu(-1)
				# If the 'Cmd' is 0x1B, then the 'Mark' is the submenu id
				menu.SetItemMark(item, id)
				menu.SetItemCmd(item, 0x1B)
	if not files:
		menu.AppendMenu(':')	# dummy item to make it selectable
	return menu

if __name__ == '__main__':
	main()
	sys.exit(1)   # To allow the user to interact...
