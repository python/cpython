#! /usr/bin/env python

"""GUI interface to webchecker.

This works as a Grail applet too!  E.g.

  <APPLET CODE=wcgui.py NAME=CheckerWindow>
  <PARAM NAME=roots VALUE="http://<yourfavoritehost>">
  </APPLET>

Checkpoints are not (yet?) supported.

User interface:

When the 'Go' checkbutton is checked, web pages will be checked, one
at a time in the Tk 'idle' time.  The other checkbuttons determine
whether the corresponding output panel is shown.  There are six
panels:

Log        -- raw output from the checker (-v, -q affect this)
To check   -- local links discovered but not yet checked
Off site   -- links discovered that point off site
Checked    -- local links that have been checked
Bad links  -- links that failed upon checking
Details    -- details about one URL; double click on a URL in any of
              the aboce list panels (not in Log) will show that URL

XXX There ought to be a list of pages known to contain at least one
bad link.

Use your window manager's Close command to quit.

Command line options:

-m bytes  -- skip HTML pages larger than this size (default %(MAXPAGE)d)
-n        -- reports only, no checking (use with -R)
-q        -- quiet operation (also suppresses external links report)
-v        -- verbose operation; repeating -v will increase verbosity
-x        -- check external links (during report phase)

Command line arguments:

rooturl   -- URL to start checking
             (default %(DEFROOT)s)

XXX The command line options should all be GUI accessible.

XXX The roots should be a list and there should be a GUI way to add a
root.

XXX The alphabetizing of the lists makes the display very jumpy.

XXX The multipanel user interface is bogus.

"""

# ' Emacs bait


import sys
import getopt
import string
from Tkinter import *
import tktools
import webchecker
import random


def main():
    checkext = 0
    try:
	opts, args = getopt.getopt(sys.argv[1:], 'm:qvx')
    except getopt.error, msg:
	sys.stdout = sys.stderr
	print msg
	print __doc__%vars(webchecker)
	sys.exit(2)
    for o, a in opts:
	if o == '-m':
	    webchecker.maxpage = string.atoi(a)
	if o == '-q':
	    webchecker.verbose = 0
	if o == '-v':
	    webchecker.verbose = webchecker.verbose + 1
	if o == '-x':
	    checkext = 1
    root = Tk(className='Webchecker')
    root.protocol("WM_DELETE_WINDOW", root.quit)
    c = CheckerWindow(root, checkext)
    for arg in args or [webchecker.DEFROOT]:
	c.addroot(arg)
    root.mainloop()


class CheckerWindow(webchecker.Checker):

    def __init__(self, parent, checkext=0, roots=None):
	self.__parent = parent
	self.__checkext = checkext
	self.__controls = Frame(parent)
	self.__controls.pack(side=TOP, fill=X)
	self.__govar = IntVar()
	self.__go = Checkbutton(self.__controls, text="GO",
				variable=self.__govar,
				command=self.go)
	self.__go.pack(side=LEFT)
	self.__status = Label(parent, text="Status: initial", anchor=W)
	self.__status.pack(side=TOP, fill=X)
	self.__checking = Label(parent, text="Checking: none", anchor=W)
	self.__checking.pack(side=TOP, fill=X)
	self.__mp = mp = MultiPanel(parent)
	sys.stdout = self.__log = LogPanel(mp, "Log")
	self.__todo = ListPanel(mp, "To check", self.showinfo)
	self.__ext = ListPanel(mp, "Off site", self.showinfo)
	self.__done = ListPanel(mp, "Checked", self.showinfo)
	self.__bad = ListPanel(mp, "Bad links", self.showinfo)
	self.__details = LogPanel(mp, "Details")
	self.__extodo = []
	webchecker.Checker.__init__(self)
	if roots:
	    roots = string.split(roots)
	    for root in roots:
		self.addroot(root)

    def go(self):
	if self.__govar.get():
	    self.__parent.after_idle(self.dosomething)
	else:
	    self.__checking.config(text="Checking: none")

    def dosomething(self):
	if self.todo:
	    i = random.randint(0, len(self.todo)-1)
	    url = self.__todo.items[i]
	    self.__checking.config(text="Checking: "+url)
	    self.__parent.update()
	    self.dopage(url)
	elif self.__checkext and self.__extodo:
	    # XXX Should have an indication of these in the todo window...
	    i = random.randint(0, len(self.__extodo)-1)
	    url = self.__extodo[i]
	    del self.__extodo[i]
	    self.__checking.config(text="Checking: "+url)
	    self.__parent.update()
	    self.checkextpage(url)
	else:
	    self.__govar.set(0)
	self.go()

    def showinfo(self, url):
	d = self.__details
	d.clear()
	d.write("URL: %s\n" % url)
	if self.bad.has_key(url):
	    d.write("Error: %s\n" % str(self.bad[url]))
	if self.done.has_key(url):
	    d.write("Status: done\n")
	    o = self.done[url]
	elif self.todo.has_key(url):
	    d.write("Status: todo\n")
	    o = self.todo[url]
	elif self.ext.has_key(url):
	    d.write("Status: ext\n")
	    o = self.ext[url]
	else:
	    d.write("Status: unknown (!)\n")
	    o = []
	if url in self.roots:
	    d.write("This is a root URL\n")
	for source, rawlink in o:
	    d.write("Origin: %s" % source)
	    if rawlink != url:
		d.write(" (%s)" % rawlink)
	    d.write("\n")
	self.__mp.showpanel("Details")

    def newstatus(self):
	self.__status.config(text="Status: "+self.status()[1:-1])
	self.__parent.update()

    def setbad(self, url, msg):
	webchecker.Checker.setbad(self, url, msg)
	self.__bad.insert(url)
	self.newstatus()

    def setgood(self, url):
	webchecker.Checker.setgood(self, url)
	self.__bad.remove(url)
	self.newstatus()

    def newextlink(self, url, origin):
	webchecker.Checker.newextlink(self, url, origin)
	self.__extodo.append(url)
	self.__ext.insert(url)
	self.newstatus()

    def newintlink(self, url, origin):
	webchecker.Checker.newintlink(self, url, origin)
	if self.done.has_key(url):
	    self.__done.insert(url)
	elif self.todo.has_key(url):
	    self.__todo.insert(url)
	self.newstatus()

    def markdone(self, url):
	webchecker.Checker.markdone(self, url)
	self.__done.insert(url)
	self.__todo.remove(url)
	self.newstatus()


