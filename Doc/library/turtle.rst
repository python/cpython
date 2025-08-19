==================================
:mod:`!turtle` --- Turtle graphics
==================================

.. module:: turtle
   :synopsis: An educational framework for simple graphics applications

.. sectionauthor:: Gregor Lingl <gregor.lingl@aon.at>

**Source code:** :source:`Lib/turtle.py`

.. testsetup:: default

   from turtle import *
   turtle = Turtle()

.. testcleanup::

   import os
   os.remove("my_drawing.ps")

--------------

Introduction
============

Turtle graphics is an implementation of `the popular geometric drawing tools
introduced in Logo <https://en.wikipedia.org/wiki/Turtle_
(robot)>`_, developed by Wally Feurzeig, Seymour Papert and Cynthia Solomon
in 1967.


Get started
===========

Imagine a robotic turtle starting at (0, 0) in the x-y plane.  After an ``import turtle``, give it the
command ``turtle.forward(15)``, and it moves (on-screen!) 15 pixels in the
direction it is facing, drawing a line as it moves.  Give it the command
``turtle.right(25)``, and it rotates in-place 25 degrees clockwise.

.. sidebar:: Turtle star

   Turtle can draw intricate shapes using programs that repeat simple
   moves.

   .. image:: turtle-star.*
      :align: center

In Python, turtle graphics provides a representation of a physical "turtle"
(a little robot with a pen) that draws on a sheet of paper on the floor.

It's an effective and well-proven way for learners to encounter
programming concepts and interaction with software, as it provides instant,
visible feedback. It also provides convenient access to graphical output
in general.

Turtle drawing was originally created as an educational tool, to be used by
teachers in the classroom. For the programmer who needs to produce some
graphical output it can be a way to do that without the overhead of
introducing more complex or external libraries into their work.


.. _turtle-tutorial:

Tutorial
========

New users should start here. In this tutorial we'll explore some of the
basics of turtle drawing.


Starting a turtle environment
-----------------------------

In a Python shell, import all the objects of the ``turtle`` module::

    from turtle import *

If you run into a ``No module named '_tkinter'`` error, you'll have to
install the :mod:`Tk interface package <tkinter>` on your system.


Basic drawing
-------------

Send the turtle forward 100 steps::

   forward(100)

You should see (most likely, in a new window on your display) a line
drawn by the turtle, heading East. Change the direction of the turtle,
so that it turns 120 degrees left (anti-clockwise)::

   left(120)

Let's continue by drawing a triangle::

   forward(100)
   left(120)
   forward(100)

Notice how the turtle, represented by an arrow, points in different
directions as you steer it.

Experiment with those commands, and also with ``backward()`` and
``right()``.


Pen control
~~~~~~~~~~~

Try changing the color - for example, ``color('blue')`` - and
width of the line - for example, ``width(3)`` - and then drawing again.

You can also move the turtle around without drawing, by lifting up the pen:
``up()`` before moving. To start drawing again, use ``down()``.


The turtle's position
~~~~~~~~~~~~~~~~~~~~~

Send your turtle back to its starting-point (useful if it has disappeared
off-screen)::

   home()

The home position is at the center of the turtle's screen. If you ever need to
know them, get the turtle's x-y coordinates with::

    pos()

Home is at ``(0, 0)``.

And after a while, it will probably help to clear the window so we can start
anew::

   clearscreen()


Making algorithmic patterns
---------------------------

Using loops, it's possible to build up geometric patterns::

    for steps in range(100):
        for c in ('blue', 'red', 'green'):
            color(c)
            forward(steps)
            right(30)


\ - which of course, are limited only by the imagination!

Let's draw the star shape at the top of this page. We want red lines,
filled in with yellow::

    color('red')
    fillcolor('yellow')

Just as ``up()`` and ``down()`` determine whether lines will be drawn,
filling can be turned on and off::

    begin_fill()

Next we'll create a loop::

    while True:
        forward(200)
        left(170)
        if abs(pos()) < 1:
            break

``abs(pos()) < 1`` is a good way to know when the turtle is back at its
home position.

Finally, complete the filling::

    end_fill()

(Note that filling only actually takes place when you give the
``end_fill()`` command.)


.. _turtle-how-to:

How to...
=========

This section covers some typical turtle use-cases and approaches.


Get started as quickly as possible
----------------------------------

One of the joys of turtle graphics is the immediate, visual feedback that's
available from simple commands - it's an excellent way to introduce children
to programming ideas, with a minimum of overhead (not just children, of
course).

The turtle module makes this possible by exposing all its basic functionality
as functions, available with ``from turtle import *``. The :ref:`turtle
graphics tutorial <turtle-tutorial>` covers this approach.

It's worth noting that many of the turtle commands also have even more terse
equivalents, such as ``fd()`` for :func:`forward`. These are especially
useful when working with learners for whom typing is not a skill.

.. _note:

    You'll need to have the :mod:`Tk interface package <tkinter>` installed on
    your system for turtle graphics to work. Be warned that this is not
    always straightforward, so check this in advance if you're planning to
    use turtle graphics with a learner.


Automatically begin and end filling
-----------------------------------

Starting with Python 3.14, you can use the :func:`fill` :term:`context manager`
instead of :func:`begin_fill` and :func:`end_fill` to automatically begin and
end fill. Here is an example::

   with fill():
       for i in range(4):
           forward(100)
           right(90)

   forward(200)

The code above is equivalent to::

   begin_fill()
   for i in range(4):
       forward(100)
       right(90)
   end_fill()

   forward(200)


Use the ``turtle`` module namespace
-----------------------------------

Using ``from turtle import *`` is convenient - but be warned that it imports a
rather large collection of objects, and if you're doing anything but turtle
graphics you run the risk of a name conflict (this becomes even more an issue
if you're using turtle graphics in a script where other modules might be
imported).

The solution is to use ``import turtle`` - ``fd()`` becomes
``turtle.fd()``, ``width()`` becomes ``turtle.width()`` and so on. (If typing
"turtle" over and over again becomes tedious, use for example ``import turtle
as t`` instead.)


Use turtle graphics in a script
-------------------------------

It's recommended to use the ``turtle`` module namespace as described
immediately above, for example::

    import turtle as t
    from random import random

    for i in range(100):
        steps = int(random() * 100)
        angle = int(random() * 360)
        t.right(angle)
        t.fd(steps)

Another step is also required though - as soon as the script ends, Python
will also close the turtle's window. Add::

    t.mainloop()

to the end of the script. The script will now wait to be dismissed and
will not exit until it is terminated, for example by closing the turtle
graphics window.


Use object-oriented turtle graphics
-----------------------------------

.. seealso:: :ref:`Explanation of the object-oriented interface <turtle-explanation>`

Other than for very basic introductory purposes, or for trying things out
as quickly as possible, it's more usual and much more powerful to use the
object-oriented approach to turtle graphics. For example, this allows
multiple turtles on screen at once.

In this approach, the various turtle commands are methods of objects (mostly of
``Turtle`` objects). You *can* use the object-oriented approach in the shell,
but it would be more typical in a Python script.

The example above then becomes::

    from turtle import Turtle
    from random import random

    t = Turtle()
    for i in range(100):
        steps = int(random() * 100)
        angle = int(random() * 360)
        t.right(angle)
        t.fd(steps)

    t.screen.mainloop()

Note the last line. ``t.screen`` is an instance of the :class:`Screen`
that a Turtle instance exists on; it's created automatically along with
the turtle.

The turtle's screen can be customised, for example::

    t.screen.title('Object-oriented turtle demo')
    t.screen.bgcolor("orange")


Turtle graphics reference
=========================

.. note::

   In the following documentation the argument list for functions is given.
   Methods, of course, have the additional first argument *self* which is
   omitted here.


Turtle methods
--------------

Turtle motion
   Move and draw
      | :func:`forward` | :func:`fd`
      | :func:`backward` | :func:`bk` | :func:`back`
      | :func:`right` | :func:`rt`
      | :func:`left` | :func:`lt`
      | :func:`goto` | :func:`setpos` | :func:`setposition`
      | :func:`teleport`
      | :func:`setx`
      | :func:`sety`
      | :func:`setheading` | :func:`seth`
      | :func:`home`
      | :func:`circle`
      | :func:`dot`
      | :func:`stamp`
      | :func:`clearstamp`
      | :func:`clearstamps`
      | :func:`undo`
      | :func:`speed`

   Tell Turtle's state
      | :func:`position` | :func:`pos`
      | :func:`towards`
      | :func:`xcor`
      | :func:`ycor`
      | :func:`heading`
      | :func:`distance`

   Setting and measurement
      | :func:`degrees`
      | :func:`radians`

Pen control
   Drawing state
      | :func:`pendown` | :func:`pd` | :func:`down`
      | :func:`penup` | :func:`pu` | :func:`up`
      | :func:`pensize` | :func:`width`
      | :func:`pen`
      | :func:`isdown`

   Color control
      | :func:`color`
      | :func:`pencolor`
      | :func:`fillcolor`

   Filling
      | :func:`filling`
      | :func:`fill`
      | :func:`begin_fill`
      | :func:`end_fill`

   More drawing control
      | :func:`reset`
      | :func:`clear`
      | :func:`write`

