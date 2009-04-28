:mod:`gl` --- *Graphics Library* interface
==========================================

.. module:: gl
   :platform: IRIX
   :synopsis: Functions from the Silicon Graphics Graphics Library.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`gl` module has been deprecated for removal in Python 3.0.


This module provides access to the Silicon Graphics *Graphics Library*. It is
available only on Silicon Graphics machines.

.. warning::

   Some illegal calls to the GL library cause the Python interpreter to dump
   core.  In particular, the use of most GL calls is unsafe before the first
   window is opened.

The module is too large to document here in its entirety, but the following
should help you to get started. The parameter conventions for the C functions
are translated to Python as follows:

* All (short, long, unsigned) int values are represented by Python integers.

* All float and double values are represented by Python floating point numbers.
  In most cases, Python integers are also allowed.

* All arrays are represented by one-dimensional Python lists. In most cases,
  tuples are also allowed.

* All string and character arguments are represented by Python strings, for
  instance, ``winopen('Hi There!')`` and ``rotate(900, 'z')``.

* All (short, long, unsigned) integer arguments or return values that are only
  used to specify the length of an array argument are omitted. For example, the C
  call ::

     lmdef(deftype, index, np, props)

  is translated to Python as ::

     lmdef(deftype, index, props)

* Output arguments are omitted from the argument list; they are transmitted as
  function return values instead. If more than one value must be returned, the
  return value is a tuple. If the C function has both a regular return value (that
  is not omitted because of the previous rule) and an output argument, the return
  value comes first in the tuple. Examples: the C call ::

     getmcolor(i, &red, &green, &blue)

  is translated to Python as ::

     red, green, blue = getmcolor(i)

The following functions are non-standard or have special argument conventions:


.. function:: varray(argument)

   Equivalent to but faster than a number of ``v3d()`` calls. The *argument* is a
   list (or tuple) of points. Each point must be a tuple of coordinates ``(x, y,
   z)`` or ``(x, y)``. The points may be 2- or 3-dimensional but must all have the
   same dimension. Float and int values may be mixed however. The points are always
   converted to 3D double precision points by assuming ``z = 0.0`` if necessary (as
   indicated in the man page), and for each point ``v3d()`` is called.

   .. XXX the argument-argument added


.. function:: nvarray()

   Equivalent to but faster than a number of ``n3f`` and ``v3f`` calls. The
   argument is an array (list or tuple) of pairs of normals and points. Each pair
   is a tuple of a point and a normal for that point. Each point or normal must be
   a tuple of coordinates ``(x, y, z)``. Three coordinates must be given. Float and
   int values may be mixed. For each pair, ``n3f()`` is called for the normal, and
   then ``v3f()`` is called for the point.


.. function:: vnarray()

   Similar to  ``nvarray()`` but the pairs have the point first and the normal
   second.


.. function:: nurbssurface(s_k, t_k, ctl, s_ord, t_ord, type)

   Defines a nurbs surface. The dimensions of ``ctl[][]`` are computed as follows:
   ``[len(s_k) - s_ord]``, ``[len(t_k) - t_ord]``.

   .. XXX s_k[], t_k[], ctl[][]


.. function:: nurbscurve(knots, ctlpoints, order, type)

   Defines a nurbs curve. The length of ctlpoints is ``len(knots) - order``.


.. function:: pwlcurve(points, type)

   Defines a piecewise-linear curve. *points* is a list of points. *type* must be
   ``N_ST``.


.. function:: pick(n)
              select(n)

   The only argument to these functions specifies the desired size of the pick or
   select buffer.


.. function:: endpick()
              endselect()

   These functions have no arguments. They return a list of integers representing
   the used part of the pick/select buffer. No method is provided to detect buffer
   overrun.

Here is a tiny but complete example GL program in Python::

   import gl, GL, time

   def main():
       gl.foreground()
       gl.prefposition(500, 900, 500, 900)
       w = gl.winopen('CrissCross')
       gl.ortho2(0.0, 400.0, 0.0, 400.0)
       gl.color(GL.WHITE)
       gl.clear()
       gl.color(GL.RED)
       gl.bgnline()
       gl.v2f(0.0, 0.0)
       gl.v2f(400.0, 400.0)
       gl.endline()
       gl.bgnline()
       gl.v2f(400.0, 0.0)
       gl.v2f(0.0, 400.0)
       gl.endline()
       time.sleep(5)

   main()


.. seealso::

   `PyOpenGL: The Python OpenGL Binding <http://pyopengl.sourceforge.net/>`_
      .. index::
         single: OpenGL
         single: PyOpenGL

      An interface to OpenGL is also available; see information about the **PyOpenGL**
      project online at http://pyopengl.sourceforge.net/.  This may be a better option
      if support for SGI hardware from before about 1996 is not required.


:mod:`DEVICE` --- Constants used with the :mod:`gl` module
==========================================================

.. module:: DEVICE
   :platform: IRIX
   :synopsis: Constants used with the gl module.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`DEVICE` module has been deprecated for removal in Python 3.0.


This modules defines the constants used by the Silicon Graphics *Graphics
Library* that C programmers find in the header file ``<gl/device.h>``. Read the
module source file for details.


:mod:`GL` --- Constants used with the :mod:`gl` module
======================================================

.. module:: GL
   :platform: IRIX
   :synopsis: Constants used with the gl module.
   :deprecated:


.. deprecated:: 2.6
    The :mod:`GL` module has been deprecated for removal in Python 3.0.

This module contains constants used by the Silicon Graphics *Graphics Library*
from the C header file ``<gl/gl.h>``. Read the module source file for details.

