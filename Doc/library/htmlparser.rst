
:mod:`HTMLParser` --- Simple HTML and XHTML parser
==================================================

.. module:: HTMLParser
   :synopsis: A simple parser that can handle HTML and XHTML.


.. index:: HTML, XHTML

This module defines a class :class:`HTMLParser` which serves as the basis for
parsing text files formatted in HTML (HyperText Mark-up Language) and XHTML.
Unlike the parser in :mod:`htmllib`, this parser is not based on the SGML parser
in :mod:`sgmllib`.


.. class:: HTMLParser()

   The :class:`HTMLParser` class is instantiated without arguments.

   An HTMLParser instance is fed HTML data and calls handler functions when tags
   begin and end.  The :class:`HTMLParser` class is meant to be overridden by the
   user to provide a desired behavior.

   Unlike the parser in :mod:`htmllib`, this parser does not check that end tags
   match start tags or call the end-tag handler for elements which are closed
   implicitly by closing an outer element.

An exception is defined as well:


.. exception:: HTMLParseError

   Exception raised by the :class:`HTMLParser` class when it encounters an error
   while parsing.  This exception provides three attributes: :attr:`msg` is a brief
   message explaining the error, :attr:`lineno` is the number of the line on which
   the broken construct was detected, and :attr:`offset` is the number of
   characters into the line at which the construct starts.

:class:`HTMLParser` instances have the following methods:


.. method:: HTMLParser.reset()

   Reset the instance.  Loses all unprocessed data.  This is called implicitly at
   instantiation time.


.. method:: HTMLParser.feed(data)

   Feed some text to the parser.  It is processed insofar as it consists of
   complete elements; incomplete data is buffered until more data is fed or
   :meth:`close` is called.


.. method:: HTMLParser.close()

   Force processing of all buffered data as if it were followed by an end-of-file
   mark.  This method may be redefined by a derived class to define additional
   processing at the end of the input, but the redefined version should always call
   the :class:`HTMLParser` base class method :meth:`close`.


.. method:: HTMLParser.getpos()

   Return current line number and offset.


.. method:: HTMLParser.get_starttag_text()

   Return the text of the most recently opened start tag.  This should not normally
   be needed for structured processing, but may be useful in dealing with HTML "as
   deployed" or for re-generating input with minimal changes (whitespace between
   attributes can be preserved, etc.).


.. method:: HTMLParser.handle_starttag(tag, attrs)

   This method is called to handle the start of a tag.  It is intended to be
   overridden by a derived class; the base class implementation does nothing.

   The *tag* argument is the name of the tag converted to lower case. The *attrs*
   argument is a list of ``(name, value)`` pairs containing the attributes found
   inside the tag's ``<>`` brackets.  The *name* will be translated to lower case,
   and quotes in the *value* have been removed, and character and entity references
   have been replaced.  For instance, for the tag ``<A
   HREF="http://www.cwi.nl/">``, this method would be called as
   ``handle_starttag('a', [('href', 'http://www.cwi.nl/')])``.

   All entity references from htmlentitydefs are replaced in the attribute
   values.


.. method:: HTMLParser.handle_startendtag(tag, attrs)

   Similar to :meth:`handle_starttag`, but called when the parser encounters an
   XHTML-style empty tag (``<a .../>``).  This method may be overridden by
   subclasses which require this particular lexical information; the default
   implementation simple calls :meth:`handle_starttag` and :meth:`handle_endtag`.


.. method:: HTMLParser.handle_endtag(tag)

   This method is called to handle the end tag of an element.  It is intended to be
   overridden by a derived class; the base class implementation does nothing.  The
   *tag* argument is the name of the tag converted to lower case.


.. method:: HTMLParser.handle_data(data)

   This method is called to process arbitrary data.  It is intended to be
   overridden by a derived class; the base class implementation does nothing.


.. method:: HTMLParser.handle_charref(name)

   This method is called to process a character reference of the form ``&#ref;``.
   It is intended to be overridden by a derived class; the base class
   implementation does nothing.


.. method:: HTMLParser.handle_entityref(name)

   This method is called to process a general entity reference of the form
   ``&name;`` where *name* is an general entity reference.  It is intended to be
   overridden by a derived class; the base class implementation does nothing.


.. method:: HTMLParser.handle_comment(data)

   This method is called when a comment is encountered.  The *comment* argument is
   a string containing the text between the ``--`` and ``--`` delimiters, but not
   the delimiters themselves.  For example, the comment ``<!--text-->`` will cause
   this method to be called with the argument ``'text'``.  It is intended to be
   overridden by a derived class; the base class implementation does nothing.


.. method:: HTMLParser.handle_decl(decl)

   Method called when an SGML declaration is read by the parser.  The *decl*
   parameter will be the entire contents of the declaration inside the ``<!``...\
   ``>`` markup.  It is intended to be overridden by a derived class; the base
   class implementation does nothing.


.. method:: HTMLParser.handle_pi(data)

   Method called when a processing instruction is encountered.  The *data*
   parameter will contain the entire processing instruction. For example, for the
   processing instruction ``<?proc color='red'>``, this method would be called as
   ``handle_pi("proc color='red'")``.  It is intended to be overridden by a derived
   class; the base class implementation does nothing.

   .. note::

      The :class:`HTMLParser` class uses the SGML syntactic rules for processing
      instructions.  An XHTML processing instruction using the trailing ``'?'`` will
      cause the ``'?'`` to be included in *data*.


.. _htmlparser-example:

Example HTML Parser Application
-------------------------------

As a basic example, below is a very basic HTML parser that uses the
:class:`HTMLParser` class to print out tags as they are encountered::

   from HTMLParser import HTMLParser

   class MyHTMLParser(HTMLParser):

       def handle_starttag(self, tag, attrs):
           print("Encountered the beginning of a %s tag" % tag)

       def handle_endtag(self, tag):
           print("Encountered the end of a %s tag" % tag)