Turtle state
   Visibility
      | :func:`showturtle` | :func:`st`
      | :func:`hideturtle` | :func:`ht`
      | :func:`isvisible`

   Appearance
      | :func:`shape`
      | :func:`resizemode`
      | :func:`shapesize` | :func:`turtlesize`
      | :func:`shearfactor`
      | :func:`tiltangle`
      | :func:`tilt`
      | :func:`shapetransform`
      | :func:`get_shapepoly`

Using events
   | :func:`onclick`
   | :func:`onrelease`
   | :func:`ondrag`

Special Turtle methods
   | :func:`poly`
   | :func:`begin_poly`
   | :func:`end_poly`
   | :func:`get_poly`
   | :func:`clone`
   | :func:`getturtle` | :func:`getpen`
   | :func:`getscreen`
   | :func:`setundobuffer`
   | :func:`undobufferentries`


Methods of TurtleScreen/Screen
------------------------------

Window control
   | :func:`bgcolor`
   | :func:`bgpic`
   | :func:`clearscreen`
   | :func:`resetscreen`
   | :func:`screensize`
   | :func:`setworldcoordinates`

Animation control
   | :func:`no_animation`
   | :func:`delay`
   | :func:`tracer`
   | :func:`update`

Using screen events
   | :func:`listen`
   | :func:`onkey` | :func:`onkeyrelease`
   | :func:`onkeypress`
   | :func:`onclick` | :func:`onscreenclick`
   | :func:`ontimer`
   | :func:`mainloop` | :func:`done`

Settings and special methods
   | :func:`mode`
   | :func:`colormode`
   | :func:`getcanvas`
   | :func:`getshapes`
   | :func:`register_shape` | :func:`addshape`
   | :func:`turtles`
   | :func:`window_height`
   | :func:`window_width`

Input methods
   | :func:`textinput`
   | :func:`numinput`

Methods specific to Screen
   | :func:`bye`
   | :func:`exitonclick`
   | :func:`save`
   | :func:`setup`
   | :func:`title`


Methods of RawTurtle/Turtle and corresponding functions
=======================================================

Most of the examples in this section refer to a Turtle instance called
``turtle``.

Turtle motion
-------------

.. function:: forward(distance)
              fd(distance)

   :param distance: a number (integer or float)

   Move the turtle forward by the specified *distance*, in the direction the
   turtle is headed.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.position()
      (0.00,0.00)
      >>> turtle.forward(25)
      >>> turtle.position()
      (25.00,0.00)
      >>> turtle.forward(-75)
      >>> turtle.position()
      (-50.00,0.00)


.. function:: back(distance)
              bk(distance)
              backward(distance)

   :param distance: a number

   Move the turtle backward by *distance*, opposite to the direction the
   turtle is headed.  Do not change the turtle's heading.

   .. doctest::
      :hide:

      >>> turtle.goto(0, 0)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.position()
      (0.00,0.00)
      >>> turtle.backward(30)
      >>> turtle.position()
      (-30.00,0.00)


.. function:: right(angle)
              rt(angle)

   :param angle: a number (integer or float)

   Turn turtle right by *angle* units.  (Units are by default degrees, but
   can be set via the :func:`degrees` and :func:`radians` functions.)  Angle
   orientation depends on the turtle mode, see :func:`mode`.

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.setheading(22)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.heading()
      22.0
      >>> turtle.right(45)
      >>> turtle.heading()
      337.0


.. function:: left(angle)
              lt(angle)

   :param angle: a number (integer or float)

   Turn turtle left by *angle* units.  (Units are by default degrees, but
   can be set via the :func:`degrees` and :func:`radians` functions.)  Angle
   orientation depends on the turtle mode, see :func:`mode`.

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.setheading(22)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.heading()
      22.0
      >>> turtle.left(45)
      >>> turtle.heading()
      67.0


.. function:: goto(x, y=None)
              setpos(x, y=None)
              setposition(x, y=None)

   :param x: a number or a pair/vector of numbers
   :param y: a number or ``None``

   If *y* is ``None``, *x* must be a pair of coordinates or a :class:`Vec2D`
   (e.g. as returned by :func:`pos`).

   Move turtle to an absolute position.  If the pen is down, draw line.  Do
   not change the turtle's orientation.

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.goto(0, 0)

   .. doctest::
      :skipif: _tkinter is None

      >>> tp = turtle.pos()
      >>> tp
      (0.00,0.00)
      >>> turtle.setpos(60,30)
      >>> turtle.pos()
      (60.00,30.00)
      >>> turtle.setpos((20,80))
      >>> turtle.pos()
      (20.00,80.00)
      >>> turtle.setpos(tp)
      >>> turtle.pos()
      (0.00,0.00)


.. function:: teleport(x, y=None, *, fill_gap=False)

   :param x: a number or ``None``
   :param y: a number or ``None``
   :param fill_gap: a boolean

   Move turtle to an absolute position. Unlike goto(x, y), a line will not
   be drawn. The turtle's orientation does not change. If currently
   filling, the polygon(s) teleported from will be filled after leaving,
   and filling will begin again after teleporting. This can be disabled
   with fill_gap=True, which makes the imaginary line traveled during
   teleporting act as a fill barrier like in goto(x, y).

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.goto(0, 0)

   .. doctest::
      :skipif: _tkinter is None

      >>> tp = turtle.pos()
      >>> tp
      (0.00,0.00)
      >>> turtle.teleport(60)
      >>> turtle.pos()
      (60.00,0.00)
      >>> turtle.teleport(y=10)
      >>> turtle.pos()
      (60.00,10.00)
      >>> turtle.teleport(20, 30)
      >>> turtle.pos()
      (20.00,30.00)

   .. versionadded:: 3.12


.. function:: setx(x)

   :param x: a number (integer or float)

   Set the turtle's first coordinate to *x*, leave second coordinate
   unchanged.

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.goto(0, 240)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.position()
      (0.00,240.00)
      >>> turtle.setx(10)
      >>> turtle.position()
      (10.00,240.00)


.. function:: sety(y)

   :param y: a number (integer or float)

   Set the turtle's second coordinate to *y*, leave first coordinate unchanged.

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.goto(0, 40)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.position()
      (0.00,40.00)
      >>> turtle.sety(-10)
      >>> turtle.position()
      (0.00,-10.00)


.. function:: setheading(to_angle)
              seth(to_angle)

   :param to_angle: a number (integer or float)

   Set the orientation of the turtle to *to_angle*.  Here are some common
   directions in degrees:

   =================== ====================
    standard mode           logo mode
   =================== ====================
      0 - east                0 - north
     90 - north              90 - east
    180 - west              180 - south
    270 - south             270 - west
   =================== ====================

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.setheading(90)
      >>> turtle.heading()
      90.0


.. function:: home()

   Move turtle to the origin -- coordinates (0,0) -- and set its heading to
   its start-orientation (which depends on the mode, see :func:`mode`).

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.setheading(90)
      >>> turtle.goto(0, -10)

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.heading()
      90.0
      >>> turtle.position()
      (0.00,-10.00)
      >>> turtle.home()
      >>> turtle.position()
      (0.00,0.00)
      >>> turtle.heading()
      0.0


.. function:: circle(radius, extent=None, steps=None)

   :param radius: a number
   :param extent: a number (or ``None``)
   :param steps: an integer (or ``None``)

   Draw a circle with given *radius*.  The center is *radius* units left of
   the turtle; *extent* -- an angle -- determines which part of the circle
   is drawn.  If *extent* is not given, draw the entire circle.  If *extent*
   is not a full circle, one endpoint of the arc is the current pen
   position.  Draw the arc in counterclockwise direction if *radius* is
   positive, otherwise in clockwise direction.  Finally the direction of the
   turtle is changed by the amount of *extent*.

   As the circle is approximated by an inscribed regular polygon, *steps*
   determines the number of steps to use.  If not given, it will be
   calculated automatically.  May be used to draw regular polygons.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.position()
      (0.00,0.00)
      >>> turtle.heading()
      0.0
      >>> turtle.circle(50)
      >>> turtle.position()
      (-0.00,0.00)
      >>> turtle.heading()
      0.0
      >>> turtle.circle(120, 180)  # draw a semicircle
      >>> turtle.position()
      (0.00,240.00)
      >>> turtle.heading()
      180.0


.. function:: dot(size=None, *color)

   :param size: an integer >= 1 (if given)
   :param color: a colorstring or a numeric color tuple

   Draw a circular dot with diameter *size*, using *color*.  If *size* is
   not given, the maximum of pensize+4 and 2*pensize is used.


   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.dot()
      >>> turtle.fd(50); turtle.dot(20, "blue"); turtle.fd(50)
      >>> turtle.position()
      (100.00,-0.00)
      >>> turtle.heading()
      0.0


.. function:: stamp()

   Stamp a copy of the turtle shape onto the canvas at the current turtle
   position.  Return a stamp_id for that stamp, which can be used to delete
   it by calling ``clearstamp(stamp_id)``.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.color("blue")
      >>> stamp_id = turtle.stamp()
      >>> turtle.fd(50)


.. function:: clearstamp(stampid)

   :param stampid: an integer, must be return value of previous
                   :func:`stamp` call

   Delete stamp with given *stampid*.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.position()
      (150.00,-0.00)
      >>> turtle.color("blue")
      >>> astamp = turtle.stamp()
      >>> turtle.fd(50)
      >>> turtle.position()
      (200.00,-0.00)
      >>> turtle.clearstamp(astamp)
      >>> turtle.position()
      (200.00,-0.00)


