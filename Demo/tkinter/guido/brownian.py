# Brownian motion -- an example of a multi-threaded Tkinter program.

from Tkinter import *
import random
import threading
import time
import sys

WIDTH = 400
HEIGHT = 300
SIGMA = 10
BUZZ = 2
RADIUS = 2
LAMBDA = 10
FILL = 'red'

stop = 0                                # Set when main loop exits

def particle(canvas):
    r = RADIUS
    x = random.gauss(WIDTH/2.0, SIGMA)
    y = random.gauss(HEIGHT/2.0, SIGMA)
    p = canvas.create_oval(x-r, y-r, x+r, y+r, fill=FILL)
    while not stop:
        dx = random.gauss(0, BUZZ)
        dy = random.gauss(0, BUZZ)
        dt = random.expovariate(LAMBDA)
        try:
            canvas.move(p, dx, dy)
        except TclError:
            break
        time.sleep(dt)

def main():
    global stop
    root = Tk()
    canvas = Canvas(root, width=WIDTH, height=HEIGHT)
    canvas.pack(fill='both', expand=1)
    np = 30
    if sys.argv[1:]:
        np = int(sys.argv[1])
    for i in range(np):
        t = threading.Thread(target=particle, args=(canvas,))
        t.start()
    try:
        root.mainloop()
    finally:
        stop = 1

main()
