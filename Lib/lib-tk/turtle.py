# LogoMation-like turtle graphics

"""
Turtle graphics is a popular way for introducing programming to
kids. It was part of the original Logo programming language developed
by Wally Feurzeig and Seymour Papert in 1966.

Imagine a robotic turtle starting at (0, 0) in the x-y plane. Give it
the command turtle.forward(15), and it moves (on-screen!) 15 pixels in
the direction it is facing, drawing a line as it moves. Give it the
command turtle.left(25), and it rotates in-place 25 degrees clockwise.

By combining together these and similar commands, intricate shapes and
pictures can easily be drawn.
"""

from math import * # Also for export
from time import sleep
import Tkinter

speeds = ['fastest', 'fast', 'normal', 'slow', 'slowest']

class Error(Exception):
    pass

class RawPen:

    def __init__(self, canvas):
        self._canvas = canvas
        self._items = []
        self._tracing = 1
        self._arrow = 0
        self._delay = 10     # default delay for drawing
        self._angle = 0.0
        self.degrees()
        self.reset()

    def degrees(self, fullcircle=360.0):
        """ Set angle measurement units to degrees.

        Example:
        >>> turtle.degrees()
        """
        # Don't try to change _angle if it is 0, because
        # _fullcircle might not be set, yet
        if self._angle:
            self._angle = (self._angle / self._fullcircle) * fullcircle
        self._fullcircle = fullcircle
        self._invradian = pi / (fullcircle * 0.5)

    def radians(self):
        """ Set the angle measurement units to radians.

        Example:
        >>> turtle.radians()
        """
        self.degrees(2.0*pi)

    def reset(self):
        """ Clear the screen, re-center the pen, and set variables to
        the default values.

        Example:
        >>> turtle.position()
        [0.0, -22.0]
        >>> turtle.heading()
        100.0
        >>> turtle.reset()
        >>> turtle.position()
        [0.0, 0.0]
        >>> turtle.heading()
        0.0
        """
        canvas = self._canvas
        self._canvas.update()
        width = canvas.winfo_width()
        height = canvas.winfo_height()
        if width <= 1:
            width = canvas['width']
        if height <= 1:
            height = canvas['height']
        self._origin = float(width)/2.0, float(height)/2.0
        self._position = self._origin
        self._angle = 0.0
        self._drawing = 1
        self._width = 1
        self._color = "black"
        self._filling = 0
        self._path = []
        self.clear()
        canvas._root().tkraise()

    def clear(self):
        """ Clear the screen. The turtle does not move.

        Example:
        >>> turtle.clear()
        """
        self.fill(0)
        canvas = self._canvas
        items = self._items
        self._items = []
        for item in items:
            canvas.delete(item)
        self._delete_turtle()
        self._draw_turtle()

    def tracer(self, flag):
        """ Set tracing on if flag is True, and off if it is False.
        Tracing means line are drawn more slowly, with an
        animation of an arrow along the line.

        Example:
        >>> turtle.tracer(False)   # turns off Tracer
        """
        self._tracing = flag
        if not self._tracing:
            self._delete_turtle()
        self._draw_turtle()

    def forward(self, distance):
        """ Go forward distance steps.

        Example:
        >>> turtle.position()
        [0.0, 0.0]
        >>> turtle.forward(25)
        >>> turtle.position()
        [25.0, 0.0]
        >>> turtle.forward(-75)
        >>> turtle.position()
        [-50.0, 0.0]
        """
        x0, y0 = start = self._position
        x1 = x0 + distance * cos(self._angle*self._invradian)
        y1 = y0 - distance * sin(self._angle*self._invradian)
        self._goto(x1, y1)

    def backward(self, distance):
        """ Go backwards distance steps.

        The turtle's heading does not change.

        Example:
        >>> turtle.position()
        [0.0, 0.0]
        >>> turtle.backward(30)
        >>> turtle.position()
        [-30.0, 0.0]
        """
        self.forward(-distance)

    def left(self, angle):
        """ Turn left angle units (units are by default degrees,
        but can be set via the degrees() and radians() functions.)

        When viewed from above, the turning happens in-place around
        its front tip.

        Example:
        >>> turtle.heading()
        22
        >>> turtle.left(45)
        >>> turtle.heading()
        67.0
        """
        self._angle = (self._angle + angle) % self._fullcircle
        self._draw_turtle()

    def right(self, angle):
        """ Turn right angle units (units are by default degrees,
        but can be set via the degrees() and radians() functions.)

        When viewed from above, the turning happens in-place around
        its front tip.

        Example:
        >>> turtle.heading()
        22
        >>> turtle.right(45)
        >>> turtle.heading()
        337.0
        """
        self.left(-angle)

    def up(self):
        """ Pull the pen up -- no drawing when moving.

        Example:
        >>> turtle.up()
        """
        self._drawing = 0

    def down(self):
        """ Put the pen down -- draw when moving.

        Example:
        >>> turtle.down()
        """
        self._drawing = 1

    def width(self, width):
        """ Set the line to thickness to width.

        Example:
        >>> turtle.width(10)
        """
        self._width = float(width)

    def color(self, *args):
        """ Set the pen color.

        Three input formats are allowed:

            color(s)
            s is a Tk specification string, such as "red" or "yellow"

            color((r, g, b))
            *a tuple* of r, g, and b, which represent, an RGB color,
            and each of r, g, and b are in the range [0..1]

            color(r, g, b)
            r, g, and b represent an RGB color, and each of r, g, and b
            are in the range [0..1]

        Example:

        >>> turtle.color('brown')
        >>> tup = (0.2, 0.8, 0.55)
        >>> turtle.color(tup)
        >>> turtle.color(0, .5, 0)
        """
        if not args:
            raise Error("no color arguments")
        if len(args) == 1:
            color = args[0]
            if type(color) == type(""):
                # Test the color first
                try:
                    id = self._canvas.create_line(0, 0, 0, 0, fill=color)
                except Tkinter.TclError:
                    raise Error("bad color string: %r" % (color,))
                self._set_color(color)
                return
            try:
                r, g, b = color
            except:
                raise Error("bad color sequence: %r" % (color,))
        else:
            try:
                r, g, b = args
            except:
                raise Error("bad color arguments: %r" % (args,))
        assert 0 <= r <= 1
        assert 0 <= g <= 1
        assert 0 <= b <= 1
        x = 255.0
        y = 0.5
        self._set_color("#%02x%02x%02x" % (int(r*x+y), int(g*x+y), int(b*x+y)))

    def _set_color(self,color):
        self._color = color
        self._draw_turtle()

    def write(self, text, move=False):
        """ Write text at the current pen position.

        If move is true, the pen is moved to the bottom-right corner
        of the text. By default, move is False.

        Example:
        >>> turtle.write('The race is on!')
        >>> turtle.write('Home = (0, 0)', True)
        """
        x, y  = self._position
        x = x-1 # correction -- calibrated for Windows
        item = self._canvas.create_text(x, y,
                                        text=str(text), anchor="sw",
                                        fill=self._color)
        self._items.append(item)
        if move:
            x0, y0, x1, y1 = self._canvas.bbox(item)
            self._goto(x1, y1)
        self._draw_turtle()

    def fill(self, flag):
        """ Call fill(1) before drawing the shape you
         want to fill, and fill(0) when done.

        Example:
        >>> turtle.fill(1)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.fill(0)
        """
        if self._filling:
            path = tuple(self._path)
            smooth = self._filling < 0
            if len(path) > 2:
                item = self._canvas._create('polygon', path,
                                            {'fill': self._color,
                                             'smooth': smooth})
                self._items.append(item)
        self._path = []
        self._filling = flag
        if flag:
            self._path.append(self._position)

    def begin_fill(self):
        """ Called just before drawing a shape to be filled.
            Must eventually be followed by a corresponding end_fill() call.
            Otherwise it will be ignored.

        Example:
        >>> turtle.begin_fill()
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.end_fill()
        """
        self._path = [self._position]
        self._filling = 1

    def end_fill(self):
        """ Called after drawing a shape to be filled.

        Example:
        >>> turtle.begin_fill()
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.left(90)
        >>> turtle.forward(100)
        >>> turtle.end_fill()
        """
        self.fill(0)

    def circle(self, radius, extent = None):
        """ Draw a circle with given radius.
        The center is radius units left of the turtle; extent
        determines which part of the circle is drawn. If not given,
        the entire circle is drawn.

        If extent is not a full circle, one endpoint of the arc is the
        current pen position. The arc is drawn in a counter clockwise
        direction if radius is positive, otherwise in a clockwise
        direction. In the process, the direction of the turtle is
        changed by the amount of the extent.

        >>> turtle.circle(50)
        >>> turtle.circle(120, 180)  # half a circle
        """
        if extent is None:
            extent = self._fullcircle
        frac = abs(extent)/self._fullcircle
        steps = 1+int(min(11+abs(radius)/6.0, 59.0)*frac)
        w = 1.0 * extent / steps
        w2 = 0.5 * w
        l = 2.0 * radius * sin(w2*self._invradian)
        if radius < 0:
            l, w, w2 = -l, -w, -w2
        self.left(w2)
        for i in range(steps):
            self.forward(l)
            self.left(w)
        self.right(w2)

    def heading(self):
        """ Return the turtle's current heading.

        Example:
        >>> turtle.heading()
        67.0
        """
        return self._angle

    def setheading(self, angle):
        """ Set the turtle facing the given angle.

        Here are some common directions in degrees:

           0 - east
          90 - north
         180 - west
         270 - south

        Example:
        >>> turtle.setheading(90)
        >>> turtle.heading()
        90
        >>> turtle.setheading(128)
        >>> turtle.heading()
        128
        """
        self._angle = angle
        self._draw_turtle()

    def window_width(self):
        """ Returns the width of the turtle window.

        Example:
        >>> turtle.window_width()
        640
        """
        width = self._canvas.winfo_width()
        if width <= 1:  # the window isn't managed by a geometry manager
            width = self._canvas['width']
        return width

    def window_height(self):
        """ Return the height of the turtle window.

        Example:
        >>> turtle.window_height()
        768
        """
        height = self._canvas.winfo_height()
        if height <= 1: # the window isn't managed by a geometry manager
            height = self._canvas['height']
        return height

    def position(self):
        """ Return the current (x, y) location of the turtle.

        Example:
        >>> turtle.position()
        [0.0, 240.0]
        """
        x0, y0 = self._origin
        x1, y1 = self._position
        return [x1-x0, -y1+y0]

    def setx(self, xpos):
        """ Set the turtle's x coordinate to be xpos.

        Example:
        >>> turtle.position()
        [10.0, 240.0]
        >>> turtle.setx(10)
        >>> turtle.position()
        [10.0, 240.0]
        """
        x0, y0 = self._origin
        x1, y1 = self._position
        self._goto(x0+xpos, y1)

    def sety(self, ypos):
        """ Set the turtle's y coordinate to be ypos.

        Example:
        >>> turtle.position()
        [0.0, 0.0]
        >>> turtle.sety(-22)
        >>> turtle.position()
        [0.0, -22.0]
        """
        x0, y0 = self._origin
        x1, y1 = self._position
        self._goto(x1, y0-ypos)

    def towards(self, *args):
        """Returs the angle, which corresponds to the line
        from turtle-position to point (x,y).

        Argument can be two coordinates or one pair of coordinates
        or a RawPen/Pen instance.

        Example:
        >>> turtle.position()
        [10.0, 10.0]
        >>> turtle.towards(0,0)
        225.0
        """
        if len(args) == 2:
            x, y = args
        else:
            arg = args[0]
            if isinstance(arg, RawPen):
                x, y = arg.position()
            else:
                x, y = arg
        x0, y0 = self.position()
        dx = x - x0
        dy = y - y0
        return (atan2(dy,dx) / self._invradian) % self._fullcircle

    def goto(self, *args):
        """ Go to the given point.

        If the pen is down, then a line will be drawn. The turtle's
        orientation does not change.

        Two input formats are accepted:

           goto(x, y)
           go to point (x, y)

           goto((x, y))
           go to point (x, y)

        Example:
        >>> turtle.position()
        [0.0, 0.0]
        >>> turtle.goto(50, -45)
        >>> turtle.position()
        [50.0, -45.0]
        """
        if len(args) == 1:
            try:
                x, y = args[0]
            except:
                raise Error("bad point argument: %r" % (args[0],))
        else:
            try:
                x, y = args
            except:
                raise Error("bad coordinates: %r" % (args[0],))
        x0, y0 = self._origin
        self._goto(x0+x, y0-y)

    def _goto(self, x1, y1):
        x0, y0 = self._position
        self._position = (float(x1), float(y1))
        if self._filling:
            self._path.append(self._position)
        if self._drawing:
            if self._tracing:
                dx = float(x1 - x0)
                dy = float(y1 - y0)
                distance = hypot(dx, dy)
                nhops = int(distance)
                item = self._canvas.create_line(x0, y0, x0, y0,
                                                width=self._width,
                                                capstyle="round",
                                                fill=self._color)
                try:
                    for i in range(1, 1+nhops):
                        x, y = x0 + dx*i/nhops, y0 + dy*i/nhops
                        self._canvas.coords(item, x0, y0, x, y)
                        self._draw_turtle((x,y))
                        self._canvas.update()
                        self._canvas.after(self._delay)
                    # in case nhops==0
                    self._canvas.coords(item, x0, y0, x1, y1)
                    self._canvas.itemconfigure(item, arrow="none")
                except Tkinter.TclError:
                    # Probably the window was closed!
                    return
            else:
                item = self._canvas.create_line(x0, y0, x1, y1,
                                                width=self._width,
                                                capstyle="round",
                                                fill=self._color)
            self._items.append(item)
        self._draw_turtle()

    def speed(self, speed):
        """ Set the turtle's speed.

        speed must one of these five strings:

            'fastest' is a 0 ms delay
            'fast' is a 5 ms delay
            'normal' is a 10 ms delay
            'slow' is a 15 ms delay
            'slowest' is a 20 ms delay

         Example:
         >>> turtle.speed('slow')
        """
        try:
            speed = speed.strip().lower()
            self._delay = speeds.index(speed) * 5
        except:
            raise ValueError("%r is not a valid speed. speed must be "
                             "one of %s" % (speed, speeds))


    def delay(self, delay):
        """ Set the drawing delay in milliseconds.

        This is intended to allow finer control of the drawing speed
        than the speed() method

        Example:
        >>> turtle.delay(15)
        """
        if int(delay) < 0:
            raise ValueError("delay must be greater than or equal to 0")
        self._delay = int(delay)

    def _draw_turtle(self, position=[]):
        if not self._tracing:
            self._canvas.update()
            return
        if position == []:
            position = self._position
        x,y = position
        distance = 8
        dx = distance * cos(self._angle*self._invradian)
        dy = distance * sin(self._angle*self._invradian)
        self._delete_turtle()
        self._arrow = self._canvas.create_line(x-dx,y+dy,x,y,
                                          width=self._width,
                                          arrow="last",
                                          capstyle="round",
                                          fill=self._color)
        self._canvas.update()

    def _delete_turtle(self):
        if self._arrow != 0:
            self._canvas.delete(self._arrow)
            self._arrow = 0


