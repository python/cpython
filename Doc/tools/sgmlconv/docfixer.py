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
import esistools
import re
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
        elif n.nodeType == xml.dom.core.ELEMENT and n.tagName == "COMMENT":
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


def handle_appendix(doc):
    # must be called after simplfy() if document is multi-rooted to begin with
    docelem = doc.documentElement
    toplevel = docelem.tagName == "manual" and "chapter" or "section"
    appendices = 0
    nodes = []
    for node in docelem.childNodes:
        if appendices:
            nodes.append(node)
        elif node.nodeType == xml.dom.core.ELEMENT:
            appnodes = node.getElementsByTagName("appendix")
            if appnodes:
                appendices = 1
                parent = appnodes[0].parentNode
                parent.removeChild(appnodes[0])
                parent.normalize()
    if nodes:
        map(docelem.removeChild, nodes)
        docelem.appendChild(doc.createTextNode("\n\n\n"))
        back = doc.createElement("back-matter")
        docelem.appendChild(back)
        back.appendChild(doc.createTextNode("\n"))
        while nodes and nodes[0].nodeType == xml.dom.core.TEXT \
              and not string.strip(nodes[0].data):
            del nodes[0]
        map(back.appendChild, nodes)
        docelem.appendChild(doc.createTextNode("\n"))


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


def fixup_trailing_whitespace(doc, wsmap):
    queue = [doc]
    while queue:
        node = queue[0]
        del queue[0]
        if node.nodeType == xml.dom.core.ELEMENT \
           and wsmap.has_key(node.tagName):
            ws = wsmap[node.tagName]
            children = node.childNodes
            children.reverse()
            if children[0].nodeType == xml.dom.core.TEXT:
                data = string.rstrip(children[0].data) + ws
                children[0].data = data
            children.reverse()
            # hack to get the title in place:
            if node.tagName == "title" \
               and node.parentNode.firstChild.nodeType == xml.dom.core.ELEMENT:
                node.parentNode.insertBefore(doc.createText("\n  "),
                                             node.parentNode.firstChild)
        for child in node.childNodes:
            if child.nodeType == xml.dom.core.ELEMENT:
                queue.append(child)


def normalize(doc):
    for node in doc.childNodes:
        if node.nodeType == xml.dom.core.ELEMENT:
            node.normalize()


def cleanup_trailing_parens(doc, element_names):
    d = {}
    for gi in element_names:
        d[gi] = gi
    rewrite_element = d.has_key
    queue = []
    for node in doc.childNodes:
        if node.nodeType == xml.dom.core.ELEMENT:
            queue.append(node)
    while queue:
        node = queue[0]
        del queue[0]
        if rewrite_element(node.tagName):
            children = node.childNodes
            if len(children) == 1 \
               and children[0].nodeType == xml.dom.core.TEXT:
                data = children[0].data
                if data[-2:] == "()":
                    children[0].data = data[:-2]
        else:
            for child in node.childNodes:
                if child.nodeType == xml.dom.core.ELEMENT:
                    queue.append(child)


def contents_match(left, right):
    left_children = left.childNodes
    right_children = right.childNodes
    if len(left_children) != len(right_children):
        return 0
    for l, r in map(None, left_children, right_children):
        nodeType = l.nodeType
        if nodeType != r.nodeType:
            return 0
        if nodeType == xml.dom.core.ELEMENT:
            if l.tagName != r.tagName:
                return 0
            # should check attributes, but that's not a problem here
            if not contents_match(l, r):
                return 0
        elif nodeType == xml.dom.core.TEXT:
            if l.data != r.data:
                return 0
        else:
            # not quite right, but good enough
            return 0
    return 1


