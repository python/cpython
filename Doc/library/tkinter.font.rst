:mod:`!tkinter.font` --- Tkinter font wrapper
=============================================

.. module:: tkinter.font
   :synopsis: Tkinter font-wrapping class

**Source code:** :source:`Lib/tkinter/font.py`

--------------

The :mod:`!tkinter.font` module provides the :class:`Font` class for creating
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

   .. versionchanged:: 3.10
      Two fonts now compare equal (``==``) only when both are :class:`Font`
      instances with the same name belonging to the same Tcl interpreter.

    arguments:

       | *font* - font specifier tuple (family, size, options)
       | *name* - unique font name
       | *exists* - self points to existing named font if true

    additional keyword options (ignored if *font* is specified):

       | *family* - font family, for example, Courier, Times
       | *size* - font size
       |     If *size* is positive it is interpreted as size in points.
       |     If *size* is a negative number its absolute value is treated
       |     as size in pixels.
       | *weight* - font emphasis (NORMAL, BOLD)
       | *slant* - ROMAN, ITALIC
       | *underline* - font underlining (0 - none, 1 - underline)
       | *overstrike* - font strikeout (0 - none, 1 - strikeout)

   .. method:: actual(option=None, displayof=None)

      Return the actual attributes of the font, which may differ from the
      requested ones because of platform limitations.
      With no *option*, return a dictionary of all the attributes; if *option*
      is given, return the value of that single attribute.

   .. method:: cget(option)

      Retrieve an attribute of the font.

   .. method:: config(**options)
      :no-typesetting:

   .. method:: configure(**options)

      Modify one or more attributes of the font.
      With no arguments, return a dictionary of the current attributes.

      :meth:`config` is an alias of :meth:`!configure`.

   .. method:: copy()

      Return new instance of the current font.

   .. method:: measure(text, displayof=None)

      Return amount of space the text would occupy on the specified display
      when formatted in the current font, as an integer number of pixels.
      If no display is specified then the main application window is assumed.

   .. method:: metrics(*options, **kw)

      Return font-specific data.
      With no options, return a dictionary mapping each metric name to its
      integer value; if one option name is given, return that metric's value as
      an integer.
      Options include:

      *ascent* - distance between baseline and highest point that a
         character of the font can occupy

      *descent* - distance between baseline and lowest point that a
         character of the font can occupy

      *linespace* - minimum vertical separation necessary between any two
         characters of the font that ensures no vertical overlap between lines.

      *fixed* - 1 if font is fixed-width else 0

.. function:: families(root=None, displayof=None)

   Return a tuple of the names of the available font families.

.. function:: names(root=None)

   Return a tuple of the names of all the defined fonts.

.. function:: nametofont(name, root=None)

   Return a :class:`Font` representation of the existing named font *name*.
   *root* is the widget whose Tcl interpreter owns the font; if omitted, the
   default root window is used.

   .. versionchanged:: 3.10
      The *root* parameter was added.