.. function:: clearstamps(n=None)

   :param n: an integer (or ``None``)

   Delete all or first/last *n* of turtle's stamps.  If *n* is ``None``, delete
   all stamps, if *n* > 0 delete first *n* stamps, else if *n* < 0 delete
   last *n* stamps.

   .. doctest::

      >>> for i in range(8):
      ...     unused_stamp_id = turtle.stamp()
      ...     turtle.fd(30)
      >>> turtle.clearstamps(2)
      >>> turtle.clearstamps(-2)
      >>> turtle.clearstamps()


.. function:: undo()

   Undo (repeatedly) the last turtle action(s).  Number of available
   undo actions is determined by the size of the undobuffer.

   .. doctest::
      :skipif: _tkinter is None

      >>> for i in range(4):
      ...     turtle.fd(50); turtle.lt(80)
      ...
      >>> for i in range(8):
      ...     turtle.undo()


.. function:: speed(speed=None)

   :param speed: an integer in the range 0..10 or a speedstring (see below)

   Set the turtle's speed to an integer value in the range 0..10.  If no
   argument is given, return current speed.

   If input is a number greater than 10 or smaller than 0.5, speed is set
   to 0.  Speedstrings are mapped to speedvalues as follows:

   * "fastest":  0
   * "fast":  10
   * "normal":  6
   * "slow":  3
   * "slowest":  1

   Speeds from 1 to 10 enforce increasingly faster animation of line drawing
   and turtle turning.

   Attention: *speed* = 0 means that *no* animation takes
   place. forward/back makes turtle jump and likewise left/right make the
   turtle turn instantly.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.speed()
      3
      >>> turtle.speed('normal')
      >>> turtle.speed()
      6
      >>> turtle.speed(9)
      >>> turtle.speed()
      9


Tell Turtle's state
-------------------

.. function:: position()
              pos()

   Return the turtle's current location (x,y) (as a :class:`Vec2D` vector).

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.pos()
      (440.00,-0.00)


.. function:: towards(x, y=None)

   :param x: a number or a pair/vector of numbers or a turtle instance
   :param y: a number if *x* is a number, else ``None``

   Return the angle between the line from turtle position to position specified
   by (x,y), the vector or the other turtle.  This depends on the turtle's start
   orientation which depends on the mode - "standard"/"world" or "logo".

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.goto(10, 10)
      >>> turtle.towards(0,0)
      225.0


.. function:: xcor()

   Return the turtle's x coordinate.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.left(50)
      >>> turtle.forward(100)
      >>> turtle.pos()
      (64.28,76.60)
      >>> print(round(turtle.xcor(), 5))
      64.27876


.. function:: ycor()

   Return the turtle's y coordinate.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.left(60)
      >>> turtle.forward(100)
      >>> print(turtle.pos())
      (50.00,86.60)
      >>> print(round(turtle.ycor(), 5))
      86.60254


.. function:: heading()

   Return the turtle's current heading (value depends on the turtle mode, see
   :func:`mode`).

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.left(67)
      >>> turtle.heading()
      67.0


.. function:: distance(x, y=None)

   :param x: a number or a pair/vector of numbers or a turtle instance
   :param y: a number if *x* is a number, else ``None``

   Return the distance from the turtle to (x,y), the given vector, or the given
   other turtle, in turtle step units.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.distance(30,40)
      50.0
      >>> turtle.distance((30,40))
      50.0
      >>> joe = Turtle()
      >>> joe.forward(77)
      >>> turtle.distance(joe)
      77.0


Settings for measurement
------------------------

.. function:: degrees(fullcircle=360.0)

   :param fullcircle: a number

   Set angle measurement units, i.e. set number of "degrees" for a full circle.
   Default value is 360 degrees.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.left(90)
      >>> turtle.heading()
      90.0

      >>> # Change angle measurement unit to grad (also known as gon,
      >>> # grade, or gradian and equals 1/100-th of the right angle.)
      >>> turtle.degrees(400.0)
      >>> turtle.heading()
      100.0
      >>> turtle.degrees(360)
      >>> turtle.heading()
      90.0


.. function:: radians()

   Set the angle measurement units to radians.  Equivalent to
   ``degrees(2*math.pi)``.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.left(90)
      >>> turtle.heading()
      90.0
      >>> turtle.radians()
      >>> turtle.heading()
      1.5707963267948966

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> turtle.degrees(360)


Pen control
-----------

Drawing state
~~~~~~~~~~~~~

.. function:: pendown()
              pd()
              down()

   Pull the pen down -- drawing when moving.


.. function:: penup()
              pu()
              up()

   Pull the pen up -- no drawing when moving.


.. function:: pensize(width=None)
              width(width=None)

   :param width: a positive number

   Set the line thickness to *width* or return it.  If resizemode is set to
   "auto" and turtleshape is a polygon, that polygon is drawn with the same line
   thickness.  If no argument is given, the current pensize is returned.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.pensize()
      1
      >>> turtle.pensize(10)   # from here on lines of width 10 are drawn


.. function:: pen(pen=None, **pendict)

   :param pen: a dictionary with some or all of the below listed keys
   :param pendict: one or more keyword-arguments with the below listed keys as keywords

   Return or set the pen's attributes in a "pen-dictionary" with the following
   key/value pairs:

   * "shown": True/False
   * "pendown": True/False
   * "pencolor": color-string or color-tuple
   * "fillcolor": color-string or color-tuple
   * "pensize": positive number
   * "speed": number in range 0..10
   * "resizemode": "auto" or "user" or "noresize"
   * "stretchfactor": (positive number, positive number)
   * "outline": positive number
   * "tilt": number

   This dictionary can be used as argument for a subsequent call to :func:`pen`
   to restore the former pen-state.  Moreover one or more of these attributes
   can be provided as keyword-arguments.  This can be used to set several pen
   attributes in one statement.

   .. doctest::
      :skipif: _tkinter is None
      :options: +NORMALIZE_WHITESPACE

      >>> turtle.pen(fillcolor="black", pencolor="red", pensize=10)
      >>> sorted(turtle.pen().items())
      [('fillcolor', 'black'), ('outline', 1), ('pencolor', 'red'),
       ('pendown', True), ('pensize', 10), ('resizemode', 'noresize'),
       ('shearfactor', 0.0), ('shown', True), ('speed', 9),
       ('stretchfactor', (1.0, 1.0)), ('tilt', 0.0)]
      >>> penstate=turtle.pen()
      >>> turtle.color("yellow", "")
      >>> turtle.penup()
      >>> sorted(turtle.pen().items())[:3]
      [('fillcolor', ''), ('outline', 1), ('pencolor', 'yellow')]
      >>> turtle.pen(penstate, fillcolor="green")
      >>> sorted(turtle.pen().items())[:3]
      [('fillcolor', 'green'), ('outline', 1), ('pencolor', 'red')]

.. function:: isdown()

   Return ``True`` if pen is down, ``False`` if it's up.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.penup()
      >>> turtle.isdown()
      False
      >>> turtle.pendown()
      >>> turtle.isdown()
      True


Color control
~~~~~~~~~~~~~

.. function:: pencolor(*args)

   Return or set the pencolor.

   Four input formats are allowed:

   ``pencolor()``
      Return the current pencolor as color specification string or
      as a tuple (see example).  May be used as input to another
      color/pencolor/fillcolor call.

   ``pencolor(colorstring)``
      Set pencolor to *colorstring*, which is a Tk color specification string,
      such as ``"red"``, ``"yellow"``, or ``"#33cc8c"``.

   ``pencolor((r, g, b))``
      Set pencolor to the RGB color represented by the tuple of *r*, *g*, and
      *b*.  Each of *r*, *g*, and *b* must be in the range 0..colormode, where
      colormode is either 1.0 or 255 (see :func:`colormode`).

   ``pencolor(r, g, b)``
      Set pencolor to the RGB color represented by *r*, *g*, and *b*.  Each of
      *r*, *g*, and *b* must be in the range 0..colormode.

   If turtleshape is a polygon, the outline of that polygon is drawn with the
   newly set pencolor.

   .. doctest::
      :skipif: _tkinter is None

      >>> colormode()
      1.0
      >>> turtle.pencolor()
      'red'
      >>> turtle.pencolor("brown")
      >>> turtle.pencolor()
      'brown'
      >>> tup = (0.2, 0.8, 0.55)
      >>> turtle.pencolor(tup)
      >>> turtle.pencolor()
      (0.2, 0.8, 0.5490196078431373)
      >>> colormode(255)
      >>> turtle.pencolor()
      (51.0, 204.0, 140.0)
      >>> turtle.pencolor('#32c18f')
      >>> turtle.pencolor()
      (50.0, 193.0, 143.0)


.. function:: fillcolor(*args)

   Return or set the fillcolor.

   Four input formats are allowed:

   ``fillcolor()``
      Return the current fillcolor as color specification string, possibly
      in tuple format (see example).  May be used as input to another
      color/pencolor/fillcolor call.

   ``fillcolor(colorstring)``
      Set fillcolor to *colorstring*, which is a Tk color specification string,
      such as ``"red"``, ``"yellow"``, or ``"#33cc8c"``.

   ``fillcolor((r, g, b))``
      Set fillcolor to the RGB color represented by the tuple of *r*, *g*, and
      *b*.  Each of *r*, *g*, and *b* must be in the range 0..colormode, where
      colormode is either 1.0 or 255 (see :func:`colormode`).

   ``fillcolor(r, g, b)``
      Set fillcolor to the RGB color represented by *r*, *g*, and *b*.  Each of
      *r*, *g*, and *b* must be in the range 0..colormode.

   If turtleshape is a polygon, the interior of that polygon is drawn
   with the newly set fillcolor.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.fillcolor("violet")
      >>> turtle.fillcolor()
      'violet'
      >>> turtle.pencolor()
      (50.0, 193.0, 143.0)
      >>> turtle.fillcolor((50, 193, 143))  # Integers, not floats
      >>> turtle.fillcolor()
      (50.0, 193.0, 143.0)
      >>> turtle.fillcolor('#ffffff')
      >>> turtle.fillcolor()
      (255.0, 255.0, 255.0)


