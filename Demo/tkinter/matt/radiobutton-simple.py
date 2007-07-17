from Tkinter import *

# This is a demo program that shows how to
# create radio buttons and how to get other widgets to
# share the information in a radio button.
#
# There are other ways of doing this too, but
# the "variable" option of radiobuttons seems to be the easiest.
#
# note how each button has a value it sets the variable to as it gets hit.


class Test(Frame):
    def printit(self):
        print("hi")

    def createWidgets(self):

        self.flavor = StringVar()
        self.flavor.set("chocolate")

        self.radioframe = Frame(self)
        self.radioframe.pack()

        # 'text' is the label
        # 'variable' is the name of the variable that all these radio buttons share
        # 'value' is the value this variable takes on when the radio button is selected
        # 'anchor' makes the text appear left justified (default is centered. ick)
        self.radioframe.choc = Radiobutton(
            self.radioframe, text="Chocolate Flavor",
            variable=self.flavor, value="chocolate",
            anchor=W)
        self.radioframe.choc.pack(fill=X)

        self.radioframe.straw = Radiobutton(
            self.radioframe, text="Strawberry Flavor",
            variable=self.flavor, value="strawberry",
            anchor=W)
        self.radioframe.straw.pack(fill=X)

        self.radioframe.lemon = Radiobutton(
            self.radioframe, text="Lemon Flavor",
            variable=self.flavor, value="lemon",
            anchor=W)
        self.radioframe.lemon.pack(fill=X)

        # this is a text entry that lets you type in the name of a flavor too.
        self.entry = Entry(self, textvariable=self.flavor)
        self.entry.pack(fill=X)
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)


    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()

test.mainloop()
