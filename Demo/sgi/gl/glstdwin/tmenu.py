# Test menus

import stdwingl

import stdwin
from stdwinevents import *

def main():
	w = stdwin.open('TestMenus')
	#
	items1 = 'Aap', 'Noot', 'Mies'
	m1 = w.menucreate('Menu-1')
	for item in items1:
		m1.additem(item, item[0])
	#
	items2 = 'Wim', 'Zus', 'Jet', 'Teun', 'Vuur'
	m2 = w.menucreate('Menu-2')
	for item in items2:
		m2.additem(item, `len(item)`)
	#
	m1.enable(1, 0)
	m2.check(1, 1)
	#
	while 1:
		type, window, detail = stdwin.getevent()
		if type == WE_CLOSE:
			break
		elif type == WE_DRAW:
			d = w.begindrawing()
			d.box(((50,50), (100,100)))
			del d
		elif type == WE_MENU:
			mp, i = detail
			if mp == m1:
				print 'Choice:', items1[i]
			elif mp == m2:
				print 'Choice:', items2[i]
			else:
				print 'Not one of my menus!'
		elif type == WE_CHAR:
			print 'Character', `detail`
	#

main()
