
# The options of a widget are described by the following attributes
# of the Pack and Widget dialogs:
#
# Dialog.current: {name: value}
# -- changes during Widget's lifetime
#
# Dialog.options: {name: (default, klass)}
# -- depends on widget class only
#
# Dialog.classes: {klass: (v0, v1, v2, ...) | 'boolean' | 'other'}
# -- totally static, though different between PackDialog and WidgetDialog
#    (but even that could be unified)

from tkinter import *

class Option:

    varclass = StringVar            # May be overridden

    def __init__(self, dialog, option):
        self.dialog = dialog
        self.option = option
        self.master = dialog.top
        self.default, self.klass = dialog.options[option]
        self.var = self.varclass(self.master)
        self.frame = Frame(self.master)
        self.frame.pack(fill=X)
        self.label = Label(self.frame, text=(option + ":"))
        self.label.pack(side=LEFT)
        self.update()
        self.addoption()

    def refresh(self):
        self.dialog.refresh()
        self.update()

    def update(self):
        try:
            self.current = self.dialog.current[self.option]
        except KeyError:
            self.current = self.default
        self.var.set(self.current)

    def set(self, e=None):          # Should be overridden
        pass

class BooleanOption(Option):

    varclass = BooleanVar

    def addoption(self):
        self.button = Checkbutton(self.frame,
                                 text='on/off',
                                 onvalue=1,
                                 offvalue=0,
                                 variable=self.var,
                                 relief=RAISED,
                                 borderwidth=2,
                                 command=self.set)
        self.button.pack(side=RIGHT)

class EnumOption(Option):

    def addoption(self):
        self.button = Menubutton(self.frame,
                                 textvariable=self.var,
                                 relief=RAISED, borderwidth=2)
        self.button.pack(side=RIGHT)
        self.menu = Menu(self.button)
        self.button['menu'] = self.menu
        for v in self.dialog.classes[self.klass]:
            self.menu.add_radiobutton(
                label=v,
                variable=self.var,
                value=v,
                command=self.set)

class StringOption(Option):

    def addoption(self):
        self.entry = Entry(self.frame,
                           textvariable=self.var,
                           width=10,
                           relief=SUNKEN,
                           borderwidth=2)
        self.entry.pack(side=RIGHT, fill=X, expand=1)
        self.entry.bind('<Return>', self.set)

class ReadonlyOption(Option):

    def addoption(self):
        self.label = Label(self.frame, textvariable=self.var,
                           anchor=E)
        self.label.pack(side=RIGHT)

class Dialog:

    def __init__(self, master):
        self.master = master
        self.fixclasses()
        self.refresh()
        self.top = Toplevel(self.master)
        self.top.title(self.__class__.__name__)
        self.top.minsize(1, 1)
        self.addchoices()

    def refresh(self): pass         # Must override

    def fixclasses(self): pass      # May override

    def addchoices(self):
        self.choices = {}
        list = []
        for k, dc in self.options.items():
            list.append((k, dc))
        list.sort()
        for k, (d, c) in list:
            try:
                cl = self.classes[c]
            except KeyError:
                cl = 'unknown'
            if type(cl) is tuple:
                cl = self.enumoption
            elif cl == 'boolean':
                cl = self.booleanoption
            elif cl == 'readonly':
                cl = self.readonlyoption
            else:
                cl = self.stringoption
            self.choices[k] = cl(self, k)

    # Must override:
    options = {}
    classes = {}

    # May override:
    booleanoption = BooleanOption
    stringoption = StringOption
    enumoption = EnumOption
    readonlyoption = ReadonlyOption

