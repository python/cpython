#! /usr/bin/env python

"""Script to create a window with a bunch of buttons.

Once the window with the buttons is displayed on-screen, capture the image
and make a copy for each GIF image.  Use xv or similar to crop individual
buttons & giftrans to make them transparent.  xv will tell you the #value
of the background if you press button-2 over a background pixel; that should
be passed as a parameter to the -t argument of giftrans.
"""
__version__ = '$Revision$'


import sys
import Tkinter
Tk = Tkinter


def add_button(w, text):
    b = Tk.Button(w, text=text,
                  font="-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*")
    b.pack(pady=5, fill=Tk.X)

def main():
    tk = Tk.Tk()
    w = Tk.Toplevel()
    w.protocol("WM_DELETE_WINDOW", tk.quit)
    tk.withdraw()
    for word in sys.argv[1:]:
        add_button(w, word)
    w.mainloop()


if __name__ == "__main__":
    main()
