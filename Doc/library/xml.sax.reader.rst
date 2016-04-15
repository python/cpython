
:mod:`xml.sax.xmlreader` --- Interface for XML parsers
======================================================

.. module:: xml.sax.xmlreader
   :synopsis: Interface which SAX-compliant XML parsers must implement.
.. moduleauthor:: Lars Marius Garshol <larsga@garshol.priv.no>
.. sectionauthor:: Martin v. Löwis <martin@v.loewis.de>


.. versionadded:: 2.0

SAX parsers implement the :class:`XMLReader` interface. They are implemented in
a Python module, which must provide a function :func:`create_parser`. This
function is invoked by  :func:`xml.sax.make_parser` with no arguments to create
a new  parser object.


.. class:: XMLReader()

   Base class which can be inherited by SAX parsers.


.. class:: IncrementalParser()

   In some cases, it is desirable not to parse an input source at once, but to feed
   chunks of the document as they get available. Note that the reader will normally
   not read the entire file, but read it in chunks as well; still :meth:`parse`
   won't return until the entire document is processed. So these interfaces should
   be used if the blocking behaviour of :meth:`parse` is not desirable.

   When the parser is instantiated it is ready to begin accepting data from the
   feed method immediately. After parsing has been finished with a call to close
   the reset method must be called to make the parser ready to accept new data,
   either from feed or using the parse method.

   Note that these methods must *not* be called during parsing, that is, after
   parse has been called and before it returns.

   By default, the class also implements the parse method of the XMLReader
   interface using the feed, close and reset methods of the IncrementalParser
   interface as a convenience to SAX 2.0 driver writers.


.. class:: Locator()

   Interface for associating a SAX event with a document location. A locator object
   will return valid results only during calls to DocumentHandler methods; at any
   other time, the results are unpredictable. If information is not available,
   methods may return ``None``.


.. class:: InputSource([systemId])

   Encapsulation of the information needed by the :class:`XMLReader` to read
   entities.

   This class may include information about the public identifier, system
   identifier, byte stream (possibly with character encoding information) and/or
   the character stream of an entity.

   Applications will create objects of this class for use in the
   :meth:`XMLReader.parse` method and for returning from
   EntityResolver.resolveEntity.

   An :class:`InputSource` belongs to the application, the :class:`XMLReader` is
   not allowed to modify :class:`InputSource` objects passed to it from the
   application, although it may make copies and modify those.


.. class:: AttributesImpl(attrs)

   This is an implementation of the :class:`Attributes` interface (see section
   :ref:`attributes-objects`).  This is a dictionary-like object which
   represents the element attributes in a :meth:`startElement` call. In addition
   to the most useful dictionary operations, it supports a number of other
   methods as described by the interface. Objects of this class should be
   instantiated by readers; *attrs* must be a dictionary-like object containing
   a mapping from attribute names to attribute values.


.. class:: AttributesNSImpl(attrs, qnames)

   Namespace-aware variant of :class:`AttributesImpl`, which will be passed to
   :meth:`startElementNS`. It is derived from :class:`AttributesImpl`, but
   understands attribute names as two-tuples of *namespaceURI* and
   *localname*. In addition, it provides a number of methods expecting qualified
   names as they appear in the original document.  This class implements the
   :class:`AttributesNS` interface (see section :ref:`attributes-ns-objects`).


.. _xmlreader-objects:

XMLReader Objects
-----------------

The :class:`XMLReader` interface supports the following methods:


.. method:: XMLReader.parse(source)

   Process an input source, producing SAX events. The *source* object can be a
   system identifier (a string identifying the input source -- typically a file
   name or a URL), a file-like object, or an :class:`InputSource` object. When
   :meth:`parse` returns, the input is completely processed, and the parser object
   can be discarded or reset. As a limitation, the current implementation only
   accepts byte streams; processing of character streams is for further study.


.. method:: XMLReader.getContentHandler()

   Return the current :class:`~xml.sax.handler.ContentHandler`.


.. method:: XMLReader.setContentHandler(handler)

   Set the current :class:`~xml.sax.handler.ContentHandler`.  If no
   :class:`~xml.sax.handler.ContentHandler` is set, content events will be
   discarded.


.. method:: XMLReader.getDTDHandler()

   Return the current :class:`~xml.sax.handler.DTDHandler`.


.. method:: XMLReader.setDTDHandler(handler)

   Set the current :class:`~xml.sax.handler.DTDHandler`.  If no
   :class:`~xml.sax.handler.DTDHandler` is set, DTD
   events will be discarded.


.. method:: XMLReader.getEntityResolver()

   Return the current :class:`~xml.sax.handler.EntityResolver`.


.. method:: XMLReader.setEntityResolver(handler)

   Set the current :class:`~xml.sax.handler.EntityResolver`.  If no
   :class:`~xml.sax.handler.EntityResolver` is set,
   attempts to resolve an external entity will result in opening the system
   identifier for the entity, and fail if it is not available.


.. method:: XMLReader.getErrorHandler()

   Return the current :class:`~xml.sax.handler.ErrorHandler`.


.. method:: XMLReader.setErrorHandler(handler)

   Set the current error handler.  If no :class:`~xml.sax.handler.ErrorHandler`
   is set, errors will be raised as exceptions, and warnings will be printed.


.. method:: XMLReader.setLocale(locale)

   Allow an application to set the locale for errors and warnings.

   SAX parsers are not required to provide localization for errors and warnings; if
   they cannot support the requested locale, however, they must raise a SAX
   exception.  Applications may request a locale change in the middle of a parse.


