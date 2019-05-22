import unittest
from itertools import zip_longest
from test import support
import os
from tkinter import _tkinter, PhotoImage
import time

turtle = support.import_module('turtle')


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


class ScrolledCanvasTestCase(unittest.TestCase):
    def tearDown(self):
        turtle.bye()

    def test_reset_large_canvas(self):
        t = turtle.Turtle()

        canvas = t.screen.cv

        canvas._canvas.config(width=2000, height=2000)
        canvas.reset(canvwidth=2000, canvheight=2000)

        self.assertTrue(canvas._canvas.xview() == (0.3405, 0.6545))
        self.assertTrue(canvas._canvas.yview() == (0.3505, 0.6445))

    def test_reset_small_canvas(self):
        t = turtle.Turtle()

        canvas = t.screen.cv

        canvas._canvas.config(width=200, height=200)
        canvas.reset(canvwidth=200, canvheight=200)

        self.assertTrue(canvas._canvas.xview() == (0.0, 1.0))
        self.assertTrue(canvas._canvas.yview() == (0.0, 1.0))

    def test_adjustScrolls_large_window(self):
        t = turtle.Turtle()

        canvas = t.screen.cv

        canvas.canvwidth = 2000
        canvas.canvheight = 2000

        canvas.adjustScrolls()

        self.assertTrue(canvas.hscroll.grid_info())
        self.assertTrue(canvas.vscroll.grid_info())

    def test_adjustScroll_small_window(self):
        t = turtle.Turtle()

        canvas = t.screen.cv

        canvas.canvwidth = 20
        canvas.canvheight = 20

        canvas.adjustScrolls()

        self.assertFalse(canvas.hscroll.grid_info())
        self.assertFalse(canvas.vscroll.grid_info())


class CustomTestCase(unittest.TestCase):
    def assertItemsAlmostEqual(self, value1, value2):
        for i, j in zip_longest(value1, value2):
            if isinstance(i, (tuple, list)):
                if isinstance(j, (tuple, list)):
                    self.assertItemsAlmostEqual(i, j)
                else:
                    self.fail("%s is not the same as %s", str(i), str(j))
            else:
                self.assertAlmostEqual(i, j)

    def _run_undo_calls(self, expected_results):
        while turtle.undobufferentries() > 0:
            heading, expected_pos = expected_results.pop()
            self.assertAlmostEqual(turtle.heading(), heading)
            pos = turtle.position()
            self.assertItemsAlmostEqual(pos, expected_pos)
            turtle.undo()


