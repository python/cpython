#!/usr/bin/python
"""       turtle-example-suite:

     tdemo-I_dont_like_tiltdemo.py

Demostrates
  (a) use of a tilted ellipse as
      turtle shape
  (b) stamping that shape

We can remove it, if you don't like it.
      Without using reset() ;-)
 ---------------------------------------
"""
from turtle import *
import time

def main():
    reset()
    shape("circle")
    resizemode("user")

    pu(); bk(24*18/6.283); rt(90); pd()
    tilt(45)

    pu()

    turtlesize(16,10,5)
    color("red", "violet")
    for i in range(18):
        fd(24)
        lt(20)
        stamp()
    color("red", "")
    for i in range(18):
        fd(24)
        lt(20)
        stamp()

    tilt(-15)
    turtlesize(3, 1, 4)
    color("blue", "yellow")
    for i in range(17):
        fd(24)
        lt(20)
        if i%2 == 0:
            stamp()
    time.sleep(1)
    while undobufferentries():
        undo()
    ht()
    write("OK, OVER!", align="center", font=("Courier", 18, "bold"))
    return "Done!"

if __name__=="__main__":
    msg = main()
    print msg
    mainloop()
