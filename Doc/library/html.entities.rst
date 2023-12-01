:mod:`html.entities` --- Definitions of HTML general entities
=============================================================

.. module:: html.entities
   :synopsis: Definitions of HTML general entities.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/html/entities.py`

--------------

This module defines four dictionaries, :data:`html5`,
:data:`name2codepoint`, :data:`codepoint2name`, and :data:`entitydefs`.


.. data:: html5

   A dictionary that maps HTML5 named character references [#]_ to the
   equivalent Unicode character(s), e.g. ``html5['gt;'] == '>'``.
   Note that the trailing semicolon is included in the name (e.g. ``'gt;'``),
   however some of the names are accepted by the standard even without the
   semicolon: in this case the name is present with and without the ``';'``.
   See also :func:`html.unescape`.

   .. versionadded:: 3.3


.. data:: entitydefs

   A dictionary mapping XHTML 1.0 entity definitions to their replacement text in
   ISO Latin-1.


.. data:: name2codepoint

   A dictionary that maps HTML4 entity names to the Unicode code points.


.. data:: codepoint2name

   A dictionary that maps Unicode code points to HTML4 entity names.


.. rubric:: Footnotes

.. [#] See https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references
