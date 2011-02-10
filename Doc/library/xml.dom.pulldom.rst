:mod:`xml.dom.pulldom` --- Support for building partial DOM trees
=================================================================

.. module:: xml.dom.pulldom
   :synopsis: Support for building partial DOM trees from SAX events.
.. moduleauthor:: Paul Prescod <paul@prescod.net>

**Source code:** :source:`Lib/xml/dom/pulldom.py`

--------------

:mod:`xml.dom.pulldom` allows building only selected portions of a Document
Object Model representation of a document from SAX events.


.. class:: PullDOM(documentFactory=None)

   :class:`xml.sax.handler.ContentHandler` implementation that ...


.. class:: DOMEventStream(stream, parser, bufsize)

   ...


.. class:: SAX2DOM(documentFactory=None)

   :class:`xml.sax.handler.ContentHandler` implementation that ...


.. function:: parse(stream_or_string, parser=None, bufsize=None)

   ...


.. function:: parseString(string, parser=None)

   ...


.. data:: default_bufsize

   Default value for the *bufsize* parameter to :func:`parse`.

   The value of this variable can be changed before calling :func:`parse` and
   the new value will take effect.


.. _domeventstream-objects:

DOMEventStream Objects
----------------------


.. method:: DOMEventStream.getEvent()

   ...


.. method:: DOMEventStream.expandNode(node)

   ...


.. method:: DOMEventStream.reset()

   ...

