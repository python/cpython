"""Chip viewer and widget.

In the lower left corner of the main Pynche window, you will see two
ChipWidgets, one for the selected color and one for the nearest color.  The
selected color is the actual RGB value expressed as an X11 #COLOR name. The
nearest color is the named color from the X11 database that is closest to the
selected color in 3D space.  There may be other colors equally close, but the
nearest one is the first one found.

Clicking on the nearest color chip selects that named color.

The ChipViewer class includes the entire lower left quandrant; i.e. both the
selected and nearest ChipWidgets.
"""

from types import StringType
from Tkinter import *
import ColorDB


class ChipWidget:
    _WIDTH = 150
    _HEIGHT = 80

    def __init__(self,
                 master = None,
                 width  = _WIDTH,
                 height = _HEIGHT,
                 text   = 'Color',
                 initialcolor = 'blue',
                 presscmd   = None,
                 releasecmd = None):
        # create the text label
        self.__label = Label(master, text=text)
        self.__label.grid(row=0, column=0)
        # create the color chip, implemented as a frame
        self.__chip = Frame(master, relief=RAISED, borderwidth=2,
                            width=width,
                            height=height,
                            background=initialcolor)
        self.__chip.grid(row=1, column=0)
        # create the color name, ctor argument must be a string
        self.__name = Label(master, text=initialcolor)
        self.__name.grid(row=2, column=0)
        #
        # set bindings
        if presscmd:
            self.__chip.bind('<ButtonPress-1>', presscmd)
        if releasecmd:
            self.__chip.bind('<ButtonRelease-1>', releasecmd)

    def set_color(self, color):
        self.__chip.config(background=color)
        self.__name.config(text=color)

    def get_color(self):
        return self.__chip['background']

    def press(self):
        self.__chip.configure(relief=SUNKEN)

    def release(self):
        self.__chip.configure(relief=RAISED)



class ChipViewer:
    def __init__(self, switchboard, master=None):
        self.__sb = switchboard
        self.__frame = Frame(master, relief=RAISED, borderwidth=1)
        self.__frame.grid(row=3, column=0, ipadx=5, sticky='NSEW')
        # create the chip that will display the currently selected color
        # exactly
        self.__sframe = Frame(self.__frame)
        self.__sframe.grid(row=0, column=0)
        self.__selected = ChipWidget(self.__sframe, text='Selected')
        # create the chip that will display the nearest real X11 color
        # database color name
        self.__nframe = Frame(self.__frame)
        self.__nframe.grid(row=0, column=1)
        self.__nearest = ChipWidget(self.__nframe, text='Nearest',
                                    presscmd = self.__buttonpress,
                                    releasecmd = self.__buttonrelease)

    def update_yourself(self, red, green, blue):
        # TBD: should exactname default to X11 color name if their is an exact
        # match for the rgb triplet?  Part of me says it's nice to see both
        # names for the color, the other part says that it's better to
        # feedback the exact match.
        rgbtuple = (red, green, blue)
        try:
            allcolors = self.__sb.colordb().find_byrgb(rgbtuple)
            exactname = allcolors[0]
        except ColorDB.BadColor:
            exactname = ColorDB.triplet_to_rrggbb(rgbtuple)
        nearest = self.__sb.colordb().nearest(red, green, blue)
        self.__selected.set_color(exactname)
        self.__nearest.set_color(nearest)

    def __buttonpress(self, event=None):
        self.__nearest.press()

    def __buttonrelease(self, event=None):
        self.__nearest.release()
        colorname = self.__nearest.get_color()
        red, green, blue = self.__sb.colordb().find_byname(colorname)
        self.__sb.update_views(red, green, blue)