class PackDialog(Dialog):

    def __init__(self, widget):
        self.widget = widget
        Dialog.__init__(self, widget)

    def refresh(self):
        self.current = self.widget.info()
        self.current['.class'] = self.widget.winfo_class()
        self.current['.name'] = self.widget._w

    class packoption: # Mix-in class
        def set(self, e=None):
            self.current = self.var.get()
            try:
                self.dialog.widget.pack(**{self.option: self.current})
            except TclError as msg:
                print(msg)
                self.refresh()

    class booleanoption(packoption, BooleanOption): pass
    class enumoption(packoption, EnumOption): pass
    class stringoption(packoption, StringOption): pass
    class readonlyoption(packoption, ReadonlyOption): pass

    options = {
            '.class': (None, 'Class'),
            '.name': (None, 'Name'),
            'after': (None, 'Widget'),
            'anchor': ('center', 'Anchor'),
            'before': (None, 'Widget'),
            'expand': ('no', 'Boolean'),
            'fill': ('none', 'Fill'),
            'in': (None, 'Widget'),
            'ipadx': (0, 'Pad'),
            'ipady': (0, 'Pad'),
            'padx': (0, 'Pad'),
            'pady': (0, 'Pad'),
            'side': ('top', 'Side'),
            }

    classes = {
            'Anchor': (N, NE, E, SE, S, SW, W, NW, CENTER),
            'Boolean': 'boolean',
            'Class': 'readonly',
            'Expand': 'boolean',
            'Fill': (NONE, X, Y, BOTH),
            'Name': 'readonly',
            'Pad': 'pixel',
            'Side': (TOP, RIGHT, BOTTOM, LEFT),
            'Widget': 'readonly',
            }

class RemotePackDialog(PackDialog):

    def __init__(self, master, app, widget):
        self.master = master
        self.app = app
        self.widget = widget
        self.refresh()
        self.top = Toplevel(self.master)
        self.top.title(self.app + ' PackDialog')
        self.top.minsize(1, 1)
        self.addchoices()

    def refresh(self):
        try:
            words = self.master.tk.splitlist(
                    self.master.send(self.app,
                                     'pack',
                                     'info',
                                     self.widget))
        except TclError as msg:
            print(msg)
            return
        dict = {}
        for i in range(0, len(words), 2):
            key = words[i][1:]
            value = words[i+1]
            dict[key] = value
        dict['.class'] = self.master.send(self.app,
                                          'winfo',
                                          'class',
                                          self.widget)
        dict['.name'] = self.widget
        self.current = dict

    class remotepackoption: # Mix-in class
        def set(self, e=None):
            self.current = self.var.get()
            try:
                self.dialog.master.send(
                        self.dialog.app,
                        'pack',
                        'config',
                        self.dialog.widget,
                        '-'+self.option,
                        self.dialog.master.tk.merge(
                                self.current))
            except TclError as msg:
                print(msg)
                self.refresh()

    class booleanoption(remotepackoption, BooleanOption): pass
    class enumoption(remotepackoption, EnumOption): pass
    class stringoption(remotepackoption, StringOption): pass
    class readonlyoption(remotepackoption, ReadonlyOption): pass

class WidgetDialog(Dialog):

    def __init__(self, widget):
        self.widget = widget
        self.klass = widget.winfo_class()
        Dialog.__init__(self, widget)

    def fixclasses(self):
        if self.klass in self.addclasses:
            classes = {}
            for c in (self.classes,
                      self.addclasses[self.klass]):
                for k in c.keys():
                    classes[k] = c[k]
            self.classes = classes

    def refresh(self):
        self.configuration = self.widget.config()
        self.update()
        self.current['.class'] = self.widget.winfo_class()
        self.current['.name'] = self.widget._w

    def update(self):
        self.current = {}
        self.options = {}
        for k, v in self.configuration.items():
            if len(v) > 4:
                self.current[k] = v[4]
                self.options[k] = v[3], v[2] # default, klass
        self.options['.class'] = (None, 'Class')
        self.options['.name'] = (None, 'Name')

    class widgetoption: # Mix-in class
        def set(self, e=None):
            self.current = self.var.get()
            try:
                self.dialog.widget[self.option] = self.current
            except TclError as msg:
                print(msg)
                self.refresh()

    class booleanoption(widgetoption, BooleanOption): pass
    class enumoption(widgetoption, EnumOption): pass
    class stringoption(widgetoption, StringOption): pass
    class readonlyoption(widgetoption, ReadonlyOption): pass

    # Universal classes
    classes = {
            'Anchor': (N, NE, E, SE, S, SW, W, NW, CENTER),
            'Aspect': 'integer',
            'Background': 'color',
            'Bitmap': 'bitmap',
            'BorderWidth': 'pixel',
            'Class': 'readonly',
            'CloseEnough': 'double',
            'Command': 'command',
            'Confine': 'boolean',
            'Cursor': 'cursor',
            'CursorWidth': 'pixel',
            'DisabledForeground': 'color',
            'ExportSelection': 'boolean',
            'Font': 'font',
            'Foreground': 'color',
            'From': 'integer',
            'Geometry': 'geometry',
            'Height': 'pixel',
            'InsertWidth': 'time',
            'Justify': (LEFT, CENTER, RIGHT),
            'Label': 'string',
            'Length': 'pixel',
            'MenuName': 'widget',
            'Name': 'readonly',
            'OffTime': 'time',
            'OnTime': 'time',
            'Orient': (HORIZONTAL, VERTICAL),
            'Pad': 'pixel',
            'Relief': (RAISED, SUNKEN, FLAT, RIDGE, GROOVE),
            'RepeatDelay': 'time',
            'RepeatInterval': 'time',
            'ScrollCommand': 'command',
            'ScrollIncrement': 'pixel',
            'ScrollRegion': 'rectangle',
            'ShowValue': 'boolean',
            'SetGrid': 'boolean',
            'Sliderforeground': 'color',
            'SliderLength': 'pixel',
            'Text': 'string',
            'TickInterval': 'integer',
            'To': 'integer',
            'Underline': 'index',
            'Variable': 'variable',
            'Value': 'string',
            'Width': 'pixel',
            'Wrap': (NONE, CHAR, WORD),
            }

    # Classes that (may) differ per widget type
    _tristate = {'State': (NORMAL, ACTIVE, DISABLED)}
    _bistate = {'State': (NORMAL, DISABLED)}
    addclasses = {
            'Button': _tristate,
            'Radiobutton': _tristate,
            'Checkbutton': _tristate,
            'Entry': _bistate,
            'Text': _bistate,
            'Menubutton': _tristate,
            'Slider': _bistate,
            }