.. function:: color(*args)

   Return or set pencolor and fillcolor.

   Several input formats are allowed.  They use 0 to 3 arguments as
   follows:

   ``color()``
      Return the current pencolor and the current fillcolor as a pair of color
      specification strings or tuples as returned by :func:`pencolor` and
      :func:`fillcolor`.

   ``color(colorstring)``, ``color((r,g,b))``, ``color(r,g,b)``
      Inputs as in :func:`pencolor`, set both, fillcolor and pencolor, to the
      given value.

   ``color(colorstring1, colorstring2)``, ``color((r1,g1,b1), (r2,g2,b2))``
      Equivalent to ``pencolor(colorstring1)`` and ``fillcolor(colorstring2)``
      and analogously if the other input format is used.

   If turtleshape is a polygon, outline and interior of that polygon is drawn
   with the newly set colors.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.color("red", "green")
      >>> turtle.color()
      ('red', 'green')
      >>> color("#285078", "#a0c8f0")
      >>> color()
      ((40.0, 80.0, 120.0), (160.0, 200.0, 240.0))


See also: Screen method :func:`colormode`.


Filling
~~~~~~~

.. doctest::
   :skipif: _tkinter is None
   :hide:

   >>> turtle.home()

.. function:: filling()

   Return fillstate (``True`` if filling, ``False`` else).

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.begin_fill()
      >>> if turtle.filling():
      ...    turtle.pensize(5)
      ... else:
      ...    turtle.pensize(3)

.. function:: fill()

   Fill the shape drawn in the ``with turtle.fill():`` block.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.color("black", "red")
      >>> with turtle.fill():
      ...     turtle.circle(80)

   Using :func:`!fill` is equivalent to adding the :func:`begin_fill` before the
   fill-block and :func:`end_fill` after the fill-block:

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.color("black", "red")
      >>> turtle.begin_fill()
      >>> turtle.circle(80)
      >>> turtle.end_fill()

   .. versionadded:: 3.14


.. function:: begin_fill()

   To be called just before drawing a shape to be filled.


.. function:: end_fill()

   Fill the shape drawn after the last call to :func:`begin_fill`.

   Whether or not overlap regions for self-intersecting polygons
   or multiple shapes are filled depends on the operating system graphics,
   type of overlap, and number of overlaps.  For example, the Turtle star
   above may be either all yellow or have some white regions.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.color("black", "red")
      >>> turtle.begin_fill()
      >>> turtle.circle(80)
      >>> turtle.end_fill()


More drawing control
~~~~~~~~~~~~~~~~~~~~

.. function:: reset()

   Delete the turtle's drawings from the screen, re-center the turtle and set
   variables to the default values.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.goto(0,-22)
      >>> turtle.left(100)
      >>> turtle.position()
      (0.00,-22.00)
      >>> turtle.heading()
      100.0
      >>> turtle.reset()
      >>> turtle.position()
      (0.00,0.00)
      >>> turtle.heading()
      0.0


.. function:: clear()

   Delete the turtle's drawings from the screen.  Do not move turtle.  State and
   position of the turtle as well as drawings of other turtles are not affected.


.. function:: write(arg, move=False, align="left", font=("Arial", 8, "normal"))

   :param arg: object to be written to the TurtleScreen
   :param move: True/False
   :param align: one of the strings "left", "center" or right"
   :param font: a triple (fontname, fontsize, fonttype)

   Write text - the string representation of *arg* - at the current turtle
   position according to *align* ("left", "center" or "right") and with the given
   font.  If *move* is true, the pen is moved to the bottom-right corner of the
   text.  By default, *move* is ``False``.

   >>> turtle.write("Home = ", True, align="center")
   >>> turtle.write((0,0), True)


Turtle state
------------

Visibility
~~~~~~~~~~

.. function:: hideturtle()
              ht()

   Make the turtle invisible.  It's a good idea to do this while you're in the
   middle of doing some complex drawing, because hiding the turtle speeds up the
   drawing observably.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.hideturtle()


.. function:: showturtle()
              st()

   Make the turtle visible.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.showturtle()


.. function:: isvisible()

   Return ``True`` if the Turtle is shown, ``False`` if it's hidden.

   >>> turtle.hideturtle()
   >>> turtle.isvisible()
   False
   >>> turtle.showturtle()
   >>> turtle.isvisible()
   True


Appearance
~~~~~~~~~~

.. function:: shape(name=None)

   :param name: a string which is a valid shapename

   Set turtle shape to shape with given *name* or, if name is not given, return
   name of current shape.  Shape with *name* must exist in the TurtleScreen's
   shape dictionary.  Initially there are the following polygon shapes: "arrow",
   "turtle", "circle", "square", "triangle", "classic".  To learn about how to
   deal with shapes see Screen method :func:`register_shape`.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.shape()
      'classic'
      >>> turtle.shape("turtle")
      >>> turtle.shape()
      'turtle'


.. function:: resizemode(rmode=None)

   :param rmode: one of the strings "auto", "user", "noresize"

   Set resizemode to one of the values: "auto", "user", "noresize".  If *rmode*
   is not given, return current resizemode.  Different resizemodes have the
   following effects:

   - "auto": adapts the appearance of the turtle corresponding to the value of pensize.
   - "user": adapts the appearance of the turtle according to the values of
     stretchfactor and outlinewidth (outline), which are set by
     :func:`shapesize`.
   - "noresize": no adaption of the turtle's appearance takes place.

   ``resizemode("user")`` is called by :func:`shapesize` when used with arguments.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.resizemode()
      'noresize'
      >>> turtle.resizemode("auto")
      >>> turtle.resizemode()
      'auto'


.. function:: shapesize(stretch_wid=None, stretch_len=None, outline=None)
              turtlesize(stretch_wid=None, stretch_len=None, outline=None)

   :param stretch_wid: positive number
   :param stretch_len: positive number
   :param outline: positive number

   Return or set the pen's attributes x/y-stretchfactors and/or outline.  Set
   resizemode to "user".  If and only if resizemode is set to "user", the turtle
   will be displayed stretched according to its stretchfactors: *stretch_wid* is
   stretchfactor perpendicular to its orientation, *stretch_len* is
   stretchfactor in direction of its orientation, *outline* determines the width
   of the shape's outline.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.shapesize()
      (1.0, 1.0, 1)
      >>> turtle.resizemode("user")
      >>> turtle.shapesize(5, 5, 12)
      >>> turtle.shapesize()
      (5, 5, 12)
      >>> turtle.shapesize(outline=8)
      >>> turtle.shapesize()
      (5, 5, 8)


.. function:: shearfactor(shear=None)

   :param shear: number (optional)

   Set or return the current shearfactor. Shear the turtleshape according to
   the given shearfactor shear, which is the tangent of the shear angle.
   Do *not* change the turtle's heading (direction of movement).
   If shear is not given: return the current shearfactor, i. e. the
   tangent of the shear angle, by which lines parallel to the
   heading of the turtle are sheared.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.shape("circle")
      >>> turtle.shapesize(5,2)
      >>> turtle.shearfactor(0.5)
      >>> turtle.shearfactor()
      0.5


.. function:: tilt(angle)

   :param angle: a number

   Rotate the turtleshape by *angle* from its current tilt-angle, but do *not*
   change the turtle's heading (direction of movement).

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.reset()
      >>> turtle.shape("circle")
      >>> turtle.shapesize(5,2)
      >>> turtle.tilt(30)
      >>> turtle.fd(50)
      >>> turtle.tilt(30)
      >>> turtle.fd(50)


.. function:: tiltangle(angle=None)

   :param angle: a number (optional)

   Set or return the current tilt-angle. If angle is given, rotate the
   turtleshape to point in the direction specified by angle,
   regardless of its current tilt-angle. Do *not* change the turtle's
   heading (direction of movement).
   If angle is not given: return the current tilt-angle, i. e. the angle
   between the orientation of the turtleshape and the heading of the
   turtle (its direction of movement).

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.reset()
      >>> turtle.shape("circle")
      >>> turtle.shapesize(5,2)
      >>> turtle.tilt(45)
      >>> turtle.tiltangle()
      45.0


.. function:: shapetransform(t11=None, t12=None, t21=None, t22=None)

   :param t11: a number (optional)
   :param t12: a number (optional)
   :param t21: a number (optional)
   :param t12: a number (optional)

   Set or return the current transformation matrix of the turtle shape.

   If none of the matrix elements are given, return the transformation
   matrix as a tuple of 4 elements.
   Otherwise set the given elements and transform the turtleshape
   according to the matrix consisting of first row t11, t12 and
   second row t21, t22. The determinant t11 * t22 - t12 * t21 must not be
   zero, otherwise an error is raised.
   Modify stretchfactor, shearfactor and tiltangle according to the
   given matrix.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle = Turtle()
      >>> turtle.shape("square")
      >>> turtle.shapesize(4,2)
      >>> turtle.shearfactor(-0.5)
      >>> turtle.shapetransform()
      (4.0, -1.0, -0.0, 2.0)


