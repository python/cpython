# LogoMation-like turtle graphics

from math import * # Also for export
import Tkinter

class Error(Exception):
    pass

class RawPen:

    def __init__(self, canvas):
        self._canvas = canvas
        self._items = []
        self._tracing = 1
        self._arrow = 0
        self.degrees()
        self.reset()

    def degrees(self, fullcircle=360.0):
        self._fullcircle = fullcircle
        self._invradian = pi / (fullcircle * 0.5)

    def radians(self):
        self.degrees(2.0*pi)

    def reset(self):
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
        self._tofill = []
        self.clear()
        canvas._root().tkraise()

    def clear(self):
        self.fill(0)
        canvas = self._canvas
        items = self._items
        self._items = []
        for item in items:
            canvas.delete(item)
        self._delete_turtle()
        self._draw_turtle()

    def tracer(self, flag):
        self._tracing = flag
        if not self._tracing:
            self._delete_turtle()
        self._draw_turtle()

    def forward(self, distance):
        x0, y0 = start = self._position
        x1 = x0 + distance * cos(self._angle*self._invradian)
        y1 = y0 - distance * sin(self._angle*self._invradian)
        self._goto(x1, y1)

    def backward(self, distance):
        self.forward(-distance)

    def left(self, angle):
        self._angle = (self._angle + angle) % self._fullcircle
        self._draw_turtle()

    def right(self, angle):
        self.left(-angle)

    def up(self):
        self._drawing = 0

    def down(self):
        self._drawing = 1

    def width(self, width):
        self._width = float(width)

    def color(self, *args):
        if not args:
            raise Error, "no color arguments"
        if len(args) == 1:
            color = args[0]
            if type(color) == type(""):
                # Test the color first
                try:
                    id = self._canvas.create_line(0, 0, 0, 0, fill=color)
                except Tkinter.TclError:
                    raise Error, "bad color string: %r" % (color,)
                self._set_color(color)
                return
            try:
                r, g, b = color
            except:
                raise Error, "bad color sequence: %r" % (color,)
        else:
            try:
                r, g, b = args
            except:
                raise Error, "bad color arguments: %r" % (args,)
        assert 0 <= r <= 1
        assert 0 <= g <= 1
        assert 0 <= b <= 1
        x = 255.0
        y = 0.5
        self._set_color("#%02x%02x%02x" % (int(r*x+y), int(g*x+y), int(b*x+y)))

    def _set_color(self,color):
        self._color = color
        self._draw_turtle()

    def write(self, arg, move=0):
        x, y = start = self._position
        x = x-1 # correction -- calibrated for Windows
        item = self._canvas.create_text(x, y,
                                        text=str(arg), anchor="sw",
                                        fill=self._color)
        self._items.append(item)
        if move:
            x0, y0, x1, y1 = self._canvas.bbox(item)
            self._goto(x1, y1)
        self._draw_turtle()

    def fill(self, flag):
        if self._filling:
            path = tuple(self._path)
            smooth = self._filling < 0
            if len(path) > 2:
                item = self._canvas._create('polygon', path,
                                            {'fill': self._color,
                                             'smooth': smooth})
                self._items.append(item)
                self._canvas.lower(item)
                if self._tofill:
                    for item in self._tofill:
                        self._canvas.itemconfigure(item, fill=self._color)
                        self._items.append(item)
        self._path = []
        self._tofill = []
        self._filling = flag
        if flag:
            self._path.append(self._position)
        self.forward(0)

    def circle(self, radius, extent=None):
        if extent is None:
            extent = self._fullcircle
        x0, y0 = self._position
        xc = x0 - radius * sin(self._angle * self._invradian)
        yc = y0 - radius * cos(self._angle * self._invradian)
        if radius >= 0.0:
            start = self._angle - 90.0
        else:
            start = self._angle + 90.0
            extent = -extent
        if self._filling:
            if abs(extent) >= self._fullcircle:
                item = self._canvas.create_oval(xc-radius, yc-radius,
                                                xc+radius, yc+radius,
                                                width=self._width,
                                                outline="")
                self._tofill.append(item)
            item = self._canvas.create_arc(xc-radius, yc-radius,
                                           xc+radius, yc+radius,
                                           style="chord",
                                           start=start,
                                           extent=extent,
                                           width=self._width,
                                           outline="")
            self._tofill.append(item)
        if self._drawing:
            if abs(extent) >= self._fullcircle:
                item = self._canvas.create_oval(xc-radius, yc-radius,
                                                xc+radius, yc+radius,
                                                width=self._width,
                                                outline=self._color)
                self._items.append(item)
            item = self._canvas.create_arc(xc-radius, yc-radius,
                                           xc+radius, yc+radius,
                                           style="arc",
                                           start=start,
                                           extent=extent,
                                           width=self._width,
                                           outline=self._color)
            self._items.append(item)
        angle = start + extent
        x1 = xc + abs(radius) * cos(angle * self._invradian)
        y1 = yc - abs(radius) * sin(angle * self._invradian)
        self._angle = (self._angle + extent) % self._fullcircle
        self._position = x1, y1
        if self._filling:
            self._path.append(self._position)
        self._draw_turtle()

    def heading(self):
        return self._angle

    def setheading(self, angle):
        self._angle = angle
        self._draw_turtle()

    def window_width(self):
        width = self._canvas.winfo_width()
        if width <= 1:  # the window isn't managed by a geometry manager
            width = self._canvas['width']
        return width

    def window_height(self):
        height = self._canvas.winfo_height()
        if height <= 1: # the window isn't managed by a geometry manager
            height = self._canvas['height']
        return height

    def position(self):
        x0, y0 = self._origin
        x1, y1 = self._position
        return [x1-x0, -y1+y0]

    def setx(self, xpos):
        x0, y0 = self._origin
        x1, y1 = self._position
        self._goto(x0+xpos, y1)

    def sety(self, ypos):
        x0, y0 = self._origin
        x1, y1 = self._position
        self._goto(x1, y0-ypos)

    def goto(self, *args):
        if len(args) == 1:
            try:
                x, y = args[0]
            except:
                raise Error, "bad point argument: %r" % (args[0],)
        else:
            try:
                x, y = args
            except:
                raise Error, "bad coordinates: %r" % (args[0],)
        x0, y0 = self._origin
        self._goto(x0+x, y0-y)

    def _goto(self, x1, y1):
        x0, y0 = start = self._position
        self._position = map(float, (x1, y1))
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
                        self._canvas.after(10)
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

    def _draw_turtle(self,position=[]):
        if not self._tracing:
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

class Pen(RawPen):

    def __init__(self):
        global _root, _canvas
        if _root is None:
            _root = Tkinter.Tk()
            _root.wm_protocol("WM_DELETE_WINDOW", self._destroy)
        if _canvas is None:
            # XXX Should have scroll bars
            _canvas = Tkinter.Canvas(_root, background="white")
            _canvas.pack(expand=1, fill="both")
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
    pen = _pen
    if not pen:
        _pen = pen = Pen()
    return pen

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
def circle(radius, extent=None): _getpen().circle(radius, extent)
def goto(*args): _getpen().goto(*args)
def heading(): return _getpen().heading()
def setheading(angle): _getpen().setheading(angle)
def position(): return _getpen().position()
def window_width(): return _getpen().window_width()
def window_height(): return _getpen().window_height()
def setx(xpos): _getpen().setx(xpos)
def sety(ypos): _getpen().sety(ypos)

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
    # more text
    write("end")
    if __name__ == '__main__':
        _root.mainloop()

if __name__ == '__main__':
    demo()
