#! /usr/bin/env python

"""GUI interface to webchecker.

This works as a Grail applet too!  E.g.

  <APPLET CODE=wcgui.py NAME=CheckerWindow></APPLET>

Checkpoints are not (yet???  ever???) supported.

User interface:

Enter a root to check in the text entry box.  To enter more than one root, 
enter them one at a time and press <Return> for each one.

Command buttons Start, Stop and "Check one" govern the checking process in 
the obvious way.  Start and "Check one" also enter the root from the text 
entry box if one is present.  There's also a check box (enabled by default)
to decide whether actually to follow external links (since this can slow
the checking down considerably).  Finally there's a Quit button.

A series of checkbuttons determines whether the corresponding output panel 
is shown.  List panels are also automatically shown or hidden when their 
status changes between empty to non-empty.  There are six panels:

Log        -- raw output from the checker (-v, -q affect this)
To check   -- links discovered but not yet checked
Checked    -- links that have been checked
Bad links  -- links that failed upon checking
Errors     -- pages containing at least one bad link
Details    -- details about one URL; double click on a URL in any of
              the above list panels (not in Log) will show details
              for that URL

Use your window manager's Close command to quit.

Command line options:

-m bytes  -- skip HTML pages larger than this size (default %(MAXPAGE)d)
-q        -- quiet operation (also suppresses external links report)
-v        -- verbose operation; repeating -v will increase verbosity
-t root   -- specify root dir which should be treated as internal (can repeat)
-a        -- don't check name anchors

Command line arguments:

rooturl   -- URL to start checking
             (default %(DEFROOT)s)

XXX The command line options (-m, -q, -v) should be GUI accessible.

XXX The roots should be visible as a list (?).

XXX The multipanel user interface is clumsy.

"""

# ' Emacs bait


import sys
import getopt
import string
from Tkinter import *
import tktools
import webchecker
import random

# Override some for a weaker platform
if sys.platform == 'mac':
    webchecker.DEFROOT = "http://grail.cnri.reston.va.us/"
    webchecker.MAXPAGE = 50000
    webchecker.verbose = 4

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 't:m:qva')
    except getopt.error, msg:
        sys.stdout = sys.stderr
        print msg
        print __doc__%vars(webchecker)
        sys.exit(2)
    webchecker.verbose = webchecker.VERBOSE
    webchecker.nonames = webchecker.NONAMES
    webchecker.maxpage = webchecker.MAXPAGE
    extra_roots = []
    for o, a in opts:
        if o == '-m':
            webchecker.maxpage = string.atoi(a)
        if o == '-q':
            webchecker.verbose = 0
        if o == '-v':
            webchecker.verbose = webchecker.verbose + 1
        if o == '-t':
            extra_roots.append(a)
        if o == '-a':
            webchecker.nonames = not webchecker.nonames
    root = Tk(className='Webchecker')
    root.protocol("WM_DELETE_WINDOW", root.quit)
    c = CheckerWindow(root)
    c.setflags(verbose=webchecker.verbose, maxpage=webchecker.maxpage,
               nonames=webchecker.nonames)
    if args:
        for arg in args[:-1]:
            c.addroot(arg)
        c.suggestroot(args[-1])
    # Usually conditioned on whether external links
    # will be checked, but since that's not a command
    # line option, just toss them in.
    for url_root in extra_roots:
        # Make sure it's terminated by a slash,
        # so that addroot doesn't discard the last
        # directory component.
        if url_root[-1] != "/":
            url_root = url_root + "/"
        c.addroot(url_root, add_to_do = 0)
    root.mainloop()


