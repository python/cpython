from Tkinter import *

# This file shows how to trap the killing of a window
# when the user uses window manager menus (typ. upper left hand corner
# menu in the decoration border).


### ******* this isn't really called -- read the comments
def my_delete_callback():
    print "whoops -- tried to delete me!"

class Test(Frame):
    def deathHandler(self, event):
        print self, "is now getting nuked. performing some save here...."

    def createWidgets(self):
        # a hello button
        self.hi_there = Button(self, text='Hello')
        self.hi_there.pack(side=LEFT)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

        ###
        ###  PREVENT WM kills from happening
        ###

        # the docs would have you do this:

#       self.master.protocol("WM_DELETE_WINDOW", my_delete_callback)

        # unfortunately, some window managers will not send this request to a window.
        # the "protocol" function seems incapable of trapping these "aggressive" window kills.
        # this line of code catches everything, tho. The window is deleted, but you have a chance
        # of cleaning up first.
        self.bind_all("<Destroy>", self.deathHandler)


test = Test()
test.mainloop()
