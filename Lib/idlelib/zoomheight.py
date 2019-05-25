"Zoom a window to maximum height."

import re
import sys

from idlelib import macosx


class ZoomHeight:

    def __init__(self, editwin):
        self.editwin = editwin

    def zoom_height_event(self, event=None):
        top = self.editwin.top
        zoomed = zoom_height(top)
        menu_status = 'Restore' if zoomed else 'Zoom'
        self.editwin.update_menu_label(menu='options', index='* Height',
                                       label=f'{menu_status} Height')
        return "break"


def zoom_height(top):
    geom = top.wm_geometry()
    m = re.match(r"(\d+)x(\d+)\+(-?\d+)\+(-?\d+)", geom)
    if not m:
        top.bell()
        return
    width, height, x, y = map(int, m.groups())
    newheight = top.winfo_screenheight()

    # The constants below for Windows and Mac Aqua are visually determined
    # to avoid taskbar or menubar and app icons.
    if sys.platform == 'win32':
        newy = 0
        newheight = newheight - 114
    elif macosx.isAquaTk():
        newy = 22
        newheight = newheight - newy - 88
    else:
        newy = 0
        newheight = newheight - 88

    newgeom = ('' if height >= newheight else f"{width}x{newheight}+{x}+{newy}")
    top.wm_geometry(newgeom)
    return newgeom != ""


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_zoomheight', verbosity=2, exit=False)

    # Add htest?
