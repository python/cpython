"""Some tests of various morefindertools functions.
This does not test the functions that are already defined and tested in findertools.
10 10 2000 erik@letterror.com
"""

import morefindertools
import os.path
import MacOS
import random

mft = morefindertools

print '\nmorefindertools version %s\nTests coming upä' %mft.__version__

# miscellaneous
print '\tfilesharing on?',	mft.filesharing()		# is file sharing on, off, starting up?
print '\tOS version', 		mft.OSversion()		# the version of the system software

# set the soundvolume in a simple way
print '\tSystem beep volume'
for i in range(0, 7):
	mft.volumelevel(i)		
	MacOS.SysBeep()

# Finder's windows, file location, file attributes
f = __file__				# get a path name that is innocent to play with and always works
mft.reveal(f)				# reveal this file in a Finder window
mft.select(f)				# select this file

base, file = os.path.split(f)
mft.closewindow(base)	# close the window this file is in	(opened by reveal)
mft.openwindow(base)		# open it again
mft.windowview(base, 1)	# set the view by list

mft.label(f, 2)				# set the label of this file to something orange
print '\tlabel', mft.label(f)	# get the label of this file

# the file location only works in a window with icon view!
print 'Random locations for an icon'
mft.windowview(base, 0)		# set the view by icon
mft.windowsize(base, (600, 600))
for i in range(50):
	mft.location(f, (random.randint(10, 590), random.randint(10, 590)))

mft.windowsize(base, (200, 400))
mft.windowview(base, 1)		# set the view by icon

orgpos = mft.windowposition(base)
print 'Animated window location'
for i in range(10):
	pos = (100+i*10, 100+i*10)
	mft.windowposition(base, pos)
	print '\twindow position', pos
mft.windowposition(base, orgpos)	# park it where it was beforeä

print 'Put a comment in file', f, ':'
print '\t', mft.comment(f)		# print the Finder comment this file has
s = 'This is a comment no one reads!'
mft.comment(f, s)			# set the Finder comment

#
#
#	the following code does not work on MacOS versions older than MacOS 9.
#
#

if 0:
	print 'MacOS9 or better specific functions'
	# processes
	pr = mft.processes()		# return a list of tuples with (active_processname, creatorcode)
	print 'Return a list of current active processes:'
	for p in pr:
		print '\t', p
	
	# get attributes of the first process in the list
	print 'Attributes of the first process in the list:'
	pinfo = mft.processinfo(pr[0][0])
	print '\t', pr[0][0]
	print '\t\tmemory partition', pinfo.partition		# the memory allocated to this process
	print '\t\tmemory used', pinfo.used			# the memory actuall used by this process
	print '\t\tis visible', pinfo.visible			# is the process visible to the user
	print '\t\tis frontmost', pinfo.frontmost		# is the process the front most one?
	print '\t\thas scripting', pinfo.hasscripting		# is the process scriptable?
	print '\t\taccepts high level events', 	pinfo.accepthighlevel	# does the process accept high level appleevents?
print 'Done.'