class CheckerWindow(webchecker.Checker):

    def __init__(self, parent, root=webchecker.DEFROOT):
        self.__parent = parent

        self.__topcontrols = Frame(parent)
        self.__topcontrols.pack(side=TOP, fill=X)
        self.__label = Label(self.__topcontrols, text="Root URL:")
        self.__label.pack(side=LEFT)
        self.__rootentry = Entry(self.__topcontrols, width=60)
        self.__rootentry.pack(side=LEFT)
        self.__rootentry.bind('<Return>', self.enterroot)
        self.__rootentry.focus_set()

        self.__controls = Frame(parent)
        self.__controls.pack(side=TOP, fill=X)
        self.__running = 0
        self.__start = Button(self.__controls, text="Run", command=self.start)
        self.__start.pack(side=LEFT)
        self.__stop = Button(self.__controls, text="Stop", command=self.stop,
                             state=DISABLED)
        self.__stop.pack(side=LEFT)
        self.__step = Button(self.__controls, text="Check one",
                             command=self.step)
        self.__step.pack(side=LEFT)
        self.__cv = BooleanVar(parent)
        self.__cv.set(self.checkext)
        self.__checkext = Checkbutton(self.__controls, variable=self.__cv,
                                      command=self.update_checkext,
                                      text="Check nonlocal links",)
        self.__checkext.pack(side=LEFT)
        self.__reset = Button(self.__controls, text="Start over", command=self.reset)
        self.__reset.pack(side=LEFT)
        if __name__ == '__main__': # No Quit button under Grail!
            self.__quit = Button(self.__controls, text="Quit",
                                 command=self.__parent.quit)
            self.__quit.pack(side=RIGHT)

        self.__status = Label(parent, text="Status: initial", anchor=W)
        self.__status.pack(side=TOP, fill=X)
        self.__checking = Label(parent, text="Idle", anchor=W)
        self.__checking.pack(side=TOP, fill=X)
        self.__mp = mp = MultiPanel(parent)
        sys.stdout = self.__log = LogPanel(mp, "Log")
        self.__todo = ListPanel(mp, "To check", self, self.showinfo)
        self.__done = ListPanel(mp, "Checked", self, self.showinfo)
        self.__bad = ListPanel(mp, "Bad links", self, self.showinfo)
        self.__errors = ListPanel(mp, "Pages w/ bad links", self, self.showinfo)
        self.__details = LogPanel(mp, "Details")
        self.root_seed = None
        webchecker.Checker.__init__(self)
        if root:
            root = string.strip(str(root))
            if root:
                self.suggestroot(root)
        self.newstatus()

    def reset(self):
        webchecker.Checker.reset(self)
        for p in self.__todo, self.__done, self.__bad, self.__errors:
            p.clear()
        if self.root_seed:
            self.suggestroot(self.root_seed)

    def suggestroot(self, root):
        self.__rootentry.delete(0, END)
        self.__rootentry.insert(END, root)
        self.__rootentry.select_range(0, END)
        self.root_seed = root

    def enterroot(self, event=None):
        root = self.__rootentry.get()
        root = string.strip(root)
        if root:
            self.__checking.config(text="Adding root "+root)
            self.__checking.update_idletasks()
            self.addroot(root)
            self.__checking.config(text="Idle")
            try:
                i = self.__todo.items.index(root)
            except (ValueError, IndexError):
                pass
            else:
                self.__todo.list.select_clear(0, END)
                self.__todo.list.select_set(i)
                self.__todo.list.yview(i)
        self.__rootentry.delete(0, END)

    def start(self):
        self.__start.config(state=DISABLED, relief=SUNKEN)
        self.__stop.config(state=NORMAL)
        self.__step.config(state=DISABLED)
        self.enterroot()
        self.__running = 1
        self.go()

    def stop(self):
        self.__stop.config(state=DISABLED, relief=SUNKEN)
        self.__running = 0

    def step(self):
        self.__start.config(state=DISABLED)
        self.__step.config(state=DISABLED, relief=SUNKEN)
        self.enterroot()
        self.__running = 0
        self.dosomething()

    def go(self):
        if self.__running:
            self.__parent.after_idle(self.dosomething)
        else:
            self.__checking.config(text="Idle")
            self.__start.config(state=NORMAL, relief=RAISED)
            self.__stop.config(state=DISABLED, relief=RAISED)
            self.__step.config(state=NORMAL, relief=RAISED)

    __busy = 0

    def dosomething(self):
        if self.__busy: return
        self.__busy = 1
        if self.todo:
            l = self.__todo.selectedindices()
            if l:
                i = l[0]
            else:
                i = 0
                self.__todo.list.select_set(i)
            self.__todo.list.yview(i)
            url = self.__todo.items[i]
            self.__checking.config(text="Checking "+self.format_url(url))
            self.__parent.update()
            self.dopage(url)
        else:
            self.stop()
        self.__busy = 0
        self.go()

    def showinfo(self, url):
        d = self.__details
        d.clear()
        d.put("URL:    %s\n" % self.format_url(url))
        if self.bad.has_key(url):
            d.put("Error:  %s\n" % str(self.bad[url]))
        if url in self.roots:
            d.put("Note:   This is a root URL\n")
        if self.done.has_key(url):
            d.put("Status: checked\n")
            o = self.done[url]
        elif self.todo.has_key(url):
            d.put("Status: to check\n")
            o = self.todo[url]
        else:
            d.put("Status: unknown (!)\n")
            o = []
        if (not url[1]) and self.errors.has_key(url[0]):
            d.put("Bad links from this page:\n")
            for triple in self.errors[url[0]]:
                link, rawlink, msg = triple
                d.put("  HREF  %s" % self.format_url(link))
                if self.format_url(link) != rawlink: d.put(" (%s)" %rawlink)
                d.put("\n")
                d.put("  error %s\n" % str(msg))
        self.__mp.showpanel("Details")
        for source, rawlink in o:
            d.put("Origin: %s" % source)
            if rawlink != self.format_url(url):
                d.put(" (%s)" % rawlink)
            d.put("\n")
        d.text.yview("1.0")

    def setbad(self, url, msg):
        webchecker.Checker.setbad(self, url, msg)
        self.__bad.insert(url)
        self.newstatus()

    def setgood(self, url):
        webchecker.Checker.setgood(self, url)
        self.__bad.remove(url)
        self.newstatus()

    def newlink(self, url, origin):
        webchecker.Checker.newlink(self, url, origin)
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

    def seterror(self, url, triple):
        webchecker.Checker.seterror(self, url, triple)
        self.__errors.insert((url, ''))
        self.newstatus()

    def newstatus(self):
        self.__status.config(text="Status: "+self.status())
        self.__parent.update()

    def update_checkext(self):
        self.checkext = self.__cv.get()


