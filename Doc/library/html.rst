:mod:`!html` --- HyperText Markup Language support
==================================================

.. module:: html
   :synopsis: Helpers for manipulating HTML.

**Source code:** :source:`Lib/html/__init__.py`

--------------

This module defines utilities to manipulate HTML.

.. function:: escape(s, quote=True)

   Convert the characters ``&``, ``<`` and ``>`` in string *s* to HTML-safe
   sequences.  Use this if you need to display text that might contain such
   characters in HTML.  If the optional flag *quote* is true (the default), the
   characters (``"``) and (``'``) are also translated; this helps for inclusion
   in an HTML attribute value delimited by quotes, as in ``<a href="...">``.
   If *quote* is set to false, the characters (``"``) and (``'``) are not
   translated.


   .. versionadded:: 3.2


.. function:: unescape(s)

   Convert all named and numeric character references (e.g. ``&gt;``,
   ``&#62;``, ``&#x3e;``) in the string *s* to the corresponding Unicode
   characters.  This function uses the rules defined by the HTML 5 standard
   for both valid and invalid character references, and the :data:`list of
   HTML 5 named character references <html.entities.html5>`.

   .. versionadded:: 3.4


.. function:: htmlcharrefreplace_errors(exception)

   Implements the ``htmlcharrefreplace`` error handling (for encoding only):
   the unencodable character is replaced by the corresponding HTML named
   character reference from :data:`html.entities.codepoint2name`, or by a
   numeric character reference if there is no name for it.

   This error handler is not registered by default, you should register it
   with :func:`codecs.register_error`::

      >>> import codecs, html
      >>> codecs.register_error('htmlcharrefreplace',
      ...                       html.htmlcharrefreplace_errors)
      >>> '∀ x∈ℜ'.encode('ascii', 'htmlcharrefreplace')
      b'&forall; x&isin;&real;'

   .. versionadded:: next

--------------

Submodules in the ``html`` package are:

* :mod:`html.parser` -- HTML/XHTML parser with lenient parsing mode
* :mod:`html.entities` -- HTML entity definitions
