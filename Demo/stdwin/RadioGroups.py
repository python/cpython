#! /usr/local/python

# radiogroups.py
#
# Demonstrate multiple groups of radio buttons

import stdwin
from Buttons import *
from WindowParent import WindowParent, MainLoop
from HVSplit import HSplit, VSplit

def main():
	#
	# Create the widget hierarchy, top-down
	#
	# 1. Create the window
	#
	window = WindowParent().create('Radio Groups', (0, 0))
	#
	# 2. Create a horizontal split to contain the groups
	#
	topsplit = HSplit().create(window)
	#
	# 3. Create vertical splits, one for each group
	#
	group1 = VSplit().create(topsplit)
	group2 = VSplit().create(topsplit)
	group3 = VSplit().create(topsplit)
	#
	# 4. Create individual radio buttons, each in their own split
	#
	b11 = RadioButton().definetext(group1, 'Group 1 button 1')
	b12 = RadioButton().definetext(group1, 'Group 1 button 2')
	b13 = RadioButton().definetext(group1, 'Group 1 button 3')
	#
	b21 = RadioButton().definetext(group2, 'Group 2 button 1')
	b22 = RadioButton().definetext(group2, 'Group 2 button 2')
	b23 = RadioButton().definetext(group2, 'Group 2 button 3')
	#
	b31 = RadioButton().definetext(group3, 'Group 3 button 1')
	b32 = RadioButton().definetext(group3, 'Group 3 button 2')
	b33 = RadioButton().definetext(group3, 'Group 3 button 3')
	#
	# 5. Define the grouping for the radio buttons.
	#    Note: this doesn't have to be the same as the
	#    grouping is splits (although it usually is).
	#    Also set the 'hook' procedure for each button
	#
	list1 = [b11, b12, b13]
	list2 = [b21, b22, b23]
	list3 = [b31, b32, b33]
	#
	for b in list1:
		b.group = list1
		b.on_hook = myhook
	for b in list2:
		b.group = list2
		b.on_hook = myhook
	for b in list3:
		b.group = list3
		b.on_hook = myhook
	#
	# 6. Select a default button in each group
	#
	b11.select(1)
	b22.select(1)
	b33.select(1)
	#
	# 6. Realize the window
	#
	window.realize()
	#
	# 7. Dispatch events until the window is closed
	#
	MainLoop()
	#
	# 8. Report final selections
	#
	print 'You selected the following choices:'
	if b11.selected: print '1.1'
	if b12.selected: print '1.2'
	if b13.selected: print '1.3'
	if b21.selected: print '2.1'
	if b22.selected: print '2.2'
	if b23.selected: print '2.3'
	if b31.selected: print '3.1'
	if b32.selected: print '3.2'
	if b33.selected: print '3.3'


# My 'hook' procedure
# This is placed as 'hook' attribute on each button.
# The example just prints the title of the selected button.
#
def myhook(self):
	print 'Selected:', self.text

main()