def create_module_info(doc, section):
    # Heavy.
    node = extract_first_element(section, "modulesynopsis")
    if node is None:
        return
    node._node.name = "synopsis"
    lastchild = node.childNodes[-1]
    if lastchild.nodeType == xml.dom.core.TEXT \
       and lastchild.data[-1:] == ".":
        lastchild.data = lastchild.data[:-1]
    if section.tagName == "section":
        modinfo_pos = 2
        modinfo = doc.createElement("moduleinfo")
        moddecl = extract_first_element(section, "declaremodule")
        name = None
        if moddecl:
            modinfo.appendChild(doc.createTextNode("\n    "))
            name = moddecl.attributes["name"].value
            namenode = doc.createElement("name")
            namenode.appendChild(doc.createTextNode(name))
            modinfo.appendChild(namenode)
            type = moddecl.attributes.get("type")
            if type:
                type = type.value
                modinfo.appendChild(doc.createTextNode("\n    "))
                typenode = doc.createElement("type")
                typenode.appendChild(doc.createTextNode(type))
                modinfo.appendChild(typenode)
        title = get_first_element(section, "title")
        if title:
            children = title.childNodes
            if len(children) >= 2 \
               and children[0].nodeType == xml.dom.core.ELEMENT \
               and children[0].tagName == "module" \
               and children[0].childNodes[0].data == name:
                # this is it; morph the <title> into <short-synopsis>
                first_data = children[1]
                if first_data.data[:4] == " ---":
                    first_data.data = string.lstrip(first_data.data[4:])
                title._node.name = "short-synopsis"
                if children[-1].data[-1:] == ".":
                    children[-1].data = children[-1].data[:-1]
                section.removeChild(title)
                section.removeChild(section.childNodes[0])
                title.removeChild(children[0])
                modinfo_pos = 0
            else:
                sys.stderr.write(
                    "module name in title doesn't match"
                    " <declaremodule>; no <short-synopsis>\n")
        else:
            sys.stderr.write(
                "Unexpected condition: <section> without <title>\n")
        modinfo.appendChild(doc.createTextNode("\n    "))
        modinfo.appendChild(node)
        if title and not contents_match(title, node):
            # The short synopsis is actually different,
            # and needs to be stored:
            modinfo.appendChild(doc.createTextNode("\n    "))
            modinfo.appendChild(title)
        modinfo.appendChild(doc.createTextNode("\n  "))
        section.insertBefore(modinfo, section.childNodes[modinfo_pos])
        section.insertBefore(doc.createTextNode("\n  "), modinfo)


def cleanup_synopses(doc):
    for node in doc.childNodes:
        if node.nodeType == xml.dom.core.ELEMENT \
           and node.tagName == "section":
            create_module_info(doc, node)


def fixup_paras(doc):
    pass


_token_rx = re.compile(r"[a-zA-Z][a-zA-Z0-9.-]*$")
  
def write_esis(doc, ofp, knownempty):
    for node in doc.childNodes:
        nodeType = node.nodeType
        if nodeType == xml.dom.core.ELEMENT:
            gi = node.tagName
            if knownempty(gi):
                if node.hasChildNodes():
                    raise ValueError, "declared-empty node has children"
                ofp.write("e\n")
            for k, v in node.attributes.items():
                value = v.value
                if _token_rx.match(value):
                    dtype = "TOKEN"
                else:
                    dtype = "CDATA"
                ofp.write("A%s %s %s\n" % (k, dtype, esistools.encode(value)))
            ofp.write("(%s\n" % gi)
            write_esis(node, ofp, knownempty)
            ofp.write(")%s\n" % gi)
        elif nodeType == xml.dom.core.TEXT:
            ofp.write("-%s\n" % esistools.encode(node.data))
        else:
            raise RuntimeError, "unsupported node type: %s" % nodeType


def convert(ifp, ofp):
    p = esistools.ExtendedEsisBuilder()
    p.feed(ifp.read())
    doc = p.document
    normalize(doc)
    handle_args(doc)
    simplify(doc)
    handle_labels(doc)
    handle_appendix(doc)
    fixup_trailing_whitespace(doc, {
        "abstract": "\n",
        "title": "",
        "chapter": "\n\n",
        "section": "\n\n",
        "subsection": "\n\n",
        "subsubsection": "\n\n",
        "paragraph": "\n\n",
        "subparagraph": "\n\n",
        })
    cleanup_root_text(doc)
    cleanup_trailing_parens(doc, ["function", "method", "cfunction"])
    cleanup_synopses(doc)
    normalize(doc)
    fixup_paras(doc)
    #
    d = {}
    for gi in p.get_empties():
        d[gi] = gi
    knownempty = d.has_key
    #
    try:
        write_esis(doc, ofp, knownempty)
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
