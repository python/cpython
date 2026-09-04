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

   The :class:`Font` class represents a font used by Tk widgets.
   It either creates a new *named font* or refers to an existing font.
   A named font is Tk's way of identifying a font as a single object that can
   be referred to by name and reconfigured in place,
   rather than respecifying its attributes at each use.

   With *exists* false (the default), a new named font is created.
   Its attributes are taken from the font description *font* if it is given,
   overridden by any keyword *options*.
   The new font is named *name*, or a generated unique name if *name* is
   omitted.

   With *exists* true, an existing font is referred to instead of being
   created.
   If *name* is given, it is the name of the font,
   which is reconfigured by *font* and *options* if either is given.
   If *name* is omitted, the font description *font* is wrapped as is,
   without creating a named font,
   so that it is used without loss of precision by :meth:`actual`,
   :meth:`measure` and :meth:`metrics`.
   In this case no keyword options are accepted,
   and the :attr:`!name` attribute is the description itself rather than a
   string.

   The font description *font* is a tuple of the family name, the size and
   zero or more styles,
   or any other form accepted by Tk, such as the name of a named font.

   The keyword *options* are:

      | *family* - font family, for example, Courier, Times
      | *size* - font size
      |     If *size* is positive it is interpreted as size in points.
      |     If *size* is a negative number its absolute value is treated
      |     as size in pixels.
      | *weight* - font emphasis (NORMAL, BOLD)
      | *slant* - ROMAN, ITALIC
      | *underline* - font underlining (0 - none, 1 - underline)
      | *overstrike* - font strikeout (0 - none, 1 - strikeout)

   .. versionchanged:: 3.10
      Two fonts now compare equal (``==``) only when both are :class:`Font`
      instances with the same name belonging to the same Tcl interpreter.

   .. versionchanged:: next
      A font description can now be wrapped without creating a new named font,
      and keyword options now override the attributes of the specified *font*.

   .. method:: actual(option=None, displayof=None)

      Return the actual attributes of the font, which may differ from the
      requested ones because of platform limitations.
      With no *option*, return a dictionary of all the attributes; if *option*
      is given, return the value of that single attribute.
      The attributes are resolved on the display of the *displayof* widget,
      or the main application window if it is not specified.

   .. method:: cget(option)

      Retrieve an attribute of the font.

      .. note::

         :meth:`!cget` and :meth:`configure` operate on a named font and raise
         :exc:`~tkinter.TclError` for a wrapped font description.
         Use :meth:`actual` to query the attributes of the latter.

   .. method:: config(**options)
      :no-typesetting:

   .. method:: configure(**options)

      Modify one or more attributes of the font.
      With no arguments, return a dictionary of the current attributes.

      :meth:`config` is an alias of :meth:`!configure`.

   .. method:: copy()

      Return a distinct copy of the current font:
      a new named font with the same attributes but a different name,
      which can be reconfigured independently of the original.
      If the current font wraps a font description,
      the copy is instead a named font with its resolved attributes.

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
