from Tkinter import *


class WidgetRedirector:

    """Support for redirecting arbitrary widget subcommands."""

    def __init__(self, widget):
        self.dict = {}
        self.widget = widget
        self.tk = tk = widget.tk
        w = widget._w
        self.orig = w + "_orig"
        tk.call("rename", w, self.orig)
        tk.createcommand(w, self.dispatch)

    def __repr__(self):
        return "WidgetRedirector(%s<%s>)" % (self.widget.__class__.__name__,
                                             self.widget._w)

    def __del__(self):
        self.close()

    def close(self):
        self.dict = {}
        widget = self.widget; del self.widget
        orig = self.orig; del self.orig
        tk = widget.tk
        w = widget._w
        tk.deletecommand(w)
        tk.call("rename", w, orig)

    def register(self, name, function):
        if self.dict.has_key(name):
            previous = function
        else:
            previous = OriginalCommand(self, name)
        self.dict[name] = function
        setattr(self.widget, name, function)
        return previous

    def dispatch(self, cmd, *args):
        m = self.dict.get(cmd)
        try:
            if m:
                return apply(m, args)
            else:
                return self.tk.call((self.orig, cmd) + args)
        except TclError:
            return ""


class OriginalCommand:

    def __init__(self, redir, name):
        self.redir = redir
        self.name = name
        self.tk = redir.tk
        self.orig = redir.orig
        self.tk_call = self.tk.call
        self.orig_and_name = (self.orig, self.name)

    def __repr__(self):
        return "OriginalCommand(%s, %s)" % (`self.redir`, `self.name`)

    def __call__(self, *args):
        return self.tk_call(self.orig_and_name + args)


def main():
    root = Tk()
    text = Text()
    text.pack()
    text.focus_set()
    redir = WidgetRedirector(text)
    global orig_insert
    def my_insert(*args):
        print "insert", args
        apply(orig_insert, args)
    orig_insert = redir.register("insert", my_insert)
    root.mainloop()

if __name__ == "__main__":
    main()
