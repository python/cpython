
:mod:`jpeg` --- Read and write JPEG files
=========================================

.. module:: jpeg
   :platform: IRIX
   :synopsis: Read and write image files in compressed JPEG format.


.. index:: single: Independent JPEG Group

The module :mod:`jpeg` provides access to the jpeg compressor and decompressor
written by the Independent JPEG Group (IJG). JPEG is a standard for compressing
pictures; it is defined in ISO 10918.  For details on JPEG or the Independent
JPEG Group software refer to the JPEG standard or the documentation provided
with the software.

.. index::
   single: Python Imaging Library
   single: PIL (the Python Imaging Library)
   single: Lundh, Fredrik

A portable interface to JPEG image files is available with the Python Imaging
Library (PIL) by Fredrik Lundh.  Information on PIL is available at
http://www.pythonware.com/products/pil/.

The :mod:`jpeg` module defines an exception and some functions.


.. exception:: error

   Exception raised by :func:`compress` and :func:`decompress` in case of errors.


.. function:: compress(data, w, h, b)

   .. index:: single: JFIF

   Treat data as a pixmap of width *w* and height *h*, with *b* bytes per pixel.
   The data is in SGI GL order, so the first pixel is in the lower-left corner.
   This means that :func:`gl.lrectread` return data can immediately be passed to
   :func:`compress`. Currently only 1 byte and 4 byte pixels are allowed, the
   former being treated as greyscale and the latter as RGB color. :func:`compress`
   returns a string that contains the compressed picture, in JFIF format.


.. function:: decompress(data)

   .. index:: single: JFIF

   Data is a string containing a picture in JFIF format. It returns a tuple
   ``(data, width, height, bytesperpixel)``.  Again, the data is suitable to pass
   to :func:`gl.lrectwrite`.


.. function:: setoption(name, value)

   Set various options.  Subsequent :func:`compress` and :func:`decompress` calls
   will use these options.  The following options are available:

   +-----------------+---------------------------------------------+
   | Option          | Effect                                      |
   +=================+=============================================+
   | ``'forcegray'`` | Force output to be grayscale, even if input |
   |                 | is RGB.                                     |
   +-----------------+---------------------------------------------+
   | ``'quality'``   | Set the quality of the compressed image to  |
   |                 | a value between ``0`` and ``100`` (default  |
   |                 | is ``75``).  This only affects compression. |
   +-----------------+---------------------------------------------+
   | ``'optimize'``  | Perform Huffman table optimization.  Takes  |
   |                 | longer, but results in smaller compressed   |
   |                 | image.  This only affects compression.      |
   +-----------------+---------------------------------------------+
   | ``'smooth'``    | Perform inter-block smoothing on            |
   |                 | uncompressed image.  Only useful for low-   |
   |                 | quality images.  This only affects          |
   |                 | decompression.                              |
   +-----------------+---------------------------------------------+


.. seealso::

   JPEG Still Image Data Compression Standard
      The canonical reference for the JPEG image format, by Pennebaker and Mitchell.

   `Information Technology - Digital Compression and Coding of Continuous-tone Still Images - Requirements and Guidelines <http://www.w3.org/Graphics/JPEG/itu-t81.pdf>`_
      The ISO standard for JPEG is also published as ITU T.81.  This is available
      online in PDF form.