.. function:: get_shapepoly()

   Return the current shape polygon as tuple of coordinate pairs. This
   can be used to define a new shape or components of a compound shape.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.shape("square")
      >>> turtle.shapetransform(4, -1, 0, 2)
      >>> turtle.get_shapepoly()
      ((50, -20), (30, 20), (-50, 20), (-30, -20))


Using events
------------

.. function:: onclick(fun, btn=1, add=None)
   :noindex:

   :param fun: a function with two arguments which will be called with the
               coordinates of the clicked point on the canvas
   :param btn: number of the mouse-button, defaults to 1 (left mouse button)
   :param add: ``True`` or ``False`` -- if ``True``, a new binding will be
               added, otherwise it will replace a former binding

   Bind *fun* to mouse-click events on this turtle.  If *fun* is ``None``,
   existing bindings are removed.  Example for the anonymous turtle, i.e. the
   procedural way:

   .. doctest::
      :skipif: _tkinter is None

      >>> def turn(x, y):
      ...     left(180)
      ...
      >>> onclick(turn)  # Now clicking into the turtle will turn it.
      >>> onclick(None)  # event-binding will be removed


.. function:: onrelease(fun, btn=1, add=None)

   :param fun: a function with two arguments which will be called with the
               coordinates of the clicked point on the canvas
   :param btn: number of the mouse-button, defaults to 1 (left mouse button)
   :param add: ``True`` or ``False`` -- if ``True``, a new binding will be
               added, otherwise it will replace a former binding

   Bind *fun* to mouse-button-release events on this turtle.  If *fun* is
   ``None``, existing bindings are removed.

   .. doctest::
      :skipif: _tkinter is None

      >>> class MyTurtle(Turtle):
      ...     def glow(self,x,y):
      ...         self.fillcolor("red")
      ...     def unglow(self,x,y):
      ...         self.fillcolor("")
      ...
      >>> turtle = MyTurtle()
      >>> turtle.onclick(turtle.glow)     # clicking on turtle turns fillcolor red,
      >>> turtle.onrelease(turtle.unglow) # releasing turns it to transparent.


.. function:: ondrag(fun, btn=1, add=None)

   :param fun: a function with two arguments which will be called with the
               coordinates of the clicked point on the canvas
   :param btn: number of the mouse-button, defaults to 1 (left mouse button)
   :param add: ``True`` or ``False`` -- if ``True``, a new binding will be
               added, otherwise it will replace a former binding

   Bind *fun* to mouse-move events on this turtle.  If *fun* is ``None``,
   existing bindings are removed.

   Remark: Every sequence of mouse-move-events on a turtle is preceded by a
   mouse-click event on that turtle.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.ondrag(turtle.goto)

   Subsequently, clicking and dragging the Turtle will move it across
   the screen thereby producing handdrawings (if pen is down).


Special Turtle methods
----------------------


.. function:: poly()

   Record the vertices of a polygon drawn in the ``with turtle.poly():`` block.
   The first and last vertices will be connected.

   .. doctest::
      :skipif: _tkinter is None

      >>> with turtle.poly():
      ...     turtle.forward(100)
      ...     turtle.right(60)
      ...     turtle.forward(100)

   .. versionadded:: 3.14


.. function:: begin_poly()

   Start recording the vertices of a polygon.  Current turtle position is first
   vertex of polygon.


.. function:: end_poly()

   Stop recording the vertices of a polygon.  Current turtle position is last
   vertex of polygon.  This will be connected with the first vertex.


.. function:: get_poly()

   Return the last recorded polygon.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.home()
      >>> turtle.begin_poly()
      >>> turtle.fd(100)
      >>> turtle.left(20)
      >>> turtle.fd(30)
      >>> turtle.left(60)
      >>> turtle.fd(50)
      >>> turtle.end_poly()
      >>> p = turtle.get_poly()
      >>> register_shape("myFavouriteShape", p)


.. function:: clone()

   Create and return a clone of the turtle with same position, heading and
   turtle properties.

   .. doctest::
      :skipif: _tkinter is None

      >>> mick = Turtle()
      >>> joe = mick.clone()


.. function:: getturtle()
              getpen()

   Return the Turtle object itself.  Only reasonable use: as a function to
   return the "anonymous turtle":

   .. doctest::
      :skipif: _tkinter is None

      >>> pet = getturtle()
      >>> pet.fd(50)
      >>> pet
      <turtle.Turtle object at 0x...>


.. function:: getscreen()

   Return the :class:`TurtleScreen` object the turtle is drawing on.
   TurtleScreen methods can then be called for that object.

   .. doctest::
      :skipif: _tkinter is None

      >>> ts = turtle.getscreen()
      >>> ts
      <turtle._Screen object at 0x...>
      >>> ts.bgcolor("pink")


.. function:: setundobuffer(size)

   :param size: an integer or ``None``

   Set or disable undobuffer.  If *size* is an integer, an empty undobuffer of
   given size is installed.  *size* gives the maximum number of turtle actions
   that can be undone by the :func:`undo` method/function.  If *size* is
   ``None``, the undobuffer is disabled.

   .. doctest::
      :skipif: _tkinter is None

      >>> turtle.setundobuffer(42)


.. function:: undobufferentries()

   Return number of entries in the undobuffer.

   .. doctest::
      :skipif: _tkinter is None

      >>> while undobufferentries():
      ...     undo()



.. _compoundshapes:

Compound shapes
---------------

To use compound turtle shapes, which consist of several polygons of different
color, you must use the helper class :class:`Shape` explicitly as described
below:

1. Create an empty Shape object of type "compound".
2. Add as many components to this object as desired, using the
   :meth:`~Shape.addcomponent` method.

   For example:

   .. doctest::
      :skipif: _tkinter is None

      >>> s = Shape("compound")
      >>> poly1 = ((0,0),(10,-5),(0,10),(-10,-5))
      >>> s.addcomponent(poly1, "red", "blue")
      >>> poly2 = ((0,0),(10,-5),(-10,-5))
      >>> s.addcomponent(poly2, "blue", "red")

3. Now add the Shape to the Screen's shapelist and use it:

   .. doctest::
      :skipif: _tkinter is None

      >>> register_shape("myshape", s)
      >>> shape("myshape")


.. note::

   The :class:`Shape` class is used internally by the :func:`register_shape`
   method in different ways.  The application programmer has to deal with the
   Shape class *only* when using compound shapes like shown above!


Methods of TurtleScreen/Screen and corresponding functions
==========================================================

Most of the examples in this section refer to a TurtleScreen instance called
``screen``.

.. doctest::
   :skipif: _tkinter is None
   :hide:

   >>> screen = Screen()

Window control
--------------

.. function:: bgcolor(*args)

   :param args: a color string or three numbers in the range 0..colormode or a
                3-tuple of such numbers


   Set or return background color of the TurtleScreen.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.bgcolor("orange")
      >>> screen.bgcolor()
      'orange'
      >>> screen.bgcolor("#800080")
      >>> screen.bgcolor()
      (128.0, 0.0, 128.0)


.. function:: bgpic(picname=None)

   :param picname: a string, name of an image file (PNG, GIF, PGM, and PPM)
                   or ``"nopic"``, or ``None``

   Set background image or return name of current backgroundimage.  If *picname*
   is a filename, set the corresponding image as background.  If *picname* is
   ``"nopic"``, delete background image, if present.  If *picname* is ``None``,
   return the filename of the current backgroundimage. ::

      >>> screen.bgpic()
      'nopic'
      >>> screen.bgpic("landscape.gif")
      >>> screen.bgpic()
      "landscape.gif"


.. function:: clear()
   :noindex:

   .. note::
      This TurtleScreen method is available as a global function only under the
      name ``clearscreen``.  The global function ``clear`` is a different one
      derived from the Turtle method ``clear``.


.. function:: clearscreen()

   Delete all drawings and all turtles from the TurtleScreen.  Reset the now
   empty TurtleScreen to its initial state: white background, no background
   image, no event bindings and tracing on.


.. function:: reset()
   :noindex:

   .. note::
      This TurtleScreen method is available as a global function only under the
      name ``resetscreen``.  The global function ``reset`` is another one
      derived from the Turtle method ``reset``.


.. function:: resetscreen()

   Reset all Turtles on the Screen to their initial state.


.. function:: screensize(canvwidth=None, canvheight=None, bg=None)

   :param canvwidth: positive integer, new width of canvas in pixels
   :param canvheight: positive integer, new height of canvas in pixels
   :param bg: colorstring or color-tuple, new background color

   If no arguments are given, return current (canvaswidth, canvasheight).  Else
   resize the canvas the turtles are drawing on.  Do not alter the drawing
   window.  To observe hidden parts of the canvas, use the scrollbars. With this
   method, one can make visible those parts of a drawing which were outside the
   canvas before.

      >>> screen.screensize()
      (400, 300)
      >>> screen.screensize(2000,1500)
      >>> screen.screensize()
      (2000, 1500)

   e.g. to search for an erroneously escaped turtle ;-)


