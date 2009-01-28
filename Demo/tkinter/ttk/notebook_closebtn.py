"""A Ttk Notebook with close buttons.

Based on an example by patthoyts, http://paste.tclers.tk/896
"""
import os
import Tkinter
import ttk

root = Tkinter.Tk()

imgdir = os.path.join(os.path.dirname(__file__), 'img')
i1 = Tkinter.PhotoImage("img_close", file=os.path.join(imgdir, 'close.gif'))
i2 = Tkinter.PhotoImage("img_closeactive",
    file=os.path.join(imgdir, 'close_active.gif'))
i3 = Tkinter.PhotoImage("img_closepressed",
    file=os.path.join(imgdir, 'close_pressed.gif'))

style = ttk.Style()

style.element_create("close", "image", "img_close",
    ("active", "pressed", "!disabled", "img_closepressed"),
    ("active", "!disabled", "img_closeactive"), border=8, sticky='')

style.layout("ButtonNotebook", [("ButtonNotebook.client", {"sticky": "nswe"})])
style.layout("ButtonNotebook.Tab", [
    ("ButtonNotebook.tab", {"sticky": "nswe", "children":
        [("ButtonNotebook.padding", {"side": "top", "sticky": "nswe",
                                     "children":
            [("ButtonNotebook.focus", {"side": "top", "sticky": "nswe",
                                       "children":
                [("ButtonNotebook.label", {"side": "left", "sticky": ''}),
                 ("ButtonNotebook.close", {"side": "left", "sticky": ''})]
            })]
        })]
    })]
)

def btn_press(event):
    x, y, widget = event.x, event.y, event.widget
    elem = widget.identify(x, y)
    index = widget.index("@%d,%d" % (x, y))

    if "close" in elem:
        widget.state(['pressed'])
        widget.pressed_index = index

def btn_release(event):
    x, y, widget = event.x, event.y, event.widget

    if not widget.instate(['pressed']):
        return

    elem =  widget.identify(x, y)
    index = widget.index("@%d,%d" % (x, y))

    if "close" in elem and widget.pressed_index == index:
        widget.forget(index)
        widget.event_generate("<<NotebookClosedTab>>")

    widget.state(["!pressed"])
    widget.pressed_index = None


root.bind_class("TNotebook", "<ButtonPress-1>", btn_press, True)
root.bind_class("TNotebook", "<ButtonRelease-1>", btn_release)

# create a ttk notebook with our custom style, and add some tabs to it
nb = ttk.Notebook(width=200, height=200, style="ButtonNotebook")
nb.pressed_index = None
f1 = Tkinter.Frame(nb, background="red")
f2 = Tkinter.Frame(nb, background="green")
f3 = Tkinter.Frame(nb, background="blue")
nb.add(f1, text='Red', padding=3)
nb.add(f2, text='Green', padding=3)
nb.add(f3, text='Blue', padding=3)
nb.pack(expand=1, fill='both')

root.mainloop()
