"Zoom a window to maximum height."

import re


class ZoomHeight:

    def __init__(self, editwin):
        self.editwin = editwin

        # cached values for maximized window dimensions
        self._max_height_and_y_coord = None

    @property
    def top(self):
        return self.editwin.top

    def zoom_height_event(self, event=None):
        zoomed = self.zoom_height()
        menu_status = 'Restore' if zoomed else 'Zoom'
        self.editwin.update_menu_label(menu='options', index='* Height',
                                       label=f'{menu_status} Height')
        return "break"

    def zoom_height(self):
        width, height, x, y = get_window_geometry(self.top)

        maxheight, maxy = self.max_height_and_y_coord

        # .wm_geometry('') makes the window revert to the size requested
        # by the widgets it contains.
        newgeom = \
            '' if height >= maxheight else f"{width}x{maxheight}+{x}+{maxy}"
        self.top.wm_geometry(newgeom)
        return newgeom != ''

    @property
    def max_height_and_y_coord(self):
        if self._max_height_and_y_coord is None:
            # Maximize the window to get the appropriate height and Y
            # coordinate.
            self.top.wm_state('zoomed')
            maxwidth, maxheight, maxx, maxy = get_window_geometry(self.top)
            self._max_height_and_y_coord = maxheight, maxy
        return self._max_height_and_y_coord


def get_window_geometry(top):
    geom = top.wm_geometry()
    m = re.match(r"(\d+)x(\d+)\+(-?\d+)\+(-?\d+)", geom)
    if not m:
        top.bell()
        return
    width, height, x, y = map(int, m.groups())
    return width, height, x, y


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_zoomheight', verbosity=2, exit=False)

    # Add htest?
