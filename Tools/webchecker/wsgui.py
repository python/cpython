#! /usr/bin/env python

"""Tkinter-based GUI for websucker.

Easy use: type or paste source URL and destination directory in
their respective text boxes, click GO or hit return, and presto.
"""

from Tkinter import *
import Tkinter
import string
import websucker
import sys
import os
try:
	import threading
except ImportError:
	threading = None

VERBOSE = 1


try:
	class Canceled(Exception):
		"Exception used to cancel run()."
except:
	Canceled = __name__ + ".Canceled"


class App(websucker.Sucker):

	def __init__(self, top=None):
		websucker.Sucker.__init__(self)
		self.setflags(verbose=VERBOSE)
		self.urlopener.addheaders = [
			('User-agent', 'websucker/%s' % websucker.__version__),
			##('Accept', 'text/html'),
			##('Accept', 'text/plain'),
			##('Accept', 'text/*'),
			##('Accept', 'image/gif'),
			##('Accept', 'image/jpeg'),
			##('Accept', 'image/*'),
			##('Accept', '*/*'),
		]

		if not top:
			top = Tk()
			top.title("websucker GUI")
			top.iconname("wsgui")
			top.wm_protocol('WM_DELETE_WINDOW', self.exit)
		self.top = top
		top.columnconfigure(99, weight=1)
		self.url_label = Label(top, text="URL:")
		self.url_label.grid(row=0, column=0, sticky='e')
		self.url_entry = Entry(top, width=60, exportselection=0)
		self.url_entry.grid(row=0, column=1, sticky='we',
				    columnspan=99)
		self.url_entry.focus_set()
		self.dir_label = Label(top, text="Directory:")
		self.dir_label.grid(row=1, column=0, sticky='e')
		self.dir_entry = Entry(top)
		self.dir_entry.grid(row=1, column=1, sticky='we',
				    columnspan=99)
		self.exit_button = Button(top, text="Exit", command=self.exit)
		self.exit_button.grid(row=2, column=0, sticky='w')
		self.go_button = Button(top, text="Go", command=self.go)
		self.go_button.grid(row=2, column=1, sticky='w')
		self.cancel_button = Button(top, text="Cancel",
					    command=self.cancel,
		                            state=DISABLED)
		self.cancel_button.grid(row=2, column=2, sticky='w')
		self.auto_button = Button(top, text="Paste+Go",
					  command=self.auto)
		self.auto_button.grid(row=2, column=3, sticky='w')
		self.status_label = Label(top, text="[idle]")
		self.status_label.grid(row=2, column=4, sticky='w')
		sys.stdout = self
		self.top.update_idletasks()
		self.top.grid_propagate(0)

	def mainloop(self):
		self.top.mainloop()
	
	def exit(self):
		self.stopit = 1
		self.message("[exiting...]")
		self.top.update_idletasks()
		self.top.quit()
	
	buffer = ""
	
	def write(self, text):
		self.top.update()
		if self.stopit:
			raise Canceled
		sys.stderr.write(text)
		lines = string.split(text, "\n")
		if len(lines) > 1:
			self.buffer = ""
		self.buffer = self.buffer + lines[-1]
		if string.strip(self.buffer):
			self.message(self.buffer)
	
	def message(self, text, *args):
		if args:
			text = text % args
		self.status_label.config(text=text)		
	stopit = 0

	def go(self):
		if self.stopit:
			return
		self.url_entry.selection_range(0, END)
		url = self.url_entry.get()
		url = string.strip(url)
		if not url:
			self.top.bell()
			self.message("[Error: No URL entered]")
			return
		self.rooturl = url
		dir = string.strip(self.dir_entry.get())
		if not dir:
			self.savedir = None
		else:
			self.savedir = dir
			self.rootdir = os.path.dirname(
				websucker.Sucker.savefilename(self, url))
		self.go_button.configure(state=DISABLED)
		self.auto_button.configure(state=DISABLED)
		self.cancel_button.configure(state=NORMAL)
		self.status_label['text'] = '[running...]'
		self.top.update_idletasks()
		if threading:
			t = threading.Thread(target=self.run1, args=(url,))
			t.start()
		else:
			self.run1(url)

	def run1(self, url):
		self.reset()
		self.addroot(url)
		self.stopit = 0
		try:
			try:
				self.run()
			except Canceled:
				self.message("[canceled]")
			else:
				self.message("[done]")
				self.top.bell()
		finally:
			self.go_button.configure(state=NORMAL)
			self.auto_button.configure(state=NORMAL)
			self.cancel_button.configure(state=DISABLED)
			self.stopit = 0
	
	def cancel(self):
		self.stopit = 1
		self.message("[canceling...]")
	
	def auto(self):
		tries = ['PRIMARY', 'CLIPBOARD']
		text = ""
		for t in tries:
			try:
				text = self.top.selection_get(selection=t)
			except TclError:
				continue
			text = string.strip(text)
			if text:
				break
		if not text:
			self.top.bell()
			self.message("[Error: clipboard is empty]")
			return
		self.url_entry.delete(0, END)
		self.url_entry.insert(0, text)
		self.top.update_idletasks()
		self.go()
	
	def savefile(self, text, path):
		self.top.update()
		if self.stopit:
			raise Canceled
		websucker.Sucker.savefile(self, text, path)
	
	def getpage(self, url):
		self.top.update()
		if self.stopit:
			raise Canceled
		return websucker.Sucker.getpage(self, url)
	
	def savefilename(self, url):
		path = websucker.Sucker.savefilename(self, url)
		if self.savedir:
			n = len(self.rootdir)
			if path[:n] == self.rootdir:
				path = path[n:]
				while path[:1] == os.sep:
					path = path[1:]
				path = os.path.join(self.savedir, path)
		return path


if __name__ == '__main__':
	App().mainloop()
