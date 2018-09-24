:mod:`xml.sax` --- Support for SAX2 parsers
===========================================

.. module:: xml.sax
   :synopsis: Package containing SAX2 base classes and convenience functions.

.. moduleauthor:: Lars Marius Garshol <larsga@garshol.priv.no>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Martin v. Löwis <martin@v.loewis.de>

**Source code:** :source:`Lib/xml/sax/__init__.py`

--------------

The :mod:`xml.sax` package provides a number of modules which implement the
Simple API for XML (SAX) interface for Python.  The package itself provides the
SAX exceptions and the convenience functions which will be most used by users of
the SAX API.


.. warning::

   The :mod:`xml.sax` module is not secure against maliciously
   constructed data.  If you need to parse untrusted or unauthenticated data see
   :ref:`xml-vulnerabilities`.

.. versionchanged:: 3.7.1

   The SAX parser no longer processes general external entities by default
   to increase security. Before, the parser created network connections
   to fetch remote files or loaded local files from the file
   system for DTD and entities. The feature can be enabled again with method
   :meth:`~xml.sax.xmlreader.XMLReader.setFeature` on the parser object
   and argument :data:`~xml.sax.handler.feature_external_ges`.

The convenience functions are:


.. function:: make_parser(parser_list=[])

   Create and return a SAX :class:`~xml.sax.xmlreader.XMLReader` object.  The
   first parser found will
   be used.  If *parser_list* is provided, it must be a list of strings which
   name modules that have a function named :func:`create_parser`.  Modules listed
   in *parser_list* will be used before modules in the default list of parsers.


.. function:: parse(filename_or_stream, handler, error_handler=handler.ErrorHandler())

   Create a SAX parser and use it to parse a document.  The document, passed in as
   *filename_or_stream*, can be a filename or a file object.  The *handler*
   parameter needs to be a SAX :class:`~handler.ContentHandler` instance.  If
   *error_handler* is given, it must be a SAX :class:`~handler.ErrorHandler`
   instance; if
   omitted,  :exc:`SAXParseException` will be raised on all errors.  There is no
   return value; all work must be done by the *handler* passed in.


.. function:: parseString(string, handler, error_handler=handler.ErrorHandler())

   Similar to :func:`parse`, but parses from a buffer *string* received as a
   parameter.  *string* must be a :class:`str` instance or a
   :term:`bytes-like object`.

   .. versionchanged:: 3.5
      Added support of :class:`str` instances.

A typical SAX application uses three kinds of objects: readers, handlers and
input sources.  "Reader" in this context is another term for parser, i.e. some
piece of code that reads the bytes or characters from the input source, and
produces a sequence of events. The events then get distributed to the handler
objects, i.e. the reader invokes a method on the handler.  A SAX application
must therefore obtain a reader object, create or open the input sources, create
the handlers, and connect these objects all together.  As the final step of
preparation, the reader is called to parse the input. During parsing, methods on
the handler objects are called based on structural and syntactic events from the
input data.

For these objects, only the interfaces are relevant; they are normally not
instantiated by the application itself.  Since Python does not have an explicit
notion of interface, they are formally introduced as classes, but applications
may use implementations which do not inherit from the provided classes.  The
:class:`~xml.sax.xmlreader.InputSource`, :class:`~xml.sax.xmlreader.Locator`,
:class:`~xml.sax.xmlreader.Attributes`, :class:`~xml.sax.xmlreader.AttributesNS`,
and :class:`~xml.sax.xmlreader.XMLReader` interfaces are defined in the
module :mod:`xml.sax.xmlreader`.  The handler interfaces are defined in
:mod:`xml.sax.handler`.  For convenience,
:class:`~xml.sax.xmlreader.InputSource` (which is often
instantiated directly) and the handler classes are also available from
:mod:`xml.sax`.  These interfaces are described below.

In addition to these classes, :mod:`xml.sax` provides the following exception
classes.


.. exception:: SAXException(msg, exception=None)

   Encapsulate an XML error or warning.  This class can contain basic error or
   warning information from either the XML parser or the application: it can be
   subclassed to provide additional functionality or to add localization.  Note
   that although the handlers defined in the
   :class:`~xml.sax.handler.ErrorHandler` interface
   receive instances of this exception, it is not required to actually raise the
   exception --- it is also useful as a container for information.

   When instantiated, *msg* should be a human-readable description of the error.
   The optional *exception* parameter, if given, should be ``None`` or an exception
   that was caught by the parsing code and is being passed along as information.

   This is the base class for the other SAX exception classes.


.. exception:: SAXParseException(msg, exception, locator)

   Subclass of :exc:`SAXException` raised on parse errors. Instances of this
   class are passed to the methods of the SAX
   :class:`~xml.sax.handler.ErrorHandler` interface to provide information
   about the parse error.  This class supports the SAX
   :class:`~xml.sax.xmlreader.Locator` interface as well as the
   :class:`SAXException` interface.


.. exception:: SAXNotRecognizedException(msg, exception=None)

   Subclass of :exc:`SAXException` raised when a SAX
   :class:`~xml.sax.xmlreader.XMLReader` is
   confronted with an unrecognized feature or property.  SAX applications and
   extensions may use this class for similar purposes.


.. exception:: SAXNotSupportedException(msg, exception=None)

   Subclass of :exc:`SAXException` raised when a SAX
   :class:`~xml.sax.xmlreader.XMLReader` is asked to
   enable a feature that is not supported, or to set a property to a value that the
   implementation does not support.  SAX applications and extensions may use this
   class for similar purposes.


.. seealso::

   `SAX: The Simple API for XML <http://www.saxproject.org/>`_
      This site is the focal point for the definition of the SAX API.  It provides a
      Java implementation and online documentation.  Links to implementations and
      historical information are also available.

   Module :mod:`xml.sax.handler`
      Definitions of the interfaces for application-provided objects.

   Module :mod:`xml.sax.saxutils`
      Convenience functions for use in SAX applications.

   Module :mod:`xml.sax.xmlreader`
      Definitions of the interfaces for parser-provided objects.


.. _sax-exception-objects:

SAXException Objects
--------------------

The :class:`SAXException` exception class supports the following methods:


.. method:: SAXException.getMessage()

   Return a human-readable message describing the error condition.


.. method:: SAXException.getException()

   Return an encapsulated exception object, or ``None``.