_root = None
_canvas = None
_pen = None
_width = 0.50                  # 50% of window width
_height = 0.75                 # 75% of window height
_startx = None
_starty = None
_title = "Turtle Graphics"     # default title

class Pen(RawPen):

    def __init__(self):
        global _root, _canvas
        if _root is None:
            _root = Tkinter.Tk()
            _root.wm_protocol("WM_DELETE_WINDOW", self._destroy)
            _root.title(_title)

        if _canvas is None:
            # XXX Should have scroll bars
            _canvas = Tkinter.Canvas(_root, background="white")
            _canvas.pack(expand=1, fill="both")

            setup(width=_width, height= _height, startx=_startx, starty=_starty)

        RawPen.__init__(self, _canvas)

    def _destroy(self):
        global _root, _canvas, _pen
        root = self._canvas._root()
        if root is _root:
            _pen = None
            _root = None
            _canvas = None
        root.destroy()

def _getpen():
    global _pen
    if not _pen:
        _pen = Pen()
    return _pen

class Turtle(Pen):
    pass

"""For documentation of the following functions see
   the RawPen methods with the same names
"""

def degrees(): _getpen().degrees()
def radians(): _getpen().radians()
def reset(): _getpen().reset()
def clear(): _getpen().clear()
def tracer(flag): _getpen().tracer(flag)
def forward(distance): _getpen().forward(distance)
def backward(distance): _getpen().backward(distance)
def left(angle): _getpen().left(angle)
def right(angle): _getpen().right(angle)
def up(): _getpen().up()
def down(): _getpen().down()
def width(width): _getpen().width(width)
def color(*args): _getpen().color(*args)
def write(arg, move=0): _getpen().write(arg, move)
def fill(flag): _getpen().fill(flag)
def begin_fill(): _getpen().begin_fill()
def end_fill(): _getpen().end_fill()
def circle(radius, extent=None): _getpen().circle(radius, extent)
def goto(*args): _getpen().goto(*args)
def heading(): return _getpen().heading()
def setheading(angle): _getpen().setheading(angle)
def position(): return _getpen().position()
def window_width(): return _getpen().window_width()
def window_height(): return _getpen().window_height()
def setx(xpos): _getpen().setx(xpos)
def sety(ypos): _getpen().sety(ypos)
def towards(*args): return _getpen().towards(*args)

