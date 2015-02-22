#!/usr/bin/env python
import sys
import os

from Tkinter import *
from idlelib.Percolator import Percolator
from idlelib.ColorDelegator import ColorDelegator
from idlelib.textView import view_file

import turtle
import time

demo_dir = os.getcwd()
if "turtleDemo.py" not in os.listdir(demo_dir):
    print "Directory of turtleDemo must be current working directory!"
    print "But in your case this is", demo_dir
    sys.exit()

STARTUP = 1
READY = 2
RUNNING = 3
DONE = 4
EVENTDRIVEN = 5

menufont = ("Arial", 12, NORMAL)
btnfont = ("Arial", 12, 'bold')
txtfont = ('Lucida Console', 8, 'normal')

def getExampleEntries():
    entries1 = [entry for entry in os.listdir(demo_dir) if
                     entry.startswith("tdemo_") and
                     not entry.endswith(".pyc")]
    entries2 = []
    for entry in entries1:
        if entry.endswith(".py"):
            entries2.append(entry)
        else:
            path = os.path.join(demo_dir, entry)
            sys.path.append(path)
            subdir = [entry]
            scripts = [script for script in os.listdir(path) if
                            script.startswith("tdemo_") and
                            script.endswith(".py")]
            entries2.append(subdir+scripts)
    return entries2

help_entries = (  # (help_label,  help_file)
    ('Turtledemo help', "demohelp.txt"),
    ('About turtledemo', "about_turtledemo.txt"),
    ('About turtle module', "about_turtle.txt"),
    )

