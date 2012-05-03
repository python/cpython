
:mod:`imageop` --- Manipulate raw image data
============================================

.. module:: imageop
   :synopsis: Manipulate raw image data.
   :deprecated:

.. deprecated:: 2.6
    The :mod:`imageop` module has been removed in Python 3.

The :mod:`imageop` module contains some useful operations on images. It operates
on images consisting of 8 or 32 bit pixels stored in Python strings.  This is
the same format as used by :func:`gl.lrectwrite` and the :mod:`imgfile` module.

The module defines the following variables and functions:


.. exception:: error

   This exception is raised on all errors, such as unknown number of bits per
   pixel, etc.


.. function:: crop(image, psize, width, height, x0, y0, x1, y1)

   Return the selected part of *image*, which should be *width* by *height* in size
   and consist of pixels of *psize* bytes. *x0*, *y0*, *x1* and *y1* are like the
   :func:`gl.lrectread` parameters, i.e. the boundary is included in the new image.
   The new boundaries need not be inside the picture.  Pixels that fall outside the
   old image will have their value set to zero.  If *x0* is bigger than *x1* the
   new image is mirrored.  The same holds for the y coordinates.


.. function:: scale(image, psize, width, height, newwidth, newheight)

   Return *image* scaled to size *newwidth* by *newheight*. No interpolation is
   done, scaling is done by simple-minded pixel duplication or removal.  Therefore,
   computer-generated images or dithered images will not look nice after scaling.


.. function:: tovideo(image, psize, width, height)

   Run a vertical low-pass filter over an image.  It does so by computing each
   destination pixel as the average of two vertically-aligned source pixels.  The
   main use of this routine is to forestall excessive flicker if the image is
   displayed on a video device that uses interlacing, hence the name.


.. function:: grey2mono(image, width, height, threshold)

   Convert a 8-bit deep greyscale image to a 1-bit deep image by thresholding all
   the pixels.  The resulting image is tightly packed and is probably only useful
   as an argument to :func:`mono2grey`.


.. function:: dither2mono(image, width, height)

   Convert an 8-bit greyscale image to a 1-bit monochrome image using a
   (simple-minded) dithering algorithm.


.. function:: mono2grey(image, width, height, p0, p1)

   Convert a 1-bit monochrome image to an 8 bit greyscale or color image. All
   pixels that are zero-valued on input get value *p0* on output and all one-value
   input pixels get value *p1* on output.  To convert a monochrome black-and-white
   image to greyscale pass the values ``0`` and ``255`` respectively.


.. function:: grey2grey4(image, width, height)

   Convert an 8-bit greyscale image to a 4-bit greyscale image without dithering.


.. function:: grey2grey2(image, width, height)

   Convert an 8-bit greyscale image to a 2-bit greyscale image without dithering.


.. function:: dither2grey2(image, width, height)

   Convert an 8-bit greyscale image to a 2-bit greyscale image with dithering.  As
   for :func:`dither2mono`, the dithering algorithm is currently very simple.


.. function:: grey42grey(image, width, height)

   Convert a 4-bit greyscale image to an 8-bit greyscale image.


.. function:: grey22grey(image, width, height)

   Convert a 2-bit greyscale image to an 8-bit greyscale image.


.. data:: backward_compatible

   If set to 0, the functions in this module use a non-backward compatible way
   of representing multi-byte pixels on little-endian systems.  The SGI for
   which this module was originally written is a big-endian system, so setting
   this variable will have no effect. However, the code wasn't originally
   intended to run on anything else, so it made assumptions about byte order
   which are not universal.  Setting this variable to 0 will cause the byte
   order to be reversed on little-endian systems, so that it then is the same as
   on big-endian systems.

