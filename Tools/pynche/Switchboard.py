class Switchboard:
    def __init__(self, app, colordb, red, green, blue):
        self.__app = app
        self.__colordb = colordb
        self.__red = red
        self.__green = green
        self.__blue = blue
        self.__views = []

    def add_view(self, view):
        self.__views.append(view)

    def update_views(self, srcview, red, green, blue):
        for v in self.__views:
            if v <> srcview:
                v.update_yourself(red, green, blue)