.. function:: setworldcoordinates(llx, lly, urx, ury)

   :param llx: a number, x-coordinate of lower left corner of canvas
   :param lly: a number, y-coordinate of lower left corner of canvas
   :param urx: a number, x-coordinate of upper right corner of canvas
   :param ury: a number, y-coordinate of upper right corner of canvas

   Set up user-defined coordinate system and switch to mode "world" if
   necessary.  This performs a ``screen.reset()``.  If mode "world" is already
   active, all drawings are redrawn according to the new coordinates.

   **ATTENTION**: in user-defined coordinate systems angles may appear
   distorted.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.reset()
      >>> screen.setworldcoordinates(-50,-7.5,50,7.5)
      >>> for _ in range(72):
      ...     left(10)
      ...
      >>> for _ in range(8):
      ...     left(45); fd(2)   # a regular octagon

   .. doctest::
      :skipif: _tkinter is None
      :hide:

      >>> screen.reset()
      >>> for t in turtles():
      ...      t.reset()


Animation control
-----------------

.. function:: no_animation()

   Temporarily disable turtle animation. The code written inside the
   ``no_animation`` block will not be animated;
   once the code block is exited, the drawing will appear.

   .. doctest::
      :skipif: _tkinter is None

      >>> with screen.no_animation():
      ...     for dist in range(2, 400, 2):
      ...         fd(dist)
      ...         rt(90)

   .. versionadded:: 3.14


.. function:: delay(delay=None)

   :param delay: positive integer

   Set or return the drawing *delay* in milliseconds.  (This is approximately
   the time interval between two consecutive canvas updates.)  The longer the
   drawing delay, the slower the animation.

   Optional argument:

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.delay()
      10
      >>> screen.delay(5)
      >>> screen.delay()
      5


.. function:: tracer(n=None, delay=None)

   :param n: nonnegative integer
   :param delay: nonnegative integer

   Turn turtle animation on/off and set delay for update drawings.  If
   *n* is given, only each n-th regular screen update is really
   performed.  (Can be used to accelerate the drawing of complex
   graphics.)  When called without arguments, returns the currently
   stored value of n. Second argument sets delay value (see
   :func:`delay`).

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.tracer(8, 25)
      >>> dist = 2
      >>> for i in range(200):
      ...     fd(dist)
      ...     rt(90)
      ...     dist += 2


.. function:: update()

   Perform a TurtleScreen update. To be used when tracer is turned off.

See also the RawTurtle/Turtle method :func:`speed`.


Using screen events
-------------------

.. function:: listen(xdummy=None, ydummy=None)

   Set focus on TurtleScreen (in order to collect key-events).  Dummy arguments
   are provided in order to be able to pass :func:`listen` to the onclick method.


.. function:: onkey(fun, key)
              onkeyrelease(fun, key)

   :param fun: a function with no arguments or ``None``
   :param key: a string: key (e.g. "a") or key-symbol (e.g. "space")

   Bind *fun* to key-release event of key.  If *fun* is ``None``, event bindings
   are removed. Remark: in order to be able to register key-events, TurtleScreen
   must have the focus. (See method :func:`listen`.)

   .. doctest::
      :skipif: _tkinter is None

      >>> def f():
      ...     fd(50)
      ...     lt(60)
      ...
      >>> screen.onkey(f, "Up")
      >>> screen.listen()


.. function:: onkeypress(fun, key=None)

   :param fun: a function with no arguments or ``None``
   :param key: a string: key (e.g. "a") or key-symbol (e.g. "space")

   Bind *fun* to key-press event of key if key is given,
   or to any key-press-event if no key is given.
   Remark: in order to be able to register key-events, TurtleScreen
   must have focus. (See method :func:`listen`.)

   .. doctest::
      :skipif: _tkinter is None

      >>> def f():
      ...     fd(50)
      ...
      >>> screen.onkey(f, "Up")
      >>> screen.listen()


.. function:: onclick(fun, btn=1, add=None)
              onscreenclick(fun, btn=1, add=None)

   :param fun: a function with two arguments which will be called with the
               coordinates of the clicked point on the canvas
   :param btn: number of the mouse-button, defaults to 1 (left mouse button)
   :param add: ``True`` or ``False`` -- if ``True``, a new binding will be
               added, otherwise it will replace a former binding

   Bind *fun* to mouse-click events on this screen.  If *fun* is ``None``,
   existing bindings are removed.

   Example for a TurtleScreen instance named ``screen`` and a Turtle instance
   named ``turtle``:

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.onclick(turtle.goto) # Subsequently clicking into the TurtleScreen will
      >>>                             # make the turtle move to the clicked point.
      >>> screen.onclick(None)        # remove event binding again

   .. note::
      This TurtleScreen method is available as a global function only under the
      name ``onscreenclick``.  The global function ``onclick`` is another one
      derived from the Turtle method ``onclick``.


.. function:: ontimer(fun, t=0)

   :param fun: a function with no arguments
   :param t: a number >= 0

   Install a timer that calls *fun* after *t* milliseconds.

   .. doctest::
      :skipif: _tkinter is None

      >>> running = True
      >>> def f():
      ...     if running:
      ...         fd(50)
      ...         lt(60)
      ...         screen.ontimer(f, 250)
      >>> f()   ### makes the turtle march around
      >>> running = False


.. function:: mainloop()
              done()

   Starts event loop - calling Tkinter's mainloop function.
   Must be the last statement in a turtle graphics program.
   Must *not* be used if a script is run from within IDLE in -n mode
   (No subprocess) - for interactive use of turtle graphics. ::

      >>> screen.mainloop()


Input methods
-------------

.. function:: textinput(title, prompt)

   :param title: string
   :param prompt: string

   Pop up a dialog window for input of a string. Parameter title is
   the title of the dialog window, prompt is a text mostly describing
   what information to input.
   Return the string input. If the dialog is canceled, return ``None``. ::

      >>> screen.textinput("NIM", "Name of first player:")


.. function:: numinput(title, prompt, default=None, minval=None, maxval=None)

   :param title: string
   :param prompt: string
   :param default: number (optional)
   :param minval: number (optional)
   :param maxval: number (optional)

   Pop up a dialog window for input of a number. title is the title of the
   dialog window, prompt is a text mostly describing what numerical information
   to input. default: default value, minval: minimum value for input,
   maxval: maximum value for input.
   The number input must be in the range minval .. maxval if these are
   given. If not, a hint is issued and the dialog remains open for
   correction.
   Return the number input. If the dialog is canceled,  return ``None``. ::

      >>> screen.numinput("Poker", "Your stakes:", 1000, minval=10, maxval=10000)


Settings and special methods
----------------------------

.. function:: mode(mode=None)

   :param mode: one of the strings "standard", "logo" or "world"

   Set turtle mode ("standard", "logo" or "world") and perform reset.  If mode
   is not given, current mode is returned.

   Mode "standard" is compatible with old :mod:`turtle`.  Mode "logo" is
   compatible with most Logo turtle graphics.  Mode "world" uses user-defined
   "world coordinates". **Attention**: in this mode angles appear distorted if
   ``x/y`` unit-ratio doesn't equal 1.

   ============ ========================= ===================
       Mode      Initial turtle heading     positive angles
   ============ ========================= ===================
    "standard"    to the right (east)       counterclockwise
      "logo"        upward    (north)         clockwise
   ============ ========================= ===================

   .. doctest::
      :skipif: _tkinter is None

      >>> mode("logo")   # resets turtle heading to north
      >>> mode()
      'logo'


.. function:: colormode(cmode=None)

   :param cmode: one of the values 1.0 or 255

   Return the colormode or set it to 1.0 or 255.  Subsequently *r*, *g*, *b*
   values of color triples have to be in the range 0..*cmode*.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.colormode(1)
      >>> turtle.pencolor(240, 160, 80)
      Traceback (most recent call last):
           ...
      TurtleGraphicsError: bad color sequence: (240, 160, 80)
      >>> screen.colormode()
      1.0
      >>> screen.colormode(255)
      >>> screen.colormode()
      255
      >>> turtle.pencolor(240,160,80)


.. function:: getcanvas()

   Return the Canvas of this TurtleScreen.  Useful for insiders who know what to
   do with a Tkinter Canvas.

   .. doctest::
      :skipif: _tkinter is None

      >>> cv = screen.getcanvas()
      >>> cv
      <turtle.ScrolledCanvas object ...>


.. function:: getshapes()

   Return a list of names of all currently available turtle shapes.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.getshapes()
      ['arrow', 'blank', 'circle', ..., 'turtle']


.. function:: register_shape(name, shape=None)
              addshape(name, shape=None)

   There are four different ways to call this function:

   (1) *name* is the name of an image file (PNG, GIF, PGM, and PPM) and *shape* is ``None``: Install the
       corresponding image shape. ::

       >>> screen.register_shape("turtle.gif")

       .. note::
          Image shapes *do not* rotate when turning the turtle, so they do not
          display the heading of the turtle!

   (2) *name* is an arbitrary string and *shape* is the name of an image file (PNG, GIF, PGM, and PPM): Install the
       corresponding image shape. ::

       >>> screen.register_shape("turtle", "turtle.gif")

       .. note::
          Image shapes *do not* rotate when turning the turtle, so they do not
          display the heading of the turtle!

   (3) *name* is an arbitrary string and *shape* is a tuple of pairs of
       coordinates: Install the corresponding polygon shape.

       .. doctest::
          :skipif: _tkinter is None

          >>> screen.register_shape("triangle", ((5,-3), (0,5), (-5,-3)))

   (4) *name* is an arbitrary string and *shape* is a (compound) :class:`Shape`
       object: Install the corresponding compound shape.

   Add a turtle shape to TurtleScreen's shapelist.  Only thusly registered
   shapes can be used by issuing the command ``shape(shapename)``.

   .. versionchanged:: 3.14
      Added support for PNG, PGM, and PPM image formats.
      Both a shape name and an image file name can be specified.


