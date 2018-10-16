"Zoom a window to maximum height."

import re
import sys

from idlelib import macosx


class ZoomHeight:

    def __init__(self, editwin):
        self.editwin = editwin

    def zoom_height_event(self, event=None):
        top = self.editwin.top
        zoom_height(top)
        return "break"


def zoom_height(top):
    geom = top.wm_geometry()
    m = re.match(r"(\d+)x(\d+)\+(-?\d+)\+(-?\d+)", geom)
    if not m:
        top.bell()
        return
    width, height, x, y = map(int, m.groups())
    newheight = top.winfo_screenheight()
    if sys.platform == 'win32':
        newy = 0
        newheight = newheight - 72

    elif macosx.isAquaTk():
        # The '88' below is a magic number that avoids placing the bottom
        # of the window below the panel on my machine. I don't know how
        # to calculate the correct value for this with tkinter.
        newy = 22
        newheight = newheight - newy - 88

    else:
        #newy = 24
        newy = 0
        #newheight = newheight - 96
        newheight = newheight - 88
    if height >= newheight:
        newgeom = ""
    else:
        newgeom = "%dx%d+%d+%d" % (width, newheight, x, newy)
    top.wm_geometry(newgeom)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_zoomheight', verbosity=2, exit=False)

    # Add htest?