class DemoWindow(object):

    def __init__(self, filename=None):
        self.root = root = turtle._root = Tk()
        root.title('Python turtle-graphics examples')
        root.wm_protocol("WM_DELETE_WINDOW", self._destroy)

        root.grid_rowconfigure(1, weight=1)
        root.grid_columnconfigure(0, weight=1)
        root.grid_columnconfigure(1, minsize=90, weight=1)
        root.grid_columnconfigure(2, minsize=90, weight=1)
        root.grid_columnconfigure(3, minsize=90, weight=1)

        self.mBar = Frame(root, relief=RAISED, borderwidth=2)
        self.ExamplesBtn = self.makeLoadDemoMenu()
        self.OptionsBtn = self.makeHelpMenu()
        self.mBar.grid(row=0, columnspan=4, sticky='news')

        pane = PanedWindow(orient=HORIZONTAL, sashwidth=5,
                           sashrelief=SOLID, bg='#ddd')
        pane.add(self.makeTextFrame(pane))
        pane.add(self.makeGraphFrame(pane))
        pane.grid(row=1, columnspan=4, sticky='news')

        self.output_lbl = Label(root, height= 1, text=" --- ", bg="#ddf",
                                font=("Arial", 16, 'normal'), borderwidth=2,
                                relief=RIDGE)
        self.start_btn = Button(root, text=" START ", font=btnfont,
                                fg="white", disabledforeground = "#fed",
                                command=self.startDemo)
        self.stop_btn = Button(root, text=" STOP ", font=btnfont,
                               fg="white", disabledforeground = "#fed",
                               command=self.stopIt)
        self.clear_btn = Button(root, text=" CLEAR ", font=btnfont,
                                fg="white", disabledforeground="#fed",
                                command = self.clearCanvas)
        self.output_lbl.grid(row=2, column=0, sticky='news', padx=(0,5))
        self.start_btn.grid(row=2, column=1, sticky='ew')
        self.stop_btn.grid(row=2, column=2, sticky='ew')
        self.clear_btn.grid(row=2, column=3, sticky='ew')

        Percolator(self.text).insertfilter(ColorDelegator())
        self.dirty = False
        self.exitflag = False
        if filename:
            self.loadfile(filename)
        self.configGUI(NORMAL, DISABLED, DISABLED, DISABLED,
                       "Choose example from menu", "black")
        self.state = STARTUP


    def onResize(self, event):
        cwidth = self._canvas.winfo_width()
        cheight = self._canvas.winfo_height()
        self._canvas.xview_moveto(0.5*(self.canvwidth-cwidth)/self.canvwidth)
        self._canvas.yview_moveto(0.5*(self.canvheight-cheight)/self.canvheight)

    def makeTextFrame(self, root):
        self.text_frame = text_frame = Frame(root)
        self.text = text = Text(text_frame, name='text', padx=5,
                                wrap='none', width=45)

        self.vbar = vbar = Scrollbar(text_frame, name='vbar')
        vbar['command'] = text.yview
        vbar.pack(side=LEFT, fill=Y)
        self.hbar = hbar = Scrollbar(text_frame, name='hbar', orient=HORIZONTAL)
        hbar['command'] = text.xview
        hbar.pack(side=BOTTOM, fill=X)

        text['font'] = txtfont
        text['yscrollcommand'] = vbar.set
        text['xscrollcommand'] = hbar.set
        text.pack(side=LEFT, fill=BOTH, expand=1)
        return text_frame

    def makeGraphFrame(self, root):
        turtle._Screen._root = root
        self.canvwidth = 1000
        self.canvheight = 800
        turtle._Screen._canvas = self._canvas = canvas = turtle.ScrolledCanvas(
                root, 800, 600, self.canvwidth, self.canvheight)
        canvas.adjustScrolls()
        canvas._rootwindow.bind('<Configure>', self.onResize)
        canvas._canvas['borderwidth'] = 0

        self.screen = _s_ = turtle.Screen()
        turtle.TurtleScreen.__init__(_s_, _s_._canvas)
        self.scanvas = _s_._canvas
        turtle.RawTurtle.screens = [_s_]
        return canvas

    def configGUI(self, menu, start, stop, clear, txt="", color="blue"):
        self.ExamplesBtn.config(state=menu)

        self.start_btn.config(state=start,
                              bg="#d00" if start == NORMAL else "#fca")
        self.stop_btn.config(state=stop,
                             bg="#d00" if stop == NORMAL else "#fca")
        self.clear_btn.config(state=clear,
                              bg="#d00" if clear == NORMAL else"#fca")
        self.output_lbl.config(text=txt, fg=color)

    def makeLoadDemoMenu(self):
        CmdBtn = Menubutton(self.mBar, text='Examples',
                            underline=0, font=menufont)
        CmdBtn.pack(side=LEFT, padx="2m")
        CmdBtn.menu = Menu(CmdBtn)

        for entry in getExampleEntries():
            def loadexample(x):
                def emit():
                    self.loadfile(x)
                return emit
            if isinstance(entry,str):
                CmdBtn.menu.add_command(label=entry[6:-3], underline=0,
                                        font=menufont,
                                        command=loadexample(entry))
            else:
                _dir, entries = entry[0], entry[1:]
                CmdBtn.menu.choices = Menu(CmdBtn.menu)
                for e in entries:
                    CmdBtn.menu.choices.add_command(
                            label=e[6:-3], underline=0, font=menufont,
                            command = loadexample(os.path.join(_dir,e)))
                CmdBtn.menu.add_cascade(
                    label=_dir[6:], menu = CmdBtn.menu.choices, font=menufont)

        CmdBtn['menu'] = CmdBtn.menu
        return CmdBtn

    def makeHelpMenu(self):
        CmdBtn = Menubutton(self.mBar, text='Help', underline=0, font = menufont)
        CmdBtn.pack(side=LEFT, padx='2m')
        CmdBtn.menu = Menu(CmdBtn)

        for help_label, help_file in help_entries:
            def show(help_label=help_label, help_file=help_file):
                view_file(self.root, help_label, os.path.join(demo_dir, help_file))
            CmdBtn.menu.add_command(label=help_label, font=menufont, command=show)

        CmdBtn['menu'] = CmdBtn.menu
        return CmdBtn

    def refreshCanvas(self):
        if not self.dirty: return
        self.screen.clear()
        self.dirty=False

    def loadfile(self,filename):
        self.refreshCanvas()
        if os.path.exists(filename) and not os.path.isdir(filename):
            # load and display file text
            f = open(filename,'r')
            chars = f.read()
            f.close()
            self.text.delete("1.0", "end")
            self.text.insert("1.0",chars)
            direc, fname = os.path.split(filename)
            self.root.title(fname[6:-3]+" - a Python turtle graphics example")
            self.module = __import__(fname[:-3])
            self.configGUI(NORMAL, NORMAL, DISABLED, DISABLED,
                           "Press start button", "red")
            self.state = READY

    def startDemo(self):
        self.refreshCanvas()
        self.dirty = True
        turtle.TurtleScreen._RUNNING = True
        self.configGUI(DISABLED, DISABLED, NORMAL, DISABLED,
                       "demo running...", "black")
        self.screen.clear()
        self.screen.mode("standard")
        self.state = RUNNING

        try:
            result = self.module.main()
            if result == "EVENTLOOP":
                self.state = EVENTDRIVEN
            else:
                self.state = DONE
        except turtle.Terminator:
            if self.root is None:
                return
            self.state = DONE
            result = "stopped!"
        if self.state == DONE:
            self.configGUI(NORMAL, NORMAL, DISABLED, NORMAL,
                           result)
        elif self.state == EVENTDRIVEN:
            self.exitflag = True
            self.configGUI(DISABLED, DISABLED, NORMAL, DISABLED,
                           "use mouse/keys or STOP", "red")

    def clearCanvas(self):
        self.refreshCanvas()
        self.scanvas.config(cursor="")
        self.configGUI(NORMAL, NORMAL, DISABLED, DISABLED)

    def stopIt(self):
        if self.exitflag:
            self.clearCanvas()
            self.exitflag = False
            self.configGUI(NORMAL, NORMAL, DISABLED, DISABLED,
                           "STOPPED!", "red")
            turtle.TurtleScreen._RUNNING = False
        else:
            turtle.TurtleScreen._RUNNING = False

    def _destroy(self):
        turtle.TurtleScreen._RUNNING = False
        self.root.destroy()
        self.root = None
        #sys.exit()

def main():
    demo = DemoWindow()
    demo.root.mainloop()

if __name__ == '__main__':
    main()