.. function:: turtles()

   Return the list of turtles on the screen.

   .. doctest::
      :skipif: _tkinter is None

      >>> for turtle in screen.turtles():
      ...     turtle.color("red")


.. function:: window_height()

   Return the height of the turtle window. ::

      >>> screen.window_height()
      480


.. function:: window_width()

   Return the width of the turtle window. ::

      >>> screen.window_width()
      640


.. _screenspecific:

Methods specific to Screen, not inherited from TurtleScreen
-----------------------------------------------------------

.. function:: bye()

   Shut the turtlegraphics window.


.. function:: exitonclick()

   Bind ``bye()`` method to mouse clicks on the Screen.


   If the value "using_IDLE" in the configuration dictionary is ``False``
   (default value), also enter mainloop.  Remark: If IDLE with the ``-n`` switch
   (no subprocess) is used, this value should be set to ``True`` in
   :file:`turtle.cfg`.  In this case IDLE's own mainloop is active also for the
   client script.


.. function:: save(filename, overwrite=False)

   Save the current turtle drawing (and turtles) as a PostScript file.

   :param filename: the path of the saved PostScript file
   :param overwrite: if ``False`` and there already exists a file with the given
                     filename, then the function will raise a
                     ``FileExistsError``. If it is ``True``, the file will be
                     overwritten.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.save("my_drawing.ps")
      >>> screen.save("my_drawing.ps", overwrite=True)

   .. versionadded:: 3.14

.. function:: setup(width=_CFG["width"], height=_CFG["height"], startx=_CFG["leftright"], starty=_CFG["topbottom"])

   Set the size and position of the main window.  Default values of arguments
   are stored in the configuration dictionary and can be changed via a
   :file:`turtle.cfg` file.

   :param width: if an integer, a size in pixels, if a float, a fraction of the
                 screen; default is 50% of screen
   :param height: if an integer, the height in pixels, if a float, a fraction of
                  the screen; default is 75% of screen
   :param startx: if positive, starting position in pixels from the left
                  edge of the screen, if negative from the right edge, if ``None``,
                  center window horizontally
   :param starty: if positive, starting position in pixels from the top
                  edge of the screen, if negative from the bottom edge, if ``None``,
                  center window vertically

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.setup (width=200, height=200, startx=0, starty=0)
      >>>              # sets window to 200x200 pixels, in upper left of screen
      >>> screen.setup(width=.75, height=0.5, startx=None, starty=None)
      >>>              # sets window to 75% of screen by 50% of screen and centers


.. function:: title(titlestring)

   :param titlestring: a string that is shown in the titlebar of the turtle
                       graphics window

   Set title of turtle window to *titlestring*.

   .. doctest::
      :skipif: _tkinter is None

      >>> screen.title("Welcome to the turtle zoo!")


Public classes
==============


.. class:: RawTurtle(canvas)
           RawPen(canvas)

   :param canvas: a :class:`!tkinter.Canvas`, a :class:`ScrolledCanvas` or a
                  :class:`TurtleScreen`

   Create a turtle.  The turtle has all methods described above as "methods of
   Turtle/RawTurtle".


.. class:: Turtle()

   Subclass of RawTurtle, has the same interface but draws on a default
   :class:`Screen` object created automatically when needed for the first time.


.. class:: TurtleScreen(cv)

   :param cv: a :class:`!tkinter.Canvas`

   Provides screen oriented methods like :func:`bgcolor` etc. that are described
   above.

.. class:: Screen()

   Subclass of TurtleScreen, with :ref:`four methods added <screenspecific>`.


.. class:: ScrolledCanvas(master)

   :param master: some Tkinter widget to contain the ScrolledCanvas, i.e.
      a Tkinter-canvas with scrollbars added

   Used by class Screen, which thus automatically provides a ScrolledCanvas as
   playground for the turtles.

.. class:: Shape(type_, data)

   :param type\_: one of the strings "polygon", "image", "compound"

   Data structure modeling shapes.  The pair ``(type_, data)`` must follow this
   specification:


   =========== ===========
   *type_*     *data*
   =========== ===========
   "polygon"   a polygon-tuple, i.e. a tuple of pairs of coordinates
   "image"     an image  (in this form only used internally!)
   "compound"  ``None`` (a compound shape has to be constructed using the
               :meth:`addcomponent` method)
   =========== ===========

   .. method:: addcomponent(poly, fill, outline=None)

      :param poly: a polygon, i.e. a tuple of pairs of numbers
      :param fill: a color the *poly* will be filled with
      :param outline: a color for the poly's outline (if given)

      Example:

      .. doctest::
         :skipif: _tkinter is None

         >>> poly = ((0,0),(10,-5),(0,10),(-10,-5))
         >>> s = Shape("compound")
         >>> s.addcomponent(poly, "red", "blue")
         >>> # ... add more components and then use register_shape()

      See :ref:`compoundshapes`.


.. class:: Vec2D(x, y)

   A two-dimensional vector class, used as a helper class for implementing
   turtle graphics.  May be useful for turtle graphics programs too.  Derived
   from tuple, so a vector is a tuple!

   Provides (for *a*, *b* vectors, *k* number):

   * ``a + b`` vector addition
   * ``a - b`` vector subtraction
   * ``a * b`` inner product
   * ``k * a`` and ``a * k`` multiplication with scalar
   * ``abs(a)`` absolute value of a
   * ``a.rotate(angle)`` rotation


.. _turtle-explanation:

Explanation
===========

A turtle object draws on a screen object, and there a number of key classes in
the turtle object-oriented interface that can be used to create them and relate
them to each other.

A :class:`Turtle` instance will automatically create a :class:`Screen`
instance if one is not already present.

``Turtle`` is a subclass of :class:`RawTurtle`, which *doesn't* automatically
create a drawing surface - a *canvas* will need to be provided or created for
it. The *canvas* can be a :class:`!tkinter.Canvas`, :class:`ScrolledCanvas`
or :class:`TurtleScreen`.


:class:`TurtleScreen` is the basic drawing surface for a
turtle. :class:`Screen` is a subclass of ``TurtleScreen``, and
includes :ref:`some additional methods <screenspecific>` for managing its
appearance (including size and title) and behaviour. ``TurtleScreen``'s
constructor needs a :class:`!tkinter.Canvas` or a
:class:`ScrolledCanvas` as an argument.

The functional interface for turtle graphics uses the various methods of
``Turtle`` and ``TurtleScreen``/``Screen``. Behind the scenes, a screen
object is automatically created whenever a function derived from a ``Screen``
method is called. Similarly, a turtle object is automatically created
whenever any of the functions derived from a Turtle method is called.

To use multiple turtles on a screen, the object-oriented interface must be
used.


Help and configuration
======================

How to use help
---------------

The public methods of the Screen and Turtle classes are documented extensively
via docstrings.  So these can be used as online-help via the Python help
facilities:

- When using IDLE, tooltips show the signatures and first lines of the
  docstrings of typed in function-/method calls.

- Calling :func:`help` on methods or functions displays the docstrings::

     >>> help(Screen.bgcolor)
     Help on method bgcolor in module turtle:

     bgcolor(self, *args) unbound turtle.Screen method
         Set or return backgroundcolor of the TurtleScreen.

         Arguments (if given): a color string or three numbers
         in the range 0..colormode or a 3-tuple of such numbers.


         >>> screen.bgcolor("orange")
         >>> screen.bgcolor()
         "orange"
         >>> screen.bgcolor(0.5,0,0.5)
         >>> screen.bgcolor()
         "#800080"

     >>> help(Turtle.penup)
     Help on method penup in module turtle:

     penup(self) unbound turtle.Turtle method
         Pull the pen up -- no drawing when moving.

         Aliases: penup | pu | up

         No argument

         >>> turtle.penup()

- The docstrings of the functions which are derived from methods have a modified
  form::

     >>> help(bgcolor)
     Help on function bgcolor in module turtle:

     bgcolor(*args)
         Set or return backgroundcolor of the TurtleScreen.

         Arguments (if given): a color string or three numbers
         in the range 0..colormode or a 3-tuple of such numbers.

         Example::

           >>> bgcolor("orange")
           >>> bgcolor()
           "orange"
           >>> bgcolor(0.5,0,0.5)
           >>> bgcolor()
           "#800080"

     >>> help(penup)
     Help on function penup in module turtle:

     penup()
         Pull the pen up -- no drawing when moving.

         Aliases: penup | pu | up

         No argument

         Example:
         >>> penup()

These modified docstrings are created automatically together with the function
definitions that are derived from the methods at import time.


Translation of docstrings into different languages
--------------------------------------------------

There is a utility to create a dictionary the keys of which are the method names
and the values of which are the docstrings of the public methods of the classes
Screen and Turtle.

.. function:: write_docstringdict(filename="turtle_docstringdict")

   :param filename: a string, used as filename

   Create and write docstring-dictionary to a Python script with the given
   filename.  This function has to be called explicitly (it is not used by the
   turtle graphics classes).  The docstring dictionary will be written to the
   Python script :file:`{filename}.py`.  It is intended to serve as a template
   for translation of the docstrings into different languages.

