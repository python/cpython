import os
import sys
from Tkinter import *


class HelpWindow:

    helpfile = "help.txt"
    helptitle = "Help Window"

    def __init__(self, root=None):
        if not root:
            import Tkinter
            root = Tkinter._default_root
        if root:
            self.top = top = Toplevel(root)
        else:
            self.top = top = root = Tk()

        helpfile = self.helpfile
        if not os.path.exists(helpfile):
            base = os.path.basename(self.helpfile)
            for dir in sys.path:
                fullname = os.path.join(dir, base)
                if os.path.exists(fullname):
                    helpfile = fullname
                    break
        try:
            f = open(helpfile)
            data = f.read()
            f.close()
        except IOError, msg:
            data = "Can't open the help file (%s)" % `helpfile`

        top.protocol("WM_DELETE_WINDOW", self.close_command)
        top.wm_title(self.helptitle)

        self.close_button = Button(top, text="close",
                                   command=self.close_command)
        self.close_button.pack(side="bottom")

        self.vbar = vbar = Scrollbar(top, name="vbar")
        self.text = text = Text(top)

        vbar["command"] = text.yview
        text["yscrollcommand"] = vbar.set

        vbar.pack(side="right", fill="y")
        text.pack(side="left", fill="both", expand=1)

        text.insert("1.0", data)

        text.config(state="disabled")
        text.see("1.0")

    def close_command(self):
        self.top.destroy()


def main():
    h = HelpWindow()
    h.top.mainloop()

if __name__ == "__main__":
    main()
