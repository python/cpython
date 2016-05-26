from Tkinter import *

# This example program creates a scrolling canvas, and demonstrates
# how to tie scrollbars and canvases together. The mechanism
# is analogus for listboxes and other widgets with
# "xscroll" and "yscroll" configuration options.

class Test(Frame):
    def printit(self):
        print "hi"

    def createWidgets(self):
        self.question = Label(self, text="Can Find The BLUE Square??????")
        self.question.pack()

        self.QUIT = Button(self, text='QUIT', background='red',
                           height=3, command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)
        spacer = Frame(self, height="0.25i")
        spacer.pack(side=BOTTOM)

        # notice that the scroll region (20" x 20") is larger than
        # displayed size of the widget (5" x 5")
        self.draw = Canvas(self, width="5i", height="5i",
                           background="white",
                           scrollregion=(0, 0, "20i", "20i"))

        self.draw.scrollX = Scrollbar(self, orient=HORIZONTAL)
        self.draw.scrollY = Scrollbar(self, orient=VERTICAL)

        # now tie the three together. This is standard boilerplate text
        self.draw['xscrollcommand'] = self.draw.scrollX.set
        self.draw['yscrollcommand'] = self.draw.scrollY.set
        self.draw.scrollX['command'] = self.draw.xview
        self.draw.scrollY['command'] = self.draw.yview

        # draw something. Note that the first square
        # is visible, but you need to scroll to see the second one.
        self.draw.create_rectangle(0, 0, "3.5i", "3.5i", fill="black")
        self.draw.create_rectangle("10i", "10i", "13.5i", "13.5i", fill="blue")

        # pack 'em up
        self.draw.scrollX.pack(side=BOTTOM, fill=X)
        self.draw.scrollY.pack(side=RIGHT, fill=Y)
        self.draw.pack(side=LEFT)


    def scrollCanvasX(self, *args):
        print "scrolling", args
        print self.draw.scrollX.get()


    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()

test.mainloop()
