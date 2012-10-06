.. _xml:

XML Processing Modules
======================

Python's interfaces for processing XML are grouped in the ``xml`` package.

It is important to note that modules in the :mod:`xml` package require that
there be at least one SAX-compliant XML parser available. The Expat parser is
included with Python, so the :mod:`xml.parsers.expat` module will always be
available.

The documentation for the :mod:`xml.dom` and :mod:`xml.sax` packages are the
definition of the Python bindings for the DOM and SAX interfaces.

The XML handling submodules are:

* :mod:`xml.etree.ElementTree`: the ElementTree API, a simple and lightweight

..

* :mod:`xml.dom`: the DOM API definition
* :mod:`xml.dom.minidom`: a lightweight DOM implementation
* :mod:`xml.dom.pulldom`: support for building partial DOM trees

..

* :mod:`xml.sax`: SAX2 base classes and convenience functions
* :mod:`xml.parsers.expat`: the Expat parser binding
