.. _xml:

XML Processing Modules
======================

.. module:: xml
   :synopsis: Package containing XML processing modules
.. sectionauthor:: Christian Heimes <christian@python.org>
.. sectionauthor:: Georg Brandl <georg@python.org>


Python's interfaces for processing XML are grouped in the ``xml`` package.

.. warning::

   The XML modules are not secure against erroneous or maliciously
   constructed data.  If you need to parse untrusted or unauthenticated data see
   :ref:`xml-vulnerabilities`.

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


.. _xml-vulnerabilities:

XML vulnerabilities
===================

The XML processing modules are not secure against maliciously constructed data.
An attacker can abuse vulnerabilities for e.g. denial of service attacks, to
access local files, to generate network connections to other machines, or
to or circumvent firewalls. The attacks on XML abuse unfamiliar features
like inline `DTD`_ (document type definition) with entities.

The following table gives an overview of the known attacks and if the various
modules are vulnerable to them.

=========================  ========  =========  =========  ========  =========
kind                       sax       etree      minidom    pulldom   xmlrpc
=========================  ========  =========  =========  ========  =========
billion laughs             **Yes**   **Yes**    **Yes**    **Yes**   **Yes**
quadratic blowup           **Yes**   **Yes**    **Yes**    **Yes**   **Yes**
external entity expansion  **Yes**   No    (1)  No    (2)  **Yes**   No    (3)
DTD retrieval              **Yes**   No         No         **Yes**   No
decompression bomb         No        No         No         No        **Yes**
=========================  ========  =========  =========  ========  =========

1. :mod:`xml.etree.ElementTree` doesn't expand external entities and raises a
   ParserError when an entity occurs.
2. :mod:`xml.dom.minidom` doesn't expand external entities and simply returns
   the unexpanded entity verbatim.
3. :mod:`xmlrpclib` doesn't expand external entities and omits them.


billion laughs / exponential entity expansion
  The `Billion Laughs`_ attack -- also known as exponential entity expansion --
  uses multiple levels of nested entities. Each entity refers to another entity
  several times, the final entity definition contains a small string. Eventually
  the small string is expanded to several gigabytes. The exponential expansion
  consumes lots of CPU time, too.

quadratic blowup entity expansion
  A quadratic blowup attack is similar to a `Billion Laughs`_ attack; it abuses
  entity expansion, too. Instead of nested entities it repeats one large entity
  with a couple of thousand chars over and over again. The attack isn't as
  efficient as the exponential case but it avoids triggering countermeasures of
  parsers against heavily nested entities.

external entity expansion
  Entity declarations can contain more than just text for replacement. They can
  also point to external resources by public identifiers or system identifiers.
  System identifiers are standard URIs or can refer to local files. The XML
  parser retrieves the resource with e.g. HTTP or FTP requests and embeds the
  content into the XML document.

DTD retrieval
  Some XML libraries like Python's mod:'xml.dom.pulldom' retrieve document type
  definitions from remote or local locations. The feature has similar
  implications as the external entity expansion issue.

decompression bomb
  The issue of decompression bombs (aka `ZIP bomb`_) apply to all XML libraries
  that can parse compressed XML stream like gzipped HTTP streams or LZMA-ed
  files. For an attacker it can reduce the amount of transmitted data by three
  magnitudes or more.

The documentation of `defusedxml`_ on PyPI has further information about
all known attack vectors with examples and references.

defused packages
----------------

These external packages are recommended for any code that parses
untrusted XML data.

`defusedxml`_ is a pure Python package with modified subclasses of all stdlib
XML parsers that prevent any potentially malicious operation. The
package also ships with example exploits and extended documentation on more
XML exploits like xpath injection.

`defusedexpat`_ provides a modified libexpat and patched replacement
:mod:`pyexpat` extension module with countermeasures against entity expansion
DoS attacks. Defusedexpat still allows a sane and configurable amount of entity
expansions. The modifications will be merged into future releases of Python.

The workarounds and modifications are not included in patch releases as they
break backward compatibility. After all inline DTD and entity expansion are
well-defined XML features.


.. _defusedxml: https://pypi.python.org/pypi/defusedxml/
.. _defusedexpat: https://pypi.python.org/pypi/defusedexpat/
.. _Billion Laughs: http://en.wikipedia.org/wiki/Billion_laughs
.. _ZIP bomb: http://en.wikipedia.org/wiki/Zip_bomb
.. _DTD: http://en.wikipedia.org/wiki/Document_Type_Definition
