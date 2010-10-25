#! /usr/bin/env python3

"""Sorting algorithms visualizer using Tkinter.

This module is comprised of three ``components'':

- an array visualizer with methods that implement basic sorting
operations (compare, swap) as well as methods for ``annotating'' the
sorting algorithm (e.g. to show the pivot element);

- a number of sorting algorithms (currently quicksort, insertion sort,
selection sort and bubble sort, as well as a randomization function),
all using the array visualizer for its basic operations and with calls
to its annotation methods;

- and a ``driver'' class which can be used as a Grail applet or as a
stand-alone application.

"""


from tkinter import *
import random


XGRID = 10
YGRID = 10
WIDTH = 6


class Array:

    class Cancelled(BaseException):
        pass

    def __init__(self, master, data=None):
        self.master = master
        self.frame = Frame(self.master)
        self.frame.pack(fill=X)
        self.label = Label(self.frame)
        self.label.pack()
        self.canvas = Canvas(self.frame)
        self.canvas.pack()
        self.report = Label(self.frame)
        self.report.pack()
        self.left = self.canvas.create_line(0, 0, 0, 0)
        self.right = self.canvas.create_line(0, 0, 0, 0)
        self.pivot = self.canvas.create_line(0, 0, 0, 0)
        self.items = []
        self.size = self.maxvalue = 0
        if data:
            self.setdata(data)

    def setdata(self, data):
        olditems = self.items
        self.items = []
        for item in olditems:
            item.delete()
        self.size = len(data)
        self.maxvalue = max(data)
        self.canvas.config(width=(self.size+1)*XGRID,
                           height=(self.maxvalue+1)*YGRID)
        for i in range(self.size):
            self.items.append(ArrayItem(self, i, data[i]))
        self.reset("Sort demo, size %d" % self.size)

    speed = "normal"

    def setspeed(self, speed):
        self.speed = speed

    def destroy(self):
        self.frame.destroy()

    in_mainloop = 0
    stop_mainloop = 0

    def cancel(self):
        self.stop_mainloop = 1
        if self.in_mainloop:
            self.master.quit()

    def step(self):
        if self.in_mainloop:
            self.master.quit()

    def wait(self, msecs):
        if self.speed == "fastest":
            msecs = 0
        elif self.speed == "fast":
            msecs = msecs//10
        elif self.speed == "single-step":
            msecs = 1000000000
        if not self.stop_mainloop:
            self.master.update()
            id = self.master.after(msecs, self.master.quit)
            self.in_mainloop = 1
            self.master.mainloop()
            self.master.after_cancel(id)
            self.in_mainloop = 0
        if self.stop_mainloop:
            self.stop_mainloop = 0
            self.message("Cancelled")
            raise Array.Cancelled

    def getsize(self):
        return self.size

    def show_partition(self, first, last):
        for i in range(self.size):
            item = self.items[i]
            if first <= i < last:
                self.canvas.itemconfig(item, fill='red')
            else:
                self.canvas.itemconfig(item, fill='orange')
        self.hide_left_right_pivot()

    def hide_partition(self):
        for i in range(self.size):
            item = self.items[i]
            self.canvas.itemconfig(item, fill='red')
        self.hide_left_right_pivot()

    def show_left(self, left):
        if not 0 <= left < self.size:
            self.hide_left()
            return
        x1, y1, x2, y2 = self.items[left].position()
##      top, bot = HIRO
        self.canvas.coords(self.left, (x1 - 2, 0, x1 - 2, 9999))
        self.master.update()

    def show_right(self, right):
        if not 0 <= right < self.size:
            self.hide_right()
            return
        x1, y1, x2, y2 = self.items[right].position()
        self.canvas.coords(self.right, (x2 + 2, 0, x2 + 2, 9999))
        self.master.update()

    def hide_left_right_pivot(self):
        self.hide_left()
        self.hide_right()
        self.hide_pivot()

    def hide_left(self):
        self.canvas.coords(self.left, (0, 0, 0, 0))

    def hide_right(self):
        self.canvas.coords(self.right, (0, 0, 0, 0))

    def show_pivot(self, pivot):
        x1, y1, x2, y2 = self.items[pivot].position()
        self.canvas.coords(self.pivot, (0, y1 - 2, 9999, y1 - 2))

    def hide_pivot(self):
        self.canvas.coords(self.pivot, (0, 0, 0, 0))

    def swap(self, i, j):
        if i == j: return
        self.countswap()
        item = self.items[i]
        other = self.items[j]
        self.items[i], self.items[j] = other, item
        item.swapwith(other)

    def compare(self, i, j):
        self.countcompare()
        item = self.items[i]
        other = self.items[j]
        return item.compareto(other)

    def reset(self, msg):
        self.ncompares = 0
        self.nswaps = 0
        self.message(msg)
        self.updatereport()
        self.hide_partition()

    def message(self, msg):
        self.label.config(text=msg)

    def countswap(self):
        self.nswaps = self.nswaps + 1
        self.updatereport()

    def countcompare(self):
        self.ncompares = self.ncompares + 1
        self.updatereport()

    def updatereport(self):
        text = "%d cmps, %d swaps" % (self.ncompares, self.nswaps)
        self.report.config(text=text)


