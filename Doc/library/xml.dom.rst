:mod:`!xml.dom` --- The Document Object Model API
=================================================

.. module:: xml.dom
   :synopsis: Document Object Model API for Python.

**Source code:** :source:`Lib/xml/dom/__init__.py`

--------------

The Document Object Model, or "DOM," is a cross-language API from the World Wide
Web Consortium (W3C) for accessing and modifying XML documents.  A DOM
implementation presents an XML document as a tree structure, or allows client
code to build such a structure from scratch.  It then gives access to the
structure through a set of objects which provided well-known interfaces.

The DOM is extremely useful for random-access applications.  SAX only allows you
a view of one bit of the document at a time.  If you are looking at one SAX
element, you have no access to another.  If you are looking at a text node, you
have no access to a containing element. When you write a SAX application, you
need to keep track of your program's position in the document somewhere in your
own code.  SAX does not do it for you.  Also, if you need to look ahead in the
XML document, you are just out of luck.

Some applications are simply impossible in an event driven model with no access
to a tree.  Of course you could build some sort of tree yourself in SAX events,
but the DOM allows you to avoid writing that code.  The DOM is a standard tree
representation for XML data.

The Document Object Model is being defined by the W3C in stages, or "levels" in
their terminology.  The Python mapping of the API is substantially based on the
DOM Level 2 recommendation.

DOM applications typically start by parsing some XML into a DOM.  How this is
accomplished is not covered at all by DOM Level 1, and Level 2 provides only
limited improvements: There is a :class:`DOMImplementation` object class which
provides access to :class:`Document` creation methods, but no way to access an
XML reader/parser/Document builder in an implementation-independent way. There
is also no well-defined way to access these methods without an existing
:class:`Document` object.  In Python, each DOM implementation will provide a
function :func:`getDOMImplementation`. DOM Level 3 adds a Load/Store
specification, which defines an interface to the reader, but this is not yet
available in the Python standard library.

Once you have a DOM document object, you can access the parts of your XML
document through its properties and methods.  These properties are defined in
the DOM specification; this portion of the reference manual describes the
interpretation of the specification in Python.

The specification provided by the W3C defines the DOM API for Java, ECMAScript,
and OMG IDL.  The Python mapping defined here is based in large part on the IDL
version of the specification, but strict compliance is not required (though
implementations are free to support the strict mapping from IDL).  See section
:ref:`dom-conformance` for a detailed discussion of mapping requirements.


.. seealso::

   `Document Object Model (DOM) Level 2 Specification <https://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113/>`_
      The W3C recommendation upon which the Python DOM API is based.

   `Document Object Model (DOM) Level 1 Specification <https://www.w3.org/TR/REC-DOM-Level-1/>`_
      The W3C recommendation for the DOM supported by :mod:`xml.dom.minidom`.

   `Python Language Mapping Specification <https://www.omg.org/spec/PYTH/1.2/PDF>`_
      This specifies the mapping from OMG IDL to Python.


Module Contents
---------------

The :mod:`!xml.dom` contains the following functions:


.. function:: registerDOMImplementation(name, factory)

   Register the *factory* function with the name *name*.  The factory function
   should return an object which implements the :class:`DOMImplementation`
   interface.  The factory function can return the same object every time, or a new
   one for each call, as appropriate for the specific implementation (e.g. if that
   implementation supports some customization).


.. function:: getDOMImplementation(name=None, features=())

   Return a suitable DOM implementation. The *name* is either well-known, the
   module name of a DOM implementation, or ``None``. If it is not ``None``, imports
   the corresponding module and returns a :class:`DOMImplementation` object if the
   import succeeds.  If no name is given, and if the environment variable
   :envvar:`!PYTHON_DOM` is set, this variable is used to find the implementation.
   The only well-known name in the standard library is ``'minidom'``,
   for :mod:`xml.dom.minidom`.

   If name is not given, this examines the available implementations to find one
   with the required feature set.  If no implementation can be found, raise an
   :exc:`ImportError`.  The features list must be a sequence of ``(feature,
   version)`` pairs which are passed to the :meth:`~DOMImplementation.hasFeature`
   method on available :class:`DOMImplementation` objects.

Some convenience constants are also provided:


.. data:: EMPTY_NAMESPACE

   The value used to indicate that no namespace is associated with a node in the
   DOM.  This is typically found as the :attr:`~Node.namespaceURI` of a node, or
   used as the *namespaceURI* parameter to a namespaces-specific method.


.. data:: XML_NAMESPACE

   The namespace URI associated with the reserved prefix ``xml``, as defined by
   `Namespaces in XML <https://www.w3.org/TR/REC-xml-names/>`_ (section 4).


.. data:: XMLNS_NAMESPACE

   The namespace URI for namespace declarations, as defined by `Document Object
   Model (DOM) Level 2 Core Specification
   <https://www.w3.org/TR/DOM-Level-2-Core/core.html>`_ (section 1.1.8).


.. data:: XHTML_NAMESPACE

   The URI of the XHTML namespace as defined by `XHTML 1.0: The Extensible
   HyperText Markup Language <https://www.w3.org/TR/xhtml1/>`_ (section 3.1.1).


In addition, :mod:`!xml.dom` contains a base :class:`Node` class and the DOM
exception classes.  The :class:`Node` class provided by this module does not
implement any of the methods or attributes defined by the DOM specification;
concrete DOM implementations must provide those.  The :class:`Node` class
provided as part of this module does provide the constants used for the
:attr:`~Node.nodeType` attribute on concrete :class:`Node` objects; they are located
within the class rather than at the module level to conform with the DOM
specifications.

.. Should the Node documentation go here?


.. _dom-objects:

Objects in the DOM
------------------

The definitive documentation for the DOM is the DOM specification from the W3C.

