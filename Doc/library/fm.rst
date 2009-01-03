
:mod:`fm` --- *Font Manager* interface
======================================

.. module:: fm
   :platform: IRIX
   :synopsis: Font Manager interface for SGI workstations.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`fm` module has been deprecated for removal in Python 3.0.



.. index::
   single: Font Manager, IRIS
   single: IRIS Font Manager

This module provides access to the IRIS *Font Manager* library.   It is
available only on Silicon Graphics machines. See also: *4Sight User's Guide*,
section 1, chapter 5: "Using the IRIS Font Manager."

This is not yet a full interface to the IRIS Font Manager. Among the unsupported
features are: matrix operations; cache operations; character operations (use
string operations instead); some details of font info; individual glyph metrics;
and printer matching.

It supports the following operations:


.. function:: init()

   Initialization function. Calls :cfunc:`fminit`. It is normally not necessary to
   call this function, since it is called automatically the first time the
   :mod:`fm` module is imported.


.. function:: findfont(fontname)

   Return a font handle object. Calls ``fmfindfont(fontname)``.


.. function:: enumerate()

   Returns a list of available font names. This is an interface to
   :cfunc:`fmenumerate`.


.. function:: prstr(string)

   Render a string using the current font (see the :func:`setfont` font handle
   method below). Calls ``fmprstr(string)``.


.. function:: setpath(string)

   Sets the font search path. Calls ``fmsetpath(string)``. (XXX Does not work!?!)


.. function:: fontpath()

   Returns the current font search path.

Font handle objects support the following operations:


.. method:: font handle.scalefont(factor)

   Returns a handle for a scaled version of this font. Calls ``fmscalefont(fh,
   factor)``.


.. method:: font handle.setfont()

   Makes this font the current font. Note: the effect is undone silently when the
   font handle object is deleted. Calls ``fmsetfont(fh)``.


.. method:: font handle.getfontname()

   Returns this font's name. Calls ``fmgetfontname(fh)``.


.. method:: font handle.getcomment()

   Returns the comment string associated with this font. Raises an exception if
   there is none. Calls ``fmgetcomment(fh)``.


.. method:: font handle.getfontinfo()

   Returns a tuple giving some pertinent data about this font. This is an interface
   to ``fmgetfontinfo()``. The returned tuple contains the following numbers:
   ``(printermatched, fixed_width, xorig, yorig, xsize, ysize, height, nglyphs)``.


.. method:: font handle.getstrwidth(string)

   Returns the width, in pixels, of *string* when drawn in this font. Calls
   ``fmgetstrwidth(fh, string)``.

