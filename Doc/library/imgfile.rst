
:mod:`imgfile` --- Support for SGI imglib files
===============================================

.. module:: imgfile
   :platform: IRIX
   :synopsis: Support for SGI imglib files.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`imgfile` module has been removed in Python 3.



The :mod:`imgfile` module allows Python programs to access SGI imglib image
files (also known as :file:`.rgb` files).  The module is far from complete, but
is provided anyway since the functionality that there is enough in some cases.
Currently, colormap files are not supported.

The module defines the following variables and functions:


.. exception:: error

   This exception is raised on all errors, such as unsupported file type, etc.


.. function:: getsizes(file)

   This function returns a tuple ``(x, y, z)`` where *x* and *y* are the size of
   the image in pixels and *z* is the number of bytes per pixel. Only 3 byte RGB
   pixels and 1 byte greyscale pixels are currently supported.


.. function:: read(file)

   This function reads and decodes the image on the specified file, and returns it
   as a Python string. The string has either 1 byte greyscale pixels or 4 byte RGBA
   pixels. The bottom left pixel is the first in the string. This format is
   suitable to pass to :func:`gl.lrectwrite`, for instance.


.. function:: readscaled(file, x, y, filter[, blur])

   This function is identical to read but it returns an image that is scaled to the
   given *x* and *y* sizes. If the *filter* and *blur* parameters are omitted
   scaling is done by simply dropping or duplicating pixels, so the result will be
   less than perfect, especially for computer-generated images.

   Alternatively, you can specify a filter to use to smooth the image after
   scaling. The filter forms supported are ``'impulse'``, ``'box'``,
   ``'triangle'``, ``'quadratic'`` and ``'gaussian'``. If a filter is specified
   *blur* is an optional parameter specifying the blurriness of the filter. It
   defaults to ``1.0``.

   :func:`readscaled` makes no attempt to keep the aspect ratio correct, so that is
   the users' responsibility.


.. function:: ttob(flag)

   This function sets a global flag which defines whether the scan lines of the
   image are read or written from bottom to top (flag is zero, compatible with SGI
   GL) or from top to bottom(flag is one, compatible with X).  The default is zero.


.. function:: write(file, data, x, y, z)

   This function writes the RGB or greyscale data in *data* to image file *file*.
   *x* and *y* give the size of the image, *z* is 1 for 1 byte greyscale images or
   3 for RGB images (which are stored as 4 byte values of which only the lower
   three bytes are used). These are the formats returned by :func:`gl.lrectread`.

