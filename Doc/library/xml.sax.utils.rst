:mod:`xml.sax.saxutils` --- SAX Utilities
=========================================

.. module:: xml.sax.saxutils
   :synopsis: Convenience functions and classes for use with SAX.

.. moduleauthor:: Lars Marius Garshol <larsga@garshol.priv.no>
.. sectionauthor:: Martin v. Löwis <martin@v.loewis.de>

**Source code:** :source:`Lib/xml/sax/saxutils.py`

--------------

The module :mod:`xml.sax.saxutils` contains a number of classes and functions
that are commonly useful when creating SAX applications, either in direct use,
or as base classes.


.. function:: escape(data, entities={})

   Escape ``'&'``, ``'<'``, and ``'>'`` in a string of data.

   You can escape other strings of data by passing a dictionary as the optional
   *entities* parameter.  The keys and values must all be strings; each key will be
   replaced with its corresponding value.  The characters ``'&'``, ``'<'`` and
   ``'>'`` are always escaped, even if *entities* is provided.


.. function:: unescape(data, entities={})

   Unescape ``'&amp;'``, ``'&lt;'``, and ``'&gt;'`` in a string of data.

   You can unescape other strings of data by passing a dictionary as the optional
   *entities* parameter.  The keys and values must all be strings; each key will be
   replaced with its corresponding value.  ``'&amp'``, ``'&lt;'``, and ``'&gt;'``
   are always unescaped, even if *entities* is provided.


.. function:: quoteattr(data, entities={})

   Similar to :func:`escape`, but also prepares *data* to be used as an
   attribute value.  The return value is a quoted version of *data* with any
   additional required replacements. :func:`quoteattr` will select a quote
   character based on the content of *data*, attempting to avoid encoding any
   quote characters in the string.  If both single- and double-quote characters
   are already in *data*, the double-quote characters will be encoded and *data*
   will be wrapped in double-quotes.  The resulting string can be used directly
   as an attribute value::

      >>> print("<element attr=%s>" % quoteattr("ab ' cd \" ef"))
      <element attr="ab ' cd &quot; ef">

   This function is useful when generating attribute values for HTML or any SGML
   using the reference concrete syntax.


.. class:: XMLGenerator(out=None, encoding='iso-8859-1', short_empty_elements=False)

   This class implements the :class:`~xml.sax.handler.ContentHandler` interface
   by writing SAX
   events back into an XML document. In other words, using an :class:`XMLGenerator`
   as the content handler will reproduce the original document being parsed. *out*
   should be a file-like object which will default to *sys.stdout*. *encoding* is
   the encoding of the output stream which defaults to ``'iso-8859-1'``.
   *short_empty_elements* controls the formatting of elements that contain no
   content:  if ``False`` (the default) they are emitted as a pair of start/end
   tags, if set to ``True`` they are emitted as a single self-closed tag.

   .. versionchanged:: 3.2
      Added the *short_empty_elements* parameter.


.. class:: XMLFilterBase(base)

   This class is designed to sit between an
   :class:`~xml.sax.xmlreader.XMLReader` and the client
   application's event handlers.  By default, it does nothing but pass requests up
   to the reader and events on to the handlers unmodified, but subclasses can
   override specific methods to modify the event stream or the configuration
   requests as they pass through.


.. function:: prepare_input_source(source, base='')

   This function takes an input source and an optional base URL and returns a
   fully resolved :class:`~xml.sax.xmlreader.InputSource` object ready for
   reading.  The input source can be given as a string, a file-like object, or
   an :class:`~xml.sax.xmlreader.InputSource` object; parsers will use this
   function to implement the polymorphic *source* argument to their
   :meth:`~xml.sax.xmlreader.XMLReader.parse` method.

