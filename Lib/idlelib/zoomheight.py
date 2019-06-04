"Zoom a window to maximum height."

import re
import sys


class ZoomHeight:

    def __init__(self, editwin):
        self.editwin = editwin

        # cached values for maximized window dimensions
        self._max_height_and_y_coord = None

        # self.top.protocol("WM_DELETE_WINDOW", lambda:print(self.top.winfo_rooty()))

    @property
    def top(self):
        return self.editwin.top

    def zoom_height_event(self, event=None):
        try:
            zoomed = self.zoom_height()
        except RuntimeError:
            return "break"

        menu_status = 'Restore' if zoomed else 'Zoom'
        self.editwin.update_menu_label(menu='options', index='* Height',
                                       label=f'{menu_status} Height')

        return "break"

    def zoom_height(self):
        width, height, x, y = get_window_geometry(self.top)

        maxheight, maxy = self.get_max_height_and_y_coord()

        if self.top.wm_state() == 'normal' and height < maxheight:
            set_window_geometry(self.top, (width, maxheight, x, maxy))
            return True
        else:
            # .wm_geometry('') makes the window revert to the size requested
            # by the widgets it contains.
            self.top.wm_geometry('')
            return False

    def get_max_height_and_y_coord(self):
        if self._max_height_and_y_coord is None:
            # Maximize the window to get the appropriate height and Y
            # coordinate.
            if self.top.wm_state() != 'normal':
                raise RuntimeError('The editor window is not in "normal" state.')

            orig_geom = get_window_geometry(self.top)

            self.top.wm_state('zoomed')
            self.top.update()
            maxwidth, maxheight, maxx, maxy = get_window_geometry(self.top)
            if sys.platform == 'win32':
                # On Windows, the returned Y coordinate is the one before
                # maximizing, so we use 0 which is correct unless a user puts
                # their dock on the top of the screen (very rare).
                maxy = 0
            maxrooty = self.top.winfo_rooty()
            self.top.wm_state('normal')
            max_y_geom = orig_geom[:3] + (maxy,)
            set_window_geometry(self.top, max_y_geom)
            self.top.update()
            max_y_geom_rooty = self.top.winfo_rooty()
            set_window_geometry(self.top, orig_geom)

            maxheight += maxrooty - max_y_geom_rooty

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


def set_window_geometry(top, geometry):
    top.wm_geometry("{:d}x{:d}{:+d}{:+d}".format(*geometry))


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_zoomheight', verbosity=2, exit=False)

    # Add htest?
