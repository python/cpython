#!/usr/bin/env python3
import sys
import os

from tkinter import *
from idlelib.Percolator import Percolator
from idlelib.ColorDelegator import ColorDelegator
from idlelib.textView import view_file # TextViewer
from imp import reload

import turtle
import time

demo_dir = os.path.dirname(os.path.abspath(__file__))

STARTUP = 1
READY = 2
RUNNING = 3
DONE = 4
EVENTDRIVEN = 5

menufont = ("Arial", 12, NORMAL)
btnfont = ("Arial", 12, 'bold')
txtfont = ('Lucida Console', 8, 'normal')

def getExampleEntries():
    return [entry[:-3] for entry in os.listdir(demo_dir) if
            entry.endswith(".py") and entry[0] != '_']

def showDemoHelp():
    view_file(demo.root, "Help on turtleDemo",
              os.path.join(demo_dir, "demohelp.txt"))

def showAboutDemo():
    view_file(demo.root, "About turtleDemo",
              os.path.join(demo_dir, "about_turtledemo.txt"))

def showAboutTurtle():
    view_file(demo.root, "About the new turtle module.",
              os.path.join(demo_dir, "about_turtle.txt"))

class DemoWindow(object):

    def __init__(self, filename=None):   #, root=None):
        self.root = root = turtle._root = Tk()
        root.wm_protocol("WM_DELETE_WINDOW", self._destroy)

        #################
        self.mBar = Frame(root, relief=RAISED, borderwidth=2)
        self.mBar.pack(fill=X)

        self.ExamplesBtn = self.makeLoadDemoMenu()
        self.OptionsBtn = self.makeHelpMenu()
        self.mBar.tk_menuBar(self.ExamplesBtn, self.OptionsBtn) #, QuitBtn)

        root.title('Python turtle-graphics examples')
        #################
        self.left_frame = left_frame = Frame(root)
        self.text_frame = text_frame = Frame(left_frame)
        self.vbar = vbar =Scrollbar(text_frame, name='vbar')
        self.text = text = Text(text_frame,
                                name='text', padx=5, wrap='none',
                                width=45)
        vbar['command'] = text.yview
        vbar.pack(side=LEFT, fill=Y)
        #####################
        self.hbar = hbar =Scrollbar(text_frame, name='hbar', orient=HORIZONTAL)
        hbar['command'] = text.xview
        hbar.pack(side=BOTTOM, fill=X)
        #####################
        text['yscrollcommand'] = vbar.set
        text.config(font=txtfont)
        text.config(xscrollcommand=hbar.set)
        text.pack(side=LEFT, fill=Y, expand=1)
        #####################
        self.output_lbl = Label(left_frame, height= 1,text=" --- ", bg = "#ddf",
                                font = ("Arial", 16, 'normal'))
        self.output_lbl.pack(side=BOTTOM, expand=0, fill=X)
        #####################
        text_frame.pack(side=LEFT, fill=BOTH, expand=0)
        left_frame.pack(side=LEFT, fill=BOTH, expand=0)
        self.graph_frame = g_frame = Frame(root)

        turtle._Screen._root = g_frame
        turtle._Screen._canvas = turtle.ScrolledCanvas(g_frame, 800, 600, 1000, 800)
        #xturtle.Screen._canvas.pack(expand=1, fill="both")
        self.screen = _s_ = turtle.Screen()
#####
        turtle.TurtleScreen.__init__(_s_, _s_._canvas)