def done(): _root.mainloop()
def delay(delay): return _getpen().delay(delay)
def speed(speed): return _getpen().speed(speed)

for methodname in dir(RawPen):
    """ copies RawPen docstrings to module functions of same name """
    if not methodname.startswith("_"):
        eval(methodname).__doc__ = RawPen.__dict__[methodname].__doc__


def setup(**geometry):
    """ Sets the size and position of the main window.

    Keywords are width, height, startx and starty:

    width: either a size in pixels or a fraction of the screen.
      Default is 50% of screen.
    height: either the height in pixels or a fraction of the screen.
      Default is 75% of screen.

    Setting either width or height to None before drawing will force
      use of default geometry as in older versions of turtle.py

    startx: starting position in pixels from the left edge of the screen.
      Default is to center window. Setting startx to None is the default
      and centers window horizontally on screen.

    starty: starting position in pixels from the top edge of the screen.
      Default is to center window. Setting starty to None is the default
      and centers window vertically on screen.

    Examples:
    >>> setup (width=200, height=200, startx=0, starty=0)

    sets window to 200x200 pixels, in upper left of screen

    >>> setup(width=.75, height=0.5, startx=None, starty=None)

    sets window to 75% of screen by 50% of screen and centers

    >>> setup(width=None)

    forces use of default geometry as in older versions of turtle.py
    """

    global _width, _height, _startx, _starty

    width = geometry.get('width',_width)
    if width is None or width >= 0:
        _width = width
    else:
        raise ValueError("width can not be less than 0")

    height = geometry.get('height',_height)
    if height is None or height >= 0:
        _height = height
    else:
        raise ValueError("height can not be less than 0")

    startx = geometry.get('startx', _startx)
    if startx is None or startx >= 0:
        _startx = _startx
    else:
        raise ValueError("startx can not be less than 0")

    starty = geometry.get('starty', _starty)
    if starty is None or starty >= 0:
        _starty = starty
    else:
        raise ValueError("startx can not be less than 0")


    if _root and _width and _height:
        if 0 < _width <= 1:
            _width = _root.winfo_screenwidth() * +width
        if 0 < _height <= 1:
            _height = _root.winfo_screenheight() * _height

        # center window on screen
        if _startx is None:
            _startx = (_root.winfo_screenwidth() - _width) / 2

        if _starty is None:
            _starty = (_root.winfo_screenheight() - _height) / 2

        _root.geometry("%dx%d+%d+%d" % (_width, _height, _startx, _starty))

