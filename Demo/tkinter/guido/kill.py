#!/usr/local/bin/tkpython
# Tkinter interface to Linux `ps' command.

from Tkinter import *
from string import splitfields
from string import split

class BarButton(Menubutton):
	def __init__(self, master=None, cnf={}):
		Menubutton.__init__(self, master)
		self.pack({'side': 'left'})
		self.config(cnf)
		self.menu = Menu(self, {'name': 'menu'})
		self['menu'] = self.menu		

class Kill(Frame):
	# List of (name, option, pid_column)
	format_list = [('Default', '', 0),
		       ('Long', '-l', 2),
		       ('User', '-u', 1),
		       ('Jobs', '-j', 1),
		       ('Signal', '-s', 1),
		       ('Memory', '-m', 0),
		       ('VM', '-v', 0),
		       ('Hex', '-X', 0)]
	def kill(self, selected):
		c = self.format_list[self.format.get()][2]
		pid = split(selected)[c]
		self.tk.call('exec', 'kill', '-9', pid)
		self.do_update()
	def do_update(self):
		name, option, column = self.format_list[self.format.get()]
		s = self.tk.call('exec', 'ps', '-w', option)
		list = splitfields(s, '\n')
		self.header.set(list[0])
		del list[0]
		y = self.frame.vscroll.get()[2]
		self.frame.list.delete(0, AtEnd())
		for line in list:
			self.frame.list.insert(0, line)
		self.frame.list.yview(y)
	def do_motion(self, e):
		e.widget.select_from(e.widget.nearest(e.y))
	def do_leave(self, e):
		e.widget.select_clear()
	def do_1(self, e):
		self.kill(e.widget.get(e.widget.nearest(e.y)))
	def __init__(self, master=None, cnf={}):
		Frame.__init__(self, master, cnf)
		self.pack({'expand': 'yes', 'fill': 'both'})
		self.bar = Frame(
			self,
			{'name': 'bar',
			 'relief': 'raised',
			 'bd': 2,
			 Pack: {'side': 'top',
				'fill': 'x'}})
		self.bar.file = BarButton(self.bar, {'text': 'File'})
		self.bar.file.menu.add_command(
			{'label': 'Quit', 'command': self.quit})
		self.bar.view = BarButton(self.bar, {'text': 'View'})
		self.format = IntVar(self)
		self.format.set(2)
		for num in range(len(self.format_list)):
			self.bar.view.menu.add_radiobutton(
				{'label': self.format_list[num][0], 
				 'command': self.do_update,
				 'variable': self.format,
				 'value': num})
		#self.bar.view.menu.add_separator()
		#XXX ...
		self.bar.tk_menuBar(self.bar.file, self.bar.view)
		self.frame = Frame(
			self, 
			{'relief': 'raised', 'bd': 2,
			 Pack: {'side': 'top',
				'expand': 'yes',
				'fill': 'both'}})
		self.header = StringVar(self)
		self.frame.label = Label(
			self.frame, 
			{'relief': 'flat',
			 'anchor': 'nw',
			 'borderwidth': 0,
			 'textvariable': self.header,
			 Pack: {'side': 'top', 
			 	'fill': 'x'}})
		self.frame.vscroll = Scrollbar(
			self.frame,
			{'orient': 'vertical'})
		self.frame.list = Listbox(
			self.frame, 
			{'relief': 'sunken',
			 'selectbackground': '#eed5b7',
			 'selectborderwidth': 0,
			 'yscroll': self.frame.vscroll.set})
		self.frame.vscroll['command'] = self.frame.list.yview
		self.frame.vscroll.pack({'side': 'right', 'fill': 'y'})
		self.frame.list.pack(
			{'side': 'top',
			 'expand': 'yes',
			 'fill': 'both'})
		self.update = Button(
			self,
			{'text': 'Update',
			 'command': self.do_update,
			 Pack: {'expand': 'yes',
				'fill': 'x'}})
		self.frame.list.bind('<Motion>', self.do_motion)
		self.frame.list.bind('<Leave>', self.do_leave)
		self.frame.list.bind('<1>', self.do_1)
		self.do_update()

if __name__ == '__main__':
	kill = Kill(None, {'bd': 5})
	kill.winfo_toplevel().title('Tkinter Process Killer')
	kill.winfo_toplevel().minsize(1, 1)
	kill.mainloop()

