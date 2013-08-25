"""DetailsViewer class.

This class implements a pure input window which allows you to meticulously
edit the current color.  You have both mouse control of the color (via the
buttons along the bottom row), and there are keyboard bindings for each of the
increment/decrement buttons.

The top three check buttons allow you to specify which of the three color
variations are tied together when incrementing and decrementing.  Red, green,
and blue are self evident.  By tying together red and green, you can modify
the yellow level of the color.  By tying together red and blue, you can modify
the magenta level of the color.  By tying together green and blue, you can
modify the cyan level, and by tying all three together, you can modify the
grey level.

The behavior at the boundaries (0 and 255) are defined by the `At boundary'
option menu:

    Stop
        When the increment or decrement would send any of the tied variations
        out of bounds, the entire delta is discarded.

    Wrap Around
        When the increment or decrement would send any of the tied variations
        out of bounds, the out of bounds variation is wrapped around to the
        other side.  Thus if red were at 238 and 25 were added to it, red
        would have the value 7.

    Preserve Distance
        When the increment or decrement would send any of the tied variations
        out of bounds, all tied variations are wrapped as one, so as to
        preserve the distance between them.  Thus if green and blue were tied,
        and green was at 238 while blue was at 223, and an increment of 25
        were applied, green would be at 15 and blue would be at 0.

    Squash
        When the increment or decrement would send any of the tied variations
        out of bounds, the out of bounds variation is set to the ceiling of
        255 or floor of 0, as appropriate.  In this way, all tied variations
        are squashed to one edge or the other.

The following key bindings can be used as accelerators.  Note that Pynche can
fall behind if you hold the key down as a key repeat:

Left arrow == -1
Right arrow == +1

Control + Left == -10
Control + Right == 10

Shift + Left == -25
Shift + Right == +25
"""

from tkinter import *

STOP = 'Stop'
WRAP = 'Wrap Around'
RATIO = 'Preserve Distance'
GRAV = 'Squash'

ADDTOVIEW = 'Details Window...'


