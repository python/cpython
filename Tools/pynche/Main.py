#! /usr/bin/env python

"""Pynche: The PYthon Natural Color and Hue Editor.

Pynche is based largely on a similar color editor I wrote years ago for the
Sunview window system.  That editor was called ICE: the Interactive Color
Editor.  I'd always wanted to port the editor to X but didn't feel like
hacking X and C code to do it.  Fast forward many years, to where Python +
Tkinter provides such a nice programming environment, with enough power, that
I finally buckled down and implemented it.  I changed the name because these
days, too many other systems have the acronym `ICE'.

This program currently requires Python 1.5 with Tkinter.  It has only been
tested on Solaris 2.6.  Feedback is greatly appreciated.  Send email to
bwarsaw@python.org

Usage: %(PROGRAM)s [-d file] [-h] [initialcolor]

Where:
    --database file
    -d file
        Alternate location of a color database file

    --help
    -h
        print this message

    initialcolor
        initial color, as a color name or #RRGGBB format

"""

__version__ = '1.0'

import sys
import getopt
import ColorDB
from PyncheWidget import PyncheWidget
from Switchboard import Switchboard
from StripViewer import StripViewer
from ChipViewer import ChipViewer
from TypeinViewer import TypeinViewer



PROGRAM = sys.argv[0]

# Default locations of rgb.txt or other textual color database
RGB_TXT = [
    # Solaris OpenWindows
    '/usr/openwin/lib/rgb.txt',
    # add more here
    ]



def usage(status, msg=''):
    print __doc__ % globals()
    if msg:
	print msg
    sys.exit(status)



def main():
    try:
	opts, args = getopt.getopt(
            sys.argv[1:],
            'hd:',
            ['database=', 'help'])
    except getopt.error, msg:
	usage(1, msg)

    if len(args) == 0:
        initialcolor = 'grey50'
    elif len(args) == 1:
        initialcolor = args[0]
    else:
	usage(1)

    for opt, arg in opts:
	if opt in ('-h', '--help'):
	    usage(0)
	elif opt in ('-d', '--database'):
	    RGB_TXT.insert(0, arg)

    # create the windows and go
    for f in RGB_TXT:
	try:
	    colordb = ColorDB.get_colordb(f)
            if colordb:
                break
	except IOError:
	    pass
    else:
        usage(1, 'No color database file found, see the -d option.')

    # get triplet for initial color
    try:
	red, green, blue = colordb.find_byname(initialcolor)
    except ColorDB.BadColor:
	# must be a #rrggbb style color
	try:
	    red, green, blue = ColorDB.rrggbb_to_triplet(initialcolor)
	except ColorDB.BadColor:
            print 'Bad initial color, using default: %s' % initialcolor
            initialcolor = 'grey50'
            try:
                red, green, blue = ColorDB.rrggbb_to_triplet(initialcolor)
            except ColorDB.BadColor:
                usage(1, 'Cannot find an initial color to use')

    # create all output widgets
    s = Switchboard(colordb)

    # create the application window decorations
    app = PyncheWidget(__version__, s)
    parent = app.parent()

    s.add_view(StripViewer(s, parent))
    s.add_view(ChipViewer(s, parent))
    s.add_view(TypeinViewer(s, parent))
    s.update_views(red, green, blue)

    try:
	app.start()
    except KeyboardInterrupt:
	pass



if __name__ == '__main__':
    main()
