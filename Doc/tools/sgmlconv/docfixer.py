#! /usr/bin/env python

"""Perform massive transformations on a document tree created from the LaTeX
of the Python documentation, and dump the ESIS data for the transformed tree.
"""
__version__ = '$Revision$'


import errno
import esistools
import re
import string
import sys
import xml.dom.core
import xml.dom.esis_builder


class ConversionError(Exception):
    pass


DEBUG_PARA_FIXER = 0

if DEBUG_PARA_FIXER:
    def para_msg(s):
        sys.stderr.write("*** %s\n" % s)
else:
    def para_msg(s):
        pass


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


def find_all_elements(doc, gi):
    nodes = []
    if doc.nodeType == xml.dom.core.ELEMENT and doc.tagName == gi:
        nodes.append(doc)
    for child in doc.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT:
            if child.tagName == gi:
                nodes.append(child)
            for node in child.getElementsByTagName(gi):
                nodes.append(node)
    return nodes        


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


DESCRIPTOR_ELEMENTS = (
    "cfuncdesc", "cvardesc", "ctypedesc",
    "classdesc", "memberdesc", "memberdescni", "methoddesc", "methoddescni",
    "excdesc", "funcdesc", "funcdescni", "opcodedesc",
    "datadesc", "datadescni",
    )

def fixup_descriptors(doc):
    sections = find_all_elements(doc, "section")
    for section in sections:
        find_and_fix_descriptors(doc, section)


def find_and_fix_descriptors(doc, container):
    children = container.childNodes
    for child in children:
        if child.nodeType == xml.dom.core.ELEMENT:
            tagName = child.tagName
            if tagName in DESCRIPTOR_ELEMENTS:
                rewrite_descriptor(doc, child)
            elif tagName == "subsection":
                find_and_fix_descriptors(doc, child)


def rewrite_descriptor(doc, descriptor):
    #
    # Do these things:
    #   1. Add an "index=noindex" attribute to the element if the tagName
    #      ends in 'ni', removing the 'ni' from the name.
    #   2. Create a <signature> from the name attribute and <args>.
    #   3. Create additional <signature>s from <*line{,ni}> elements,
    #      if found.
    #   4. If a <versionadded> is found, move it to an attribute on the
    #      descriptor.
    #   5. Move remaining child nodes to a <description> element.
    #   6. Put it back together.
    #
    descname = descriptor.tagName
    index = 1
    if descname[-2:] == "ni":
        descname = descname[:-2]
        descriptor.setAttribute("index", "noindex")
        descriptor._node.name = descname
        index = 0
    desctype = descname[:-4] # remove 'desc'
    linename = desctype + "line"
    if not index:
        linename = linename + "ni"
    # 2.
    signature = doc.createElement("signature")
    name = doc.createElement("name")
    signature.appendChild(doc.createTextNode("\n    "))
    signature.appendChild(name)
    name.appendChild(doc.createTextNode(descriptor.getAttribute("name")))
    descriptor.removeAttribute("name")
    if descriptor.attributes.has_key("var"):
        variable = descriptor.getAttribute("var")
        if variable:
            args = doc.createElement("args")
            args.appendChild(doc.createTextNode(variable))
            signature.appendChild(doc.createTextNode("\n    "))
            signature.appendChild(args)
        descriptor.removeAttribute("var")
    newchildren = [signature]
    children = descriptor.childNodes
    pos = skip_leading_nodes(children, 0)
    if pos < len(children):
        child = children[pos]
        if child.nodeType == xml.dom.core.ELEMENT and child.tagName == "args":
            # create an <args> in <signature>:
            args = doc.createElement("args")
            argchildren = []
            map(argchildren.append, child.childNodes)
            for n in argchildren:
                child.removeChild(n)
                args.appendChild(n)
            signature.appendChild(doc.createTextNode("\n    "))
            signature.appendChild(args)
    signature.appendChild(doc.createTextNode("\n  "))
    # 3, 4.
    pos = skip_leading_nodes(children, pos + 1)
    while pos < len(children) \
          and children[pos].nodeType == xml.dom.core.ELEMENT \
          and children[pos].tagName in (linename, "versionadded"):
        if children[pos].tagName == linename:
            # this is really a supplemental signature, create <signature>
            sig = methodline_to_signature(doc, children[pos])
            newchildren.append(sig)
        else:
            # <versionadded added=...>
            descriptor.setAttribute(
                "added", children[pos].getAttribute("version"))
        pos = skip_leading_nodes(children, pos + 1)
    # 5.
    description = doc.createElement("description")
    description.appendChild(doc.createTextNode("\n"))
    newchildren.append(description)
    move_children(descriptor, description, pos)
    last = description.childNodes[-1]
    if last.nodeType == xml.dom.core.TEXT:
        last.data = string.rstrip(last.data) + "\n  "
    # 6.
    # should have nothing but whitespace and signature lines in <descriptor>;
    # discard them
    while descriptor.childNodes:
        descriptor.removeChild(descriptor.childNodes[0])
    for node in newchildren:
        descriptor.appendChild(doc.createTextNode("\n  "))
        descriptor.appendChild(node)
    descriptor.appendChild(doc.createTextNode("\n"))


