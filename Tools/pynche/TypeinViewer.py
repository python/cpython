"""TypeinViewer class.

The TypeinViewer is what you see at the lower right of the main Pynche
widget.  It contains three text entry fields, one each for red, green, blue.
Input into these windows is highly constrained; it only allows you to enter
values that are legal for a color axis.  This usually means 0-255 for decimal
input and 0x0 - 0xff for hex input.

You can toggle whether you want to view and input the values in either decimal
or hex by clicking on Hexadecimal.  By clicking on Update while typing, the
color selection will be made on every change to the text field.  Otherwise,
you must hit Return or Tab to select the color.
"""

from Tkinter import *
import string
import re

class TypeinViewer:
    def __init__(self, switchboard, master=None):
        # non-gui ivars
        self.__sb = switchboard
        optiondb = switchboard.optiondb()
        self.__hexp = BooleanVar()
        self.__hexp.set(optiondb.get('HEXTYPE', 0))
        self.__uwtyping = BooleanVar()
        self.__uwtyping.set(optiondb.get('UPWHILETYPE', 0))
        # create the gui
        self.__frame = Frame(master, relief=RAISED, borderwidth=1)
        self.__frame.grid(row=3, column=1, sticky='NSEW')
        # Red
        self.__xl = Label(self.__frame, text='Red:')
        self.__xl.grid(row=0, column=0, sticky=E)
        self.__x = Entry(self.__frame, width=4)
        self.__x.grid(row=0, column=1)
        self.__x.bindtags(self.__x.bindtags() + ('Normalize', 'Update'))
        self.__x.bind_class('Normalize', '<Key>', self.__normalize)
        self.__x.bind_class('Update'   , '<Key>', self.__maybeupdate)
        # Green
        self.__yl = Label(self.__frame, text='Green:')
        self.__yl.grid(row=1, column=0, sticky=E)
        self.__y = Entry(self.__frame, width=4)
        self.__y.grid(row=1, column=1)
        self.__y.bindtags(self.__y.bindtags() + ('Normalize', 'Update'))
        # Blue
        self.__zl = Label(self.__frame, text='Blue:')
        self.__zl.grid(row=2, column=0, sticky=E)
        self.__z = Entry(self.__frame, width=4)
        self.__z.grid(row=2, column=1)
        self.__z.bindtags(self.__z.bindtags() + ('Normalize', 'Update'))
        # Update while typing?
        self.__uwt = Checkbutton(self.__frame,
                                 text='Update while typing',
                                 variable=self.__uwtyping)
        self.__uwt.grid(row=3, column=0, columnspan=2, sticky=W)
        # Hex/Dec
        self.__hex = Checkbutton(self.__frame,
                                 text='Hexadecimal',
                                 variable=self.__hexp,
                                 command=self.__togglehex)
        self.__hex.grid(row=4, column=0, columnspan=2, sticky=W)

    def __togglehex(self, event=None):
        red, green, blue = self.__sb.current_rgb()
        self.update_yourself(red, green, blue)

    def __normalize(self, event=None):
        ew = event.widget
        contents = ew.get()
        icursor = ew.index(INSERT)
        if contents == '':
            contents = '0'
        if contents[0] in 'xX' and self.__hexp.get():
            contents = '0' + contents
        # figure out what the contents value is in the current base
        try:
            if self.__hexp.get():
                v = string.atoi(contents, 16)
            else:
                v = string.atoi(contents)
        except ValueError:
            v = None
        # if value is not legal, delete the last character inserted and ring
        # the bell
        if v is None or v < 0 or v > 255:
            i = ew.index(INSERT)
            if event.char:
                contents = contents[:i-1] + contents[i:]
                icursor = icursor-1
            ew.bell()
        elif self.__hexp.get():
            contents = hex(v)
        else:
            contents = int(v)
        ew.delete(0, END)
        ew.insert(0, contents)
        ew.icursor(icursor)

    def __maybeupdate(self, event=None):
        if self.__uwtyping.get() or event.keysym in ('Return', 'Tab'):
            self.__update(event)

    def __update(self, event=None):
        redstr = self.__x.get()
        greenstr = self.__y.get()
        bluestr = self.__z.get()
        if self.__hexp.get():
            red = string.atoi(redstr, 16)
            green = string.atoi(greenstr, 16)
            blue = string.atoi(bluestr, 16)
        else:
            red, green, blue = map(string.atoi, (redstr, greenstr, bluestr))
        self.__sb.update_views(red, green, blue)

    def update_yourself(self, red, green, blue):
        if self.__hexp.get():
            redstr, greenstr, bluestr = map(hex, (red, green, blue))
        else:
            redstr, greenstr, bluestr = red, green, blue
        x, y, z = self.__x, self.__y, self.__z
        xicursor = x.index(INSERT)
        yicursor = y.index(INSERT)
        zicursor = z.index(INSERT)
        x.delete(0, END)
        y.delete(0, END)
        z.delete(0, END)
        x.insert(0, redstr)
        y.insert(0, greenstr)
        z.insert(0, bluestr)
        x.icursor(xicursor)
        y.icursor(yicursor)
        z.icursor(zicursor)

    def hexp_var(self):
        return self.__hexp

    def save_options(self, optiondb):
        optiondb['HEXTYPE'] = self.__hexp.get()
        optiondb['UPWHILETYPE'] = self.__uwtyping.get()