If you (or your students) want to use :mod:`turtle` with online help in your
native language, you have to translate the docstrings and save the resulting
file as e.g. :file:`turtle_docstringdict_german.py`.

If you have an appropriate entry in your :file:`turtle.cfg` file this dictionary
will be read in at import time and will replace the original English docstrings.

At the time of this writing there are docstring dictionaries in German and in
Italian.  (Requests please to glingl@aon.at.)



How to configure Screen and Turtles
-----------------------------------

The built-in default configuration mimics the appearance and behaviour of the
old turtle module in order to retain best possible compatibility with it.

If you want to use a different configuration which better reflects the features
of this module or which better fits to your needs, e.g. for use in a classroom,
you can prepare a configuration file ``turtle.cfg`` which will be read at import
time and modify the configuration according to its settings.

The built in configuration would correspond to the following ``turtle.cfg``:

.. code-block:: ini

   width = 0.5
   height = 0.75
   leftright = None
   topbottom = None
   canvwidth = 400
   canvheight = 300
   mode = standard
   colormode = 1.0
   delay = 10
   undobuffersize = 1000
   shape = classic
   pencolor = black
   fillcolor = black
   resizemode = noresize
   visible = True
   language = english
   exampleturtle = turtle
   examplescreen = screen
   title = Python Turtle Graphics
   using_IDLE = False

Short explanation of selected entries:

- The first four lines correspond to the arguments of the :func:`Screen.setup <setup>`
  method.
- Line 5 and 6 correspond to the arguments of the method
  :func:`Screen.screensize <screensize>`.
- *shape* can be any of the built-in shapes, e.g: arrow, turtle, etc.  For more
  info try ``help(shape)``.
- If you want to use no fill color (i.e. make the turtle transparent), you have
  to write ``fillcolor = ""`` (but all nonempty strings must not have quotes in
  the cfg file).
- If you want to reflect the turtle its state, you have to use ``resizemode =
  auto``.
- If you set e.g. ``language = italian`` the docstringdict
  :file:`turtle_docstringdict_italian.py` will be loaded at import time (if
  present on the import path, e.g. in the same directory as :mod:`turtle`).
- The entries *exampleturtle* and *examplescreen* define the names of these
  objects as they occur in the docstrings.  The transformation of
  method-docstrings to function-docstrings will delete these names from the
  docstrings.
- *using_IDLE*: Set this to ``True`` if you regularly work with IDLE and its ``-n``
  switch ("no subprocess").  This will prevent :func:`exitonclick` to enter the
  mainloop.

There can be a :file:`turtle.cfg` file in the directory where :mod:`turtle` is
stored and an additional one in the current working directory.  The latter will
override the settings of the first one.

The :file:`Lib/turtledemo` directory contains a :file:`turtle.cfg` file.  You can
study it as an example and see its effects when running the demos (preferably
not from within the demo-viewer).


:mod:`turtledemo` --- Demo scripts
==================================

.. module:: turtledemo
   :synopsis: A viewer for example turtle scripts

The :mod:`turtledemo` package includes a set of demo scripts.  These
scripts can be run and viewed using the supplied demo viewer as follows::

   python -m turtledemo

Alternatively, you can run the demo scripts individually.  For example, ::

   python -m turtledemo.bytedesign

The :mod:`turtledemo` package directory contains:

- A demo viewer :file:`__main__.py` which can be used to view the sourcecode
  of the scripts and run them at the same time.
- Multiple scripts demonstrating different features of the :mod:`turtle`
  module.  Examples can be accessed via the Examples menu.  They can also
  be run standalone.
- A :file:`turtle.cfg` file which serves as an example of how to write
  and use such files.

The demo scripts are:

.. currentmodule:: turtle

.. tabularcolumns:: |l|L|L|

+----------------+------------------------------+-----------------------+
| Name           | Description                  | Features              |
+================+==============================+=======================+
| bytedesign     | complex classical            | :func:`tracer`, delay,|
|                | turtle graphics pattern      | :func:`update`        |
+----------------+------------------------------+-----------------------+
| chaos          | graphs Verhulst dynamics,    | world coordinates     |
|                | shows that computer's        |                       |
|                | computations can generate    |                       |
|                | results sometimes against the|                       |
|                | common sense expectations    |                       |
+----------------+------------------------------+-----------------------+
| clock          | analog clock showing time    | turtles as clock's    |
|                | of your computer             | hands, ontimer        |
+----------------+------------------------------+-----------------------+
| colormixer     | experiment with r, g, b      | :func:`ondrag`        |
+----------------+------------------------------+-----------------------+
| forest         | 3 breadth-first trees        | randomization         |
+----------------+------------------------------+-----------------------+
| fractalcurves  | Hilbert & Koch curves        | recursion             |
+----------------+------------------------------+-----------------------+
| lindenmayer    | ethnomathematics             | L-System              |
|                | (indian kolams)              |                       |
+----------------+------------------------------+-----------------------+
| minimal_hanoi  | Towers of Hanoi              | Rectangular Turtles   |
|                |                              | as Hanoi discs        |
|                |                              | (shape, shapesize)    |
+----------------+------------------------------+-----------------------+
| nim            | play the classical nim game  | turtles as nimsticks, |
|                | with three heaps of sticks   | event driven (mouse,  |
|                | against the computer.        | keyboard)             |
+----------------+------------------------------+-----------------------+
| paint          | super minimalistic           | :func:`onclick`       |
|                | drawing program              |                       |
+----------------+------------------------------+-----------------------+
| peace          | elementary                   | turtle: appearance    |
|                |                              | and animation         |
+----------------+------------------------------+-----------------------+
| penrose        | aperiodic tiling with        | :func:`stamp`         |
|                | kites and darts              |                       |
+----------------+------------------------------+-----------------------+
| planet_and_moon| simulation of                | compound shapes,      |
|                | gravitational system         | :class:`Vec2D`        |
+----------------+------------------------------+-----------------------+
| rosette        | a pattern from the wikipedia | :func:`clone`,        |
|                | article on turtle graphics   | :func:`undo`          |
+----------------+------------------------------+-----------------------+
| round_dance    | dancing turtles rotating     | compound shapes, clone|
|                | pairwise in opposite         | shapesize, tilt,      |
|                | direction                    | get_shapepoly, update |
+----------------+------------------------------+-----------------------+
| sorting_animate| visual demonstration of      | simple alignment,     |
|                | different sorting methods    | randomization         |
+----------------+------------------------------+-----------------------+
| tree           | a (graphical) breadth        | :func:`clone`         |
|                | first tree (using generators)|                       |
+----------------+------------------------------+-----------------------+
| two_canvases   | simple design                | turtles on two        |
|                |                              | canvases              |
+----------------+------------------------------+-----------------------+
| yinyang        | another elementary example   | :func:`circle`        |
+----------------+------------------------------+-----------------------+

Have fun!


Changes since Python 2.6
========================

- The methods :func:`Turtle.tracer <tracer>`, :func:`Turtle.window_width <window_width>` and
  :func:`Turtle.window_height <window_height>` have been eliminated.
  Methods with these names and functionality are now available only
  as methods of :class:`Screen`. The functions derived from these remain
  available. (In fact already in Python 2.6 these methods were merely
  duplications of the corresponding
  :class:`TurtleScreen`/:class:`Screen` methods.)

- The method :func:`!Turtle.fill` has been eliminated.
  The behaviour of :func:`begin_fill` and :func:`end_fill`
  have changed slightly: now every filling process must be completed with an
  ``end_fill()`` call.

- A method :func:`Turtle.filling <filling>` has been added. It returns a boolean
  value: ``True`` if a filling process is under way, ``False`` otherwise.
  This behaviour corresponds to a ``fill()`` call without arguments in
  Python 2.6.

Changes since Python 3.0
========================

- The :class:`Turtle` methods :func:`shearfactor`, :func:`shapetransform` and
  :func:`get_shapepoly` have been added. Thus the full range of
  regular linear transforms is now available for transforming turtle shapes.
  :func:`tiltangle` has been enhanced in functionality: it now can
  be used to get or set the tilt angle.

- The :class:`Screen` method :func:`onkeypress` has been added as a complement to
  :func:`onkey`. As the latter binds actions to the key release event,
  an alias: :func:`onkeyrelease` was also added for it.

- The method :func:`Screen.mainloop <mainloop>` has been added,
  so there is no longer a need to use the standalone :func:`mainloop` function
  when working with :class:`Screen` and :class:`Turtle` objects.

- Two input methods have been added: :func:`Screen.textinput <textinput>` and
  :func:`Screen.numinput <numinput>`. These pop up input dialogs and return
  strings and numbers respectively.


.. doctest::
   :skipif: _tkinter is None
   :hide:

   >>> for turtle in turtles():
   ...      turtle.reset()
   >>> turtle.penup()
   >>> turtle.goto(-200,25)
   >>> turtle.pendown()
   >>> turtle.write("No one expects the Spanish Inquisition!",
   ...      font=("Arial", 20, "normal"))
   >>> turtle.penup()
   >>> turtle.goto(-100,-50)
   >>> turtle.pendown()
   >>> turtle.write("Our two chief Turtles are...",
   ...      font=("Arial", 16, "normal"))
   >>> turtle.penup()
   >>> turtle.goto(-450,-75)
   >>> turtle.write(str(turtles()))
