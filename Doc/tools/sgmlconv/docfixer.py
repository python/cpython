#! /usr/bin/env python

"""Promote the IDs from <label/> elements to the enclosing section / chapter /
whatever, then remove the <label/> elements.  This allows *ML style internal
linking rather than the bogus LaTeX model.

Note that <label/>s in <title> elements are promoted two steps, since the
<title> elements are artificially created from the section parameter, and the
label really refers to the sectioning construct.
"""
__version__ = '$Revision$'


import errno
import string
import sys
import xml.dom.core
import xml.dom.esis_builder


# Workaround to deal with invalid documents (multiple root elements).  This
# does not indicate a bug in the DOM implementation.
#
def get_documentElement(self):
    docelem = None
    for n in self._node.children:
        if n.type == xml.dom.core.ELEMENT:
            docelem = xml.dom.core.Element(n, self, self)
    return docelem

xml.dom.core.Document.get_documentElement = get_documentElement


# Replace get_childNodes for the Document class; without this, children
# accessed from the Document object via .childNodes (no matter how many
# levels of access are used) will be given an ownerDocument of None.
#
def get_childNodes(self):
    return xml.dom.core.NodeList(self._node.children, self, self)

xml.dom.core.Document.get_childNodes = get_childNodes


def get_first_element(doc, gi):
    for n in doc.childNodes:
        if n.nodeType == xml.dom.core.ELEMENT and n.tagName == gi:
            return n

def extract_first_element(doc, gi):
    node = get_first_element(doc, gi)
    if node is not None:
        doc.removeChild(node)
    return node


def simplify(doc):
    # Try to rationalize the document a bit, since these things are simply
    # not valid SGML/XML documents as they stand, and need a little work.
    documentclass = "document"
    inputs = []
    node = extract_first_element(doc, "documentclass")
    if node is not None:
        documentclass = node.getAttribute("classname")
    node = extract_first_element(doc, "title")
    if node is not None:
        inputs.append(node)
    # update the name of the root element
    node = get_first_element(doc, "document")
    if node is not None:
        node._node.name = documentclass
    while 1:
        node = extract_first_element(doc, "input")
        if node is None:
            break
        inputs.append(node)
    if inputs:
        docelem = doc.documentElement
        inputs.reverse()
        for node in inputs:
            text = doc.createTextNode("\n")
            docelem.insertBefore(text, docelem.firstChild)
            docelem.insertBefore(node, text)
        docelem.insertBefore(doc.createTextNode("\n"), docelem.firstChild)
    while doc.firstChild.nodeType == xml.dom.core.TEXT:
        doc.removeChild(doc.firstChild)


def cleanup_root_text(doc):
    discards = []
    skip = 0
    for n in doc.childNodes:
        prevskip = skip
        skip = 0
        if n.nodeType == xml.dom.core.TEXT and not prevskip:
            discards.append(n)
        elif n.nodeType == xml.dom.core.COMMENT:
            skip = 1
    for node in discards:
        doc.removeChild(node)


def rewrite_desc_entries(doc, argname_gi):
    argnodes = doc.getElementsByTagName(argname_gi)
    for node in argnodes:
        parent = node.parentNode
        nodes = []
        for n in parent.childNodes:
            if n.nodeType != xml.dom.core.ELEMENT or n.tagName != argname_gi:
                nodes.append(n)
        desc = doc.createElement("description")
        for n in nodes:
            parent.removeChild(n)
            desc.appendChild(n)
        if node.childNodes:
            # keep the <args>...</args>, newline & indent
            parent.insertBefore(doc.createText("\n  "), node)
        else:
            # no arguments, remove the <args/> node
            parent.removeChild(node)
        parent.appendChild(doc.createText("\n  "))
        parent.appendChild(desc)
        parent.appendChild(doc.createText("\n"))

def handle_args(doc):
    rewrite_desc_entries(doc, "args")
    rewrite_desc_entries(doc, "constructor-args")


def handle_comments(doc, node=None):
    if node is None:
        node = doc
    for n in node.childNodes:
        if n.nodeType == xml.dom.core.ELEMENT:
            if n.tagName == "COMMENT":
                comment = doc.createComment(n.childNodes[0].data)
                node.replaceChild(comment, n)
            else:
                handle_comments(doc, n)


def handle_labels(doc):
    labels = doc.getElementsByTagName("label")
    for label in labels:
        id = label.getAttribute("id")
        if not id:
            continue
        parent = label.parentNode
        if parent.tagName == "title":
            parent.parentNode.setAttribute("id", id)
        else:
            parent.setAttribute("id", id)
        # now, remove <label id="..."/> from parent:
        parent.removeChild(label)


def convert(ifp, ofp):
    p = xml.dom.esis_builder.EsisBuilder()
    p.feed(ifp.read())
    doc = p.document
    handle_args(doc)
    handle_comments(doc)
    simplify(doc)
    handle_labels(doc)
    cleanup_root_text(doc)
    try:
        ofp.write(doc.toxml())
        ofp.write("\n")
    except IOError, (err, msg):
        # Ignore EPIPE; it just means that whoever we're writing to stopped
        # reading.  The rest of the output would be ignored.  All other errors
        # should still be reported,
        if err != errno.EPIPE:
            raise


def main():
    if len(sys.argv) == 1:
        ifp = sys.stdin
        ofp = sys.stdout
    elif len(sys.argv) == 2:
        ifp = open(sys.argv[1])
        ofp = sys.stdout
    elif len(sys.argv) == 3:
        ifp = open(sys.argv[1])
        ofp = open(sys.argv[2], "w")
    else:
        usage()
        sys.exit(2)
    convert(ifp, ofp)


if __name__ == "__main__":
    main()
