from Tkinter import *
from Dialog import Dialog

# this shows how to create a new window with a button in it
# that can create new windows

class Test(Frame):
    def printit(self):
        print("hi")

    def makeWindow(self):
        """Create a top-level dialog with some buttons.

        This uses the Dialog class, which is a wrapper around the Tcl/Tk
        tk_dialog script.  The function returns 0 if the user clicks 'yes'
        or 1 if the user clicks 'no'.
        """
        # the parameters to this call are as follows:
        d = Dialog(
            self,                       ## name of a toplevel window
            title="fred the dialog box",## title on the window
            text="click on a choice",   ## message to appear in window
            bitmap="info",              ## bitmap (if any) to appear;
                                        ## if none, use ""
            #     legal values here are:
            #      string      what it looks like
            #      ----------------------------------------------
            #      error       a circle with a slash through it
            #      grey25      grey square
            #      grey50      darker grey square
            #      hourglass   use for "wait.."
            #      info        a large, lower case "i"
            #      questhead   a human head with a "?" in it
            #      question    a large "?"
            #      warning     a large "!"
            #        @fname    X bitmap where fname is the path to the file
            #
            default=0,    # the index of the default button choice.
                          # hitting return selects this
            strings=("yes", "no"))
                          # values of the 'strings' key are the labels for the
                          # buttons that appear left to right in the dialog box
        return d.num


    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=LEFT, fill=BOTH)

        # a hello button
        self.hi_there = Button(self, text='Make a New Window',
                               command=self.makeWindow)
        self.hi_there.pack(side=LEFT)


    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.windownum = 0
        self.createWidgets()

test = Test()
test.mainloop()
