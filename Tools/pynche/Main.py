#! /usr/bin/env python

"""Pynche: The PYthon Natural Color and Hue Editor.

Pynche is based largely on a similar color editor I wrote years ago for the
Sunview window system.  That editor was called ICE: the Interactive Color
Editor.  I'd always wanted to port the editor to X but didn't feel like
hacking X and C code to do it.  Fast forward many years, to where Python +
Tkinter + Pmw provides such a nice programming environment, with enough
power, that I finally buckled down and implemented it.  I changed the name
because these days, too many other systems have the acronym `ICE'.

This program currently requires Python 1.5 with Tkinter.  It also requires at
least Pmw 0.6.1.  It has only been tested on Solaris 2.6.  Feedback is greatly 
appreciated.  Send email to bwarsaw@python.org

Usage: %(PROGRAM)s [-c color] [-h]

Where:
    --color color
    -c color
        initial color, as an X color name or #RRGGBB format

    --help
    -h
        print this message

"""

__version__ = '1.0'

import sys
import getopt
import Pmw
import ColorDB
from Tkinter import *
from PyncheWidget import PyncheWidget



PROGRAM = sys.argv[0]

# Milliseconds between interrupt checks
KEEPALIVE_TIMER = 500

RGBCOLOR = 1
HSICOLOR = 2
NAMEDCOLOR = 3

# Default locations of rgb.txt or other textual color database
RGB_TXT = [
    # Solaris OpenWindows
    '/usr/openwin/lib/rgb.txt',
    # add more here
    ]



def usage(status, msg=''):
    if msg:
	print msg
    print __doc__ % globals()
    sys.exit(status)



app = None

def keepalive():
    # Exercise the Python interpreter regularly so keybard interrupts get
    # through.
    app.tk.createtimerhandler(KEEPALIVE_TIMER, keepalive)


def main():
    global app

    initialcolor = 'grey50'
    try:
	opts, args = getopt.getopt(sys.argv[1:],
				   'hc:',
				   ['color=', 'help'])
    except getopt.error, msg:
	usage(1, msg)

    if args:
	usage(1)

    for opt, arg in opts:
	if opt in ('-h', '--help'):
	    usage(0)
	elif opt in ('-c', '--color'):
	    initialcolor = arg

    # create the windows and go
    for f in RGB_TXT:
	try:
	    colordb = ColorDB.get_colordb(f)
	    break
	except IOError:
	    pass
    else:
	raise IOError('No color database file found')

    app = Pmw.initialise(fontScheme='pmw1')
    app.title('Pynche %s' % __version__)
    app.tk.createtimerhandler(KEEPALIVE_TIMER, keepalive)

    # get triplet for initial color
    try:
	red, green, blue = colordb.find_byname(initialcolor)
    except ColorDB.BadColor:
	# must be a #rrggbb style color
	try:
	    red, green, blue = ColorDB.rrggbb_to_triplet(initialcolor)
	except ColorDB.BadColor:
	    usage(1, 'Bad initial color: %s' % initialcolor)

    p = PyncheWidget(colordb, app, color=(red, green, blue))
    try:
	keepalive()
	app.mainloop()
    except KeyboardInterrupt:
	pass



if __name__ == '__main__':
    main()
