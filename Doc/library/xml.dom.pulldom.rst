:mod:`!xml.dom.pulldom` --- Support for building partial DOM trees
==================================================================

.. module:: xml.dom.pulldom
   :synopsis: Support for building partial DOM trees from SAX events.

**Source code:** :source:`Lib/xml/dom/pulldom.py`

.. The module was written by Paul Prescod and added in Python 2.0.
   It is not based on any specification: the implementation is the only
   reference.  The Java Streaming API for XML (StAX, JSR 173) is based on
   it, among other pull parsers.

--------------

The :mod:`!xml.dom.pulldom` module provides a "pull parser" which can also be
asked to produce DOM-accessible fragments of the document where necessary. The
basic concept involves pulling "events" from a stream of incoming XML and
processing them. In contrast to SAX which also employs an event-driven
processing model together with callbacks, the user of a pull parser is
responsible for explicitly pulling events from the stream, looping over those
events until either processing is finished or an error condition occurs.


.. note::

   If you need to parse untrusted or unauthenticated data, see
   :ref:`xml-security`.

.. versionchanged:: 3.7.1

   The SAX parser no longer processes general external entities by default to
   increase security by default. To enable processing of external entities,
   pass a custom parser instance in::

      from xml.dom.pulldom import parse
      from xml.sax import make_parser
      from xml.sax.handler import feature_external_ges

      parser = make_parser()
      parser.setFeature(feature_external_ges, True)
      parse(filename, parser=parser)


Example::

   from xml.dom import pulldom

   doc = pulldom.parse('sales_items.xml')
   for event, node in doc:
       if event == pulldom.START_ELEMENT and node.tagName == 'item':
           if int(node.getAttribute('price')) > 50:
               doc.expandNode(node)
               print(node.toxml())

``event`` is one of the following constants,
and ``node`` is the node which the event is about.
The nodes implement the :mod:`xml.dom` interfaces;
they are created by the DOM implementation given to :class:`PullDOM`,
which is :mod:`xml.dom.minidom` by default.


.. data:: START_DOCUMENT
          END_DOCUMENT

   The start and the end of the document.
   *node* is the :class:`~xml.dom.Document`.


.. data:: START_ELEMENT
          END_ELEMENT

   The start tag and the end tag of an element.
   *node* is the :class:`~xml.dom.Element`.


.. data:: CHARACTERS

   Character data.
   *node* is the :class:`~xml.dom.Text` node.


.. data:: IGNORABLE_WHITESPACE

   White space in element content, as declared in the DTD.
   *node* is the :class:`~xml.dom.Text` node.


.. data:: COMMENT

   A comment.
   *node* is the :class:`~xml.dom.Comment` node.


.. data:: PROCESSING_INSTRUCTION

   A processing instruction.
   *node* is the :class:`~xml.dom.ProcessingInstruction` node.

Since the document is treated as a "flat" stream of events, the document "tree"
is implicitly traversed and the desired elements are found regardless of their
depth in the tree. In other words, one does not need to consider hierarchical
issues such as recursive searching of the document nodes, although if the
context of elements were important, one would either need to maintain some
context-related state (i.e. remembering where one is in the document at any
given point) or to make use of the :func:`DOMEventStream.expandNode` method
and switch to DOM-related processing.


.. class:: PullDOM(documentFactory=None)

   Subclass of :class:`xml.sax.handler.ContentHandler` which turns SAX events
   into the events of the pull parser.
   The nodes are created, but they are not added to the tree,
   unless :meth:`~DOMEventStream.expandNode` is called.
   *documentFactory*, if given, is a DOM implementation used to create
   the document; by default the implementation of :mod:`xml.dom.minidom`
   is used.


.. class:: SAX2DOM(documentFactory=None)

   Subclass of :class:`PullDOM` which also adds every created node
   to the tree, so that the complete document is built.


.. function:: parse(stream_or_string, parser=None, bufsize=None)

   Return a :class:`DOMEventStream` from the given input. *stream_or_string* may be
   either a file name, or a file-like object. *parser*, if given, must be an
   :class:`~xml.sax.xmlreader.XMLReader` object. This function will change the
   document handler of the
   parser and activate namespace support; other parser configuration (like
   setting an entity resolver) must have been done in advance.

If you have XML in a string, you can use the :func:`parseString` function instead:

.. function:: parseString(string, parser=None)

   Return a :class:`DOMEventStream` that represents the *string*.
   *string* must be a :class:`str` instance;
   to parse bytes, pass a binary file object to :func:`parse`.

.. data:: default_bufsize

   Default value for the *bufsize* parameter to :func:`parse`.

   The value of this variable can be changed before calling :func:`parse` and
   the new value will take effect.

.. _domeventstream-objects:

DOMEventStream Objects
----------------------

.. class:: DOMEventStream(stream, parser, bufsize)

   Produce the events for the data read from the file object *stream*
   by the :class:`~xml.sax.xmlreader.XMLReader` *parser*.
   The data is read by *bufsize* bytes, or characters for a text stream,
   at a time.

   .. versionchanged:: 3.11
      Support for :meth:`~object.__getitem__` method has been removed.

   .. method:: getEvent()

      Return the next ``(event, node)`` tuple,
      or ``None`` at the end of the document.
      See above for the events and the corresponding nodes.
      The current node does not contain information about its children, unless
      :meth:`expandNode` is called.

   .. method:: expandNode(node)

      Expands all children of *node* into *node*. Example::

          from xml.dom import pulldom

          xml = '<html><title>Foo</title> <p>Some text <div>and more</div></p> </html>'
          doc = pulldom.parseString(xml)
          for event, node in doc:
              if event == pulldom.START_ELEMENT and node.tagName == 'p':
                  # Following statement only prints '<p/>'
                  print(node.toxml())
                  doc.expandNode(node)
                  # Following statement prints node with all its children '<p>Some text <div>and more</div></p>'
                  print(node.toxml())

   .. method:: reset()

      Discard the events which are not read yet
      and prepare the object for parsing a new document.


   .. method:: clear()

      Release the parser and the document.
      The stream is not closed, and the object can no longer be used.
