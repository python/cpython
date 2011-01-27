:mod:`html` --- HyperText Markup Language support
=================================================

.. module:: html
   :synopsis: Helpers for manipulating HTML.

.. versionadded:: 3.2

**Source code:** :source:`Lib/html/__init__.py`

--------------

This module defines utilities to manipulate HTML.

.. function:: escape(s, quote=True)

   Convert the characters ``&``, ``<`` and ``>`` in string *s* to HTML-safe
   sequences.  Use this if you need to display text that might contain such
   characters in HTML.  If the optional flag *quote* is true, the characters
   (``"``) and (``'``) are also translated; this helps for inclusion in an HTML
   attribute value delimited by quotes, as in ``<a href="...">``.
