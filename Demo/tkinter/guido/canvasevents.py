#! /usr/bin/env python

from Tkinter import *
from Canvas import Oval, Group, CanvasText


# Fix a bug in Canvas.Group as distributed in Python 1.4.  The
# distributed bind() method is broken.  This is what should be used:

class Group(Group):
    def bind(self, sequence=None, command=None):
        return self.canvas.tag_bind(self.id, sequence, command)

class Object:

    """Base class for composite graphical objects.

    Objects belong to a canvas, and can be moved around on the canvas.
    They also belong to at most one ``pile'' of objects, and can be
    transferred between piles (or removed from their pile).

    Objects have a canonical ``x, y'' position which is moved when the
    object is moved.  Where the object is relative to this position
    depends on the object; for simple objects, it may be their center.

    Objects have mouse sensitivity.  They can be clicked, dragged and
    double-clicked.  The behavior may actually be determined by the pile
    they are in.

    All instance attributes are public since the derived class may
    need them.

    """

    def __init__(self, canvas, x=0, y=0, fill='red', text='object'):
        self.canvas = canvas
        self.x = x
        self.y = y
        self.pile = None
        self.group = Group(self.canvas)
        self.createitems(fill, text)

    def __str__(self):
        return str(self.group)

    def createitems(self, fill, text):
        self.__oval = Oval(self.canvas,
                           self.x-20, self.y-10, self.x+20, self.y+10,
                           fill=fill, width=3)
        self.group.addtag_withtag(self.__oval)
        self.__text = CanvasText(self.canvas,
                           self.x, self.y, text=text)
        self.group.addtag_withtag(self.__text)

    def moveby(self, dx, dy):
        if dx == dy == 0:
            return
        self.group.move(dx, dy)
        self.x = self.x + dx
        self.y = self.y + dy

    def moveto(self, x, y):
        self.moveby(x - self.x, y - self.y)

    def transfer(self, pile):
        if self.pile:
            self.pile.delete(self)
            self.pile = None
        self.pile = pile
        if self.pile:
            self.pile.add(self)

    def tkraise(self):
        self.group.tkraise()


class Bottom(Object):

    """An object to serve as the bottom of a pile."""

    def createitems(self, *args):
        self.__oval = Oval(self.canvas,
                           self.x-20, self.y-10, self.x+20, self.y+10,
                           fill='gray', outline='')
        self.group.addtag_withtag(self.__oval)


class Pile:

    """A group of graphical objects."""

    def __init__(self, canvas, x, y, tag=None):
        self.canvas = canvas
        self.x = x
        self.y = y
        self.objects = []
        self.bottom = Bottom(self.canvas, self.x, self.y)
        self.group = Group(self.canvas, tag=tag)
        self.group.addtag_withtag(self.bottom.group)
        self.bindhandlers()

    def bindhandlers(self):
        self.group.bind('<1>', self.clickhandler)
        self.group.bind('<Double-1>', self.doubleclickhandler)

    def add(self, object):
        self.objects.append(object)
        self.group.addtag_withtag(object.group)
        self.position(object)

    def delete(self, object):
        object.group.dtag(self.group)
        self.objects.remove(object)

    def position(self, object):
        object.tkraise()
        i = self.objects.index(object)
        object.moveto(self.x + i*4, self.y + i*8)

    def clickhandler(self, event):
        pass

    def doubleclickhandler(self, event):
        pass


class MovingPile(Pile):

    def bindhandlers(self):
        Pile.bindhandlers(self)
        self.group.bind('<B1-Motion>', self.motionhandler)
        self.group.bind('<ButtonRelease-1>', self.releasehandler)

    movethis = None

    def clickhandler(self, event):
        tags = self.canvas.gettags('current')
        for i in range(len(self.objects)):
            o = self.objects[i]
            if o.group.tag in tags:
                break
        else:
            self.movethis = None
            return
        self.movethis = self.objects[i:]
        for o in self.movethis:
            o.tkraise()
        self.lastx = event.x
        self.lasty = event.y

    doubleclickhandler = clickhandler

    def motionhandler(self, event):
        if not self.movethis:
            return
        dx = event.x - self.lastx
        dy = event.y - self.lasty
        self.lastx = event.x
        self.lasty = event.y
        for o in self.movethis:
            o.moveby(dx, dy)

    def releasehandler(self, event):
        objects = self.movethis
        if not objects:
            return
        self.movethis = None
        self.finishmove(objects)

    def finishmove(self, objects):
        for o in objects:
            self.position(o)


class Pile1(MovingPile):

    x = 50
    y = 50
    tag = 'p1'

    def __init__(self, demo):
        self.demo = demo
        MovingPile.__init__(self, self.demo.canvas, self.x, self.y, self.tag)

    def doubleclickhandler(self, event):
        try:
            o = self.objects[-1]
        except IndexError:
            return
        o.transfer(self.other())
        MovingPile.doubleclickhandler(self, event)

    def other(self):
        return self.demo.p2

    def finishmove(self, objects):
        o = objects[0]
        p = self.other()
        x, y = o.x, o.y
        if (x-p.x)**2 + (y-p.y)**2 < (x-self.x)**2 + (y-self.y)**2:
            for o in objects:
                o.transfer(p)
        else:
            MovingPile.finishmove(self, objects)

class Pile2(Pile1):

    x = 150
    y = 50
    tag = 'p2'

    def other(self):
        return self.demo.p1


class Demo:

    def __init__(self, master):
        self.master = master
        self.canvas = Canvas(master,
                             width=200, height=200,
                             background='yellow',
                             relief=SUNKEN, borderwidth=2)
        self.canvas.pack(expand=1, fill=BOTH)
        self.p1 = Pile1(self)
        self.p2 = Pile2(self)
        o1 = Object(self.canvas, fill='red', text='o1')
        o2 = Object(self.canvas, fill='green', text='o2')
        o3 = Object(self.canvas, fill='light blue', text='o3')
        o1.transfer(self.p1)
        o2.transfer(self.p1)
        o3.transfer(self.p2)


# Main function, run when invoked as a stand-alone Python program.

def main():
    root = Tk()
    demo = Demo(root)
    root.protocol('WM_DELETE_WINDOW', root.quit)
    root.mainloop()

if __name__ == '__main__':
    main()
