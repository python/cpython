
:mod:`xmllib` --- A parser for XML documents
============================================

.. module:: xmllib
   :synopsis: A parser for XML documents.
   :deprecated:
.. moduleauthor:: Sjoerd Mullender <Sjoerd.Mullender@cwi.nl>
.. sectionauthor:: Sjoerd Mullender <Sjoerd.Mullender@cwi.nl>


.. index::
   single: XML
   single: Extensible Markup Language

.. deprecated:: 2.0
   Use :mod:`xml.sax` instead.  The newer XML package includes full support for XML
   1.0.

.. versionchanged:: 1.5.2
   Added namespace support.

This module defines a class :class:`XMLParser` which serves as the basis  for
parsing text files formatted in XML (Extensible Markup Language).


.. class:: XMLParser()

   The :class:`XMLParser` class must be instantiated without arguments. [#]_

   This class provides the following interface methods and instance variables:


   .. attribute:: attributes

      A mapping of element names to mappings.  The latter mapping maps attribute
      names that are valid for the element to the default value of the
      attribute, or if there is no default to ``None``.  The default value is
      the empty dictionary.  This variable is meant to be overridden, not
      extended since the default is shared by all instances of
      :class:`XMLParser`.


   .. attribute:: elements

      A mapping of element names to tuples.  The tuples contain a function for
      handling the start and end tag respectively of the element, or ``None`` if
      the method :meth:`unknown_starttag` or :meth:`unknown_endtag` is to be
      called.  The default value is the empty dictionary.  This variable is
      meant to be overridden, not extended since the default is shared by all
      instances of :class:`XMLParser`.


   .. attribute:: entitydefs

      A mapping of entitynames to their values.  The default value contains
      definitions for ``'lt'``, ``'gt'``, ``'amp'``, ``'quot'``, and ``'apos'``.


   .. method:: reset()

      Reset the instance.  Loses all unprocessed data.  This is called
      implicitly at the instantiation time.


   .. method:: setnomoretags()

      Stop processing tags.  Treat all following input as literal input (CDATA).


   .. method:: setliteral()

      Enter literal mode (CDATA mode).  This mode is automatically exited when
      the close tag matching the last unclosed open tag is encountered.


   .. method:: feed(data)

      Feed some text to the parser.  It is processed insofar as it consists of
      complete tags; incomplete data is buffered until more data is fed or
      :meth:`close` is called.


   .. method:: close()

      Force processing of all buffered data as if it were followed by an
      end-of-file mark.  This method may be redefined by a derived class to
      define additional processing at the end of the input, but the redefined
      version should always call :meth:`close`.


   .. method:: translate_references(data)

      Translate all entity and character references in *data* and return the
      translated string.


   .. method:: getnamespace()

      Return a mapping of namespace abbreviations to namespace URIs that are
      currently in effect.


   .. method:: handle_xml(encoding, standalone)

      This method is called when the ``<?xml ...?>`` tag is processed. The
      arguments are the values of the encoding and standalone attributes in the
      tag.  Both encoding and standalone are optional.  The values passed to
      :meth:`handle_xml` default to ``None`` and the string ``'no'``
      respectively.


   .. method:: handle_doctype(tag, pubid, syslit, data)

      .. index::
         single: DOCTYPE declaration
         single: Formal Public Identifier

      This method is called when the ``<!DOCTYPE...>`` declaration is processed.
      The arguments are the tag name of the root element, the Formal Public
      Identifier (or ``None`` if not specified), the system identifier, and the
      uninterpreted contents of the internal DTD subset as a string (or ``None``
      if not present).


   .. method:: handle_starttag(tag, method, attributes)

      This method is called to handle start tags for which a start tag handler
      is defined in the instance variable :attr:`elements`.  The *tag* argument
      is the name of the tag, and the *method* argument is the function (method)
      which should be used to support semantic interpretation of the start tag.
      The *attributes* argument is a dictionary of attributes, the key being the
      *name* and the value being the *value* of the attribute found inside the
      tag's ``<>`` brackets.  Character and entity references in the *value*
      have been interpreted.  For instance, for the start tag ``<A
      HREF="http://www.cwi.nl/">``, this method would be called as
      ``handle_starttag('A', self.elements['A'][0], {'HREF':
      'http://www.cwi.nl/'})``.  The base implementation simply calls *method*
      with *attributes* as the only argument.


   .. method:: handle_endtag(tag, method)

      This method is called to handle endtags for which an end tag handler is
      defined in the instance variable :attr:`elements`.  The *tag* argument is
      the name of the tag, and the *method* argument is the function (method)
      which should be used to support semantic interpretation of the end tag.
      For instance, for the endtag ``</A>``, this method would be called as
      ``handle_endtag('A', self.elements['A'][1])``.  The base implementation
      simply calls *method*.


   .. method:: handle_data(data)

      This method is called to process arbitrary data.  It is intended to be
      overridden by a derived class; the base class implementation does nothing.


   .. method:: handle_charref(ref)

      This method is called to process a character reference of the form
      ``&#ref;``.  *ref* can either be a decimal number, or a hexadecimal number
      when preceded by an ``'x'``. In the base implementation, *ref* must be a
      number in the range 0-255.  It translates the character to ASCII and calls
      the method :meth:`handle_data` with the character as argument.  If *ref*
      is invalid or out of range, the method ``unknown_charref(ref)`` is called
      to handle the error.  A subclass must override this method to provide
      support for character references outside of the ASCII range.


   .. method:: handle_comment(comment)

      This method is called when a comment is encountered.  The *comment*
      argument is a string containing the text between the ``<!--`` and ``-->``
      delimiters, but not the delimiters themselves.  For example, the comment
      ``<!--text-->`` will cause this method to be called with the argument
      ``'text'``.  The default method does nothing.


   .. method:: handle_cdata(data)

      This method is called when a CDATA element is encountered.  The *data*
      argument is a string containing the text between the ``<![CDATA[`` and
      ``]]>`` delimiters, but not the delimiters themselves.  For example, the
      entity ``<![CDATA[text]]>`` will cause this method to be called with the
      argument ``'text'``.  The default method does nothing, and is intended to
      be overridden.


   .. method:: handle_proc(name, data)

      This method is called when a processing instruction (PI) is encountered.
      The *name* is the PI target, and the *data* argument is a string
      containing the text between the PI target and the closing delimiter, but
      not the delimiter itself.  For example, the instruction ``<?XML text?>``
      will cause this method to be called with the arguments ``'XML'`` and
      ``'text'``.  The default method does nothing.  Note that if a document
      starts with ``<?xml ..?>``, :meth:`handle_xml` is called to handle it.


   .. method:: handle_special(data)

      .. index:: single: ENTITY declaration

      This method is called when a declaration is encountered.  The *data*
      argument is a string containing the text between the ``<!`` and ``>``
      delimiters, but not the delimiters themselves.  For example, the entity
      declaration ``<!ENTITY text>`` will cause this method to be called with
      the argument ``'ENTITY text'``.  The default method does nothing.  Note
      that ``<!DOCTYPE ...>`` is handled separately if it is located at the
      start of the document.


   .. method:: syntax_error(message)

      This method is called when a syntax error is encountered.  The *message*
      is a description of what was wrong.  The default method raises a
      :exc:`RuntimeError` exception.  If this method is overridden, it is
      permissible for it to return.  This method is only called when the error
      can be recovered from.  Unrecoverable errors raise a :exc:`RuntimeError`
      without first calling :meth:`syntax_error`.


   .. method:: unknown_starttag(tag, attributes)

      This method is called to process an unknown start tag.  It is intended to
      be overridden by a derived class; the base class implementation does nothing.


   .. method:: unknown_endtag(tag)

      This method is called to process an unknown end tag.  It is intended to be
      overridden by a derived class; the base class implementation does nothing.


   .. method:: unknown_charref(ref)

      This method is called to process unresolvable numeric character
      references.  It is intended to be overridden by a derived class; the base
      class implementation does nothing.


   .. method:: unknown_entityref(ref)

      This method is called to process an unknown entity reference.  It is
      intended to be overridden by a derived class; the base class
      implementation calls :meth:`syntax_error` to signal an error.


.. seealso::

   `Extensible Markup Language (XML) 1.0 <http://www.w3.org/TR/REC-xml>`_
      The XML specification, published by the World Wide Web Consortium (W3C), defines
      the syntax and processor requirements for XML.  References to additional
      material on XML, including translations of the specification, are available at
      http://www.w3.org/XML/.

   `Python and XML Processing <https://www.python.org/topics/xml/>`_
      The Python XML Topic Guide provides a great deal of information on using XML
      from Python and links to other sources of information on XML.

   `SIG for XML Processing in Python <https://www.python.org/sigs/xml-sig/>`_
      The Python XML Special Interest Group is developing substantial support for
      processing XML from Python.


.. _xml-namespace:

XML Namespaces
--------------

.. index:: pair: XML; namespaces

This module has support for XML namespaces as defined in the XML Namespaces
proposed recommendation.

Tag and attribute names that are defined in an XML namespace are handled as if
the name of the tag or element consisted of the namespace (the URL that defines
the namespace) followed by a space and the name of the tag or attribute.  For
instance, the tag ``<html xmlns='http://www.w3.org/TR/REC-html40'>`` is treated
as if  the tag name was ``'http://www.w3.org/TR/REC-html40 html'``, and the tag
``<html:a href='http://frob.com'>`` inside the above mentioned element is
treated as if the tag name were ``'http://www.w3.org/TR/REC-html40 a'`` and the
attribute name as if it were ``'http://www.w3.org/TR/REC-html40 href'``.

An older draft of the XML Namespaces proposal is also recognized, but triggers a
warning.


.. seealso::

   `Namespaces in XML <http://www.w3.org/TR/REC-xml-names/>`_
      This World Wide Web Consortium recommendation describes the proper syntax and
      processing requirements for namespaces in XML.

.. rubric:: Footnotes

.. [#] Actually, a number of keyword arguments are recognized which influence the
   parser to accept certain non-standard constructs.  The following keyword
   arguments are currently recognized.  The defaults for all of these is ``0``
   (false) except for the last one for which the default is ``1`` (true).
   *accept_unquoted_attributes* (accept certain attribute values without requiring
   quotes), *accept_missing_endtag_name* (accept end tags that look like ``</>``),
   *map_case* (map upper case to lower case in tags and attributes), *accept_utf8*
   (allow UTF-8 characters in input; this is required according to the XML
   standard, but Python does not as yet deal properly with these characters, so
   this is not the default), *translate_attribute_references* (don't attempt to
   translate character and entity references in attribute values).

