.. _markup:

**********************************
Structured Markup Processing Tools
**********************************

Python supports a variety of modules to work with various forms of structured
data markup.  This includes modules to work with the Standard Generalized Markup
Language (SGML) and the Hypertext Markup Language (HTML), and several interfaces
for working with the Extensible Markup Language (XML).

It is important to note that modules in the :mod:`xml` package require that
there be at least one SAX-compliant XML parser available. The Expat parser is
included with Python, so the :mod:`xml.parsers.expat` module will always be
available.

The documentation for the :mod:`xml.dom` and :mod:`xml.sax` packages are the
definition of the Python bindings for the DOM and SAX interfaces.


.. toctree::

   html.rst
   html.parser.rst
   html.entities.rst
   xml.rst
   xml.etree.elementtree.rst
   xml.dom.rst
   xml.dom.minidom.rst
   xml.dom.pulldom.rst
   xml.sax.rst
   xml.sax.handler.rst
   xml.sax.utils.rst
   xml.sax.reader.rst
   pyexpat.rst
