"""Pynche -- The PYthon Natural Color and Hue Editor.

Contact: Barry Warsaw
Email:   bwarsaw@python.org
Version: %(__version__)s

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

Usage: %(PROGRAM)s [-d file] [-i file] [-X] [-v] [-h] [initialcolor]

Where:
    --database file
    -d file
        Alternate location of a color database file

    --initfile file
    -i file
        Alternate location of the initialization file.  This file contains a
        persistent database of the current Pynche options and color.  This
        means that Pynche restores its option settings and current color when
        it restarts, using this file (unless the -X option is used).  The
        default is ~/.pynche

    --ignore
    -X
        Ignore the initialization file when starting up.  Pynche will still
        write the current option settings to this file when it quits.

    --version
    -v
        print the version number

    --help
    -h
        print this message

    initialcolor
        initial color, as a color name or #RRGGBB format

"""

__version__ = '0.1'

import sys
import os
import string
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
    # The X11R6.4 rgb.txt file
    os.path.join(sys.path[0], 'X/rgb.txt'),
    # add more here
    ]



def docstring():
    return string.rstrip(__doc__ % globals())


def usage(status, msg=''):
    print docstring()
    if msg:
        print msg
    sys.exit(status)



def initial_color(s, colordb):
    # function called on every color
    def scan_color(s, colordb=colordb):
        try:
            r, g, b = colordb.find_byname(s)
        except ColorDB.BadColor:
            try:
                r, g, b = ColorDB.rrggbb_to_triplet(s)
            except ColorDB.BadColor:
                return None, None, None
        return r, g, b
    #
    # First try the passed in color
    r, g, b = scan_color(s)
    if r is None:
        # try the same color with '#' prepended, since some shells require
        # this to be escaped, which is a pain
        r, g, b = scan_color('#' + s)
    if r is None:
        print 'Bad initial color, using gray50:', s
        r, g, b = scan_color('gray50')
    if r is None:
        usage(1, 'Cannot find an initial color to use')
        # does not return
    return r, g, b



def build(master=None, initialcolor=None, initfile=None, ignore=None):
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

    # create all output widgets
    s = Switchboard(colordb, not ignore and initfile)

    # create the application window decorations
    app = PyncheWidget(__version__, s, master=master)
    w = app.window()

    s.add_view(StripViewer(s, w))
    s.add_view(ChipViewer(s, w))
    s.add_view(TypeinViewer(s, w))

    # get the initial color as components and set the color on all views.  if
    # there was no initial color given on the command line, use the one that's 
    # stored in the option database
    if initialcolor is None:
        optiondb = s.optiondb()
        red = optiondb.get('RED')
        green = optiondb.get('GREEN')
        blue = optiondb.get('BLUE')
        # but if there wasn't any stored in the database, use grey50
        if red is None or blue is None or green is None:
            red, green, blue = initial_color('grey50', colordb)
    else:
        red, green, blue = initial_color(initialcolor, colordb)
    s.update_views(red, green, blue)
    return app, s


def run(app, s):
    try:
	app.start()
    except KeyboardInterrupt:
	pass



def main():
    try:
	opts, args = getopt.getopt(
            sys.argv[1:],
            'hd:i:Xv',
            ['database=', 'initfile=', 'ignore', 'help', 'version'])
    except getopt.error, msg:
	usage(1, msg)

    if len(args) == 0:
        initialcolor = None
    elif len(args) == 1:
        initialcolor = args[0]
    else:
	usage(1)

    ignore = 0
    initfile = os.path.expanduser('~/.pynche')
    for opt, arg in opts:
	if opt in ('-h', '--help'):
	    usage(0)
        elif opt in ('-v', '--version'):
            print '''\
Pynche -- The PYthon Natural Color and Hue Editor.
Contact: Barry Warsaw
Email:   bwarsaw@python.org
Version: %s''' % __version__
            sys.exit(0)
	elif opt in ('-d', '--database'):
	    RGB_TXT.insert(0, arg)
        elif opt in ('-X', '--ignore'):
            ignore = 1
        elif opt in ('-i', '--initfile'):
            initfile = arg

    app, sb = build(initialcolor=initialcolor,
                    initfile=initfile,
                    ignore=ignore)
    run(app, sb)
    sb.save_views()


if __name__ == '__main__':
    main()