#####
        self.scanvas = _s_._canvas
        #xturtle.RawTurtle.canvases = [self.scanvas]
        turtle.RawTurtle.screens = [_s_]

        self.scanvas.pack(side=TOP, fill=BOTH, expand=1)

        self.btn_frame = btn_frame = Frame(g_frame, height=100)
        self.start_btn = Button(btn_frame, text=" START ", font=btnfont, fg = "white",
                                disabledforeground = "#fed", command=self.startDemo)
        self.start_btn.pack(side=LEFT, fill=X, expand=1)
        self.stop_btn = Button(btn_frame, text=" STOP ",  font=btnfont, fg = "white",
                                disabledforeground = "#fed", command = self.stopIt)
        self.stop_btn.pack(side=LEFT, fill=X, expand=1)
        self.clear_btn = Button(btn_frame, text=" CLEAR ",  font=btnfont, fg = "white",
                                disabledforeground = "#fed", command = self.clearCanvas)
        self.clear_btn.pack(side=LEFT, fill=X, expand=1)

        self.btn_frame.pack(side=TOP, fill=BOTH, expand=0)
        self.graph_frame.pack(side=TOP, fill=BOTH, expand=1)

        Percolator(text).insertfilter(ColorDelegator())
        self.dirty = False
        self.exitflag = False
        if filename:
            self.loadfile(filename)
        self.configGUI(NORMAL, DISABLED, DISABLED, DISABLED,
                       "Choose example from menu", "black")
        self.state = STARTUP

    def _destroy(self):
        self.root.destroy()
        sys.exit()

    def configGUI(self, menu, start, stop, clear, txt="", color="blue"):
        self.ExamplesBtn.config(state=menu)

        self.start_btn.config(state=start)
        if start == NORMAL:
            self.start_btn.config(bg="#d00")
        else:
            self.start_btn.config(bg="#fca")

        self.stop_btn.config(state=stop)
        if stop == NORMAL:
            self.stop_btn.config(bg="#d00")
        else:
            self.stop_btn.config(bg="#fca")
        self.clear_btn.config(state=clear)

        self.clear_btn.config(state=clear)
        if clear == NORMAL:
            self.clear_btn.config(bg="#d00")
        else:
            self.clear_btn.config(bg="#fca")

        self.output_lbl.config(text=txt, fg=color)


    def makeLoadDemoMenu(self):
        CmdBtn = Menubutton(self.mBar, text='Examples', underline=0, font=menufont)
        CmdBtn.pack(side=LEFT, padx="2m")
        CmdBtn.menu = Menu(CmdBtn)

        for entry in getExampleEntries():
            def loadexample(x):
                def emit():
                    self.loadfile(x)
                return emit
            CmdBtn.menu.add_command(label=entry, underline=0,
                                    font=menufont, command=loadexample(entry))

        CmdBtn['menu'] = CmdBtn.menu
        return CmdBtn

    def makeHelpMenu(self):
        CmdBtn = Menubutton(self.mBar, text='Help', underline=0, font=menufont)
        CmdBtn.pack(side=LEFT, padx='2m')
        CmdBtn.menu = Menu(CmdBtn)

        CmdBtn.menu.add_command(label='About turtle.py', font=menufont,
                                command=showAboutTurtle)
        CmdBtn.menu.add_command(label='turtleDemo - Help', font=menufont,
                                command=showDemoHelp)
        CmdBtn.menu.add_command(label='About turtleDemo', font=menufont,
                                command=showAboutDemo)

        CmdBtn['menu'] = CmdBtn.menu
        return CmdBtn

    def refreshCanvas(self):
        if not self.dirty: return
        self.screen.clear()
        #self.screen.mode("standard")
        self.dirty=False

    def loadfile(self, filename):
        self.refreshCanvas()
        modname = 'turtledemo.' + filename
        __import__(modname)
        self.module = sys.modules[modname]
        with open(self.module.__file__, 'r') as f:
            chars = f.read()
        self.text.delete("1.0", "end")
        self.text.insert("1.0", chars)
        self.root.title(filename + " - a Python turtle graphics example")
        reload(self.module)
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
        self.screen._delete("all")
        self.scanvas.config(cursor="")
        self.configGUI(NORMAL, NORMAL, DISABLED, DISABLED)

    def stopIt(self):
        if self.exitflag:
            self.clearCanvas()
            self.exitflag = False
            self.configGUI(NORMAL, NORMAL, DISABLED, DISABLED,
                           "STOPPED!", "red")
            turtle.TurtleScreen._RUNNING = False
            #print "stopIT: exitflag = True"
        else:
            turtle.TurtleScreen._RUNNING = False
            #print "stopIt: exitflag = False"

if __name__ == '__main__':
    demo = DemoWindow()
    RUN = True
    while RUN:
        try:
            #print("ENTERING mainloop")
            demo.root.mainloop()
        except AttributeError:
            #print("AttributeError!- WAIT A MOMENT!")
            time.sleep(0.3)
            print("GOING ON ..")
            demo.ckearCanvas()
        except TypeError:
            demo.screen._delete("all")
            #print("CRASH!!!- WAIT A MOMENT!")
            time.sleep(0.3)
            #print("GOING ON ..")
            demo.clearCanvas()
        except:
            print("BYE!")
            RUN = False
