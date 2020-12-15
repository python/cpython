:mod:`tkinter.font` --- Tkinter font wrapper
============================================

.. module:: tkinter.font
   :platform: Tk
   :synopsis: Tkinter font-wrapping class

**Source code:** :source:`Lib/tkinter/font.py`

--------------

The :mod:`tkinter.font` module provides the :class:`Font` class for creating
and using named fonts.

The different font weights and slants are:

.. data:: NORMAL
          BOLD
          ITALIC
          ROMAN

.. class:: Font(root=None, font=None, name=None, exists=False, **options)

   The :class:`Font` class represents a named font. *Font* instances are given
   unique names and can be specified by their family, size, and style
   configuration. Named fonts are Tk's method of creating and identifying
   fonts as a single object, rather than specifying a font by its attributes
   with each occurrence.

    arguments:

       | *font* - font specifier tuple (family, size, options)
       | *name* - unique font name
       | *exists* - self points to existing named font if true

    additional keyword options (ignored if *font* is specified):

       | *family* - font family i.e. Courier, Times
       | *size* - font size
       |     If *size* is positive it is interpreted as size in points.
       |     If *size* is a negative number its absolute value is treated
       |     as size in pixels.
       | *weight* - font emphasis (NORMAL, BOLD)
       | *slant* - ROMAN, ITALIC
       | *underline* - font underlining (0 - none, 1 - underline)
       | *overstrike* - font strikeout (0 - none, 1 - strikeout)

   .. method:: actual(option=None, displayof=None)

      Returns the actual value for attributes when the given font is used on
      the widget given by *displayof*. *displayof* accepts any tkinter widget
      and, if omitted, will default to the main window (in most circumstances,
      this will be your ``Tk`` instance).

      Due to some differences between operating systems (such as pointsizes
      and the availability of font families), some values returned by this
      method may vary from those given as keyword arguments to either the
      constructor or :meth:`Font.config` (and retrieved by :meth:`Font.cget`).

      If *option* is specified, the value of just that attribute is returned.
      If it is omitted or ``None``, the returned value is a dictionary of all
      the attributes and their values. See above for a list of acceptable
      attributes.

   .. method:: cget(option)

      Retrieve an attribute of the font.

   .. method:: config(**options)

      Modify attributes of the font.

   .. method:: copy()

      Return new instance of the current font with a different *name*.

   .. method:: measure(text, displayof=None)

      Return amount of space the text would occupy on the specified display
      when formatted in the current font. If no display is specified then the
      main application window is assumed.

   .. method:: metrics(*options, **kw)

      Return font-specific data.
      Options include:

      *ascent* - distance between baseline and highest point that a
         character of the font can occupy

      *descent* - distance between baseline and lowest point that a
         character of the font can occupy

      *linespace* - minimum vertical separation necessary between any two
         characters of the font that ensures no vertical overlap between lines.

      *fixed* - 1 if font is fixed-width else 0

.. function:: families(root=None, displayof=None)

   Return the different font families.

.. function:: names(root=None)

   Return the names of defined fonts.

.. function:: nametofont(name)

   Return a :class:`Font` representation of a tk named font.
