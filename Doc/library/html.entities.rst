:mod:`html.entities` --- Definitions of HTML general entities
=============================================================

.. module:: html.entities
   :synopsis: Definitions of HTML general entities.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/html/entities.py`

--------------

This module defines four dictionaries, :data:`html5`,
:data:`name2codepoint`, :data:`codepoint2name`, and :data:`entitydefs`.
:data:`entitydefs` is used to provide the :attr:`entitydefs`
attribute of the :class:`html.parser.HTMLParser` class.  The definition provided
here contains all the entities defined by XHTML 1.0 that can be handled using
simple textual substitution in the Latin-1 character set (ISO-8859-1).


.. data:: html5

   A dictionary that maps HTML5 named character references [#]_ to the
   equivalent Unicode character(s), e.g. ``html5['gt;'] == '>'``.
   Note that the trailing semicolon is included in the name (e.g. ``'gt;'``),
   however some of the names are accepted by the standard even without the
   semicolon: in this case the name is present with and without the ``';'``.

   .. versionadded:: 3.3


.. data:: entitydefs

   A dictionary mapping XHTML 1.0 entity definitions to their replacement text in
   ISO Latin-1.


.. data:: name2codepoint

   A dictionary that maps HTML entity names to the Unicode codepoints.


.. data:: codepoint2name

   A dictionary that maps Unicode codepoints to HTML entity names.


.. rubric:: Footnotes

.. [#] See http://www.w3.org/TR/html5/named-character-references.html
