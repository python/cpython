#! /usr/bin/env python

"""Tkinter-based GUI for websucker.

Easy use: type or paste source URL and destination directory in
their respective text boxes, click GO or hit return, and presto.
"""

from Tkinter import *
import Tkinter
import websucker
import sys
import os
import threading
import Queue
import time

VERBOSE = 2


try:
    class Canceled(Exception):
        "Exception used to cancel run()."
except (NameError, TypeError):
    Canceled = __name__ + ".Canceled"


class SuckerThread(websucker.Sucker):

    stopit = 0
    savedir = None
    rootdir = None

    def __init__(self, msgq):
        self.msgq = msgq
        websucker.Sucker.__init__(self)
        self.setflags(verbose=VERBOSE)
        self.urlopener.addheaders = [
            ('User-agent', 'websucker/%s' % websucker.__version__),
        ]

    def message(self, format, *args):
        if args:
            format = format%args
        ##print format
        self.msgq.put(format)

    def run1(self, url):
        try:
            try:
                self.reset()
                self.addroot(url)
                self.run()
            except Canceled:
                self.message("[canceled]")
            else:
                self.message("[done]")
        finally:
            self.msgq.put(None)

    def savefile(self, text, path):
        if self.stopit:
            raise Canceled
        websucker.Sucker.savefile(self, text, path)

    def getpage(self, url):
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

    def XXXaddrobot(self, *args):
        pass

    def XXXisallowed(self, *args):
        return 1


class App:

    sucker = None
    msgq = None

    def __init__(self, top):
        self.top = top
        top.columnconfigure(99, weight=1)
        self.url_label = Label(top, text="URL:")
        self.url_label.grid(row=0, column=0, sticky='e')
        self.url_entry = Entry(top, width=60, exportselection=0)
        self.url_entry.grid(row=0, column=1, sticky='we',
                    columnspan=99)
        self.url_entry.focus_set()
        self.url_entry.bind("<Key-Return>", self.go)
        self.dir_label = Label(top, text="Directory:")
        self.dir_label.grid(row=1, column=0, sticky='e')
        self.dir_entry = Entry(top)
        self.dir_entry.grid(row=1, column=1, sticky='we',
                    columnspan=99)
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
        self.top.update_idletasks()
        self.top.grid_propagate(0)

    def message(self, text, *args):
        if args:
            text = text % args
        self.status_label.config(text=text)

    def check_msgq(self):
        while not self.msgq.empty():
            msg = self.msgq.get()
            if msg is None:
                self.go_button.configure(state=NORMAL)
                self.auto_button.configure(state=NORMAL)
                self.cancel_button.configure(state=DISABLED)
                if self.sucker:
                    self.sucker.stopit = 0
                self.top.bell()
            else:
                self.message(msg)
        self.top.after(100, self.check_msgq)

    def go(self, event=None):
        if not self.msgq:
            self.msgq = Queue.Queue(0)
            self.check_msgq()
        if not self.sucker:
            self.sucker = SuckerThread(self.msgq)
        if self.sucker.stopit:
            return
        self.url_entry.selection_range(0, END)
        url = self.url_entry.get()
        url = url.strip()
        if not url:
            self.top.bell()
            self.message("[Error: No URL entered]")
            return
        self.rooturl = url
        dir = self.dir_entry.get().strip()
        if not dir:
            self.sucker.savedir = None
        else:
            self.sucker.savedir = dir
            self.sucker.rootdir = os.path.dirname(
                websucker.Sucker.savefilename(self.sucker, url))
        self.go_button.configure(state=DISABLED)
        self.auto_button.configure(state=DISABLED)
        self.cancel_button.configure(state=NORMAL)
        self.message( '[running...]')
        self.sucker.stopit = 0
        t = threading.Thread(target=self.sucker.run1, args=(url,))
        t.start()

    def cancel(self):
        if self.sucker:
            self.sucker.stopit = 1
        self.message("[canceling...]")

    def auto(self):
        tries = ['PRIMARY', 'CLIPBOARD']
        text = ""
        for t in tries:
            try:
                text = self.top.selection_get(selection=t)
            except TclError:
                continue
            text = text.strip()
            if text:
                break
        if not text:
            self.top.bell()
            self.message("[Error: clipboard is empty]")
            return
        self.url_entry.delete(0, END)
        self.url_entry.insert(0, text)
        self.go()


class AppArray:

    def __init__(self, top=None):
        if not top:
            top = Tk()
            top.title("websucker GUI")
            top.iconname("wsgui")
            top.wm_protocol('WM_DELETE_WINDOW', self.exit)
        self.top = top
        self.appframe = Frame(self.top)
        self.appframe.pack(fill='both')
        self.applist = []
        self.exit_button = Button(top, text="Exit", command=self.exit)
        self.exit_button.pack(side=RIGHT)
        self.new_button = Button(top, text="New", command=self.addsucker)
        self.new_button.pack(side=LEFT)
        self.addsucker()
        ##self.applist[0].url_entry.insert(END, "http://www.python.org/doc/essays/")

    def addsucker(self):
        self.top.geometry("")
        frame = Frame(self.appframe, borderwidth=2, relief=GROOVE)
        frame.pack(fill='x')
        app = App(frame)
        self.applist.append(app)

    done = 0

    def mainloop(self):
        while not self.done:
            time.sleep(0.1)
            self.top.update()

    def exit(self):
        for app in self.applist:
            app.cancel()
            app.message("[exiting...]")
        self.done = 1


def main():
    AppArray().mainloop()

if __name__ == '__main__':
    main()