class ArrayItem:

    def __init__(self, array, index, value):
        self.array = array
        self.index = index
        self.value = value
        self.canvas = array.canvas
        x1, y1, x2, y2 = self.position()
        self.item = array.canvas.create_rectangle(x1, y1, x2, y2,
            fill='red', outline='black', width=1)
        array.canvas.tag_bind(self.item, '<Button-1>', self.mouse_down)
        array.canvas.tag_bind(self.item, '<Button1-Motion>', self.mouse_move)
        array.canvas.tag_bind(self.item, '<ButtonRelease-1>', self.mouse_up)

    def delete(self):
        item = self.item
        self.array = None
        self.item = None
        item.delete()

    def mouse_down(self, event):
        self.lastx = event.x
        self.lasty = event.y
        self.origx = event.x
        self.origy = event.y
        self.item.tkraise()

    def mouse_move(self, event):
        self.item.move(event.x - self.lastx, event.y - self.lasty)
        self.lastx = event.x
        self.lasty = event.y

    def mouse_up(self, event):
        i = self.nearestindex(event.x)
        if i >= self.array.getsize():
            i = self.array.getsize() - 1
        if i < 0:
            i = 0
        other = self.array.items[i]
        here = self.index
        self.array.items[here], self.array.items[i] = other, self
        self.index = i
        x1, y1, x2, y2 = self.position()
        self.canvas.coords(self.item, (x1, y1, x2, y2))
        other.setindex(here)

    def setindex(self, index):
        nsteps = steps(self.index, index)
        if not nsteps: return
        if self.array.speed == "fastest":
            nsteps = 0
        oldpts = self.position()
        self.index = index
        newpts = self.position()
        trajectory = interpolate(oldpts, newpts, nsteps)
        self.item.tkraise()
        for pts in trajectory:
            self.canvas.coords(self.item, pts)
            self.array.wait(50)

    def swapwith(self, other):
        nsteps = steps(self.index, other.index)
        if not nsteps: return
        if self.array.speed == "fastest":
            nsteps = 0
        myoldpts = self.position()
        otheroldpts = other.position()
        self.index, other.index = other.index, self.index
        mynewpts = self.position()
        othernewpts = other.position()
        myfill = self.canvas.itemcget(self.item, 'fill')
        otherfill = self.canvas.itemcget(other.item, 'fill')
        self.canvas.itemconfig(self.item, fill='green')
        self.canvas.itemconfig(other.item, fill='yellow')
        self.array.master.update()
        if self.array.speed == "single-step":
            self.canvas.coords(self.item, mynewpts)
            self.canvas.coords(other.item, othernewpts)
            self.array.master.update()
            self.canvas.itemconfig(self.item, fill=myfill)
            self.canvas.itemconfig(other.item, fill=otherfill)
            self.array.wait(0)
            return
        mytrajectory = interpolate(myoldpts, mynewpts, nsteps)
        othertrajectory = interpolate(otheroldpts, othernewpts, nsteps)
        if self.value > other.value:
            self.canvas.tag_raise(self.item)
            self.canvas.tag_raise(other.item)
        else:
            self.canvas.tag_raise(other.item)
            self.canvas.tag_raise(self.item)
        try:
            for i in range(len(mytrajectory)):
                mypts = mytrajectory[i]
                otherpts = othertrajectory[i]
                self.canvas.coords(self.item, mypts)
                self.canvas.coords(other.item, otherpts)
                self.array.wait(50)
        finally:
            mypts = mytrajectory[-1]
            otherpts = othertrajectory[-1]
            self.canvas.coords(self.item, mypts)
            self.canvas.coords(other.item, otherpts)
            self.canvas.itemconfig(self.item, fill=myfill)
            self.canvas.itemconfig(other.item, fill=otherfill)

    def compareto(self, other):
        myfill = self.canvas.itemcget(self.item, 'fill')
        otherfill = self.canvas.itemcget(other.item, 'fill')
        if self.value < other.value:
            myflash = 'white'
            otherflash = 'black'
            outcome = -1
        elif self.value > other.value:
            myflash = 'black'
            otherflash = 'white'
            outcome = 1
        else:
            myflash = otherflash = 'grey'
            outcome = 0
        try:
            self.canvas.itemconfig(self.item, fill=myflash)
            self.canvas.itemconfig(other.item, fill=otherflash)
            self.array.wait(500)
        finally:
            self.canvas.itemconfig(self.item, fill=myfill)
            self.canvas.itemconfig(other.item, fill=otherfill)
        return outcome

    def position(self):
        x1 = (self.index+1)*XGRID - WIDTH//2
        x2 = x1+WIDTH
        y2 = (self.array.maxvalue+1)*YGRID
        y1 = y2 - (self.value)*YGRID
        return x1, y1, x2, y2

    def nearestindex(self, x):
        return int(round(float(x)/XGRID)) - 1