The names documented in this section are DOM interfaces.
With the exception of :class:`Node` and the exception classes,
they are not provided by the :mod:`!xml.dom` module itself,
but by concrete DOM implementations, such as :mod:`xml.dom.minidom`.

Note that DOM attributes may also be manipulated as nodes instead of as simple
strings.  It is fairly rare that you must do this, however, so this usage is not
yet documented.

+--------------------------------+-----------------------------------+---------------------------------+
| Interface                      | Section                           | Purpose                         |
+================================+===================================+=================================+
| :class:`DOMImplementation`     | :ref:`dom-implementation-objects` | Interface to the underlying     |
|                                |                                   | implementation.                 |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Node`                  | :ref:`dom-node-objects`           | Base interface for most objects |
|                                |                                   | in a document.                  |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`NodeList`              | :ref:`dom-nodelist-objects`       | Interface for a sequence of     |
|                                |                                   | nodes.                          |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`DocumentType`          | :ref:`dom-documenttype-objects`   | Information about the           |
|                                |                                   | declarations needed to process  |
|                                |                                   | a document.                     |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Document`              | :ref:`dom-document-objects`       | Object which represents an      |
|                                |                                   | entire document.                |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Element`               | :ref:`dom-element-objects`        | Element nodes in the document   |
|                                |                                   | hierarchy.                      |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Attr`                  | :ref:`dom-attr-objects`           | Attribute value nodes on        |
|                                |                                   | element nodes.                  |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Comment`               | :ref:`dom-comment-objects`        | Representation of comments in   |
|                                |                                   | the source document.            |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`Text`                  | :ref:`dom-text-objects`           | Nodes containing textual        |
|                                |                                   | content from the document.      |
+--------------------------------+-----------------------------------+---------------------------------+
| :class:`ProcessingInstruction` | :ref:`dom-pi-objects`             | Processing instruction          |
|                                |                                   | representation.                 |
+--------------------------------+-----------------------------------+---------------------------------+

An additional section describes the exceptions defined for working with the DOM
in Python.


.. _dom-implementation-objects:

DOMImplementation Objects
^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: DOMImplementation
   :no-typesetting:

The :class:`DOMImplementation` interface provides a way for applications to
determine the availability of particular features in the DOM they are using.
DOM Level 2 added the ability to create new :class:`Document` and
:class:`DocumentType` objects using the :class:`DOMImplementation` as well.


.. method:: DOMImplementation.hasFeature(feature, version)

   Return ``True`` if the feature identified by the pair of strings *feature* and
   *version* is implemented.


.. method:: DOMImplementation.createDocument(namespaceUri, qualifiedName, doctype)

   Return a new :class:`Document` object (the root of the DOM), with a child
   :class:`Element` object having the given *namespaceUri* and *qualifiedName*. The
   *doctype* must be a :class:`DocumentType` object created by
   :meth:`createDocumentType`, or ``None``. In the Python DOM API, the first two
   arguments can also be ``None`` in order to indicate that no :class:`Element`
   child is to be created.


.. method:: DOMImplementation.createDocumentType(qualifiedName, publicId, systemId)

   Return a new :class:`DocumentType` object that encapsulates the given
   *qualifiedName*, *publicId*, and *systemId* strings, representing the
   information contained in an XML document type declaration.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. _dom-node-objects:

Node Objects
^^^^^^^^^^^^

.. class:: Node
   :no-typesetting:

All of the components of an XML document are subclasses of :class:`Node`.

Only nodes of the following types can have children,
and only children of the listed types:

:class:`Document`
   at most one :class:`Element`, at most one :class:`DocumentType`,
   :class:`ProcessingInstruction` and :class:`Comment`

:class:`DocumentFragment` and :class:`Element`
   :class:`Element`, :class:`Text`, :class:`CDATASection`,
   :class:`ProcessingInstruction` and :class:`Comment`

:class:`Attr`
   :class:`Text`

Nodes of other types cannot have children.
Inserting a child of a not allowed type raises :exc:`HierarchyRequestErr`.


.. attribute:: Node.nodeType

   An integer representing the node type.  Symbolic constants for the types are on
   the :class:`Node` object.
   This is a read-only attribute.


.. data:: Node.ELEMENT_NODE
          Node.ATTRIBUTE_NODE
          Node.TEXT_NODE
          Node.CDATA_SECTION_NODE
          Node.ENTITY_REFERENCE_NODE
          Node.ENTITY_NODE
          Node.PROCESSING_INSTRUCTION_NODE
          Node.COMMENT_NODE
          Node.DOCUMENT_NODE
          Node.DOCUMENT_TYPE_NODE
          Node.DOCUMENT_FRAGMENT_NODE
          Node.NOTATION_NODE

   Integer constants for the possible values
   of the :attr:`~Node.nodeType` attribute.


.. attribute:: Node.parentNode

   The parent of the current node, or ``None`` for the document node. The value is
   always a :class:`Node` object or ``None``.  For :class:`Element` nodes, this
   will be the parent element, except for the root element, in which case it will
   be the :class:`Document` object. For :class:`Attr` nodes, this is always
   ``None``. This is a read-only attribute.


.. attribute:: Node.attributes

   A :class:`NamedNodeMap` of attribute objects.  Only elements have actual values
   for this; others provide ``None`` for this attribute. This is a read-only
   attribute.


.. attribute:: Node.previousSibling

   The node that immediately precedes this one with the same parent.  For
   instance the element with an end-tag that comes just before the *self*
   element's start-tag.  Of course, XML documents are made up of more than just
   elements so the previous sibling could be text, a comment, or something else.
   If this node is the first child of the parent, this attribute will be
   ``None``. This is a read-only attribute.


.. attribute:: Node.nextSibling

   The node that immediately follows this one with the same parent.  See also
   :attr:`previousSibling`.  If this is the last child of the parent, this
   attribute will be ``None``. This is a read-only attribute.


.. attribute:: Node.childNodes

   A :class:`NodeList` of the children of this node.
   If the node has no children, the list is empty.
   This is a read-only attribute.


.. attribute:: Node.firstChild

   The first child of the node, if there are any, or ``None``. This is a read-only
   attribute.


.. attribute:: Node.lastChild

   The last child of the node, if there are any, or ``None``. This is a read-only
   attribute.


.. attribute:: Node.localName

   The part of the :attr:`~Element.tagName` following the colon if there is one,
   else the entire :attr:`~Element.tagName`.  The value is a string.


.. attribute:: Node.prefix

   The part of the :attr:`~Element.tagName` preceding the colon if there is one,
   else the empty string.  The value is a string, or ``None``.


.. attribute:: Node.namespaceURI

   The namespace associated with the element name.  This will be a string or
   ``None``.  This is a read-only attribute.


.. attribute:: Node.ownerDocument

   The :class:`Document` object to which this node belongs, or ``None``
   for a document itself.
   This is a read-only attribute.


.. method:: Node.isSupported(feature, version)

   Return whether the DOM implementation supports a particular *feature*,
   as :meth:`DOMImplementation.hasFeature` does.


.. method:: Node.setUserData(key, data, handler)

   Associate *data* with *key* on this node and return the data previously
   associated with *key*, or ``None``.
   If *data* is ``None``, the association is removed.
   *handler* is called when the node is cloned, imported, renamed or deleted;
   pass ``None`` if no notification is needed.


.. method:: Node.getUserData(key)

   Return the data associated with *key* on this node
   by :meth:`~Node.setUserData`, or ``None``.


.. attribute:: Node.nodeName

   The name of this node, depending on its type; see the table below.
   You can always get the information you would get here from another
   property such as the :attr:`~Element.tagName` property for elements or the
   :attr:`~Attr.name` property for attributes.
   This is a read-only attribute.


.. attribute:: Node.nodeValue

   The value of this node, depending on its type; see the table below.
   The value is a string or ``None``.


The values of :attr:`~Node.nodeName` and :attr:`~Node.nodeValue`
for each node type are:

+--------------------------------+---------------------------------------+-------------------------------------+
| Node type                      | nodeName                              | nodeValue                           |
+================================+=======================================+=====================================+
| :class:`Attr`                  | :attr:`~Attr.name`                    | :attr:`~Attr.value`                 |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`CDATASection`          | ``'#cdata-section'``                  | the content                         |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Comment`               | ``'#comment'``                        | the content                         |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Document`              | ``'#document'``                       | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`DocumentFragment`      | ``'#document-fragment'``              | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`DocumentType`          | :attr:`~DocumentType.name`            | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Element`               | :attr:`~Element.tagName`              | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Entity`                | the name of the entity                | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Notation`              | the name of the notation              | ``None``                            |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`ProcessingInstruction` | :attr:`~ProcessingInstruction.target` | :attr:`~ProcessingInstruction.data` |
+--------------------------------+---------------------------------------+-------------------------------------+
| :class:`Text`                  | ``'#text'``                           | the content                         |
+--------------------------------+---------------------------------------+-------------------------------------+

