from Tkinter import *

class WindowList:

    def __init__(self):
        self.dict = {}

    def add(self, window):
        self.dict[str(window)] = window

    def delete(self, window):
        try:
            del self.dict[str(window)]
        except KeyError:
            # Sometimes, destroy() is called twice
            pass

    def add_windows_to_menu(self,  menu):
        list = []
        for key in self.dict.keys():
            window = self.dict[key]
            title = window.get_title()
            list.append((title, window))
        list.sort()
        for title, window in list:
            if title == "Python Shell":
                # Hack -- until we have a better way to this
                continue
            menu.add_command(label=title, command=window.wakeup)

registry = WindowList()

def add_windows_to_menu(menu):
    registry.add_windows_to_menu(menu)

class ListedToplevel(Toplevel):

    def __init__(self, master, **kw):
        Toplevel.__init__(self, master, kw)
        registry.add(self)

    def destroy(self):
        registry.delete(self)
        Toplevel.destroy(self)

    def get_title(self):
        # Subclass can override
        return self.wm_title()

    def wakeup(self):
        self.tkraise()
        self.wm_deiconify()
        self.focus_set()
