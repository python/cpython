# Show how to do switchable panels.

from Tkinter import *

class App:

    def __init__(self, top=None, master=None):
        if top is None:
            if master is None:
                top = Tk()
            else:
                top = Toplevel(master)
        self.top = top
        self.buttonframe = Frame(top)
        self.buttonframe.pack()
        self.panelframe = Frame(top,  borderwidth=2, relief=GROOVE)
        self.panelframe.pack(expand=1, fill=BOTH)
        self.panels = {}
        self.curpanel = None

    def addpanel(self, name, klass):
        button = Button(self.buttonframe, text=name,
                        command=lambda self=self, name=name: self.show(name))
        button.pack(side=LEFT)
        frame = Frame(self.panelframe)
        instance = klass(frame)
        self.panels[name] = (button, frame, instance)
        if self.curpanel is None:
            self.show(name)

    def show(self, name):
        (button, frame, instance) = self.panels[name]
        if self.curpanel:
            self.curpanel.pack_forget()
        self.curpanel = frame
        frame.pack(expand=1, fill="both")

class LabelPanel:
    def __init__(self, frame):
        self.label = Label(frame, text="Hello world")
        self.label.pack()

class ButtonPanel:
    def __init__(self, frame):
        self.button = Button(frame, text="Press me")
        self.button.pack()

def main():
    app = App()
    app.addpanel("label", LabelPanel)
    app.addpanel("button", ButtonPanel)
    app.top.mainloop()

if __name__ == '__main__':
    main()
