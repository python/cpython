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

    keyword arguments:

       | *font* - font specifier tuple (family, size, \*\*options)
       | *name* - unique font name
       | *exists* - self points to existing named font if true

    allowed keyword options (ignored if *font* is specified):

       | *family* - font family i.e. Courier, Times
       | *size* - font size (pt)
       | *weight* - font emphasis {NORMAL, BOLD}
       | *slant* - {ROMAN - oblique text, ITALIC}
       | *underline* - font underlining (0 - none, 1 - underline)
       | *overstrike* - font strikeout (0 - none, 1 - strikeout)

   .. method:: actual(option=None, displayof=None)

      Returns the attributes of the font.

   .. method:: cget(option)

      Retrieves an attribute of the font.

   .. method:: config(**options)

      Modify attributes of the font.

   .. method:: copy()

      Returns new instance of the current font.

   .. method:: measure(text, displayof=None)

      Returns amount of space the text would occupy on the specified display
      when formatted in the current font. If no display is specified then the
      main application window is assumed.

   .. method:: metrics(*options, **kw)

      Returns font-specific data.
      Options include:

      *ascent* - distance between baseline and highest point that a
         character of the font can occupy

      *descent* - distance between baseline and lowest point that a
         character of the font can occupy

      *linespace* - minimum vertical separation necessary between any two
         characters of the font that ensures no vertical overlap between lines.

      *fixed* - 1 if font is fixed-width else 0

.. function:: families(root=None, displayof=None)

   Returns the different font families.

.. function:: names(root=None)

   Returns the names of defined fonts.

.. function:: nametofont(name)

   Returns a :class:`Font` representation of a tk named font.