class TestTurtleScreen(CustomTestCase):
    def test_bgcolor(self):
        turtle.bgcolor("orange")
        self.assertEqual(turtle.bgcolor(), "orange")
        turtle.bgcolor(0.5,0,0.5)
        expected = (0.5019607843137255, 0.0, 0.5019607843137255)
        self.assertItemsAlmostEqual(turtle.bgcolor(), expected)
        turtle.bgcolor("#FFFFFF")
        expected = (1.0, 1.0, 1.0)
        self.assertItemsAlmostEqual(turtle.bgcolor(), expected)

    def test_bgpic(self):
        gif = os.path.dirname(os.path.abspath(__file__)) + '/imghdrdata/python.gif'
        turtle.bgpic(gif)
        self.assertEqual(turtle.bgpic(), gif)
        self.assertRaises(_tkinter.TclError, turtle.bgpic, "this is garbage")

    def test_mode(self):
        turtle.mode("logo")
        turtle_id = turtle.getturtle().turtle._item
        canvas = turtle.getcanvas()
        expected = [0.0, 0.0, -5.0, 9.0, 0.0, 7.0, 5.0, 9.0]
        self.assertItemsAlmostEqual(canvas.coords(turtle_id), expected)
        self.assertEqual(turtle.mode(), "logo")
        turtle.forward(10)
        self.assertItemsAlmostEqual(turtle.position(), (0, 10))
        turtle.mode("standard")
        expected = [0.0, 0.0, -9.0, -5.0, -7.0, 0.0, -9.0, 5.0]
        self.assertItemsAlmostEqual(canvas.coords(turtle_id), expected)
        self.assertEqual(turtle.mode(), "standard")
        turtle.forward(10)
        self.assertItemsAlmostEqual(turtle.position(), (10, 0))
        turtle.mode("world")
        self.assertEqual(turtle.mode(), "world")
        self.assertRaises(turtle.TurtleGraphicsError, turtle.mode, "garbage")

    def test_register_shape(self):
        turtle.register_shape("tri", ((0,0), (10,10), (-10,10)))
        try:
            turtle.shape('tri')
            self.assertEqual(turtle.getturtle().turtle.shapeIndex, 'tri')
        except turtle.TurtleGraphicsError:
            self.fail("Shape did not get registered")

    def test_colormode(self):
        turtle.colormode(255)
        self.assertEqual(turtle.colormode(), 255)
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.color,
                          *(300, 300, 300))
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.color,
                          *(.5, 1, -1))
        turtle.colormode(1.0)
        self.assertAlmostEqual(turtle.colormode(), 1.0)
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.color,
                          *(2, .5, 200))
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.color,
                          *(-1, .5, .33))
        turtle.colormode('garbage')
        self.assertAlmostEqual(turtle.colormode(), 1.0)

    def test_window_size(self):
        self.assertEqual(turtle.window_height(), 600)
        self.assertEqual(turtle.window_width(), 640)
        turtle.getscreen()._root.geometry('300x300+320+100')
        turtle.update()
        result = turtle.window_height()
        self.assertEqual(result, 300)
        result = turtle.window_width()
        self.assertEqual(turtle.window_width(), 300)

    def test_screen_clear(self):
        turtle.bgcolor('orange')
        turtle.clone()
        prev_delay = turtle.delay()
        turtle.delay(100)
        screen = turtle.getscreen()
        self.assertTrue(turtle.bgcolor() == 'orange')
        self.assertTrue(len(screen._turtles) == 2)
        self.assertTrue(turtle.delay() == 100)
        screen.clear()
        self.assertTrue(turtle.bgcolor() == 'white')
        self.assertTrue(len(screen._turtles) == 0)
        self.assertTrue(turtle.delay() == prev_delay)

    def test_tracer(self):
        turtle.tracer(3)
        turtle.forward(100)
        canvas = turtle.getcanvas()
        turtle_id = turtle.getturtle().turtle._item
        expected = [0.0, 0.0]
        self.assertItemsAlmostEqual(canvas.coords(turtle_id)[:2], expected)
        turtle.update()
        expected = [100.0, 0.0]
        self.assertItemsAlmostEqual(canvas.coords(turtle_id)[:2], expected)

    def test_setworldcoordinates(self):
        turtle.setworldcoordinates(0,0,50,50)
        self.assertEqual(turtle.mode(), 'world')

    def test_delay(self):
        turtle.delay(100)
        self.assertTrue(turtle.delay()==100)

    def test_screenszie(self):
        turtle.screensize(canvwidth=300, canvheight=300, bg='orange')
        canvas = turtle.getcanvas()
        self.assertTrue(canvas.canvwidth==300)
        self.assertTrue(canvas.canvheight==300)
        self.assertTrue(turtle.bgcolor()=='orange')

    def tearDown(self):
        turtle.bye()


class TestTurtleState(CustomTestCase):

    def test_turtle_shape(self):
        shapes = ['arrow', 'turtle', 'circle', 'square', 'triangle', 'classic']
        for shape in shapes:
            try:
                turtle.shape(shape)
                self.assertTrue(turtle.getturtle().turtle.shapeIndex == shape)
            except turtle.TurtleGraphicsError:
                self.fail("%s could not be initalized" % shape)

        # Test bad input
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.shape,
                          "akljsdlfsjfjlkds")

    def test_shapetransform(self):
        # Test for singularity
        singular = [(0,0,0,0), (0,0,0,1), (0,0,1,0), (0,0,1,1), (0,1,0,0),
                    (0,1,0,1), (1,0,0,0), (1,0,1,0), (1,1,0,0), (1,1,1,1)]

        for matrix in singular:
            self.assertRaises(turtle.TurtleGraphicsError,
                              turtle.shapetransform, *matrix)

        # Test for incomplete arguments
        self.assertRaises(TypeError, turtle.shapetransform, 0)
        self.assertRaises(TypeError, turtle.shapetransform, 0,1.0)
        self.assertRaises(TypeError, turtle.shapetransform, 0,1,1)

        # Test return value if no arguments are givin
        turtle.shapetransform(4, -1, 0, 2)
        result = turtle.shapetransform()
        self.assertItemsAlmostEqual(result, (4, -1, 0, 2))
        result = turtle.shapesize()
        self.assertItemsAlmostEqual(result, (4, 2, 1))
        result = turtle.shearfactor()
        self.assertAlmostEqual(result, -0.5)

    def test_undo_limit(self):
        turtle.setundobuffer(2)
        turtle.left(90)
        turtle.forward(10)
        turtle.right(90)
        turtle.forward(100)
        turtle.left(90)
        turtle.forward(10)
        expected = [(90, (0, 0)),
                   (90, (0, 10)),
                   (0, (0, 10)),
                   (0, (100, 10)),
                   (90, (100, 10)),
                   (90, (100, 20))]
        self._run_undo_calls(expected)
        self.assertEqual(len(expected), 4)
        turtle.undo()
        heading, expected_pos = expected[-1]
        self.assertAlmostEqual(turtle.heading(), heading)
        pos = turtle.position()
        self.assertAlmostEqual(pos[0], expected_pos[0])
        self.assertAlmostEqual(pos[1], expected_pos[1])


