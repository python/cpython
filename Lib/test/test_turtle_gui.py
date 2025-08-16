import unittest
from itertools import zip_longest
import os
from tkinter import _tkinter, PhotoImage
import time
from test import support
from test.support import import_helper
from test.support import os_helper

turtle = import_helper.import_module('turtle')

try:
    t = turtle.Turtle()
except turtle.Terminator:
    raise unittest.SkipTest("GUI required test")


def simulate_mouse_input(w, coords):
    init_pos = coords[0]
    end_pos = coords[-1]
    w.event_generate('<Enter>', x=0, y=0)
    w.event_generate('<Motion>', x=init_pos[0], y=init_pos[1])
    w.event_generate('<ButtonPress-1>', x=init_pos[0], y=init_pos[1])
    for c in coords:
        w.event_generate('<Motion>', x=c[0], y=c[1])
    w.event_generate('<Button1-ButtonRelease>', x=end_pos[0], y=end_pos[1])


square = ((0,0),(0,20),(20,20),(20,0))


class ScreenBaseTestCase(unittest.TestCase):
    def tearDown(self):
        turtle.bye()

    def test_blankimage(self):
        img = turtle.getscreen()._blankimage()
        self.assertTrue(type(img) is PhotoImage)

    def test_image(self):
        gif = os.path.dirname(os.path.abspath(__file__)) + '/imghdrdata/python.gif'
        img = turtle.getscreen()._image(gif)
        self.assertTrue(type(img) is PhotoImage)

    def test_createpoly(self):
        p = turtle.getscreen()._createpoly()
        self.assertTrue(turtle.getscreen().cv.coords(p) == [0.0, 0.0, 0.0, 0.0, 0.0, 0.0])

    def test_createline(self):
        l = turtle.getscreen()._createline()
        self.assertTrue(turtle.getscreen().cv.coords(l) == [0.0, 0.0, 0.0, 0.0])
        self.assertTrue(turtle.getscreen().cv.itemcget(l, 'capstyle') == 'round')

    def test_drawpoly(self):
        p = turtle.getscreen()._createpoly()
        coords = [(0.0, 0.0), (20.0, 0.0), (20.0, -20.0), (0.0, -20.0)]
        turtle.getscreen()._drawpoly(p, coords, fill='red')
        self.assertTrue(turtle.getscreen().cv.coords(p) == [0.0, 0.0, 20.0, 0.0, 20.0, 20.0, 0.0, 20.0])
        self.assertTrue(turtle.getscreen().cv.itemcget(p, 'fill') == 'red')

    def test_drawline(self):
        l = turtle.getscreen()._createline()
        turtle.getscreen()._drawline(l, coordlist=[(0.0, 0.0), (20.0, 0.0)], fill='red', width=5)
        self.assertTrue(turtle.getscreen().cv.coords(l) == [0.0, 0.0, 20.0, 0.0])
        self.assertTrue(turtle.getscreen().cv.itemcget(l, 'fill') == 'red')
        self.assertTrue(turtle.getscreen().cv.itemcget(l, 'width') == '5.0')

    def test_iscolorstring(self):
        self.assertTrue(turtle.getscreen()._iscolorstring('cyan'))

    def test_iscolorstring_hex(self):
        self.assertTrue(turtle.getscreen()._iscolorstring('#00ffff'))

    def test_iscolorstring_invalid_string(self):
        self.assertFalse(turtle.getscreen()._iscolorstring('#000ffff'))

    def test_get_bgcolor(self):
        self.assertTrue(turtle.getscreen()._bgcolor() == 'white')

    def test_set_bgcolor(self):
        turtle.getscreen()._bgcolor('red')
        self.assertTrue(turtle.getscreen()._bgcolor() == 'red')

    def test_write(self):
        font = ('Arial', 8, 'normal')
        item, x = turtle.getscreen()._write((0.0, 0.0), 'testing', 'center', font, 'black')

        self.assertTrue(turtle.getscreen().cv.coords(item) == [-1.0, 0.0])

    def test_screen_onclick(self):
        clicks = []

        turtle.addshape('sq', square)
        shape = turtle.Turtle(shape='sq')
        shape.turtle.screen._root.withdraw()

        click = lambda x, y: clicks.append((x, y))

        turtle.getscreen()._onclick(shape.turtle._item, click)

        simulate_mouse_input(shape.screen.cv._canvas, [(333, 317)])

        turtle.getscreen().cv._canvas.update()

        self.assertTrue(len(clicks) == 1)

    def test_onrelease(self):
        releases = []

        turtle.addshape('sq', square)
        shape = turtle.Turtle(shape='sq')
        shape.turtle.screen._root.withdraw()

        release = lambda x, y: releases.append((x, y))

        turtle.getscreen()._onrelease(shape.turtle._item, release)

        simulate_mouse_input(shape.screen.cv._canvas, [(333, 317)])
        turtle.getscreen().cv._canvas.update()

        self.assertTrue(len(releases) > 0)

    def test_ondrag(self):
        drags = []

        turtle.addshape('sq', square)
        shape = turtle.Turtle(shape='sq')
        shape.turtle.screen._root.withdraw()

        drag = lambda x, y: drags.append((x, y))

        turtle.getscreen()._ondrag(shape.turtle._item, drag)

        simulate_mouse_input(shape.screen.cv._canvas, [(333, 317), (334, 318)])
        turtle.getscreen().cv._canvas.update()

        self.assertTrue(len(drags) > 0)

    def test_onscreenclick(self):
        clicks = []

        click = lambda x, y: clicks.append((x, y))

        turtle.getscreen()._onscreenclick(click)

        simulate_mouse_input(turtle.getscreen().cv._canvas, [(333, 317)])
        turtle.getscreen().cv._canvas.update()

        self.assertTrue(len(clicks) == 1)

    def test_onkeyrelease(self):
        canvas = turtle.getscreen().cv._canvas
        releases = []

        release = lambda: releases.append('1')

        turtle.getscreen()._onkeyrelease(release, 'Up')

        turtle.getscreen()._listen()
        canvas.update()

        canvas.event_generate('<KeyRelease-Up>')
        canvas.update()

        self.assertTrue(len(releases) == 1)

    def test_onkeypress(self):
        canvas = turtle.getscreen().cv._canvas
        presses = []

        press = lambda: presses.append('1')

        turtle.getscreen()._onkeypress(press, 'Up')

        turtle.getscreen()._listen()
        canvas.update()

        canvas.event_generate('<KeyPress-Up>')
        canvas.update()

        self.assertTrue(len(presses) == 1)

    def test_ontimer(self):
        events = []

        after = lambda: events.append('')

        turtle.getscreen()._ontimer(after, 1)

        time.sleep(2)
        turtle.getscreen().cv._canvas.update()
        self.assertTrue(len(events) == 1)

    def test_ontimer_zero(self):
        events = []

        after = lambda: events.append('')

        turtle.getscreen()._ontimer(after, 0)
        turtle.getscreen().cv._canvas.update()
        time.sleep(1)
        self.assertTrue(len(events) == 1)

    def test_createimage(self):

        gif = os.path.dirname(os.path.abspath(__file__)) + '/imghdrdata/python.gif'
        img = turtle.getscreen()._image(gif)
        i = turtle.getscreen()._createimage(img)
        self.assertTrue(turtle.getscreen().cv.coords(i) == [0.0, 0.0])

    def test_drawimage(self):

        gif = os.path.dirname(os.path.abspath(__file__)) + '/imghdrdata/python.gif'
        img = turtle.getscreen()._image(gif)
        i = turtle.getscreen()._createimage(img)
        turtle.getscreen()._drawimage(i, (5.0, 5.0), img)
        turtle.getscreen().cv._canvas.update()

        self.assertTrue(turtle.getscreen().cv.coords(i) == [5.0, -5.0])

    def test_setbgpic(self):
        gif = os.path.dirname(os.path.abspath(__file__)) + '/imghdrdata/python.gif'
        img = turtle.getscreen()._image(gif)
        i = turtle.getscreen()._createimage(img)
        self.assertTrue(turtle.getscreen().cv.coords(i) == [0.0, 0.0])

    def test_pointlist(self):
        pointlist = turtle.getscreen()._pointlist(turtle.getturtle().turtle._item)
        self.assertTrue(pointlist == [(0.0, -0.0), (-9.0, 5.0), (-7.0, -0.0), (-9.0, -5.0)])

    def test_setscrollregion(self):
        turtle.getscreen()._setscrollregion(-500, -100, 500, 100)

        self.assertTrue(turtle.getscreen().cv.cget('scrollregion') == '-500 -100 500 100')

    def test_rescale(self):
        p = turtle.getscreen()._createpoly()
        coords = [(0.0, 0.0), (20.0, 0.0), (20.0, -20.0), (0.0, -20.0)]
        turtle.getscreen()._drawpoly(p, coords, fill='red')

        turtle.getscreen()._rescale(50, 50)

        self.assertTrue(turtle.getscreen().cv.coords(p) == [0.0, 0.0, 1000.0, 0.0, 1000.0, 1000.0, 0.0, 1000.0])

    def test_resize(self):
        turtle.getscreen()._resize(canvwidth=8000, canvheight=8000)

        self.assertTrue(turtle.getscreen().cv._canvas.xview() == (0.460125, 0.538625))
        self.assertTrue(turtle.getscreen().cv._canvas.yview() == (0.462625, 0.536125))

    def test_get_window_size(self):
        self.assertTrue(turtle.getscreen()._window_size() == (640, 600))
