from Tkinter import *

class Tree:

	def __init__(self, master, cnf = {}):
		self.master = master
		self.outerframe = Frame(self.master,
					{'name': 'outerframe',
					 Pack: {},
					 })
		self.innerframe = Frame(self.outerframe,
					{'name': 'innerframe',
					 Pack: {'side': 'left',
						'fill': 'y'},
					 })
		self.button = Menubutton(self.innerframe,
					 {'name': 'button',
					  Pack: {},
					  })
		# menu?

	def addchild(self):
		return Tree(self.outerframe, {})