class DetailsViewer:
    def __init__(self, switchboard, master=None):
        self.__sb = switchboard
        optiondb = switchboard.optiondb()
        self.__red, self.__green, self.__blue = switchboard.current_rgb()
        # GUI
        root = self.__root = Toplevel(master, class_='Pynche')
        root.protocol('WM_DELETE_WINDOW', self.withdraw)
        root.title('Pynche Details Window')
        root.iconname('Pynche Details Window')
        root.bind('<Alt-q>', self.__quit)
        root.bind('<Alt-Q>', self.__quit)
        root.bind('<Alt-w>', self.withdraw)
        root.bind('<Alt-W>', self.withdraw)
        # accelerators
        root.bind('<KeyPress-Left>', self.__minus1)
        root.bind('<KeyPress-Right>', self.__plus1)
        root.bind('<Control-KeyPress-Left>', self.__minus10)
        root.bind('<Control-KeyPress-Right>', self.__plus10)
        root.bind('<Shift-KeyPress-Left>', self.__minus25)
        root.bind('<Shift-KeyPress-Right>', self.__plus25)
        #
        # color ties
        frame = self.__frame = Frame(root)
        frame.pack(expand=YES, fill=X)
        self.__l1 = Label(frame, text='Move Sliders:')
        self.__l1.grid(row=1, column=0, sticky=E)
        self.__rvar = IntVar()
        self.__rvar.set(optiondb.get('RSLIDER', 4))
        self.__radio1 = Checkbutton(frame, text='Red',
                                    variable=self.__rvar,
                                    command=self.__effect,
                                    onvalue=4, offvalue=0)
        self.__radio1.grid(row=1, column=1, sticky=W)
        self.__gvar = IntVar()
        self.__gvar.set(optiondb.get('GSLIDER', 2))
        self.__radio2 = Checkbutton(frame, text='Green',
                                    variable=self.__gvar,
                                    command=self.__effect,
                                    onvalue=2, offvalue=0)
        self.__radio2.grid(row=2, column=1, sticky=W)
        self.__bvar = IntVar()
        self.__bvar.set(optiondb.get('BSLIDER', 1))
        self.__radio3 = Checkbutton(frame, text='Blue',
                                    variable=self.__bvar,
                                    command=self.__effect,
                                    onvalue=1, offvalue=0)
        self.__radio3.grid(row=3, column=1, sticky=W)
        self.__l2 = Label(frame)
        self.__l2.grid(row=4, column=1, sticky=W)
        self.__effect()
        #
        # Boundary behavior
        self.__l3 = Label(frame, text='At boundary:')
        self.__l3.grid(row=5, column=0, sticky=E)
        self.__boundvar = StringVar()
        self.__boundvar.set(optiondb.get('ATBOUND', STOP))
        self.__omenu = OptionMenu(frame, self.__boundvar,
                                  STOP, WRAP, RATIO, GRAV)
        self.__omenu.grid(row=5, column=1, sticky=W)
        self.__omenu.configure(width=17)
        #
        # Buttons
        frame = self.__btnframe = Frame(frame)
        frame.grid(row=0, column=0, columnspan=2, sticky='EW')
        self.__down25 = Button(frame, text='-25',
                               command=self.__minus25)
        self.__down10 = Button(frame, text='-10',
                               command=self.__minus10)
        self.__down1 = Button(frame, text='-1',
                              command=self.__minus1)
        self.__up1 = Button(frame, text='+1',
                            command=self.__plus1)
        self.__up10 = Button(frame, text='+10',
                             command=self.__plus10)
        self.__up25 = Button(frame, text='+25',
                             command=self.__plus25)
        self.__down25.pack(expand=YES, fill=X, side=LEFT)
        self.__down10.pack(expand=YES, fill=X, side=LEFT)
        self.__down1.pack(expand=YES, fill=X, side=LEFT)
        self.__up1.pack(expand=YES, fill=X, side=LEFT)
        self.__up10.pack(expand=YES, fill=X, side=LEFT)
        self.__up25.pack(expand=YES, fill=X, side=LEFT)

    def __effect(self, event=None):
        tie = self.__rvar.get() + self.__gvar.get() + self.__bvar.get()
        if tie in (0, 1, 2, 4):
            text = ''
        else:
            text = '(= %s Level)' % {3: 'Cyan',
                                     5: 'Magenta',
                                     6: 'Yellow',
                                     7: 'Grey'}[tie]
        self.__l2.configure(text=text)

    def __quit(self, event=None):
        self.__root.quit()

    def withdraw(self, event=None):
        self.__root.withdraw()

    def deiconify(self, event=None):
        self.__root.deiconify()

    def __minus25(self, event=None):
        self.__delta(-25)

    def __minus10(self, event=None):
        self.__delta(-10)

    def __minus1(self, event=None):
        self.__delta(-1)

    def __plus1(self, event=None):
        self.__delta(1)

    def __plus10(self, event=None):
        self.__delta(10)

    def __plus25(self, event=None):
        self.__delta(25)

    def __delta(self, delta):
        tie = []
        if self.__rvar.get():
            red = self.__red + delta
            tie.append(red)
        else:
            red = self.__red
        if self.__gvar.get():
            green = self.__green + delta
            tie.append(green)
        else:
            green = self.__green
        if self.__bvar.get():
            blue = self.__blue + delta
            tie.append(blue)
        else:
            blue = self.__blue
        # now apply at boundary behavior
        atbound = self.__boundvar.get()
        if atbound == STOP:
            if red < 0 or green < 0 or blue < 0 or \
               red > 255 or green > 255 or blue > 255:
                # then
                red, green, blue = self.__red, self.__green, self.__blue
        elif atbound == WRAP or (atbound == RATIO and len(tie) < 2):
            if red < 0:
                red += 256
            if green < 0:
                green += 256
            if blue < 0:
                blue += 256
            if red > 255:
                red -= 256
            if green > 255:
                green -= 256
            if blue > 255:
                blue -= 256
        elif atbound == RATIO:
            # for when 2 or 3 colors are tied together
            dir = 0
            for c in tie:
                if c < 0:
                    dir = -1
                elif c > 255:
                    dir = 1
            if dir == -1:
                delta = max(tie)
                if self.__rvar.get():
                    red = red + 255 - delta
                if self.__gvar.get():
                    green = green + 255 - delta
                if self.__bvar.get():
                    blue = blue + 255 - delta
            elif dir == 1:
                delta = min(tie)
                if self.__rvar.get():
                    red = red - delta
                if self.__gvar.get():
                    green = green - delta
                if self.__bvar.get():
                    blue = blue - delta
        elif atbound == GRAV:
            if red < 0:
                red = 0
            if green < 0:
                green = 0
            if blue < 0:
                blue = 0
            if red > 255:
                red = 255
            if green > 255:
                green = 255
            if blue > 255:
                blue = 255
        self.__sb.update_views(red, green, blue)
        self.__root.update_idletasks()

    def update_yourself(self, red, green, blue):
        self.__red = red
        self.__green = green
        self.__blue = blue

    def save_options(self, optiondb):
        optiondb['RSLIDER'] = self.__rvar.get()
        optiondb['GSLIDER'] = self.__gvar.get()
        optiondb['BSLIDER'] = self.__bvar.get()
        optiondb['ATBOUND'] = self.__boundvar.get()
