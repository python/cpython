class Switchboard:
    def __init__(self, colordb):
        self.__views = []
        self.__colordb = colordb

    def add_view(self, view):
        self.__views.append(view)

    def update_views(self, red, green, blue):
        for v in self.__views:
            v.update_yourself(red, green, blue)

    def colordb(self):
        return self.__colordb