def title(title):
    """Set the window title.

    By default this is set to 'Turtle Graphics'

    Example:
    >>> title("My Window")
    """

    global _title
    _title = title

def demo():
    reset()
    tracer(1)
    up()
    backward(100)
    down()
    # draw 3 squares; the last filled
    width(3)
    for i in range(3):
        if i == 2:
            fill(1)
        for j in range(4):
            forward(20)
            left(90)
        if i == 2:
            color("maroon")
            fill(0)
        up()
        forward(30)
        down()
    width(1)
    color("black")
    # move out of the way
    tracer(0)
    up()
    right(90)
    forward(100)
    right(90)
    forward(100)
    right(180)
    down()
    # some text
    write("startstart", 1)
    write("start", 1)
    color("red")
    # staircase
    for i in range(5):
        forward(20)
        left(90)
        forward(20)
        right(90)
    # filled staircase
    fill(1)
    for i in range(5):
        forward(20)
        left(90)
        forward(20)
        right(90)
    fill(0)
    tracer(1)
    # more text
    write("end")

def demo2():
    # exercises some new and improved features
    speed('fast')
    width(3)

    # draw a segmented half-circle
    setheading(towards(0,0))
    x,y = position()
    r = (x**2+y**2)**.5/2.0
    right(90)
    pendown = True
    for i in range(18):
        if pendown:
            up()
            pendown = False
        else:
            down()
            pendown = True
        circle(r,10)
    sleep(2)

    reset()
    left(90)

    # draw a series of triangles
    l = 10
    color("green")
    width(3)
    left(180)
    sp = 5
    for i in range(-2,16):
        if i > 0:
            color(1.0-0.05*i,0,0.05*i)
            fill(1)
            color("green")
        for j in range(3):
            forward(l)
            left(120)
        l += 10
        left(15)
        if sp > 0:
            sp = sp-1
            speed(speeds[sp])
    color(0.25,0,0.75)
    fill(0)

    # draw and fill a concave shape
    left(120)
    up()
    forward(70)
    right(30)
    down()
    color("red")
    speed("fastest")
    fill(1)
    for i in range(4):
        circle(50,90)
        right(90)
        forward(30)
        right(90)
    color("yellow")
    fill(0)
    left(90)
    up()
    forward(30)
    down();

    color("red")

    # create a second turtle and make the original pursue and catch it
    turtle=Turtle()
    turtle.reset()
    turtle.left(90)
    turtle.speed('normal')
    turtle.up()
    turtle.goto(280,40)
    turtle.left(24)
    turtle.down()
    turtle.speed('fast')
    turtle.color("blue")
    turtle.width(2)
    speed('fastest')

    # turn default turtle towards new turtle object
    setheading(towards(turtle))
    while ( abs(position()[0]-turtle.position()[0])>4 or
            abs(position()[1]-turtle.position()[1])>4):
        turtle.forward(3.5)
        turtle.left(0.6)
        # turn default turtle towards new turtle object
        setheading(towards(turtle))
        forward(4)
    write("CAUGHT! ", move=True)



if __name__ == '__main__':
    demo()
    sleep(3)
    demo2()
    done()
