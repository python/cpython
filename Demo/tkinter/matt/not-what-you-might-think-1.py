from Tkinter import *


class Test(Frame):
    def createWidgets(self):

        self.Gpanel = Frame(self, width='1i', height='1i',
                            background='green')
        self.Gpanel.pack(side=LEFT)

        # a QUIT button
        self.Gpanel.QUIT = Button(self.Gpanel, text='QUIT',
                                  foreground='red',
                                  command=self.quit)
        self.Gpanel.QUIT.pack(side=LEFT)


    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()

test.master.title('packer demo')
test.master.iconname('packer')

test.mainloop()
