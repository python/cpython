# A CallTip window class for Tkinter/IDLE.
# After ToolTip.py, which uses ideas gleaned from PySol

# Used by the CallTips IDLE extension.
import os
from Tkinter import *

class CallTip:

    def __init__(self, widget):
        self.widget = widget
        self.tipwindow = None
        self.id = None
        self.x = self.y = 0

    def showtip(self, text):
        self.text = text
        if self.tipwindow or not self.text:
            return
        self.widget.see("insert")
        x, y, cx, cy = self.widget.bbox("insert")
        x = x + self.widget.winfo_rootx() + 2
        y = y + cy + self.widget.winfo_rooty()
        self.tipwindow = tw = Toplevel(self.widget)
        tw.wm_overrideredirect(1)
        tw.wm_geometry("+%d+%d" % (x, y))
        label = Label(tw, text=self.text, justify=LEFT,
                      background="#ffffe0", relief=SOLID, borderwidth=1,
                      font = self.widget['font'])
        label.pack()
                      
    def hidetip(self):
        tw = self.tipwindow
        self.tipwindow = None
        if tw:
            tw.destroy()


###############################
#
# Test Code
#
class container: # Conceptually an editor_window
    def __init__(self):
        root = Tk()
        text = self.text = Text(root)
        text.pack(side=LEFT, fill=BOTH, expand=1)
        text.insert("insert", "string.split")
        root.update()
        self.calltip = CallTip(text)

        text.event_add("<<calltip-show>>", "(")
        text.event_add("<<calltip-hide>>", ")")
        text.bind("<<calltip-show>>", self.calltip_show)
        text.bind("<<calltip-hide>>", self.calltip_hide)
        
        text.focus_set()
        # root.mainloop() # not in idle

    def calltip_show(self, event):
        self.calltip.showtip("Hello world")

    def calltip_hide(self, event):
        self.calltip.hidetip()

def main():
    # Test code
    c=container()

if __name__=='__main__':
    main()