class TestTurtleMotion(CustomTestCase):

    def test_undo(self):
        turtle.setundobuffer(42)
        expected = [(0, (0, 0)),
                    (90, (0, 0)),
                    (90, (0, 10))]
        turtle.left(90)
        turtle.forward(10)
        self._run_undo_calls(expected)
        for i in range(10):
            turtle.undo()
        self.assertAlmostEqual(turtle.heading(), 0)
        pos = turtle.position()
        self.assertItemsAlmostEqual(pos, (0, 0))

    def test_setundobuffer(self):
        turtle.setundobuffer(3)
        self.assertTrue(turtle.getturtle().undobuffer.bufsize == 3)

        turtle.setundobuffer(10)
        self.assertTrue(turtle.getturtle().undobuffer.bufsize == 10)

        # Anything negative gets an empty buffer.
        turtle.setundobuffer(0)
        self.assertTrue(turtle.getturtle().undobuffer == None)
        turtle.forward(10)
        turtle.undo()
        self.assertItemsAlmostEqual(turtle.pos(), (10, 0))
        self.assertRaises(TypeError, turtle.setundobuffer, 2.5)

        turtle.setundobuffer(None)
        self.assertTrue(turtle.getturtle().undobuffer == None)

        turtle.setundobuffer(1000000)
        self.assertTrue(turtle.getturtle().undobuffer.bufsize == 1000000)

    def test_mutlipe_undo_calls(self):
        turtle.left(90)
        turtle.forward(10)
        turtle.right(90)
        turtle.forward(100)
        turtle.left(90)
        turtle.forward(10)

        # Array of that stores a tuple of heading and the position of the
        # turtle.
        expected = [(90, (0, 0)),
                   (90, (0, 10)),
                   (0, (0, 10)),
                   (0, (100, 10)),
                   (90, (100, 10)),
                   (90, (100, 20))]

        self._run_undo_calls(expected)

    def test_clear(self):
        # Test with stamps
        turtle.forward(100)
        turtle.stamp()
        self.assertTrue(len(turtle.getturtle().stampItems) == 1)
        turtle.clear()
        self.assertTrue(len(turtle.getturtle().stampItems) == 0)

        # Test with fill and lines
        turtle.color('red', 'yellow')
        turtle.begin_fill()
        for i in range(3):
            turtle.forward(50)
            turtle.left(170)
        turtle.end_fill()
        turtle.clear()
        self.assertTrue(len(turtle.getturtle().stampItems) == 0)
        self.assertTrue(len(turtle.getturtle().items) == 1)

    def test_stamp(self):
        turtle.forward(100)
        turtle.left(45.3)
        turtle.forward(240.3)
        turtle_id = turtle.getturtle().turtle._item
        coord = turtle.getscreen().cv.coords(turtle_id)
        stamp_id = turtle.stamp()
        stamp_coord = turtle.getscreen().cv.coords(stamp_id)
        turtle.forward(300)
        for stamp_num, num in zip(stamp_coord, coord):
            self.assertAlmostEqual(stamp_num, num)

    def test_tilt(self):
        turtle.tilt(30)
        turtle.tilt(30)
        self.assertAlmostEqual(turtle.tiltangle(), 60)
        expected_transform = (0.5, -0.8660254037844386, 0.8660254037844386, 0.5)
        self.assertItemsAlmostEqual(turtle.shapetransform(),
                                     expected_transform)
        turtle.tilt(-61.5)
        self.assertAlmostEqual(turtle.tiltangle(), 358.5)

    def test_clearstamp(self):
        for extent in range(0, 360, 40):
            turtle.circle(30, 40)
            turtle.stamp()
        stamps = turtle.getturtle().stampItems[:]
        while stamps:
            self.assertEqual(turtle.getturtle().stampItems, stamps)
            stamp_id = stamps.pop()
            turtle.clearstamp(stamp_id)

    def test_clearstamps(self):
        # Draw a circle of stamps then clear
        for extent in range(0, 360, 40):
            turtle.circle(30, 40)
            turtle.stamp()
        self.assertEqual(len(turtle.getturtle().stampItems), 360/40)
        turtle.clearstamps()
        self.assertEqual(len(turtle.getturtle().stampItems), 0)

    def test_filling(self):
        self.assertFalse(turtle.filling())
        turtle.begin_fill()
        self.assertTrue(turtle.filling())
        turtle.end_fill()
        self.assertFalse(turtle.filling())

    def test_fill(self):
        expected = [(0.00,0.00), (30.00,0.00), (30.00,30.00), (0.00,30.00)]
        turtle.color('blue')
        turtle.begin_fill()
        for i in range(3):
            turtle.forward(30)
            turtle.left(90)
        self.assertItemsAlmostEqual(turtle.getturtle()._fillpath, expected)
        poly_id = turtle.getturtle()._fillitem
        turtle.end_fill()
        screen = turtle.getscreen()
        expected = [0.0, 0.0, 30.0, 0.0, 30.0, -30.0, 0.0, -30]
        self.assertItemsAlmostEqual(screen.cv.coords(poly_id), expected)
        self.assertEqual(screen.cv.itemcget(poly_id, 'fill'), 'blue')

        # Calling begin_fill twice
        turtle.reset()
        turtle.begin_fill()
        for i in range(3):
            turtle.forward(30)
            turtle.right(90)
        expected = [(0.00,0.00), (30.00,0.00), (30.00,-30.00), (0.00,-30.00)]
        self.assertItemsAlmostEqual(turtle.getturtle()._fillpath, expected)
        turtle.begin_fill()
        self.assertItemsAlmostEqual(turtle.getturtle()._fillpath, [(0, -30)])
        poly_id = turtle.getturtle()._fillitem
        turtle.end_fill()
        expected =  [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        self.assertItemsAlmostEqual(screen.cv.coords(poly_id), expected)

    def test_poly(self):
        turtle.begin_poly()
        for i in range(3):
            turtle.forward(10)
            turtle.left(90)
        turtle.end_poly()
        expected = ((0.00,0.00), (10.00,-0.00), (10.00,10.00), (0.00,10.00))
        self.assertItemsAlmostEqual(turtle.get_poly(), expected)

    def test_color(self):
        colors = ['indian red', 'saddle brown', 'sandy brown',
                  'dark salmon', 'salmon', 'light salmon', 'orange',
                  'dark orange', 'coral', 'light coral', 'tomato', 'orange red',
                  'red', 'hot pink', 'deep pink', 'pink', 'light pink',
                  'pale violet red', 'maroon', 'medium violet red',
                  'violet red', 'medium orchid', 'dark orchid']
        for color in colors:
            turtle.color(color)
            self.assertEqual(turtle.color(), (color, color))
        turtle.color('deep pink', 'red')
        self.assertEqual(turtle.color(), ('deep pink', 'red'))
        self.assertRaises(TypeError, turtle.color, *('red', 'green', 'blue'))
        turtle.colormode(255)
        turtle.color('#FF69B4', '#6B8E23')
        expected = ((255,105,180), (107,142,35))
        self.assertItemsAlmostEqual(turtle.color(), expected)
        turtle.colormode(1.0)
        expected = ((1.0, 0.4117647058823529, 0.7058823529411765),
                    (0.4196078431372549, 0.5568627450980, 0.137254901960784))
        self.assertItemsAlmostEqual(turtle.color(), expected)
        self.assertRaises(turtle.TurtleGraphicsError,
                          turtle.color,
                          'bad values')
        self.assertRaises(UnboundLocalError,
                          turtle.color,
                          *(0.1, 0.1, 0.1, 0.1))
        turtle.color(0.5, 0.5, 0.5)
        expected = ((0.5019607843137, 0.5019607843137, 0.5019607843137),
                    (0.5019607843137, 0.5019607843137, 0.5019607843137))
        self.assertItemsAlmostEqual(turtle.color(), expected)

    def test_clone(self):
        turtle.shape('circle')
        turtle.color('light blue', 'purple')
        turtle.begin_fill()
        for i in range(3):
            turtle.fd(30)
            turtle.left(90)
        turtle.end_fill()
        clone = turtle.clone()
        self.assertEqual(clone.shape(), turtle.shape())
        self.assertItemsAlmostEqual(clone.shapetransform(),
                                    turtle.shapetransform())
        self.assertItemsAlmostEqual(clone.color(), turtle.color())
        self.assertEqual(clone.getturtle().items,
                         turtle.getturtle().items)

    def reverse_coordinates(self, c):
        reversed_coords = [(c[i-1], c[i]) for i in range(len(c)-1, -1, -2)]
        return [item for sublist in reversed_coords for item in sublist]

    def test_circle(self):
        unit_circle = [0.0, 0.0, 0.5, -0.13397459621556132, 0.8660254037844386,
                       -0.5,1.0, -1, 0.8660254037844388, -1.5, 0.5,
                       -1.8660254037844386, 0, -2.0, -0.5, -1.8660254037844388,
                       -0.8660254037844387, -1.5, -1, -1, -0.8660254037844395,
                       -0.5, -0.5, -0.13397459621556135,
                        0, 0]
        turtle.circle(1)
        id = turtle.getturtle().items[0]
        screen = turtle.getscreen()
        self.assertItemsAlmostEqual(screen.cv.coords(id), unit_circle)
        turtle.clear()
        id = turtle.getturtle().items[0]
        for upper_bound in range(8, 27, 6):
            turtle.circle(1, 90)
            expected = unit_circle[0:upper_bound]
            self.assertItemsAlmostEqual(screen.cv.coords(id), expected)
        turtle.clear()
        id = turtle.getturtle().items[0]
        steps = [18, 12, 6, 0]
        for step in steps:
            turtle.circle(1, extent=-90)
            expected = self.reverse_coordinates(unit_circle[step:26])
            self.assertItemsAlmostEqual(screen.cv.coords(id), expected)
        expected = [0, 0, 12.990381056766632, -22.5,
                    -12.990381056766632, -22.5, 0, 0]
        turtle.clear()
        id = turtle.getturtle().items[0]
        turtle.circle(15, steps=3)
        self.assertItemsAlmostEqual(screen.cv.coords(id), expected)
        turtle.clear()
        id = turtle.getturtle().items[0]
        turtle.circle(15, steps=6)
        # Steps is calculated by doubleing for x,y coordinates,
        # and +2 for origin
        expected = 6*2+2
        self.assertEqual(len(screen.cv.coords(id)), expected)
        turtle.clear()
        id = turtle.getturtle().items[0]
        expected = [0, 0, 12.990381056766605, 22.5,
                    -12.990381056766562, 22.5, 0, 0]
        turtle.circle(-15, steps=3)
        self.assertItemsAlmostEqual(screen.cv.coords(id), expected)
        self.assertRaises(TypeError, turtle.circle, 10, steps=10.5)
        self.assertRaises(TypeError, turtle.circle, '10')

    def test_shapesize(self):
        turtle.shapesize(10)
        expected = (10.0, 0.0, 0.0, 10.0)
        self.assertItemsAlmostEqual(turtle.shapetransform(), expected)
        self.assertItemsAlmostEqual(turtle.shapesize(),  (10, 10, 1))
        turtle.shapesize(-5)
        expected = (-5, 0.0, 0.0, -5)
        self.assertItemsAlmostEqual(turtle.shapetransform(), expected)
        self.assertItemsAlmostEqual(turtle.shapesize(),  (-5, -5, 1))
        turtle.shapesize(5, 3)
        expected = (5.0, 0.0, -0.0, 3.0)
        self.assertItemsAlmostEqual(turtle.shapetransform(), expected)
        self.assertItemsAlmostEqual(turtle.shapesize(),  (5, 3, 1))
        self.assertRaises(TypeError, turtle.shapesize, 'sjdlf')

    def test_shearfactor(self):
        turtle.shearfactor(0.5)
        self.assertEqual(turtle.shearfactor(), 0.5)
        expected = (1, 0.5, 0.0, 1)
        self.assertItemsAlmostEqual(turtle.shapetransform(), expected)

    def dot(self):
        remove = turtle.getturtle().items[:]
        turtle.dot()
        dot_id = [x for x in turtle.getturtle().items if x not in remove][0]
        screen = turtle.getscreen()
        self.assertAlmostEqual(screen.cv.itemcget(dot_id, 'width'),
                               turtle.getturtle()._pensize * 2)
        turtle.dot(10, 'blue')
        remove.append(dot_id)
        dot_id = [x for x in turtle.getturtle().items if x not in remove][0]
        self.assertAlmostEqual(screen.cv.itemcget(dot_id, 'width'), 10)
        self.assertAlmostEqual(screen.cv.itemcget(dot_id, 'fill'), 'blue')

    def tearDown(self):
        turtle.bye()