def methodline_to_signature(doc, methodline):
    signature = doc.createElement("signature")
    signature.appendChild(doc.createTextNode("\n    "))
    name = doc.createElement("name")
    name.appendChild(doc.createTextNode(methodline.getAttribute("name")))
    methodline.removeAttribute("name")
    signature.appendChild(name)
    if len(methodline.childNodes):
        args = doc.createElement("args")
        signature.appendChild(doc.createTextNode("\n    "))
        signature.appendChild(args)
        move_children(methodline, args)
    signature.appendChild(doc.createTextNode("\n  "))
    return signature


def move_children(origin, dest, start=0):
    children = origin.childNodes
    while start < len(children):
        node = children[start]
        origin.removeChild(node)
        dest.appendChild(node)


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
    for label in find_all_elements(doc, "label"):
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
    modauthor = extract_first_element(section, "moduleauthor")
    if modauthor:
        modauthor._node.name = "author"
        modauthor.appendChild(doc.createTextNode(
            modauthor.getAttribute("name")))
        modauthor.removeAttribute("name")
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
        versionadded = extract_first_element(section, "versionadded")
        if versionadded:
            modinfo.setAttribute("added", versionadded.getAttribute("version"))
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
                if children[-1].nodeType == xml.dom.core.TEXT \
                   and children[-1].data[-1:] == ".":
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
        if modauthor:
            modinfo.appendChild(doc.createTextNode("\n    "))
            modinfo.appendChild(modauthor)
        modinfo.appendChild(doc.createTextNode("\n  "))
        section.insertBefore(modinfo, section.childNodes[modinfo_pos])
        section.insertBefore(doc.createTextNode("\n  "), modinfo)


def cleanup_synopses(doc):
    for node in find_all_elements(doc, "section"):
        create_module_info(doc, node)


def remap_element_names(root, name_map):
    queue = []
    for child in root.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT:
            queue.append(child)
    while queue:
        node = queue.pop()
        tagName = node.tagName
        if name_map.has_key(tagName):
            name, attrs = name_map[tagName]
            node._node.name = name
            for attr, value in attrs.items():
                node.setAttribute(attr, value)
        for child in node.childNodes:
            if child.nodeType == xml.dom.core.ELEMENT:
                queue.append(child)


def fixup_table_structures(doc):
    # must be done after remap_element_names(), or the tables won't be found
    for table in find_all_elements(doc, "table"):
        fixup_table(doc, table)


