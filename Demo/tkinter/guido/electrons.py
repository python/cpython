#! /usr/bin/env python

# Simulate "electrons" migrating across the screen.
# An optional bitmap file in can be in the background.
#
# Usage: electrons [n [bitmapfile]]
#
# n is the number of electrons to animate; default is 30.
#
# The bitmap file can be any X11 bitmap file (look in
# /usr/include/X11/bitmaps for samples); it is displayed as the
# background of the animation.  Default is no bitmap.

from tkinter import *
import random


# The graphical interface
class Electrons:

    # Create our objects
    def __init__(self, n, bitmap = None):
        self.n = n
        self.tk = tk = Tk()
        self.canvas = c = Canvas(tk)
        c.pack()
        width, height = tk.getint(c['width']), tk.getint(c['height'])

        # Add background bitmap
        if bitmap:
            self.bitmap = c.create_bitmap(width/2, height/2,
                                          bitmap=bitmap,
                                          foreground='blue')

        self.pieces = []
        x1, y1, x2, y2 = 10,70,14,74
        for i in range(n):
            p = c.create_oval(x1, y1, x2, y2, fill='red')
            self.pieces.append(p)
            y1, y2 = y1 +2, y2 + 2
        self.tk.update()

    def random_move(self, n):
        c = self.canvas
        for p in self.pieces:
            x = random.choice(range(-2,4))
            y = random.choice(range(-3,4))
            c.move(p, x, y)
        self.tk.update()

    # Run -- allow 500 movemens
    def run(self):
        try:
            for i in range(500):
                self.random_move(self.n)
        except TclError:
            try:
                self.tk.destroy()
            except TclError:
                pass


# Main program
def main():
    import sys

    # First argument is number of electrons, default 30
    if sys.argv[1:]:
        n = int(sys.argv[1])
    else:
        n = 30

    # Second argument is bitmap file, default none
    if sys.argv[2:]:
        bitmap = sys.argv[2]
        # Reverse meaning of leading '@' compared to Tk
        if bitmap[0] == '@': bitmap = bitmap[1:]
        else: bitmap = '@' + bitmap
    else:
        bitmap = None

    # Create the graphical objects...
    h = Electrons(n, bitmap)

    # ...and run!
    h.run()


# Call main when run as script
if __name__ == '__main__':
    main()
