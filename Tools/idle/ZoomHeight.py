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
        geom = top.wm_geometry()
        m = re.match(r"(\d+)x(\d+)\+(-?\d+)\+(-?\d+)", geom)
        if not m:
            top.bell()
            return
        width, height, x, y = map(int, m.groups())
        height = top.winfo_screenheight()
        if sys.platform == 'win32':
            y = 0
            height = height = 72
        else:
            y = 24
            height = height - 64
        newgeom = "%dx%d+%d+%d" % (width, height, x, y)
        if geom == newgeom:
            newgeom = ""
        top.wm_geometry(newgeom)