.. method:: Node.hasAttributes()

   Return ``True`` if the node has any attributes.


.. method:: Node.hasChildNodes()

   Return ``True`` if the node has any child nodes.


.. method:: Node.isSameNode(other)

   Return ``True`` if *other* refers to the same node as this node. This is especially
   useful for DOM implementations which use any sort of proxy architecture (because
   more than one object can refer to the same node).

   .. note::

      This is based on a proposed DOM Level 3 API which is still in the "working
      draft" stage, but this particular interface appears uncontroversial.  Changes
      from the W3C will not necessarily affect this method in the Python DOM interface
      (though any new W3C API for this would also be supported).


.. method:: Node.appendChild(newChild)

   Add a new child node to this node at the end of the list of
   children, returning *newChild*. If the node was already in
   the tree, it is removed first.

   Raise :exc:`WrongDocumentErr` if *newChild* was created by another
   document, and :exc:`HierarchyRequestErr` if it is this node itself or
   its ancestor.


.. method:: Node.insertBefore(newChild, refChild)

   Insert a new child node before an existing child.  It must be the case that
   *refChild* is a child of this node; if not, :exc:`NotFoundErr` is raised.
   *newChild* is returned. If *refChild* is ``None``, it inserts *newChild* at the
   end of the children's list.

   Raise :exc:`WrongDocumentErr` if *newChild* was created by another
   document, and :exc:`HierarchyRequestErr` if it is this node itself or
   its ancestor.


.. method:: Node.removeChild(oldChild)

   Remove a child node.  *oldChild* must be a child of this node; if not,
   :exc:`NotFoundErr` is raised.  *oldChild* is returned on success.  If *oldChild*
   will not be used further, its :meth:`~xml.dom.minidom.Node.unlink` method
   should be called.


.. method:: Node.replaceChild(newChild, oldChild)

   Replace an existing node with a new node. It must be the case that  *oldChild*
   is a child of this node; if not, :exc:`NotFoundErr` is raised.

   Raise :exc:`WrongDocumentErr` if *newChild* was created by another
   document, and :exc:`HierarchyRequestErr` if it is this node itself or
   its ancestor.


.. method:: Node.normalize()

   Join adjacent text nodes so that all stretches of text are stored as single
   :class:`Text` instances.  This simplifies processing text from a DOM tree for
   many applications.


.. method:: Node.cloneNode(deep)

   Clone this node.  Setting *deep* means to clone all child nodes as well.  This
   returns the clone.