# Subroutines that don't need an object

def steps(here, there):
    nsteps = abs(here - there)
    if nsteps <= 3:
        nsteps = nsteps * 3
    elif nsteps <= 5:
        nsteps = nsteps * 2
    elif nsteps > 10:
        nsteps = 10
    return nsteps

def interpolate(oldpts, newpts, n):
    if len(oldpts) != len(newpts):
        raise ValueError("can't interpolate arrays of different length")
    pts = [0]*len(oldpts)
    res = [tuple(oldpts)]
    for i in range(1, n):
        for k in range(len(pts)):
            pts[k] = oldpts[k] + (newpts[k] - oldpts[k])*i//n
        res.append(tuple(pts))
    res.append(tuple(newpts))
    return res


# Various (un)sorting algorithms

def uniform(array):
    size = array.getsize()
    array.setdata([(size+1)//2] * size)
    array.reset("Uniform data, size %d" % size)

def distinct(array):
    size = array.getsize()
    array.setdata(range(1, size+1))
    array.reset("Distinct data, size %d" % size)

def randomize(array):
    array.reset("Randomizing")
    n = array.getsize()
    for i in range(n):
        j = random.randint(0, n-1)
        array.swap(i, j)
    array.message("Randomized")

def insertionsort(array):
    size = array.getsize()
    array.reset("Insertion sort")
    for i in range(1, size):
        j = i-1
        while j >= 0:
            if array.compare(j, j+1) <= 0:
                break
            array.swap(j, j+1)
            j = j-1
    array.message("Sorted")

def selectionsort(array):
    size = array.getsize()
    array.reset("Selection sort")
    try:
        for i in range(size):
            array.show_partition(i, size)
            for j in range(i+1, size):
                if array.compare(i, j) > 0:
                    array.swap(i, j)
        array.message("Sorted")
    finally:
        array.hide_partition()

def bubblesort(array):
    size = array.getsize()
    array.reset("Bubble sort")
    for i in range(size):
        for j in range(1, size):
            if array.compare(j-1, j) > 0:
                array.swap(j-1, j)
    array.message("Sorted")

def quicksort(array):
    size = array.getsize()
    array.reset("Quicksort")
    try:
        stack = [(0, size)]
        while stack:
            first, last = stack[-1]
            del stack[-1]
            array.show_partition(first, last)
            if last-first < 5:
                array.message("Insertion sort")
                for i in range(first+1, last):
                    j = i-1
                    while j >= first:
                        if array.compare(j, j+1) <= 0:
                            break
                        array.swap(j, j+1)
                        j = j-1
                continue
            array.message("Choosing pivot")
            j, i, k = first, (first+last) // 2, last-1
            if array.compare(k, i) < 0:
                array.swap(k, i)
            if array.compare(k, j) < 0:
                array.swap(k, j)
            if array.compare(j, i) < 0:
                array.swap(j, i)
            pivot = j
            array.show_pivot(pivot)
            array.message("Pivot at left of partition")
            array.wait(1000)
            left = first
            right = last
            while 1:
                array.message("Sweep right pointer")
                right = right-1
                array.show_right(right)
                while right > first and array.compare(right, pivot) >= 0:
                    right = right-1
                    array.show_right(right)
                array.message("Sweep left pointer")
                left = left+1
                array.show_left(left)
                while left < last and array.compare(left, pivot) <= 0:
                    left = left+1
                    array.show_left(left)
                if left > right:
                    array.message("End of partition")
                    break
                array.message("Swap items")
                array.swap(left, right)
            array.message("Swap pivot back")
            array.swap(pivot, right)
            n1 = right-first
            n2 = last-left
            if n1 > 1: stack.append((first, right))
            if n2 > 1: stack.append((left, last))
        array.message("Sorted")
    finally:
        array.hide_partition()

def demosort(array):
    while 1:
        for alg in [quicksort, insertionsort, selectionsort, bubblesort]:
            randomize(array)
            alg(array)


# Sort demo class -- usable as a Grail applet

class SortDemo:

    def __init__(self, master, size=15):
        self.master = master
        self.size = size
        self.busy = 0
        self.array = Array(self.master)

        self.botframe = Frame(master)
        self.botframe.pack(side=BOTTOM)
        self.botleftframe = Frame(self.botframe)
        self.botleftframe.pack(side=LEFT, fill=Y)
        self.botrightframe = Frame(self.botframe)
        self.botrightframe.pack(side=RIGHT, fill=Y)

        self.b_qsort = Button(self.botleftframe,
                              text="Quicksort", command=self.c_qsort)
        self.b_qsort.pack(fill=X)
        self.b_isort = Button(self.botleftframe,
                              text="Insertion sort", command=self.c_isort)
        self.b_isort.pack(fill=X)
        self.b_ssort = Button(self.botleftframe,
                              text="Selection sort", command=self.c_ssort)
        self.b_ssort.pack(fill=X)
        self.b_bsort = Button(self.botleftframe,
                              text="Bubble sort", command=self.c_bsort)
        self.b_bsort.pack(fill=X)

        # Terrible hack to overcome limitation of OptionMenu...
        class MyIntVar(IntVar):
            def __init__(self, master, demo):
                self.demo = demo
                IntVar.__init__(self, master)
            def set(self, value):
                IntVar.set(self, value)
                if str(value) != '0':
                    self.demo.resize(value)

        self.v_size = MyIntVar(self.master, self)
        self.v_size.set(size)
        sizes = [1, 2, 3, 4] + list(range(5, 55, 5))
        if self.size not in sizes:
            sizes.append(self.size)
            sizes.sort()
        self.m_size = OptionMenu(self.botleftframe, self.v_size, *sizes)
        self.m_size.pack(fill=X)

        self.v_speed = StringVar(self.master)
        self.v_speed.set("normal")
        self.m_speed = OptionMenu(self.botleftframe, self.v_speed,
                                  "single-step", "normal", "fast", "fastest")
        self.m_speed.pack(fill=X)

        self.b_step = Button(self.botleftframe,
                             text="Step", command=self.c_step)
        self.b_step.pack(fill=X)

        self.b_randomize = Button(self.botrightframe,
                                  text="Randomize", command=self.c_randomize)
        self.b_randomize.pack(fill=X)
        self.b_uniform = Button(self.botrightframe,
                                  text="Uniform", command=self.c_uniform)
        self.b_uniform.pack(fill=X)
        self.b_distinct = Button(self.botrightframe,
                                  text="Distinct", command=self.c_distinct)
        self.b_distinct.pack(fill=X)
        self.b_demo = Button(self.botrightframe,
                             text="Demo", command=self.c_demo)
        self.b_demo.pack(fill=X)
        self.b_cancel = Button(self.botrightframe,
                               text="Cancel", command=self.c_cancel)
        self.b_cancel.pack(fill=X)
        self.b_cancel.config(state=DISABLED)
        self.b_quit = Button(self.botrightframe,
                             text="Quit", command=self.c_quit)
        self.b_quit.pack(fill=X)

    def resize(self, newsize):
        if self.busy:
            self.master.bell()
            return
        self.size = newsize
        self.array.setdata(range(1, self.size+1))

    def c_qsort(self):
        self.run(quicksort)

    def c_isort(self):
        self.run(insertionsort)

    def c_ssort(self):
        self.run(selectionsort)

    def c_bsort(self):
        self.run(bubblesort)

    def c_demo(self):
        self.run(demosort)

    def c_randomize(self):
        self.run(randomize)

    def c_uniform(self):
        self.run(uniform)

    def c_distinct(self):
        self.run(distinct)

    def run(self, func):
        if self.busy:
            self.master.bell()
            return
        self.busy = 1
        self.array.setspeed(self.v_speed.get())
        self.b_cancel.config(state=NORMAL)
        try:
            func(self.array)
        except Array.Cancelled:
            pass
        self.b_cancel.config(state=DISABLED)
        self.busy = 0

    def c_cancel(self):
        if not self.busy:
            self.master.bell()
            return
        self.array.cancel()

    def c_step(self):
        if not self.busy:
            self.master.bell()
            return
        self.v_speed.set("single-step")
        self.array.setspeed("single-step")
        self.array.step()

    def c_quit(self):
        if self.busy:
            self.array.cancel()
        self.master.after_idle(self.master.quit)


# Main program -- for stand-alone operation outside Grail

def main():
    root = Tk()
    demo = SortDemo(root)
    root.protocol('WM_DELETE_WINDOW', demo.c_quit)
    root.mainloop()

if __name__ == '__main__':
    main()
