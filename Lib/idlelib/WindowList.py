from Tkinter import *

class WindowList:

    def __init__(self):
        self.dict = {}
        self.callbacks = []

    def add(self, window):
        window.after_idle(self.call_callbacks)
        self.dict[str(window)] = window

    def delete(self, window):
        try:
            del self.dict[str(window)]
        except KeyError:
            # Sometimes, destroy() is called twice
            pass
        self.call_callbacks()

    def add_windows_to_menu(self,  menu):
        list = []
        for key in self.dict.keys():
            window = self.dict[key]
            try:
                title = window.get_title()
            except TclError:
                continue
            list.append((title, window))
        list.sort()
        for title, window in list:
            if title == "Python Shell":
                # Hack -- until we have a better way to this
                continue
            menu.add_command(label=title, command=window.wakeup)

    def register_callback(self, callback):
        self.callbacks.append(callback)

    def unregister_callback(self, callback):
        try:
           self.callbacks.remove(callback)
        except ValueError:
            pass

    def call_callbacks(self):
        for callback in self.callbacks:
            try:
                callback()
            except:
                print "warning: callback failed in WindowList", \
                      sys.exc_type, ":", sys.exc_value

registry = WindowList()

add_windows_to_menu = registry.add_windows_to_menu
register_callback = registry.register_callback
unregister_callback = registry.unregister_callback


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
        try:
            if self.wm_state() == "iconic":
                self.wm_deiconify()
            else:
                self.tkraise()
            self.focus_set()
        except TclError:
            # This can happen when the window menu was torn off.
            # Simply ignore it.
            pass