class ListPanel:

    def __init__(self, mp, name, checker, showinfo=None):
        self.mp = mp
        self.name = name
        self.showinfo = showinfo
        self.checker = checker
        self.panel = mp.addpanel(name)
        self.list, self.frame = tktools.make_list_box(
            self.panel, width=60, height=5)
        self.list.config(exportselection=0)
        if showinfo:
            self.list.bind('<Double-Button-1>', self.doubleclick)
        self.items = []

    def clear(self):
        self.items = []
        self.list.delete(0, END)
        self.mp.hidepanel(self.name)

    def doubleclick(self, event):
        l = self.selectedindices()
        if l:
            self.showinfo(self.items[l[0]])

    def selectedindices(self):
        l = self.list.curselection()
        if not l: return []
        return map(string.atoi, l)

    def insert(self, url):
        if url not in self.items:
            if not self.items:
                self.mp.showpanel(self.name)
            # (I tried sorting alphabetically, but the display is too jumpy)
            i = len(self.items)
            self.list.insert(i, self.checker.format_url(url))
            self.list.yview(i)
            self.items.insert(i, url)

    def remove(self, url):
        try:
            i = self.items.index(url)
        except (ValueError, IndexError):
            pass
        else:
            was_selected = i in self.selectedindices()
            self.list.delete(i)
            del self.items[i]
            if not self.items:
                self.mp.hidepanel(self.name)
            elif was_selected:
                if i >= len(self.items):
                    i = len(self.items) - 1
                self.list.select_set(i)


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

    def put(self, s):
        self.text.insert(END, s)
        if '\n' in s:
            self.text.yview(END)

    def write(self, s):
        self.text.insert(END, s)
        if '\n' in s:
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
        v = StringVar(self.parent)
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