.. _dom-nodelist-objects:

NodeList Objects
^^^^^^^^^^^^^^^^

.. class:: NodeList
   :no-typesetting:

A :class:`NodeList` represents a sequence of nodes.  These objects are used in
two ways in the DOM Core recommendation:  an :class:`Element` object provides
one as its list of child nodes, and the :meth:`~Element.getElementsByTagName`
and :meth:`~Element.getElementsByTagNameNS` methods of :class:`Node` return
objects with this interface to represent query results.

:class:`NodeList` does *not* inherit from :class:`Node`.

The DOM Level 2 recommendation defines one method and one attribute for these
objects:


.. method:: NodeList.item(i)

   Return the *i*'th item from the sequence,
   or ``None`` if *i* is out of range.
   Negative indices are not supported.


.. attribute:: NodeList.length

   The number of nodes in the sequence.

In addition, the Python DOM interface requires that some additional support is
provided to allow :class:`NodeList` objects to be used as Python sequences.  All
:class:`NodeList` implementations must include support for
:meth:`~object.__len__` and
:meth:`~object.__getitem__`; this allows iteration over the :class:`NodeList` in
:keyword:`for` statements and proper support for the :func:`len` built-in
function.

If a DOM implementation supports modification of the document, the
:class:`NodeList` implementation must also support the
:meth:`~object.__setitem__` and :meth:`~object.__delitem__` methods.


.. _dom-documenttype-objects:

DocumentType Objects
^^^^^^^^^^^^^^^^^^^^

.. class:: DocumentType
   :no-typesetting:

Information about the notations and entities declared by a document (including
the external subset if the parser uses it and can provide the information) is
available from a :class:`DocumentType` object.  The :class:`DocumentType` for a
document is available from the :class:`Document` object's :attr:`~Document.doctype`
attribute; if there is no ``DOCTYPE`` declaration for the document, the
document's :attr:`~Document.doctype` attribute will be set to ``None`` instead of an
instance of this interface.

:class:`DocumentType` is a specialization of :class:`Node`, and adds the
following attributes:


.. attribute:: DocumentType.publicId

   The public identifier for the external subset of the document type definition,
   or ``None`` if the ``DOCTYPE`` declaration does not specify it.


.. attribute:: DocumentType.systemId

   The system identifier, a URI, for the external subset of the document type
   definition, or ``None`` if the ``DOCTYPE`` declaration does not specify it.


.. attribute:: DocumentType.internalSubset

   A string giving the complete internal subset from the document. This does not
   include the brackets which enclose the subset.  If the document has no internal
   subset, this should be ``None``.


.. attribute:: DocumentType.name

   The name of the root element as given in the ``DOCTYPE`` declaration, if
   present.


.. attribute:: DocumentType.entities

   This is a :class:`NamedNodeMap` of :class:`Entity` nodes
   giving the definitions of external entities.
   For entity names defined more than once, only the first definition is provided
   (others are ignored as required by the XML recommendation).  This may be
   ``None`` if the information is not provided by the parser, or if no entities are
   defined.


.. attribute:: DocumentType.notations

   This is a :class:`NamedNodeMap` of :class:`Notation` nodes
   giving the definitions of notations. For
   notation names defined more than once, only the first definition is provided
   (others are ignored as required by the XML recommendation).  This may be
   ``None`` if the information is not provided by the parser, or if no notations
   are defined.


.. _dom-document-objects:

Document Objects
^^^^^^^^^^^^^^^^

.. class:: Document
   :no-typesetting:

A :class:`Document` represents an entire XML document, including its constituent
elements, attributes, processing instructions, comments etc.  Remember that it
inherits properties from :class:`Node`.


.. attribute:: Document.documentElement

   The one and only root element of the document.


.. attribute:: Document.doctype

   The :class:`DocumentType` node of the document, or ``None``.
   This is a read-only attribute.


.. attribute:: Document.implementation

   The :class:`DOMImplementation` object which created this document.
   This is a read-only attribute.


.. attribute:: Document.strictErrorChecking

   Whether error checking is enforced.


.. attribute:: Document.documentURI

   The location of the document, or ``None`` if it is unknown.


.. method:: Document.createDocumentFragment()

   Create and return an empty :class:`DocumentFragment` node.


.. method:: Document.createCDATASection(data)

   Create and return a :class:`CDATASection` node containing *data*.


.. method:: Document.importNode(importedNode, deep)

   Return a copy of *importedNode* which belongs to this document.
   The original node is not removed from its document.
   If *deep* is true, the descendants of the node are copied too.


.. method:: Document.createElement(tagName)

   Create and return a new element node.  The element is not inserted into the
   document when it is created.  You need to explicitly insert it with one of the
   other methods such as :meth:`~Node.insertBefore` or :meth:`~Node.appendChild`.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Document.createElementNS(namespaceURI, tagName)

   Create and return a new element with a namespace.  The *tagName* may have a
   prefix.  The element is not inserted into the document when it is created.  You
   need to explicitly insert it with one of the other methods such as
   :meth:`~Node.insertBefore` or :meth:`~Node.appendChild`.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Document.createTextNode(data)

   Create and return a text node containing the data passed as a parameter.  As
   with the other creation methods, this one does not insert the node into the
   tree.


.. method:: Document.createEntityReference(name)

   Create and return a new entity reference node.
   The node is not inserted into the document when it is created.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.

   .. versionadded:: next


.. method:: Document.createComment(data)

   Create and return a comment node containing the data passed as a parameter.  As
   with the other creation methods, this one does not insert the node into the
   tree.


