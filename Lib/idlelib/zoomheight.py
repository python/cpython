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
        zoomed = self.zoom_height()

        if zoomed is not None:
            menu_status = 'Restore' if zoomed else 'Zoom'
            self.editwin.update_menu_label(menu='options', index='* Height',
                                           label=f'{menu_status} Height')

        return "break"

    def zoom_height(self):
        width, height, x, y = get_window_geometry(self.top)

        if self.top.wm_state() != 'normal':
            # Can't zoom/restore window height for windows not in the 'normal'
            # state, e.g. maximized and full-screen windows.
            return None

        maxheight, maxy = self.get_max_height_and_y_coord()
        if height != maxheight:
            # Maximize the window's height.
            set_window_geometry(self.top, (width, maxheight, x, maxy))
            return True
        else:
            # Restore the window's height.
            #
            # .wm_geometry('') makes the window revert to the size requested
            # by the widgets it contains.
            self.top.wm_geometry('')
            return False

    def get_max_height_and_y_coord(self):
        if self._max_height_and_y_coord is None:
            orig_state = self.top.wm_state()

            # Get window geometry info for maximized windows.
            self.top.wm_state('zoomed')
            self.top.update()
            maxwidth, maxheight, maxx, maxy = get_window_geometry(self.top)
            if sys.platform == 'win32':
                # On Windows, the returned Y coordinate is the one before
                # maximizing, so we use 0 which is correct unless a user puts
                # their dock on the top of the screen (very rare).
                maxy = 0
            maxrooty = self.top.winfo_rooty()

            # Get the "root y" coordinate for non-maximized windows with their
            # y coordinate set to that of maximized windows.  This is needed
            # to properly handle different title bar heights for non-maximized
            # vs. maximized windows, as seen e.g. in Windows 10.
            self.top.wm_state('normal')
            self.top.update()
            orig_geom = get_window_geometry(self.top)
            max_y_geom = orig_geom[:3] + (maxy,)
            set_window_geometry(self.top, max_y_geom)
            self.top.update()
            max_y_geom_rooty = self.top.winfo_rooty()

            # Adjust the maximum window height to account for the different
            # title bar heights of non-maximized vs. maximized windows.
            maxheight += maxrooty - max_y_geom_rooty

            self._max_height_and_y_coord = maxheight, maxy

            set_window_geometry(self.top, orig_geom)
            self.top.wm_state(orig_state)

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