class RemoteWidgetDialog(WidgetDialog):

    def __init__(self, master, app, widget):
        self.app = app
        self.widget = widget
        self.klass = master.send(self.app,
                                 'winfo',
                                 'class',
                                 self.widget)
        Dialog.__init__(self, master)

    def refresh(self):
        try:
            items = self.master.tk.splitlist(
                    self.master.send(self.app,
                                     self.widget,
                                     'config'))
        except TclError as msg:
            print(msg)
            return
        dict = {}
        for item in items:
            words = self.master.tk.splitlist(item)
            key = words[0][1:]
            value = (key,) + words[1:]
            dict[key] = value
        self.configuration = dict
        self.update()
        self.current['.class'] = self.klass
        self.current['.name'] = self.widget

    class remotewidgetoption: # Mix-in class
        def set(self, e=None):
            self.current = self.var.get()
            try:
                self.dialog.master.send(
                        self.dialog.app,
                        self.dialog.widget,
                        'config',
                        '-'+self.option,
                        self.current)
            except TclError as msg:
                print(msg)
                self.refresh()

    class booleanoption(remotewidgetoption, BooleanOption): pass
    class enumoption(remotewidgetoption, EnumOption): pass
    class stringoption(remotewidgetoption, StringOption): pass
    class readonlyoption(remotewidgetoption, ReadonlyOption): pass

def test():
    import sys
    root = Tk()
    root.minsize(1, 1)
    if sys.argv[1:]:
        remotetest(root, sys.argv[1])
    else:
        frame = Frame(root, name='frame')
        frame.pack(expand=1, fill=BOTH)
        button = Button(frame, name='button', text='button')
        button.pack(expand=1)
        canvas = Canvas(frame, name='canvas')
        canvas.pack()
        fpd = PackDialog(frame)
        fwd = WidgetDialog(frame)
        bpd = PackDialog(button)
        bwd = WidgetDialog(button)
        cpd = PackDialog(canvas)
        cwd = WidgetDialog(canvas)
    root.mainloop()

def remotetest(root, app):
    from listtree import listtree
    list = listtree(root, app)
    list.bind('<Any-Double-1>', opendialogs)
    list.app = app                  # Pass it on to handler

def opendialogs(e):
    list = e.widget
    sel = list.curselection()
    for i in sel:
        item = list.get(i)
        widget = item.split()[0]
        RemoteWidgetDialog(list, list.app, widget)
        if widget == '.': continue
        try:
            RemotePackDialog(list, list.app, widget)
        except TclError as msg:
            print(msg)

test()