class ListPanel:

    def __init__(self, mp, name, showinfo=None):
	self.mp = mp
	self.name = name
	self.showinfo = showinfo
	self.panel = mp.addpanel(name)
	self.list, self.frame = tktools.make_list_box(
	    self.panel, width=60, height=5)
	self.list.config(exportselection=0)
	if showinfo:
	    self.list.bind('<Double-Button-1>', self.event)
	self.items = []

    def event(self, dummy):
	l = self.list.curselection()
	if not l: return
	self.showinfo(self.list.get(string.atoi(l[0])))

    def insert(self, url):
	if url not in self.items:
	    if not self.items:
		self.mp.showpanel(self.name)
	    # (I tried sorting alphabetically, but the display is too jumpy)
	    i = len(self.items)
	    self.list.insert(i, url)
	    self.list.select_clear(0, END)
	    self.list.select_set(i)
	    self.list.yview(i)
	    self.items.insert(i, url)

    def remove(self, url):
	try:
	    i = self.items.index(url)
	except (ValueError, IndexError):
	    pass
	else:
	    self.list.delete(i)
	    self.list.select_clear(i)
	    del self.items[i]
	    if not self.items:
		self.mp.hidepanel(self.name)


class LogPanel:

    def __init__(self, mp, name):
	self.mp = mp
	self.name = name
	self.panel = mp.addpanel(name)
	self.text, self.frame = tktools.make_text_box(self.panel, height=10)
	self.text.config(wrap=NONE)

    def clear(self):
	self.text.delete("1.0", END)
	self.text.yview("1.0")

    def write(self, s):
	self.text.insert(END, s)
	self.text.yview(END)
	self.panel.update()


class MultiPanel:

    def __init__(self, parent):
	self.parent = parent
	self.frame = Frame(self.parent)
	self.frame.pack(expand=1, fill=BOTH)
	self.topframe = Frame(self.frame, borderwidth=2, relief=RAISED)
	self.topframe.pack(fill=X)
	self.botframe = Frame(self.frame)
	self.botframe.pack(expand=1, fill=BOTH)
	self.panelnames = []
	self.panels = {}

    def addpanel(self, name, on=0):
	v = StringVar()
	if on:
	    v.set(name)
	else:
	    v.set("")
	check = Checkbutton(self.topframe, text=name,
			    offvalue="", onvalue=name, variable=v,
			    command=self.checkpanel)
	check.pack(side=LEFT)
	panel = Frame(self.botframe)
	label = Label(panel, text=name, borderwidth=2, relief=RAISED, anchor=W)
	label.pack(side=TOP, fill=X)
	t = v, check, panel
	self.panelnames.append(name)
	self.panels[name] = t
	if on:
	    panel.pack(expand=1, fill=BOTH)
	return panel

    def showpanel(self, name):
	v, check, panel = self.panels[name]
	v.set(name)
	panel.pack(expand=1, fill=BOTH)

    def hidepanel(self, name):
	v, check, panel = self.panels[name]
	v.set("")
	panel.pack_forget()

    def checkpanel(self):
	for name in self.panelnames:
	    v, check, panel = self.panels[name]
	    panel.pack_forget()
	for name in self.panelnames:
	    v, check, panel = self.panels[name]
	    if v.get():
		panel.pack(expand=1, fill=BOTH)


if __name__ == '__main__':
    main()
