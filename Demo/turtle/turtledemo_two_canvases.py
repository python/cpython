#!/usr/bin/env python
## DEMONSTRATES USE OF 2 CANVASES, SO CANNOT BE RUN IN DEMOVIEWER!
"""turtle example: Using TurtleScreen and RawTurtle
for drawing on two distinct canvases.
"""
from turtle import TurtleScreen, RawTurtle, TK

root = TK.Tk()
cv1 = TK.Canvas(root, width=300, height=200, bg="#ddffff")
cv2 = TK.Canvas(root, width=300, height=200, bg="#ffeeee")
cv1.pack()
cv2.pack()

s1 = TurtleScreen(cv1)
s1.bgcolor(0.85, 0.85, 1)
s2 = TurtleScreen(cv2)
s2.bgcolor(1, 0.85, 0.85)

p = RawTurtle(s1)
q = RawTurtle(s2)

p.color("red", "white")
p.width(3)
q.color("blue", "black")
q.width(3)

for t in p,q:
    t.shape("turtle")
    t.lt(36)

q.lt(180)

for i in range(5):
    for t in p, q:
        t.fd(50)
        t.lt(72)
for t in p,q:
    t.lt(54)
    t.pu()
    t.bk(50)

## Want to get some info?

print s1, s2
print p, q
print s1.turtles()
print s2.turtles()

TK.mainloop()