.. method:: Document.createProcessingInstruction(target, data)

   Create and return a processing instruction node containing the *target* and
   *data* passed as parameters.  As with the other creation methods, this one does
   not insert the node into the tree.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Document.createAttribute(name)

   Create and return an attribute node.  This method does not associate the
   attribute node with any particular element.  You must use
   :meth:`~Element.setAttributeNode` on the appropriate :class:`Element` object
   to use the newly created attribute instance.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Document.createAttributeNS(namespaceURI, qualifiedName)

   Create and return an attribute node with a namespace.  The *tagName* may have a
   prefix.  This method does not associate the attribute node with any particular
   element.  You must use :meth:`~Element.setAttributeNode` on the appropriate
   :class:`Element` object to use the newly created attribute instance.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Document.getElementById(id)

   Return the element with the given ID, or ``None``.
   Only attributes declared as being of type ID in the DTD
   or by :meth:`Element.setIdAttribute` are searched.


.. method:: Document.getElementsByTagName(tagName)

   Search for all descendants (direct children, children's children, etc.) with a
   particular element type name.


.. method:: Document.getElementsByTagNameNS(namespaceURI, localName)

   Search for all descendants (direct children, children's children, etc.) with a
   particular namespace URI and localname.  The localname is the part of the
   namespace after the prefix.


.. method:: Document.renameNode(n, namespaceURI, name)

   Rename the element or attribute node *n*
   and return it.
   *namespaceURI* is the new namespace URI, or
   :data:`~xml.dom.EMPTY_NAMESPACE` if the node does not belong to a namespace.
   *name* is the new qualified name.

   Raise :exc:`WrongDocumentErr` if *n* was created by another document,
   and :exc:`NotSupportedErr` if it is neither an element nor an attribute.


.. _dom-element-objects:

Element Objects
^^^^^^^^^^^^^^^

.. class:: Element
   :no-typesetting:

:class:`Element` is a subclass of :class:`Node`, so inherits all the attributes
of that class.


.. attribute:: Element.tagName

   The element type name.  In a namespace-using document it may have colons in it.
   The value is a string.


.. method:: Element.setIdAttribute(name)

   Declare that the attribute *name* is of type ID,
   so that the element is found by :meth:`Document.getElementById`.
   Raise :exc:`NotFoundErr` if the element has no such attribute.


.. method:: Element.setIdAttributeNS(namespaceURI, localName)

   The same as :meth:`~Element.setIdAttribute`,
   but for an attribute specified by its namespace URI and local name.


.. method:: Element.setIdAttributeNode(idAttr)

   The same as :meth:`~Element.setIdAttribute`,
   but for an already retrieved attribute node.


.. method:: Element.getElementsByTagName(tagName)

   Same as equivalent method in the :class:`Document` class.


.. method:: Element.getElementsByTagNameNS(namespaceURI, localName)

   Same as equivalent method in the :class:`Document` class.


.. method:: Element.hasAttribute(name)

   Return ``True`` if the element has an attribute named by *name*.


.. method:: Element.hasAttributeNS(namespaceURI, localName)

   Return ``True`` if the element has an attribute named by *namespaceURI* and
   *localName*.


.. method:: Element.getAttribute(name)

   Return the value of the attribute named by *name* as a string. If no such
   attribute exists, an empty string is returned, as if the attribute had no value.


.. method:: Element.getAttributeNode(attrname)

   Return the :class:`Attr` node for the attribute named by *attrname*.


.. method:: Element.getAttributeNS(namespaceURI, localName)

   Return the value of the attribute named by *namespaceURI* and *localName* as a
   string. If no such attribute exists, an empty string is returned, as if the
   attribute had no value.


.. method:: Element.getAttributeNodeNS(namespaceURI, localName)

   Return an attribute value as a node, given a *namespaceURI* and *localName*.


.. method:: Element.removeAttribute(name)

   Remove an attribute by name.


.. method:: Element.removeAttributeNode(oldAttr)

   Remove and return *oldAttr* from the attribute list, if present. If *oldAttr* is
   not present, :exc:`NotFoundErr` is raised.


.. method:: Element.removeAttributeNS(namespaceURI, localName)

   Remove an attribute by name.  Note that it uses a localName, not a qname.


.. method:: Element.setAttribute(name, value)

   Set an attribute value from a string.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. method:: Element.setAttributeNode(newAttr)

   Add a new attribute node to the element, replacing an existing attribute if
   necessary if the :attr:`~Attr.name` attribute matches.  If a replacement
   occurs, the old attribute node will be returned.  If *newAttr* is already in use,
   :exc:`InuseAttributeErr` will be raised.

   Raise :exc:`WrongDocumentErr` if *newAttr* was created by another document.


.. method:: Element.setAttributeNodeNS(newAttr)

   Add a new attribute node to the element, replacing an existing attribute if
   necessary if the :attr:`~Node.namespaceURI` and :attr:`~Attr.localName`
   attributes match.  If a replacement occurs, the old attribute node will be
   returned.  If *newAttr* is already in use, :exc:`InuseAttributeErr` will be
   raised.

   Raise :exc:`WrongDocumentErr` if *newAttr* was created by another document.


.. method:: Element.setAttributeNS(namespaceURI, qname, value)

   Set an attribute value from a string, given a *namespaceURI* and a *qname*.
   Note that a qname is the whole attribute name.  This is different than above.

   Raise :exc:`InvalidCharacterErr` if the name is not a valid XML name.


.. _dom-attr-objects:

Attr Objects
^^^^^^^^^^^^

.. class:: Attr
   :no-typesetting:

:class:`Attr` inherits from :class:`Node`, so inherits all its attributes.

Attribute nodes are not part of the document tree.
They are contained in the :attr:`~Node.attributes` map of an element,
not in its children,
and their :attr:`~Node.parentNode`, :attr:`~Node.previousSibling`
and :attr:`~Node.nextSibling` are always ``None``.


.. attribute:: Attr.name

   The attribute name.
   In a namespace-using document it may include a colon.


.. attribute:: Attr.localName

   The part of the name following the colon if there is one, else the
   entire name.
   This is a read-only attribute.


.. attribute:: Attr.prefix

   The part of the name preceding the colon if there is one, else the
   empty string.


.. attribute:: Attr.isId

   Whether this attribute is of type ID,
   either because it is declared as such in the DTD
   or because :meth:`Element.setIdAttribute` was used.
   This is a read-only attribute.


.. attribute:: Attr.ownerElement

   The :class:`Element` node to which this attribute belongs,
   or ``None`` if it is not used.
   This is a read-only attribute.


.. attribute:: Attr.specified

   Whether the value of the attribute was explicitly set in the document,
   as opposed to being defaulted from the DTD.
   This is a read-only attribute.


.. attribute:: Attr.value

   The text value of the attribute.  This is a synonym for the
   :attr:`~Node.nodeValue` attribute.


.. _dom-attributelist-objects:

NamedNodeMap Objects
^^^^^^^^^^^^^^^^^^^^

.. class:: NamedNodeMap
   :no-typesetting:

:class:`NamedNodeMap` does *not* inherit from :class:`Node`.


.. attribute:: NamedNodeMap.length

   The length of the attribute list.


.. method:: NamedNodeMap.item(index)

   Return an attribute with a particular index.  The order you get the attributes
   in is arbitrary but will be consistent for the life of a DOM.  Each item is an
   attribute node.  Get its value with the :attr:`~Attr.value` attribute.


.. method:: NamedNodeMap.getNamedItem(name)

   Return the node with the given :attr:`~Attr.name`,
   or ``None`` if there is no such node.


.. method:: NamedNodeMap.getNamedItemNS(namespaceURI, localName)

   Return the node with the given namespace URI and local name,
   or ``None`` if there is no such node.


.. method:: NamedNodeMap.setNamedItem(node)

   Add *node* to the map, using its :attr:`~Attr.name` as the key.
   Return the node which it replaces, or ``None`` if it replaces no node.

   Raise :exc:`WrongDocumentErr` if *node* was created by another document,
   and :exc:`InuseAttributeErr` if it belongs to another element.


.. method:: NamedNodeMap.setNamedItemNS(node)

   Add *node* to the map,
   using its namespace URI and local name as the key.
   Return the node which it replaces, or ``None`` if it replaces no node.

   Raise :exc:`WrongDocumentErr` if *node* was created by another document,
   and :exc:`InuseAttributeErr` if it belongs to another element.


.. method:: NamedNodeMap.removeNamedItem(name)

   Remove and return the node with the given :attr:`~Attr.name`.
   Raise :exc:`NotFoundErr` if there is no such node.


.. method:: NamedNodeMap.removeNamedItemNS(namespaceURI, localName)

   Remove and return the node with the given namespace URI and local name.
   Raise :exc:`NotFoundErr` if there is no such node.

You can also use the standardized :meth:`!getAttribute\*` family of methods
on the :class:`Element` objects.


.. _dom-documentfragment-objects:

DocumentFragment Objects
^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: DocumentFragment
   :no-typesetting:

:class:`DocumentFragment` is a lightweight container of nodes.
It is a subclass of :class:`Node`.
When it is inserted into the document tree,
its children are inserted instead of it,
and it becomes empty.


.. _dom-characterdata-objects:

CharacterData Objects
^^^^^^^^^^^^^^^^^^^^^

.. class:: CharacterData
   :no-typesetting:

:class:`CharacterData` represents text-like data in the XML document.
It is a subclass of :class:`Node`, and the base class
of :class:`Text`, :class:`CDATASection` and :class:`Comment`.
Such nodes cannot have child nodes.


.. attribute:: CharacterData.data

   The content of the node as a string.


.. attribute:: CharacterData.length

   The number of characters in :attr:`~CharacterData.data`.
   This is a read-only attribute.


.. method:: CharacterData.substringData(offset, count)

   Return the substring of :attr:`~CharacterData.data`
   of *count* characters starting at *offset*.


.. method:: CharacterData.appendData(arg)

   Append the string *arg* to :attr:`~CharacterData.data`.


.. method:: CharacterData.insertData(offset, arg)

   Insert the string *arg* into :attr:`~CharacterData.data` at *offset*.


.. method:: CharacterData.deleteData(offset, count)

   Remove *count* characters from :attr:`~CharacterData.data`
   starting at *offset*.


.. method:: CharacterData.replaceData(offset, count, arg)

   Replace *count* characters of :attr:`~CharacterData.data`
   starting at *offset* with the string *arg*.


.. _dom-comment-objects:

Comment Objects
^^^^^^^^^^^^^^^

.. class:: Comment
   :no-typesetting:

:class:`Comment` represents a comment in the XML document.
It is a subclass of :class:`CharacterData`.


.. attribute:: Comment.data

   The content of the comment as a string.  The attribute contains all characters
   between the leading ``<!-``\ ``-`` and trailing ``-``\ ``->``, but does not
   include them.


.. _dom-text-objects:

Text and CDATASection Objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: Text
   :no-typesetting:

.. class:: CDATASection
   :no-typesetting:

The :class:`Text` interface represents text in the XML document.  If the parser
and DOM implementation support the DOM's XML extension, portions of the text
enclosed in CDATA marked sections are stored in :class:`CDATASection` objects.
These two interfaces are identical, but provide different values for the
:attr:`~Node.nodeType` attribute.

:class:`Text` extends the :class:`CharacterData` interface,
and :class:`CDATASection` extends :class:`Text`.


.. attribute:: Text.data

   The content of the text node as a string.


.. attribute:: Text.wholeText

   The text of all :class:`Text` nodes logically adjacent to this node,
   concatenated in document order.
   This is a read-only attribute.


.. method:: Text.replaceWholeText(content)

   Replace the text of all :class:`Text` nodes logically adjacent
   to this node with *content*, removing the other nodes.
   Return this node, or ``None`` if *content* is empty.


.. method:: Text.splitText(offset)

   Split this node into two nodes at *offset*,
   keeping the first part in this node
   and returning a new sibling node with the rest.

.. note::

   The use of a :class:`CDATASection` node does not indicate that the node
   represents a complete CDATA marked section, only that the content of the node
   was part of a CDATA section.  A single CDATA section may be represented by more
   than one node in the document tree.  There is no way to determine whether two
   adjacent :class:`CDATASection` nodes represent different CDATA marked sections.


.. _dom-pi-objects:

ProcessingInstruction Objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: ProcessingInstruction
   :no-typesetting:

Represents a processing instruction in the XML document; this inherits from the
:class:`Node` interface and cannot have child nodes.


.. attribute:: ProcessingInstruction.target

   The content of the processing instruction up to the first whitespace character.
   This is a read-only attribute.


.. attribute:: ProcessingInstruction.data

   The content of the processing instruction following the first whitespace
   character.


.. _dom-entity-objects:

Entity Objects
^^^^^^^^^^^^^^

.. class:: Entity
   :no-typesetting:

:class:`Entity` represents a parsed or unparsed entity declared in the DTD.
It is a subclass of :class:`Node`.
Entity nodes are contained in :attr:`DocumentType.entities`
and cannot be inserted into the document tree.
The name of the entity is its :attr:`~Node.nodeName`.


.. attribute:: Entity.publicId

   The public identifier of the entity,
   or ``None`` if it is not specified.
   This is a read-only attribute.


.. attribute:: Entity.systemId

   The system identifier of the entity,
   or ``None`` if it is not specified.
   This is a read-only attribute.


.. attribute:: Entity.notationName

   The name of the notation for an unparsed entity,
   or ``None`` for a parsed entity.
   This is a read-only attribute.


.. _dom-notation-objects:

Notation Objects
^^^^^^^^^^^^^^^^

.. class:: Notation
   :no-typesetting:

:class:`Notation` represents a notation declared in the DTD.
It is a subclass of :class:`Node` and cannot have child nodes.
Notation nodes are contained in :attr:`DocumentType.notations`
and cannot be inserted into the document tree.
The name of the notation is its :attr:`~Node.nodeName`.


.. attribute:: Notation.publicId

   The public identifier of the notation,
   or ``None`` if it is not specified.
   This is a read-only attribute.


.. attribute:: Notation.systemId

   The system identifier of the notation,
   or ``None`` if it is not specified.
   This is a read-only attribute.
.. _dom-entityreference-objects:

EntityReference Objects
^^^^^^^^^^^^^^^^^^^^^^^

.. class:: EntityReference
   :no-typesetting:

:class:`EntityReference` represents an entity reference in the XML document.
It is a subclass of :class:`Node`.
The name of the referenced entity is its :attr:`~Node.nodeName`.
Its children are the replacement text of the entity, and are read-only.

Parsers may expand entity references,
so such a node only occurs in a document if it was created explicitly.

.. versionadded:: next


.. _dom-exceptions:

Exceptions
^^^^^^^^^^

The DOM Level 2 recommendation defines a single exception, :exc:`DOMException`,
and a number of constants that allow applications to determine what sort of
error occurred. :exc:`DOMException` instances carry a :attr:`code` attribute
that provides the appropriate value for the specific exception.

The Python DOM interface provides the constants, but also expands the set of
exceptions so that a specific exception exists for each of the exception codes
defined by the DOM.  The implementations must raise the appropriate specific
exception, each of which carries the appropriate value for the :attr:`code`
attribute.


.. exception:: DOMException

   Base exception class used for all specific DOM exceptions.  This exception class
   cannot be directly instantiated.


.. exception:: DomstringSizeErr

   Raised when a specified range of text does not fit into a string. This is not
   known to be used in the Python DOM implementations, but may be received from DOM
   implementations not written in Python.


.. exception:: HierarchyRequestErr

   Raised when an attempt is made to insert a node where the node type is not
   allowed.


.. exception:: IndexSizeErr

   Raised when an index or size parameter to a method is negative or exceeds the
   allowed values.


.. exception:: InuseAttributeErr

   Raised when an attempt is made to insert an :class:`Attr` node that is already
   present elsewhere in the document.


.. exception:: InvalidAccessErr

   Raised if a parameter or an operation is not supported on the underlying object.


.. exception:: InvalidCharacterErr

   This exception is raised when a string parameter contains a character that is
   not permitted in the context it's being used in by the XML 1.0 recommendation.
   For example, attempting to create an :class:`Element` node with a space in the
   element type name will cause this error to be raised.


.. exception:: InvalidModificationErr

   Raised when an attempt is made to modify the type of a node.


.. exception:: InvalidStateErr

   Raised when an attempt is made to use an object that is not defined or is no
   longer usable.


.. exception:: NamespaceErr

   If an attempt is made to change any object in a way that is not permitted with
   regard to the `Namespaces in XML <https://www.w3.org/TR/REC-xml-names/>`_
   recommendation, this exception is raised.


.. exception:: NotFoundErr

   Exception when a node does not exist in the referenced context.  For example,
   :meth:`NamedNodeMap.removeNamedItem` will raise this if the node passed in does
   not exist in the map.


.. exception:: NotSupportedErr

   Raised when the implementation does not support the requested type of object or
   operation.


.. exception:: NoDataAllowedErr

   This is raised if data is specified for a node which does not support data.

   .. XXX  a better explanation is needed!


.. exception:: NoModificationAllowedErr

   Raised on attempts to modify an object where modifications are not allowed (such
   as for read-only nodes).


.. exception:: SyntaxErr

   Raised when an invalid or illegal string is specified.

   .. XXX  how is this different from InvalidCharacterErr?


.. exception:: ValidationErr

   Raised when an operation would make the document invalid
   with respect to partial validity.
   This is not known to be used in the Python DOM implementations,
   but may be received from DOM implementations not written in Python.


.. exception:: WrongDocumentErr

   Raised when a node is inserted in a different document than it currently belongs
   to, and the implementation does not support migrating the node from one document
   to the other.


The exception codes defined in the DOM recommendation map to the exceptions
described above according to this table:

+---------------------------------------+---------------------------------+
| Constant                              | Exception                       |
+=======================================+=================================+
| .. data:: DOMSTRING_SIZE_ERR          | :exc:`DomstringSizeErr`         |
+---------------------------------------+---------------------------------+
| .. data:: HIERARCHY_REQUEST_ERR       | :exc:`HierarchyRequestErr`      |
+---------------------------------------+---------------------------------+
| .. data:: INDEX_SIZE_ERR              | :exc:`IndexSizeErr`             |
+---------------------------------------+---------------------------------+
| .. data:: INUSE_ATTRIBUTE_ERR         | :exc:`InuseAttributeErr`        |
+---------------------------------------+---------------------------------+
| .. data:: INVALID_ACCESS_ERR          | :exc:`InvalidAccessErr`         |
+---------------------------------------+---------------------------------+
| .. data:: INVALID_CHARACTER_ERR       | :exc:`InvalidCharacterErr`      |
+---------------------------------------+---------------------------------+
| .. data:: INVALID_MODIFICATION_ERR    | :exc:`InvalidModificationErr`   |
+---------------------------------------+---------------------------------+
| .. data:: INVALID_STATE_ERR           | :exc:`InvalidStateErr`          |
+---------------------------------------+---------------------------------+
| .. data:: NAMESPACE_ERR               | :exc:`NamespaceErr`             |
+---------------------------------------+---------------------------------+
| .. data:: NOT_FOUND_ERR               | :exc:`NotFoundErr`              |
+---------------------------------------+---------------------------------+
| .. data:: NOT_SUPPORTED_ERR           | :exc:`NotSupportedErr`          |
+---------------------------------------+---------------------------------+
| .. data:: NO_DATA_ALLOWED_ERR         | :exc:`NoDataAllowedErr`         |
+---------------------------------------+---------------------------------+
| .. data:: NO_MODIFICATION_ALLOWED_ERR | :exc:`NoModificationAllowedErr` |
+---------------------------------------+---------------------------------+
| .. data:: SYNTAX_ERR                  | :exc:`SyntaxErr`                |
+---------------------------------------+---------------------------------+
| .. data:: VALIDATION_ERR              | :exc:`ValidationErr`            |
+---------------------------------------+---------------------------------+
| .. data:: WRONG_DOCUMENT_ERR          | :exc:`WrongDocumentErr`         |
+---------------------------------------+---------------------------------+


.. _dom-conformance:

Conformance
-----------

This section describes the conformance requirements and relationships between
the Python DOM API, the W3C DOM recommendations, and the OMG IDL mapping for
Python.


.. _dom-type-mapping:

Type Mapping
^^^^^^^^^^^^

The IDL types used in the DOM specification are mapped to Python types
according to the following table.

+------------------+-------------------------------------------+
| IDL Type         | Python Type                               |
+==================+===========================================+
| ``boolean``      | ``bool`` or ``int``                       |
+------------------+-------------------------------------------+
| ``int``          | ``int``                                   |
+------------------+-------------------------------------------+
| ``long int``     | ``int``                                   |
+------------------+-------------------------------------------+
| ``unsigned int`` | ``int``                                   |
+------------------+-------------------------------------------+
| ``DOMString``    | ``str`` or ``bytes``                      |
+------------------+-------------------------------------------+
| ``null``         | ``None``                                  |
+------------------+-------------------------------------------+

.. _dom-accessor-methods:

Accessor Methods
^^^^^^^^^^^^^^^^

The mapping from OMG IDL to Python defines accessor functions for IDL
``attribute`` declarations in much the way the Java mapping does.
Mapping the IDL declarations ::

   readonly attribute string someValue;
            attribute string anotherValue;

yields three accessor functions:  a "get" method for :attr:`!someValue`
(:meth:`!_get_someValue`), and "get" and "set" methods for :attr:`!anotherValue`
(:meth:`!_get_anotherValue` and :meth:`!_set_anotherValue`).  The mapping, in
particular, does not require that the IDL attributes are accessible as normal
Python attributes:  ``object.someValue`` is *not* required to work, and may
raise an :exc:`AttributeError`.

The Python DOM API, however, *does* require that normal attribute access work.
This means that the typical surrogates generated by Python IDL compilers are not
likely to work, and wrapper objects may be needed on the client if the DOM
objects are accessed via CORBA. While this does require some additional
consideration for CORBA DOM clients, the implementers with experience using DOM
over CORBA from Python do not consider this a problem.  Attributes that are
declared ``readonly`` may not restrict write access in all DOM
implementations.

In the Python DOM API, accessor functions are not required.  If provided, they
should take the form defined by the Python IDL mapping, but these methods are
considered unnecessary since the attributes are accessible directly from Python.
"Set" accessors should never be provided for ``readonly`` attributes.

The IDL definitions do not fully embody the requirements of the W3C DOM API,
such as the notion of certain objects, such as the return value of
:meth:`~Element.getElementsByTagName`, being "live".  The Python DOM API does
not require implementations to enforce such requirements.

