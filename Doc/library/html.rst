:mod:`html` --- HyperText Markup Language support
=================================================

.. module:: html
   :synopsis: Helpers for manipulating HTML.

**Source code:** :source:`Lib/html/__init__.py`

--------------

This module defines utilities to manipulate HTML.

.. function:: escape(s, quote=True)

   Convert the characters ``&``, ``<`` and ``>`` in string *s* to HTML-safe
   sequences.  Use this if you need to display text that might contain such
   characters in HTML.  If the optional flag *quote* is true, the characters
   (``"``) and (``'``) are also translated; this helps for inclusion in an HTML
   attribute value delimited by quotes, as in ``<a href="...">``.

   .. versionadded:: 3.2


.. function:: unescape(s)

   Convert all named and numeric character references (e.g. ``&gt;``,
   ``&#62;``, ``&x3e;``) in the string *s* to the corresponding unicode
   characters.  This function uses the rules defined by the HTML 5 standard
   for both valid and invalid character references, and the :data:`list of
   HTML 5 named character references <html.entities.html5>`.

   .. versionadded:: 3.4

--------------

Submodules in the ``html`` package are:

* :mod:`html.parser` -- HTML/XHTML parser with lenient parsing mode
* :mod:`html.entities` -- HTML entity definitions
