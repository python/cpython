import sys
from Tkinter import *
from pynche import __version__
import ColorDB

class ListViewer:
    def __init__(self, switchboard, parent=None):
        self.__sb = switchboard
        self.__lastbox = None
        root = self.__root = Toplevel(parent, class_='Pynche')
        root.protocol('WM_DELETE_WINDOW', self.__withdraw)
        root.title('Pynche %s' % __version__)
        root.iconname('Pynche Color List')
        root.bind('<Alt-q>', self.__quit)
        root.bind('<Alt-Q>', self.__quit)
        #
        # create the canvas which holds everything, and its scrollbar
        #
        frame = self.__frame = Frame(root)
        frame.pack()
        canvas = self.__canvas = Canvas(frame, width=160, height=300)
        self.__scrollbar = Scrollbar(frame)
        self.__scrollbar.pack(fill=Y, side=RIGHT)
        canvas.pack(fill=BOTH, expand=1)
        canvas.configure(yscrollcommand=(self.__scrollbar, 'set'))
        self.__scrollbar.configure(command=(canvas, 'yview'))
        #
        # create all the buttons
        colordb = switchboard.colordb()
        row = 0
        names = colordb.all_names()
        names.sort()
        widest = 0
        bboxes = self.__bboxes = []
        for name in names:
            exactcolor = ColorDB.triplet_to_rrggbb(colordb.find_byname(name))
            canvas.create_rectangle(5, row*20 + 5,
                                    20, row*20 + 20,
                                    fill=exactcolor)
            textid = canvas.create_text(25, row*20 + 13,
                                        text=name,
                                        anchor=W)
            x1, y1, textend, y2 = canvas.bbox(textid)
            boxid = canvas.create_rectangle(3, row*20+3,
                                            textend+3, row*20 + 23,
                                            outline='',
                                            tags=(exactcolor,))
            canvas.bind('<ButtonRelease>', self.__onrelease)
            bboxes.append(boxid)
            if textend+3 > widest:
                widest = textend+3
            row = row + 1
        canvheight = (row-1)*20 + 25
        canvas.config(scrollregion=(0, 0, 150, canvheight))
        for box in bboxes:
            x1, y1, x2, y2 = canvas.coords(box)
            canvas.coords(box, x1, y1, widest, y2)

    def __onrelease(self, event=None):
        canvas = self.__canvas
        # find the current box
        x = canvas.canvasx(event.x)
        y = canvas.canvasy(event.y)
        ids = canvas.find_overlapping(x, y, x, y)
        for boxid in ids:
            if boxid in self.__bboxes:
                break
        else:
##            print 'No box found!'
            return
        tags = self.__canvas.gettags(boxid)
        for t in tags:
            if t[0] == '#':
                break
        else:
##            print 'No color tag found!'
            return
        red, green, blue = ColorDB.rrggbb_to_triplet(t)
        self.__sb.update_views(red, green, blue)

    def __quit(self, event=None):
        sys.exit(0)

    def __withdraw(self, event=None):
        self.__root.withdraw()

    def deiconify(self, event=None):
        self.__root.deiconify()

    def update_yourself(self, red, green, blue):
        canvas = self.__canvas
        # turn off the last box
        if self.__lastbox:
            canvas.itemconfigure(self.__lastbox, outline='')
        # turn on the current box
        colortag = ColorDB.triplet_to_rrggbb((red, green, blue))
        canvas.itemconfigure(colortag, outline='black')
        self.__lastbox = colortag
