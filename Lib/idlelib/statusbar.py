from tkinter import Frame, Label

class MultiStatusBar(Frame):

    def __init__(self, master, **kw):
        Frame.__init__(self, master, **kw)
        self.labels = {}

    def set_label(self, name, text='', side='left', width=0):
        if name not in self.labels:
            label = Label(self, borderwidth=0, anchor='w')
            label.pack(side=side, pady=0, padx=4)
            self.labels[name] = label
        else:
            label = self.labels[name]
        if width != 0:
            label.config(width=width)
        label.config(text=text)

def _multistatus_bar(parent):
    import re
    from tkinter import Toplevel, Frame, Text, Button
    top = Toplevel(parent)
    width, height, x, y = list(map(int, re.split('[x+]', parent.geometry())))
    top.geometry("+%d+%d" %(x, y + 150))
    top.title("Test multistatus bar")
    frame = Frame(top)
    text = Text(frame)
    text.pack()
    msb = MultiStatusBar(frame)
    msb.set_label("one", "hello")
    msb.set_label("two", "world")
    msb.pack(side='bottom', fill='x')

    def change():
        msb.set_label("one", "foo")
        msb.set_label("two", "bar")

    button = Button(top, text="Update status", command=change)
    button.pack(side='bottom')
    frame.pack()

if __name__ == '__main__':
    from idlelib.idle_test.htest import run
    run(_multistatus_bar)
