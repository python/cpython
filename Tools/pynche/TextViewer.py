"""TextViewer class.

The TextViewer allows you to see how the selected color would affect various
characteristics of a Tk text widget.  This is an output viewer only.

In the top part of the window is a standard text widget with some sample text
in it.  You are free to edit this text in any way you want (TBD: allow you to
change font characteristics).  If you want changes in other viewers to update
text characteristics, turn on Track color changes.

To select which characteristic tracks the change, select one of the radio
buttons in the window below.  Text foreground and background affect the text
in the window above.  The Selection is what you see when you click the middle
button and drag it through some text.  The Insertion is the insertion cursor
in the text window (which only has a background).
"""

from Tkinter import *
import ColorDB

class TextViewer:
    def __init__(self, switchboard, master=None):
        self.__sb = switchboard
        optiondb = switchboard.optiondb()
        root = self.__root = Toplevel(master, class_='Pynche')
        root.protocol('WM_DELETE_WINDOW', self.withdraw)
        root.title('Pynche Text Window')
        root.iconname('Pynche Text Window')
        root.bind('<Alt-q>', self.__quit)
        root.bind('<Alt-Q>', self.__quit)
        root.bind('<Alt-w>', self.withdraw)
        root.bind('<Alt-W>', self.withdraw)
        #
        # create the text widget
        #
        self.__text = Text(root, relief=SUNKEN,
                           background=optiondb.get('TEXTBG', 'black'),
                           foreground=optiondb.get('TEXTFG', 'white'),
                           width=35, height=15)
        sfg = optiondb.get('TEXT_SFG')
        if sfg:
            self.__text.configure(selectforeground=sfg)
        sbg = optiondb.get('TEXT_SBG')
        if sbg:
            self.__text.configure(selectbackground=sbg)
        ibg = optiondb.get('TEXT_IBG')
        if ibg:
            self.__text.configure(insertbackground=ibg)
        self.__text.pack()
        self.__text.insert(0.0, optiondb.get('TEXT', '''\
Insert some stuff here and play
with the buttons below to see
how the colors interact in
textual displays.

See how the selection can also
be affected by tickling the buttons
and choosing a color.'''))
        insert = optiondb.get('TEXTINS')
        if insert:
            self.__text.mark_set(INSERT, insert)
        try:
            start, end = optiondb.get('TEXTSEL', (6.0, END))
            self.__text.tag_add(SEL, start, end)
        except ValueError:
            # selection wasn't set
            pass
        self.__text.focus_set()
        #
        # variables
        self.__trackp = BooleanVar()
        self.__trackp.set(optiondb.get('TRACKP', 0))
        self.__which = IntVar()
        self.__which.set(optiondb.get('WHICH', 0))
        #
        # track toggle
        self.__t = Checkbutton(root, text='Track color changes',
                               variable=self.__trackp,
                               relief=GROOVE,
                               command=self.__toggletrack)
        self.__t.pack(fill=X, expand=YES)
        frame = self.__frame = Frame(root)
        frame.pack()
        #
        # labels
        self.__labels = []
        row = 2
        for text in ('Text:', 'Selection:', 'Insertion:'):
            l = Label(frame, text=text)
            l.grid(row=row, column=0, sticky=E)
            self.__labels.append(l)
            row = row + 1
        col = 1
        for text in ('Foreground', 'Background'):
            l = Label(frame, text=text)
            l.grid(row=1, column=col)
            self.__labels.append(l)
            col = col + 1
        #
        # radios
        self.__radios = []
        val = 0
        for col in (1, 2):
            for row in (2, 3, 4):
                # there is no insertforeground option
                if row==4 and col==1:
                    continue
                r = Radiobutton(frame, variable=self.__which,
                                value=(row-2)*2 + col-1)
                r.grid(row=row, column=col)
                self.__radios.append(r)
        self.__toggletrack()

    def __quit(self, event=None):
        self.__root.quit()

    def withdraw(self, event=None):
        self.__root.withdraw()

    def deiconify(self, event=None):
        self.__root.deiconify()

    def __forceupdate(self, event=None):
        self.__sb.update_views_current()

    def __toggletrack(self, event=None):
        if self.__trackp.get():
            state = NORMAL
            fg = self.__radios[0]['foreground']
        else:
            state = DISABLED
            fg = self.__radios[0]['disabledforeground']
        for r in self.__radios:
            r.configure(state=state)
        for l in self.__labels:
            l.configure(foreground=fg)

    def update_yourself(self, red, green, blue):
        if self.__trackp.get():
            colorname = ColorDB.triplet_to_rrggbb((red, green, blue))
            which = self.__which.get()
            if which == 0:
                self.__text.configure(foreground=colorname)
            elif which == 1:
                self.__text.configure(background=colorname)
            elif which == 2:
                self.__text.configure(selectforeground=colorname)
            elif which == 3:
                self.__text.configure(selectbackground=colorname)
            elif which == 5:
                self.__text.configure(insertbackground=colorname)

    def save_options(self, optiondb):
        optiondb['TRACKP'] = self.__trackp.get()
        optiondb['WHICH'] = self.__which.get()
        optiondb['TEXT'] = self.__text.get(0.0, 'end - 1c')
        optiondb['TEXTSEL'] = self.__text.tag_ranges(SEL)[0:2]
        optiondb['TEXTINS'] = self.__text.index(INSERT)
        optiondb['TEXTFG'] = self.__text['foreground']
        optiondb['TEXTBG'] = self.__text['background']
        optiondb['TEXT_SFG'] = self.__text['selectforeground']
        optiondb['TEXT_SBG'] = self.__text['selectbackground']
        optiondb['TEXT_IBG'] = self.__text['insertbackground']
