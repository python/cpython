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
    newy, bot_y = ((0, 72) if sys.platform == 'win32' else
                   (22, 88) if macosx.isAquaTk() else
                   (0, 88) ) # Guess for anything else.
    newheight = newheight - newy - bot_y
    newgeom = '' if height >= newheight else f"{width}x{newheight}+{x}+{newy}"
    top.wm_geometry(newgeom)
    return newgeom != ""


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_zoomheight', verbosity=2, exit=False)

    # Add htest?
