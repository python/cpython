#
# Tkinter
# $Id$
#
# font wrapper
#
# written by Fredrik Lundh <fredrik@pythonware.com>, February 1998
#
# FIXME: should add 'displayof' option where relevant (actual, families,
#        measure, and metrics)
#
# Copyright (c) Secret Labs AB 1998.
#
# info@pythonware.com
# http://www.pythonware.com
#

__version__ = "0.9"

import Tkinter

# weight/slant
NORMAL = "normal"
BOLD   = "bold"
ITALIC = "italic"

class Font:

    """Represents a named font.

    Constructor options are:

    font -- font specifier (name, system font, or (family, size, style)-tuple)

       or any combination of

    family -- font 'family', e.g. Courier, Times, Helvetica
    size -- font size in points
    weight -- font thickness: NORMAL, BOLD
    slant -- font slant: NORMAL, ITALIC
    underline -- font underlining: false (0), true (1)
    overstrike -- font strikeout: false (0), true (1)
    name -- name to use for this font configuration (defaults to a unique name)
    """

    def _set(self, kw):
        options = []
        for k, v in kw.items():
            options.append("-"+k)
            options.append(str(v))
        return tuple(options)

    def _get(self, args):
        options = []
        for k in args:
            options.append("-"+k)
        return tuple(options)

    def _mkdict(self, args):
        options = {}
        for i in range(0, len(args), 2):
            options[args[i][1:]] = args[i+1]
        return options

    def __init__(self, root=None, font=None, name=None, **options):
        if not root:
            root = Tkinter._default_root
        if font:
            # get actual settings corresponding to the given font
            font = root.tk.splitlist(root.tk.call("font", "actual", font))
        else:
            font = self._set(options)
        if not name:
            name = "font" + str(id(self))
        self.name = name
        apply(root.tk.call, ("font", "create", name) + font)
        # backlinks!
        self._root  = root
        self._split = root.tk.splitlist
        self._call  = root.tk.call

    def __str__(self):
        return self.name

    def __del__(self):
        try:
            self._call("font", "delete", self.name)
        except (AttributeError, Tkinter.TclError):
            pass

    def copy(self):
        "Return a distinct copy of the current font"
        return apply(Font, (self._root,), self.actual())

    def actual(self, option=None):
        "Return actual font attributes"
        if option:
            return self._call("font", "actual", self.name, "-"+option)
        else:
            return self._mkdict(
                self._split(self._call("font", "actual", self.name))
                )

    def cget(self, option):
        "Get font attribute"
        return self._call("font", "config", self.name, "-"+option)

    def config(self, **options):
        "Modify font attributes"
        if options:
            apply(self._call, ("font", "config", self.name) +
                  self._set(options))
        else:
            return self._mkdict(
                self._split(self._call("font", "config", self.name))
                )

    configure = config

    def measure(self, text):
        "Return text width"
        return int(self._call("font", "measure", self.name, text))

    def metrics(self, *options):
        """Return font metrics.

        For best performance, create a dummy widget
        using this font before calling this method."""

        if options:
            return int(
                self._call("font", "metrics", self.name, self._get(options))
                )
        else:
            res = self._split(self._call("font", "metrics", self.name))
            options = {}
            for i in range(0, len(res), 2):
                options[res[i][1:]] = int(res[i+1])
            return options

def families(root=None):
    "Get font families (as a tuple)"
    if not root:
        root = Tkinter._default_root
    return root.tk.splitlist(root.tk.call("font", "families"))

def names(root=None):
    "Get names of defined fonts (as a tuple)"
    if not root:
        root = Tkinter._default_root
    return root.tk.splitlist(root.tk.call("font", "names"))

# --------------------------------------------------------------------
# test stuff

if __name__ == "__main__":

    root = Tkinter.Tk()

    # create a font
    f = Font(family="times", size=30, weight=NORMAL)

    print f.actual()
    print f.actual("family")
    print f.actual("weight")

    print f.config()
    print f.cget("family")
    print f.cget("weight")

    print names()

    print f.measure("hello"), f.metrics("linespace")

    print f.metrics()

    f = Font(font=("Courier", 20, "bold"))
    print f.measure("hello"), f.metrics("linespace")

    w = Tkinter.Label(root, text="Hello, world", font=f)
    w.pack()

    w = Tkinter.Button(root, text="Quit!", command=root.destroy)
    w.pack()

    fb = Font(font=w["font"]).copy()
    fb.config(weight=BOLD)

    w.config(font=fb)

    Tkinter.mainloop()
