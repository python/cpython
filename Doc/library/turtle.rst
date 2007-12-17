
:mod:`turtle` --- Turtle graphics for Tk
========================================

.. module:: turtle
   :platform: Tk
   :synopsis: An environment for turtle graphics.
.. moduleauthor:: Guido van Rossum <guido@python.org>


.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`turtle` module provides turtle graphics primitives, in both an
object-oriented and procedure-oriented ways. Because it uses :mod:`Tkinter` for
the underlying graphics, it needs a version of python installed with Tk support.

The procedural interface uses a pen and a canvas which are automagically created
when any of the functions are called.

The :mod:`turtle` module defines the following functions:


.. function:: degrees()

   Set angle measurement units to degrees.


.. function:: radians()

   Set angle measurement units to radians.


.. function:: setup(**kwargs)

   Sets the size and position of the main window.  Keywords are:

   * ``width``: either a size in pixels or a fraction of the screen. The default is
     50% of the screen.

   * ``height``: either a size in pixels or a fraction of the screen. The default
     is 50% of the screen.

   * ``startx``: starting position in pixels from the left edge of the screen.
     ``None`` is the default value and  centers the window horizontally on screen.

   * ``starty``: starting position in pixels from the top edge of the screen.
     ``None`` is the default value and  centers the window vertically on screen.

   Examples::

      # Uses default geometry: 50% x 50% of screen, centered.
      setup()  

      # Sets window to 200x200 pixels, in upper left of screen
      setup (width=200, height=200, startx=0, starty=0)

      # Sets window to 75% of screen by 50% of screen, and centers it.
      setup(width=.75, height=0.5, startx=None, starty=None)


.. function:: title(title_str)

   Set the window's title to *title*.


.. function:: done()

   Enters the Tk main loop.  The window will continue to  be displayed until the
   user closes it or the process is killed.


.. function:: reset()

   Clear the screen, re-center the pen, and set variables to the default values.


.. function:: clear()

   Clear the screen.


.. function:: tracer(flag)

   Set tracing on/off (according to whether flag is true or not). Tracing means
   line are drawn more slowly, with an animation of an arrow along the  line.


.. function:: speed(speed)

   Set the speed of the turtle. Valid values for the parameter *speed* are
   ``'fastest'`` (no delay), ``'fast'``, (delay 5ms), ``'normal'`` (delay 10ms),
   ``'slow'`` (delay 15ms), and ``'slowest'`` (delay 20ms).


.. function:: delay(delay)

   Set the speed of the turtle to *delay*, which is given in ms.


.. function:: forward(distance)

   Go forward *distance* steps.


.. function:: backward(distance)

   Go backward *distance* steps.


.. function:: left(angle)

   Turn left *angle* units. Units are by default degrees, but can be set via the
   :func:`degrees` and :func:`radians` functions.


.. function:: right(angle)

   Turn right *angle* units. Units are by default degrees, but can be set via the
   :func:`degrees` and :func:`radians` functions.


.. function:: up()

   Move the pen up --- stop drawing.


.. function:: down()

   Move the pen down --- draw when moving.


.. function:: width(width)

   Set the line width to *width*.


.. function:: color(s)
              color((r, g, b))
              color(r, g, b)

   Set the pen color.  In the first form, the color is specified as a Tk color
   specification as a string.  The second form specifies the color as a tuple of
   the RGB values, each in the range [0..1].  For the third form, the color is
   specified giving the RGB values as three separate parameters (each in the range
   [0..1]).


.. function:: write(text[, move])

   Write *text* at the current pen position. If *move* is true, the pen is moved to
   the bottom-right corner of the text. By default, *move* is false.


.. function:: fill(flag)

   The complete specifications are rather complex, but the recommended  usage is:
   call ``fill(1)`` before drawing a path you want to fill, and call ``fill(0)``
   when you finish to draw the path.


.. function:: begin_fill()

   Switch turtle into filling mode;  Must eventually be followed by a corresponding
   end_fill() call. Otherwise it will be ignored.


.. function:: end_fill()

   End filling mode, and fill the shape; equivalent to ``fill(0)``.


.. function:: circle(radius[, extent])

   Draw a circle with radius *radius* whose center-point is *radius* units left of
   the turtle. *extent* determines which part of a circle is drawn: if not given it
   defaults to a full circle.

   If *extent* is not a full circle, one endpoint of the arc is the current pen
   position. The arc is drawn in a counter clockwise direction if *radius* is
   positive, otherwise in a clockwise direction.  In the process, the direction of
   the turtle is changed by the amount of the *extent*.


.. function:: goto(x, y)
              goto((x, y))

   Go to co-ordinates *x*, *y*.  The co-ordinates may be specified either as two
   separate arguments or as a 2-tuple.


.. function:: towards(x, y)

   Return the angle of the line from the turtle's position to the point *x*, *y*.
   The co-ordinates may be specified either as two separate arguments, as a
   2-tuple, or as another pen object.


.. function:: heading()

   Return the current orientation of the turtle.


.. function:: setheading(angle)

   Set the orientation of the turtle to *angle*.


.. function:: position()

   Return the current location of the turtle as an ``(x,y)`` pair.


.. function:: setx(x)

   Set the x coordinate of the turtle to *x*.


.. function:: sety(y)

   Set the y coordinate of the turtle to *y*.


.. function:: window_width()

   Return the width of the canvas window.


.. function:: window_height()

   Return the height of the canvas window.


This module also does ``from math import *``, so see the documentation for the
:mod:`math` module for additional constants and functions useful for turtle
graphics.


.. function:: demo()

   Exercise the module a bit.


.. exception:: Error

   Exception raised on any error caught by this module.

For examples, see the code of the :func:`demo` function.

This module defines the following classes:


.. class:: Pen()

   Define a pen. All above functions can be called as a methods on the given pen.
   The constructor automatically creates a canvas do be drawn on.


.. class:: Turtle()

   Define a pen. This is essentially a synonym for ``Pen()``; :class:`Turtle` is an
   empty subclass of :class:`Pen`.


.. class:: RawPen(canvas)

   Define a pen which draws on a canvas *canvas*. This is useful if  you want to
   use the module to create graphics in a "real" program.


.. _pen-rawpen-objects:

Turtle, Pen and RawPen Objects
------------------------------

Most of the global functions available in the module are also available as
methods of the :class:`Turtle`, :class:`Pen` and :class:`RawPen` classes,
affecting only the state of the given pen.

The only method which is more powerful as a method is :func:`degrees`, which
takes an optional argument letting  you specify the number of units
corresponding to a full circle:


.. method:: Turtle.degrees([fullcircle])

   *fullcircle* is by default 360. This can cause the pen to have any angular units
   whatever: give *fullcircle* ``2*pi`` for radians, or 400 for gradians.