def fixup_table(doc, table):
    # create the table head
    thead = doc.createElement("thead")
    row = doc.createElement("row")
    move_elements_by_name(doc, table, row, "entry")
    thead.appendChild(doc.createTextNode("\n    "))
    thead.appendChild(row)
    thead.appendChild(doc.createTextNode("\n    "))
    # create the table body
    tbody = doc.createElement("tbody")
    prev_row = None
    last_was_hline = 0
    children = table.childNodes
    for child in children:
        if child.nodeType == xml.dom.core.ELEMENT:
            tagName = child.tagName
            if tagName == "hline" and prev_row is not None:
                prev_row.setAttribute("rowsep", "1")
            elif tagName == "row":
                prev_row = child
    # save the rows:
    tbody.appendChild(doc.createTextNode("\n    "))
    move_elements_by_name(doc, table, tbody, "row", sep="\n    ")
    # and toss the rest:
    while children:
        child = children[0]
        nodeType = child.nodeType
        if nodeType == xml.dom.core.TEXT:
            if string.strip(child.data):
                raise ConversionError("unexpected free data in table")
            table.removeChild(child)
            continue
        if nodeType == xml.dom.core.ELEMENT:
            if child.tagName != "hline":
                raise ConversionError(
                    "unexpected <%s> in table" % child.tagName)
            table.removeChild(child)
            continue
        raise ConversionError(
            "unexpected %s node in table" % child.__class__.__name__)
    # nothing left in the <table>; add the <thead> and <tbody>
    tgroup = doc.createElement("tgroup")
    tgroup.appendChild(doc.createTextNode("\n  "))
    tgroup.appendChild(thead)
    tgroup.appendChild(doc.createTextNode("\n  "))
    tgroup.appendChild(tbody)
    tgroup.appendChild(doc.createTextNode("\n  "))
    table.appendChild(tgroup)
    # now make the <entry>s look nice:
    for row in table.getElementsByTagName("row"):
        fixup_row(doc, row)


def fixup_row(doc, row):
    entries = []
    map(entries.append, row.childNodes[1:])
    for entry in entries:
        row.insertBefore(doc.createTextNode("\n         "), entry)
#    row.appendChild(doc.createTextNode("\n      "))


def move_elements_by_name(doc, source, dest, name, sep=None):
    nodes = []
    for child in source.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT and child.tagName == name:
            nodes.append(child)
    for node in nodes:
        source.removeChild(node)
        dest.appendChild(node)
        if sep:
            dest.appendChild(doc.createTextNode(sep))


RECURSE_INTO_PARA_CONTAINERS = (
    "chapter", "abstract", "enumerate",
    "section", "subsection", "subsubsection",
    "paragraph", "subparagraph",
    "howto", "manual",
    )

PARA_LEVEL_ELEMENTS = (
    "moduleinfo", "title", "verbatim", "enumerate", "item",
    "opcodedesc", "classdesc", "datadesc",
    "funcdesc", "methoddesc", "excdesc",
    "funcdescni", "methoddescni", "excdescni",
    "tableii", "tableiii", "tableiv", "localmoduletable",
    "sectionauthor", "seealso",
    # include <para>, so we can just do it again to get subsequent paras:
    "para",
    )

PARA_LEVEL_PRECEEDERS = (
    "index", "indexii", "indexiii", "indexiv", "setindexsubitem",
    "stindex", "obindex", "COMMENT", "label", "input", "title",
    )


def fixup_paras(doc):
    for child in doc.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT \
           and child.tagName in RECURSE_INTO_PARA_CONTAINERS:
            #
            fixup_paras_helper(doc, child)
    descriptions = find_all_elements(doc, "description")
    for description in descriptions:
        fixup_paras_helper(doc, description)


def fixup_paras_helper(doc, container, depth=0):
    # document is already normalized
    children = container.childNodes
    start = 0
    while len(children) > start:
        start = skip_leading_nodes(children, start)
        if start >= len(children):
            break
        #
        # Either paragraph material or something to recurse into:
        #
        if (children[start].nodeType == xml.dom.core.ELEMENT) \
           and (children[start].tagName in RECURSE_INTO_PARA_CONTAINERS):
            fixup_paras_helper(doc, children[start])
            start = skip_leading_nodes(children, start + 1)
            continue
        #
        # paragraph material:
        #
        build_para(doc, container, start, len(children))
        if DEBUG_PARA_FIXER and depth == 10:
            sys.exit(1)
        start = start + 1


