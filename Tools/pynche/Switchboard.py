class Switchboard:
    def __init__(self, colordb):
        self.__views = []
        self.__colordb = colordb
        self.__red = 0
        self.__green = 0
        self.__blue = 0

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

    def colordb(self):
        return self.__colordb
