
:mod:`htmllib` --- A parser for HTML documents
==============================================

.. module:: htmllib
   :synopsis: A parser for HTML documents.


.. index::
   single: HTML
   single: hypertext

.. index::
   module: sgmllib
   module: formatter
   single: SGMLParser (in module sgmllib)

This module defines a class which can serve as a base for parsing text files
formatted in the HyperText Mark-up Language (HTML).  The class is not directly
concerned with I/O --- it must be provided with input in string form via a
method, and makes calls to methods of a "formatter" object in order to produce
output.  The :class:`HTMLParser` class is designed to be used as a base class
for other classes in order to add functionality, and allows most of its methods
to be extended or overridden.  In turn, this class is derived from and extends
the :class:`SGMLParser` class defined in module :mod:`sgmllib`.  The
:class:`HTMLParser` implementation supports the HTML 2.0 language as described
in :rfc:`1866`.  Two implementations of formatter objects are provided in the
:mod:`formatter` module; refer to the documentation for that module for
information on the formatter interface.

The following is a summary of the interface defined by
:class:`sgmllib.SGMLParser`:

* The interface to feed data to an instance is through the :meth:`feed` method,
  which takes a string argument.  This can be called with as little or as much
  text at a time as desired; ``p.feed(a); p.feed(b)`` has the same effect as
  ``p.feed(a+b)``.  When the data contains complete HTML markup constructs, these
  are processed immediately; incomplete constructs are saved in a buffer.  To
  force processing of all unprocessed data, call the :meth:`close` method.

  For example, to parse the entire contents of a file, use::

     parser.feed(open('myfile.html').read())
     parser.close()

* The interface to define semantics for HTML tags is very simple: derive a class
  and define methods called :meth:`start_tag`, :meth:`end_tag`, or :meth:`do_tag`.
  The parser will call these at appropriate moments: :meth:`start_tag` or
  :meth:`do_tag` is called when an opening tag of the form ``<tag ...>`` is
  encountered; :meth:`end_tag` is called when a closing tag of the form ``<tag>``
  is encountered.  If an opening tag requires a corresponding closing tag, like
  ``<H1>`` ... ``</H1>``, the class should define the :meth:`start_tag` method; if
  a tag requires no closing tag, like ``<P>``, the class should define the
  :meth:`do_tag` method.

The module defines a parser class and an exception:


.. class:: HTMLParser(formatter)

   This is the basic HTML parser class.  It supports all entity names required by
   the XHTML 1.0 Recommendation (http://www.w3.org/TR/xhtml1).   It also defines
   handlers for all HTML 2.0 and many HTML 3.0 and 3.2 elements.


.. exception:: HTMLParseError

   Exception raised by the :class:`HTMLParser` class when it encounters an error
   while parsing.


.. seealso::

   Module :mod:`formatter`
      Interface definition for transforming an abstract flow of formatting events into
      specific output events on writer objects.

   Module :mod:`HTMLParser`
      Alternate HTML parser that offers a slightly lower-level view of the input, but
      is designed to work with XHTML, and does not implement some of the SGML syntax
      not used in "HTML as deployed" and which isn't legal for XHTML.

   Module :mod:`htmlentitydefs`
      Definition of replacement text for XHTML 1.0  entities.

   Module :mod:`sgmllib`
      Base class for :class:`HTMLParser`.


.. _html-parser-objects:

HTMLParser Objects
------------------

In addition to tag methods, the :class:`HTMLParser` class provides some
additional methods and instance variables for use within tag methods.


.. attribute:: HTMLParser.formatter

   This is the formatter instance associated with the parser.


.. attribute:: HTMLParser.nofill

   Boolean flag which should be true when whitespace should not be collapsed, or
   false when it should be.  In general, this should only be true when character
   data is to be treated as "preformatted" text, as within a ``<PRE>`` element.
   The default value is false.  This affects the operation of :meth:`handle_data`
   and :meth:`save_end`.


.. method:: HTMLParser.anchor_bgn(href, name, type)

   This method is called at the start of an anchor region.  The arguments
   correspond to the attributes of the ``<A>`` tag with the same names.  The
   default implementation maintains a list of hyperlinks (defined by the ``HREF``
   attribute for ``<A>`` tags) within the document.  The list of hyperlinks is
   available as the data attribute :attr:`anchorlist`.


.. method:: HTMLParser.anchor_end()

   This method is called at the end of an anchor region.  The default
   implementation adds a textual footnote marker using an index into the list of
   hyperlinks created by :meth:`anchor_bgn`.


.. method:: HTMLParser.handle_image(source, alt[, ismap[, align[, width[, height]]]])

   This method is called to handle images.  The default implementation simply
   passes the *alt* value to the :meth:`handle_data` method.


.. method:: HTMLParser.save_bgn()

   Begins saving character data in a buffer instead of sending it to the formatter
   object.  Retrieve the stored data via :meth:`save_end`. Use of the
   :meth:`save_bgn` / :meth:`save_end` pair may not be nested.


.. method:: HTMLParser.save_end()

   Ends buffering character data and returns all data saved since the preceding
   call to :meth:`save_bgn`.  If the :attr:`nofill` flag is false, whitespace is
   collapsed to single spaces.  A call to this method without a preceding call to
   :meth:`save_bgn` will raise a :exc:`TypeError` exception.


:mod:`htmlentitydefs` --- Definitions of HTML general entities
==============================================================

.. module:: htmlentitydefs
   :synopsis: Definitions of HTML general entities.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


This module defines three dictionaries, ``name2codepoint``, ``codepoint2name``,
and ``entitydefs``. ``entitydefs`` is used by the :mod:`htmllib` module to
provide the :attr:`entitydefs` member of the :class:`HTMLParser` class.  The
definition provided here contains all the entities defined by XHTML 1.0  that
can be handled using simple textual substitution in the Latin-1 character set
(ISO-8859-1).


.. data:: entitydefs

   A dictionary mapping XHTML 1.0 entity definitions to their replacement text in
   ISO Latin-1.


.. data:: name2codepoint

   A dictionary that maps HTML entity names to the Unicode codepoints.


.. data:: codepoint2name

   A dictionary that maps Unicode codepoints to HTML entity names.

