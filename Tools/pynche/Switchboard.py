"""Switchboard class.

This class is used to coordinate updates among all Viewers.  Every Viewer must
conform to the following interface:

    - it must include a method called update_yourself() which takes three
      arguments; the red, green, and blue values of the selected color.

    - When a Viewer selects a color and wishes to update all other Views, it
      should call update_views() on the Switchboard object.  Not that the
      Viewer typically does *not* update itself before calling update_views(), 
      since this would cause it to get updated twice.
"""

from types import DictType
import marshal

class Switchboard:
    def __init__(self, colordb, initfile):
        self.__colordb = colordb
        self.__optiondb = {}
        self.__views = []
        self.__red = 0
        self.__green = 0
        self.__blue = 0
        # read the initialization file
        fp = None
        if initfile:
            try:
                try:
                    fp = open(initfile)
                    self.__optiondb = marshal.load(fp)
                    if type(self.__optiondb) <> DictType:
                        print 'Problem reading options from file:', initfile
                        self.__optiondb = {}
                except (IOError, EOFError):
                    pass
            finally:
                if fp:
                    fp.close()

    def add_view(self, view):
        self.__views.append(view)

    def update_views(self, red, green, blue):
        self.__red = red
        self.__green = green
        self.__blue = blue
        for v in self.__views:
            v.update_yourself(red, green, blue)

    def update_views_current(self):
        self.update_views(self.__red, self.__green, self.__blue)

    def current_rgb(self):
        return self.__red, self.__green, self.__blue

    def colordb(self):
        return self.__colordb

    def optiondb(self):
        return self.__optiondb

    def save_views(self, file):
        # save the current color
        self.__optiondb['RED'] = self.__red
        self.__optiondb['GREEN'] = self.__green
        self.__optiondb['BLUE'] = self.__blue
        for v in self.__views:
            v.save_options(self.__optiondb)
        fp = None
        try:
            try:
                fp = open(file, 'w')
            except IOError:
                print 'Cannot write options to file:', file
            else:
                marshal.dump(self.__optiondb, fp)
        finally:
            if fp:
                fp.close()