def build_para(doc, parent, start, i):
    children = parent.childNodes
    after = start + 1
    have_last = 0
    BREAK_ELEMENTS = PARA_LEVEL_ELEMENTS + RECURSE_INTO_PARA_CONTAINERS
    # Collect all children until \n\n+ is found in a text node or a
    # member of BREAK_ELEMENTS is found.
    for j in range(start, i):
        after = j + 1
        child = children[j]
        nodeType = child.nodeType
        if nodeType == xml.dom.core.ELEMENT:
            if child.tagName in BREAK_ELEMENTS:
                after = j
                break
        elif nodeType == xml.dom.core.TEXT:
            pos = string.find(child.data, "\n\n")
            if pos == 0:
                after = j
                break
            if pos >= 1:
                child.splitText(pos)
                break
    else:
        have_last = 1
    if (start + 1) > after:
        raise ConversionError(
            "build_para() could not identify content to turn into a paragraph")
    if children[after - 1].nodeType == xml.dom.core.TEXT:
        # we may need to split off trailing white space:
        child = children[after - 1]
        data = child.data
        if string.rstrip(data) != data:
            have_last = 0
            child.splitText(len(string.rstrip(data)))
    para = doc.createElement("para")
    prev = None
    indexes = range(start, after)
    indexes.reverse()
    for j in indexes:
        node = parent.childNodes[j]
        parent.removeChild(node)
        para.insertBefore(node, prev)
        prev = node
    if have_last:
        parent.appendChild(para)
        return len(parent.childNodes)
    else:
        parent.insertBefore(para, parent.childNodes[start])
        return start + 1


def skip_leading_nodes(children, start):
    """Return index into children of a node at which paragraph building should
    begin or a recursive call to fixup_paras_helper() should be made (for
    subsections, etc.).

    When the return value >= len(children), we've built all the paras we can
    from this list of children.
    """
    i = len(children)
    while i > start:
        # skip over leading comments and whitespace:
        child = children[start]
        nodeType = child.nodeType
        if nodeType == xml.dom.core.TEXT:
            data = child.data
            shortened = string.lstrip(data)
            if shortened:
                if data != shortened:
                    # break into two nodes: whitespace and non-whitespace
                    child.splitText(len(data) - len(shortened))
                    return start + 1
                return start
            # all whitespace, just skip
        elif nodeType == xml.dom.core.ELEMENT:
            tagName = child.tagName
            if tagName in RECURSE_INTO_PARA_CONTAINERS:
                return start
            if tagName not in PARA_LEVEL_ELEMENTS + PARA_LEVEL_PRECEEDERS:
                return start
        start = start + 1
    return start


def fixup_rfc_references(doc):
    for rfcnode in find_all_elements(doc, "rfc"):
        rfcnode.appendChild(doc.createTextNode(
            "RFC " + rfcnode.getAttribute("num")))


def fixup_signatures(doc):
    for child in doc.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT:
            args = child.getElementsByTagName("args")
            for arg in args:
                fixup_args(doc, arg)
                arg.normalize()
            args = child.getElementsByTagName("constructor-args")
            for arg in args:
                fixup_args(doc, arg)
                arg.normalize()


def fixup_args(doc, arglist):
    for child in arglist.childNodes:
        if child.nodeType == xml.dom.core.ELEMENT \
           and child.tagName == "optional":
            # found it; fix and return
            arglist.insertBefore(doc.createTextNode("["), child)
            optkids = child.childNodes
            while optkids:
                k = optkids[0]
                child.removeChild(k)
                arglist.insertBefore(k, child)
            arglist.insertBefore(doc.createTextNode("]"), child)
            arglist.removeChild(child)
            return fixup_args(doc, arglist)


def fixup_sectionauthors(doc):
    for sectauth in find_all_elements(doc, "sectionauthor"):
        section = sectauth.parentNode
        section.removeChild(sectauth)
        sectauth._node.name = "author"
        sectauth.appendChild(doc.createTextNode(
            sectauth.getAttribute("name")))
        sectauth.removeAttribute("name")
        after = section.childNodes[2]
        title = section.childNodes[1]
        if title.nodeType == xml.dom.core.ELEMENT and title.tagName != "title":
            after = section.childNodes[0]
        section.insertBefore(doc.createTextNode("\n  "), after)
        section.insertBefore(sectauth, after)


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
    fixup_descriptors(doc)
    normalize(doc)
    fixup_paras(doc)
    fixup_sectionauthors(doc)
    remap_element_names(doc, {
        "tableii": ("table", {"cols": "2"}),
        "tableiii": ("table", {"cols": "3"}),
        "tableiv": ("table", {"cols": "4"}),
        "lineii": ("row", {}),
        "lineiii": ("row", {}),
        "lineiv": ("row", {}),
        "refmodule": ("module", {"link": "link"}),
        })
    fixup_table_structures(doc)
    fixup_rfc_references(doc)
    fixup_signatures(doc)
    #
    d = {}
    for gi in p.get_empties():
        d[gi] = gi
    if d.has_key("rfc"):
        del d["rfc"]
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
