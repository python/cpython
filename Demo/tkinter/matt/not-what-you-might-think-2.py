from Tkinter import *


class Test(Frame):
    def createWidgets(self):

	self.Gpanel = Frame(self, {'width': '1i', 
				   'height' : '1i',
				   'bg' : 'green'})

	# this line turns off the recalculation of geometry by masters.
	self.Gpanel.tk.call('pack', 'propagate', str(self.Gpanel), "0")

	self.Gpanel.pack({'side' : 'left'})



	# a QUIT button
	self.Gpanel.QUIT = Button(self.Gpanel, {'text': 'QUIT', 
						'fg': 'red',
						'command': self.quit})
	self.Gpanel.QUIT.pack( {'side': 'left'})

	


    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()

test.master.title('packer demo')
test.master.iconname('packer')

test.mainloop()
