:mod:`tkinter.font` --- Tkinter font wrapper
============================================

.. module:: tkinter.font
   :platform: Tk
   :synopsis: Tkinter font-wrapping class

**Source code:** :source:`Lib/tkinter/font.py`

--------------

The :mod:`tkinter.font` module provides the :class:`Font` class for creating
and using named fonts.

.. data:: NORMAL

       no slant or emphasis

.. data:: BOLD

       added font emphasis

.. data:: ITALIC

       italiced text

.. data:: ROMAN

       slanted font style

.. class:: Font(self, root=None, font=None, name=None, exists=False, **options)

   The :class:`Font` class represents a named font.

    options:

       | *font* - font specifier tuple (family, size, style))
       | *name* - unique font name
       | *exists* - self points to existing named font if true

    additional options (redundant if *font* is specified):

       | *family* - font family i.e. Courier, Times
       | *size* - font size (pt)
       | *weight* - font emphasis {NORMAL, BOLD}
       | *slant* - {ROMAN - oblique text, ITALIC}
       | *underline* - font underlining {0 - none, 1 - underline}
       | *overstrike* - font strikeout {0 - none, 1 - strikeout}

    .. function:: actual(self, option=None, displayof=None)

       Returns the attributes of the font

    .. function:: cget(self, option)

       Retrieves an attribute of the font

    .. function:: config(self, **options)

       Modify attributes of the font

    .. function:: copy(self)

       Returns new instance of the current font

    .. function:: measure(self, text, displayof=None)

       Returns width of the text

    .. function:: metrics(self, *options, **kw)

       Returns the metrics of the font



.. method:: families(root=None, displayof=None)

   Returns the different font families

.. method:: names(root=None)

   Returns the names of defined fonts

.. method:: nametofont(name)

   Returns a Font representation of a tk named font