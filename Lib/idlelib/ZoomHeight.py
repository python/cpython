# Sample extension: zoom a window to maximum height

import re
import sys

class ZoomHeight:

    menudefs = [
        ('windows', [
            ('_Zoom Height', '<<zoom-height>>'),
         ])
    ]

    windows_keydefs = {
        '<<zoom-height>>': ['<Alt-F2>'],
    }
    unix_keydefs = {
        '<<zoom-height>>': ['<Control-x><Control-z>'],
    }

    def __init__(self, editwin):
        self.editwin = editwin

    def zoom_height_event(self, event):
        top = self.editwin.top
        zoom_height(top)

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
    else:
        newy = 24
        newheight = newheight - 96
    if height >= newheight:
        newgeom = ""
    else:
        newgeom = "%dx%d+%d+%d" % (width, newheight, x, newy)
    top.wm_geometry(newgeom)