.. method:: XMLReader.getFeature(featurename)

   Return the current setting for feature *featurename*.  If the feature is not
   recognized, :exc:`SAXNotRecognizedException` is raised. The well-known
   featurenames are listed in the module :mod:`xml.sax.handler`.


.. method:: XMLReader.setFeature(featurename, value)

   Set the *featurename* to *value*. If the feature is not recognized,
   :exc:`SAXNotRecognizedException` is raised. If the feature or its setting is not
   supported by the parser, *SAXNotSupportedException* is raised.


.. method:: XMLReader.getProperty(propertyname)

   Return the current setting for property *propertyname*. If the property is not
   recognized, a :exc:`SAXNotRecognizedException` is raised. The well-known
   propertynames are listed in the module :mod:`xml.sax.handler`.


.. method:: XMLReader.setProperty(propertyname, value)

   Set the *propertyname* to *value*. If the property is not recognized,
   :exc:`SAXNotRecognizedException` is raised. If the property or its setting is
   not supported by the parser, *SAXNotSupportedException* is raised.


.. _incremental-parser-objects:

IncrementalParser Objects
-------------------------

Instances of :class:`IncrementalParser` offer the following additional methods:


.. method:: IncrementalParser.feed(data)

   Process a chunk of *data*.


.. method:: IncrementalParser.close()

   Assume the end of the document. That will check well-formedness conditions that
   can be checked only at the end, invoke handlers, and may clean up resources
   allocated during parsing.


.. method:: IncrementalParser.reset()

   This method is called after close has been called to reset the parser so that it
   is ready to parse new documents. The results of calling parse or feed after
   close without calling reset are undefined.


.. _locator-objects:

Locator Objects
---------------

Instances of :class:`Locator` provide these methods:


.. method:: Locator.getColumnNumber()

   Return the column number where the current event ends.


.. method:: Locator.getLineNumber()

   Return the line number where the current event ends.


.. method:: Locator.getPublicId()

   Return the public identifier for the current event.


.. method:: Locator.getSystemId()

   Return the system identifier for the current event.


.. _input-source-objects:

InputSource Objects
-------------------


.. method:: InputSource.setPublicId(id)

   Sets the public identifier of this :class:`InputSource`.


.. method:: InputSource.getPublicId()

   Returns the public identifier of this :class:`InputSource`.


.. method:: InputSource.setSystemId(id)

   Sets the system identifier of this :class:`InputSource`.


.. method:: InputSource.getSystemId()

   Returns the system identifier of this :class:`InputSource`.


.. method:: InputSource.setEncoding(encoding)

   Sets the character encoding of this :class:`InputSource`.

   The encoding must be a string acceptable for an XML encoding declaration (see
   section 4.3.3 of the XML recommendation).

   The encoding attribute of the :class:`InputSource` is ignored if the
   :class:`InputSource` also contains a character stream.


.. method:: InputSource.getEncoding()

   Get the character encoding of this InputSource.


.. method:: InputSource.setByteStream(bytefile)

   Set the byte stream (a Python file-like object which does not perform
   byte-to-character conversion) for this input source.

   The SAX parser will ignore this if there is also a character stream specified,
   but it will use a byte stream in preference to opening a URI connection itself.

   If the application knows the character encoding of the byte stream, it should
   set it with the setEncoding method.


.. method:: InputSource.getByteStream()

   Get the byte stream for this input source.

   The getEncoding method will return the character encoding for this byte stream,
   or None if unknown.


.. method:: InputSource.setCharacterStream(charfile)

   Set the character stream for this input source. (The stream must be a Python 1.6
   Unicode-wrapped file-like that performs conversion to Unicode strings.)

   If there is a character stream specified, the SAX parser will ignore any byte
   stream and will not attempt to open a URI connection to the system identifier.


.. method:: InputSource.getCharacterStream()

   Get the character stream for this input source.


.. _attributes-objects:

The :class:`Attributes` Interface
---------------------------------

:class:`Attributes` objects implement a portion of the mapping protocol,
including the methods :meth:`~collections.Mapping.copy`,
:meth:`~collections.Mapping.get`,
:meth:`~collections.Mapping.has_key`,
:meth:`~collections.Mapping.items`,
:meth:`~collections.Mapping.keys`,
and :meth:`~collections.Mapping.values`.  The following methods
are also provided:


.. method:: Attributes.getLength()

   Return the number of attributes.


.. method:: Attributes.getNames()

   Return the names of the attributes.


.. method:: Attributes.getType(name)

   Returns the type of the attribute *name*, which is normally ``'CDATA'``.


.. method:: Attributes.getValue(name)

   Return the value of attribute *name*.

.. getValueByQName, getNameByQName, getQNameByName, getQNames available
.. here already, but documented only for derived class.


.. _attributes-ns-objects:

The :class:`AttributesNS` Interface
-----------------------------------

This interface is a subtype of the :class:`Attributes` interface (see section
:ref:`attributes-objects`).  All methods supported by that interface are also
available on :class:`AttributesNS` objects.

The following methods are also available:


.. method:: AttributesNS.getValueByQName(name)

   Return the value for a qualified name.


.. method:: AttributesNS.getNameByQName(name)

   Return the ``(namespace, localname)`` pair for a qualified *name*.


.. method:: AttributesNS.getQNameByName(name)

   Return the qualified name for a ``(namespace, localname)`` pair.


.. method:: AttributesNS.getQNames()

   Return the qualified names of all attributes.

