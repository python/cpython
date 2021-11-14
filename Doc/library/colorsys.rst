:mod:`colorsys` --- Conversions between color systems
=====================================================

.. module:: colorsys
   :synopsis: Conversion functions between RGB and other color systems.

.. sectionauthor:: David Ascher <da@python.net>

**Source code:** :source:`Lib/colorsys.py`

--------------

The :mod:`colorsys` module defines bidirectional conversions of color values
between colors expressed in the RGB (Red Green Blue) color space used in
computer monitors and four other coordinate systems: YIQ, YUV, HLS (Hue
Lightness Saturation) and HSV (Hue Saturation Value).  Coordinates in all of
these color spaces are floating point values.  In the YIQ and YUV space, the
Y coordinate is between 0 and 1, but the I, Q, U and V coordinates can be
positive or negative.  In all other spaces, the coordinates are all between
0 and 1.

.. seealso::

   More information about color spaces can be found at
   https://poynton.ca/ColorFAQ.html and
   https://www.cambridgeincolour.com/tutorials/color-spaces.htm.

The :mod:`colorsys` module defines the following functions:


.. function:: rgb_to_yiq(r, g, b)

   Convert the color from RGB coordinates to YIQ coordinates, using the
   FCC NTSC constants.


.. function:: yiq_to_rgb(y, i, q)

   Convert the color from YIQ coordinates to RGB coordinates, using the
   FCC NTSC constants.


.. function:: rgb_to_yuv(r, g, b)

   Convert the color from RGB coordinates to YUV coordinates, using the
   ATSC BT.709 constants.

   .. versionadded:: 3.11


.. function:: yuv_to_rgb(y, i, q)

   Convert the color from YUV coordinates to RGB coordinates, using the
   ATSC BT.709 constants.

   .. versionadded:: 3.11


.. function:: rgb_to_hls(r, g, b)

   Convert the color from RGB coordinates to HLS coordinates.


.. function:: hls_to_rgb(h, l, s)

   Convert the color from HLS coordinates to RGB coordinates.


.. function:: rgb_to_hsv(r, g, b)

   Convert the color from RGB coordinates to HSV coordinates.


.. function:: hsv_to_rgb(h, s, v)

   Convert the color from HSV coordinates to RGB coordinates.

Example::

   >>> import colorsys
   >>> colorsys.rgb_to_hsv(0.2, 0.4, 0.4)
   (0.5, 0.5, 0.4)
   >>> colorsys.hsv_to_rgb(0.5, 0.5, 0.4)
   (0.2, 0.4, 0.4)
