#! /usr/bin/env python

"""Perform massive transformations on a document tree created from the LaTeX
of the Python documentation, and dump the ESIS data for the transformed tree.
"""


import errno
import esistools
import re
import string
import sys
import xml.dom
import xml.dom.minidom

ELEMENT = xml.dom.Node.ELEMENT_NODE
ENTITY_REFERENCE = xml.dom.Node.ENTITY_REFERENCE_NODE
TEXT = xml.dom.Node.TEXT_NODE


class ConversionError(Exception):
    pass


ewrite = sys.stderr.write
try:
    # We can only do this trick on Unix (if tput is on $PATH)!
    if sys.platform != "posix" or not sys.stderr.isatty():
        raise ImportError
    import commands
except ImportError:
    bwrite = ewrite
else:
    def bwrite(s, BOLDON=commands.getoutput("tput bold"),
               BOLDOFF=commands.getoutput("tput sgr0")):
        ewrite("%s%s%s" % (BOLDON, s, BOLDOFF))


PARA_ELEMENT = "para"

DEBUG_PARA_FIXER = 0

if DEBUG_PARA_FIXER:
    def para_msg(s):
        ewrite("*** %s\n" % s)
else:
    def para_msg(s):
        pass


def get_first_element(doc, gi):
    for n in doc.childNodes:
        if n.nodeName == gi:
            return n

def extract_first_element(doc, gi):
    node = get_first_element(doc, gi)
    if node is not None:
        doc.removeChild(node)
    return node


def get_documentElement(node):
    result = None
    for child in node.childNodes:
        if child.nodeType == ELEMENT:
            result = child
    return result


def set_tagName(elem, gi):
    elem.nodeName = elem.tagName = gi


def find_all_elements(doc, gi):
    nodes = []
    if doc.nodeName == gi:
        nodes.append(doc)
    for child in doc.childNodes:
        if child.nodeType == ELEMENT:
            if child.tagName == gi:
                nodes.append(child)
            for node in child.getElementsByTagName(gi):
                nodes.append(node)
    return nodes

def find_all_child_elements(doc, gi):
    nodes = []
    for child in doc.childNodes:
        if child.nodeName == gi:
            nodes.append(child)
    return nodes


def find_all_elements_from_set(doc, gi_set):
    return __find_all_elements_from_set(doc, gi_set, [])

def __find_all_elements_from_set(doc, gi_set, nodes):
    if doc.nodeName in gi_set:
        nodes.append(doc)
    for child in doc.childNodes:
        if child.nodeType == ELEMENT:
            __find_all_elements_from_set(child, gi_set, nodes)
    return nodes


def simplify(doc, fragment):
    # Try to rationalize the document a bit, since these things are simply
    # not valid SGML/XML documents as they stand, and need a little work.
    documentclass = "document"
    inputs = []
    node = extract_first_element(fragment, "documentclass")
    if node is not None:
        documentclass = node.getAttribute("classname")
    node = extract_first_element(fragment, "title")
    if node is not None:
        inputs.append(node)
    # update the name of the root element
    node = get_first_element(fragment, "document")
    if node is not None:
        set_tagName(node, documentclass)
    while 1:
        node = extract_first_element(fragment, "input")
        if node is None:
            break
        inputs.append(node)
    if inputs:
        docelem = get_documentElement(fragment)
        inputs.reverse()
        for node in inputs:
            text = doc.createTextNode("\n")
            docelem.insertBefore(text, docelem.firstChild)
            docelem.insertBefore(node, text)
        docelem.insertBefore(doc.createTextNode("\n"), docelem.firstChild)
    while fragment.firstChild and fragment.firstChild.nodeType == TEXT:
        fragment.removeChild(fragment.firstChild)


def cleanup_root_text(doc):
    discards = []
    skip = 0
    for n in doc.childNodes:
        prevskip = skip
        skip = 0
        if n.nodeType == TEXT and not prevskip:
            discards.append(n)
        elif n.nodeName == "COMMENT":
            skip = 1
    for node in discards:
        doc.removeChild(node)


DESCRIPTOR_ELEMENTS = (
    "cfuncdesc", "cvardesc", "ctypedesc",
    "classdesc", "memberdesc", "memberdescni", "methoddesc", "methoddescni",
    "excdesc", "funcdesc", "funcdescni", "opcodedesc",
    "datadesc", "datadescni",
    )

def fixup_descriptors(doc, fragment):
    sections = find_all_elements(fragment, "section")
    for section in sections:
        find_and_fix_descriptors(doc, section)


def find_and_fix_descriptors(doc, container):
    children = container.childNodes
    for child in children:
        if child.nodeType == ELEMENT:
            tagName = child.tagName
            if tagName in DESCRIPTOR_ELEMENTS:
                rewrite_descriptor(doc, child)
            elif tagName == "subsection":
                find_and_fix_descriptors(doc, child)


def rewrite_descriptor(doc, descriptor):
    #
    # Do these things:
    #   1. Add an "index='no'" attribute to the element if the tagName
    #      ends in 'ni', removing the 'ni' from the name.
    #   2. Create a <signature> from the name attribute
    #   2a.Create an <args> if it appears to be available.
    #   3. Create additional <signature>s from <*line{,ni}> elements,
    #      if found.
    #   4. If a <versionadded> is found, move it to an attribute on the
    #      descriptor.
    #   5. Move remaining child nodes to a <description> element.
    #   6. Put it back together.
    #
    # 1.
    descname = descriptor.tagName
    index = 1
    if descname[-2:] == "ni":
        descname = descname[:-2]
        descriptor.setAttribute("index", "no")
        set_tagName(descriptor, descname)
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
    # 2a.
    if descriptor.hasAttribute("var"):
        if descname != "opcodedesc":
            raise RuntimeError, \
                  "got 'var' attribute on descriptor other than opcodedesc"
        variable = descriptor.getAttribute("var")
        if variable:
            args = doc.createElement("args")
            args.appendChild(doc.createTextNode(variable))
            signature.appendChild(doc.createTextNode("\n    "))
            signature.appendChild(args)
        descriptor.removeAttribute("var")
    newchildren = [signature]
    children = descriptor.childNodes
    pos = skip_leading_nodes(children)
    if pos < len(children):
        child = children[pos]
        if child.nodeName == "args":
            # move <args> to <signature>, or remove if empty:
            child.parentNode.removeChild(child)
            if len(child.childNodes):
                signature.appendChild(doc.createTextNode("\n    "))
                signature.appendChild(child)
    signature.appendChild(doc.createTextNode("\n  "))
    # 3, 4.
    pos = skip_leading_nodes(children, pos)
    while pos < len(children) \
          and children[pos].nodeName in (linename, "versionadded"):
        if children[pos].tagName == linename:
            # this is really a supplemental signature, create <signature>
            oldchild = children[pos].cloneNode(1)
            try:
                sig = methodline_to_signature(doc, children[pos])
            except KeyError:
                print oldchild.toxml()
                raise
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
    if last.nodeType == TEXT:
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


def handle_appendix(doc, fragment):
    # must be called after simplfy() if document is multi-rooted to begin with
    docelem = get_documentElement(fragment)
    toplevel = docelem.tagName == "manual" and "chapter" or "section"
    appendices = 0
    nodes = []
    for node in docelem.childNodes:
        if appendices:
            nodes.append(node)
        elif node.nodeType == ELEMENT:
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
        while nodes and nodes[0].nodeType == TEXT \
              and not string.strip(nodes[0].data):
            del nodes[0]
        map(back.appendChild, nodes)
        docelem.appendChild(doc.createTextNode("\n"))


def handle_labels(doc, fragment):
    for label in find_all_elements(fragment, "label"):
        id = label.getAttribute("id")
        if not id:
            continue
        parent = label.parentNode
        parentTagName = parent.tagName
        if parentTagName == "title":
            parent.parentNode.setAttribute("id", id)
        else:
            parent.setAttribute("id", id)
        # now, remove <label id="..."/> from parent:
        parent.removeChild(label)
        if parentTagName == "title":
            parent.normalize()
            children = parent.childNodes
            if children[-1].nodeType == TEXT:
                children[-1].data = string.rstrip(children[-1].data)


def fixup_trailing_whitespace(doc, wsmap):
    queue = [doc]
    while queue:
        node = queue[0]
        del queue[0]
        if wsmap.has_key(node.nodeName):
            ws = wsmap[node.tagName]
            children = node.childNodes
            children.reverse()
            if children[0].nodeType == TEXT:
                data = string.rstrip(children[0].data) + ws
                children[0].data = data
            children.reverse()
            # hack to get the title in place:
            if node.tagName == "title" \
               and node.parentNode.firstChild.nodeType == ELEMENT:
                node.parentNode.insertBefore(doc.createText("\n  "),
                                             node.parentNode.firstChild)
        for child in node.childNodes:
            if child.nodeType == ELEMENT:
                queue.append(child)


def normalize(doc):
    for node in doc.childNodes:
        if node.nodeType == ELEMENT:
            node.normalize()


def cleanup_trailing_parens(doc, element_names):
    d = {}
    for gi in element_names:
        d[gi] = gi
    rewrite_element = d.has_key
    queue = []
    for node in doc.childNodes:
        if node.nodeType == ELEMENT:
            queue.append(node)
    while queue:
        node = queue[0]
        del queue[0]
        if rewrite_element(node.tagName):
            children = node.childNodes
            if len(children) == 1 \
               and children[0].nodeType == TEXT:
                data = children[0].data
                if data[-2:] == "()":
                    children[0].data = data[:-2]
        else:
            for child in node.childNodes:
                if child.nodeType == ELEMENT:
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
        if nodeType == ELEMENT:
            if l.tagName != r.tagName:
                return 0
            # should check attributes, but that's not a problem here
            if not contents_match(l, r):
                return 0
        elif nodeType == TEXT:
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
    set_tagName(node, "synopsis")
    lastchild = node.childNodes[-1]
    if lastchild.nodeType == TEXT \
       and lastchild.data[-1:] == ".":
        lastchild.data = lastchild.data[:-1]
    modauthor = extract_first_element(section, "moduleauthor")
    if modauthor:
        set_tagName(modauthor, "author")
        modauthor.appendChild(doc.createTextNode(
            modauthor.getAttribute("name")))
        modauthor.removeAttribute("name")
    platform = extract_first_element(section, "platform")
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
               and children[0].nodeName == "module" \
               and children[0].childNodes[0].data == name:
                # this is it; morph the <title> into <short-synopsis>
                first_data = children[1]
                if first_data.data[:4] == " ---":
                    first_data.data = string.lstrip(first_data.data[4:])
                set_tagName(title, "short-synopsis")
                if children[-1].nodeType == TEXT \
                   and children[-1].data[-1:] == ".":
                    children[-1].data = children[-1].data[:-1]
                section.removeChild(title)
                section.removeChild(section.childNodes[0])
                title.removeChild(children[0])
                modinfo_pos = 0
            else:
                ewrite("module name in title doesn't match"
                       " <declaremodule/>; no <short-synopsis/>\n")
        else:
            ewrite("Unexpected condition: <section/> without <title/>\n")
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
        if platform:
            modinfo.appendChild(doc.createTextNode("\n    "))
            modinfo.appendChild(platform)
        modinfo.appendChild(doc.createTextNode("\n  "))
        section.insertBefore(modinfo, section.childNodes[modinfo_pos])
        section.insertBefore(doc.createTextNode("\n  "), modinfo)
        #
        # The rest of this removes extra newlines from where we cut out
        # a lot of elements.  A lot of code for minimal value, but keeps
        # keeps the generated *ML from being too funny looking.
        #
        section.normalize()
        children = section.childNodes
        for i in range(len(children)):
            node = children[i]
            if node.nodeName == "moduleinfo":
                nextnode = children[i+1]
                if nextnode.nodeType == TEXT:
                    data = nextnode.data
                    if len(string.lstrip(data)) < (len(data) - 4):
                        nextnode.data = "\n\n\n" + string.lstrip(data)


def cleanup_synopses(doc, fragment):
    for node in find_all_elements(fragment, "section"):
        create_module_info(doc, node)


def fixup_table_structures(doc, fragment):
    for table in find_all_elements(fragment, "table"):
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
        if child.nodeType == ELEMENT:
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
        if nodeType == TEXT:
            if string.strip(child.data):
                raise ConversionError("unexpected free data in <%s>: %r"
                                      % (table.tagName, child.data))
            table.removeChild(child)
            continue
        if nodeType == ELEMENT:
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
        if child.nodeName == name:
            nodes.append(child)
    for node in nodes:
        source.removeChild(node)
        dest.appendChild(node)
        if sep:
            dest.appendChild(doc.createTextNode(sep))


RECURSE_INTO_PARA_CONTAINERS = (
    "chapter", "abstract", "enumerate",
    "section", "subsection", "subsubsection",
    "paragraph", "subparagraph", "back-matter",
    "howto", "manual",
    "item", "itemize", "fulllineitems", "enumeration", "descriptionlist",
    "definitionlist", "definition",
    )

PARA_LEVEL_ELEMENTS = (
    "moduleinfo", "title", "verbatim", "enumerate", "item",
    "interpreter-session", "back-matter", "interactive-session",
    "opcodedesc", "classdesc", "datadesc",
    "funcdesc", "methoddesc", "excdesc", "memberdesc", "membderdescni",
    "funcdescni", "methoddescni", "excdescni",
    "tableii", "tableiii", "tableiv", "localmoduletable",
    "sectionauthor", "seealso", "itemize",
    # include <para>, so we can just do it again to get subsequent paras:
    PARA_ELEMENT,
    )

PARA_LEVEL_PRECEEDERS = (
    "setindexsubitem", "author",
    "stindex", "obindex", "COMMENT", "label", "input", "title",
    "versionadded", "versionchanged", "declaremodule", "modulesynopsis",
    "moduleauthor", "indexterm", "leader",
    )


def fixup_paras(doc, fragment):
    for child in fragment.childNodes:
        if child.nodeName in RECURSE_INTO_PARA_CONTAINERS:
            fixup_paras_helper(doc, child)
    descriptions = find_all_elements(fragment, "description")
    for description in descriptions:
        fixup_paras_helper(doc, description)


def fixup_paras_helper(doc, container, depth=0):
    # document is already normalized
    children = container.childNodes
    start = skip_leading_nodes(children)
    while len(children) > start:
        if children[start].nodeName in RECURSE_INTO_PARA_CONTAINERS:
            # Something to recurse into:
            fixup_paras_helper(doc, children[start])
        else:
            # Paragraph material:
            build_para(doc, container, start, len(children))
            if DEBUG_PARA_FIXER and depth == 10:
                sys.exit(1)
        start = skip_leading_nodes(children, start + 1)


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
        if nodeType == ELEMENT:
            if child.tagName in BREAK_ELEMENTS:
                after = j
                break
        elif nodeType == TEXT:
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
    if children[after - 1].nodeType == TEXT:
        # we may need to split off trailing white space:
        child = children[after - 1]
        data = child.data
        if string.rstrip(data) != data:
            have_last = 0
            child.splitText(len(string.rstrip(data)))
    para = doc.createElement(PARA_ELEMENT)
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
        parent.appendChild(doc.createTextNode("\n\n"))
        return len(parent.childNodes)
    else:
        nextnode = parent.childNodes[start]
        if nextnode.nodeType == TEXT:
            if nextnode.data and nextnode.data[0] != "\n":
                nextnode.data = "\n" + nextnode.data
        else:
            newnode = doc.createTextNode("\n")
            parent.insertBefore(newnode, nextnode)
            nextnode = newnode
            start = start + 1
        parent.insertBefore(para, nextnode)
        return start + 1


def skip_leading_nodes(children, start=0):
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
        if nodeType == TEXT:
            data = child.data
            shortened = string.lstrip(data)
            if shortened:
                if data != shortened:
                    # break into two nodes: whitespace and non-whitespace
                    child.splitText(len(data) - len(shortened))
                    return start + 1
                return start
            # all whitespace, just skip
        elif nodeType == ELEMENT:
            tagName = child.tagName
            if tagName in RECURSE_INTO_PARA_CONTAINERS:
                return start
            if tagName not in PARA_LEVEL_ELEMENTS + PARA_LEVEL_PRECEEDERS:
                return start
        start = start + 1
    return start


def fixup_rfc_references(doc, fragment):
    for rfcnode in find_all_elements(fragment, "rfc"):
        rfcnode.appendChild(doc.createTextNode(
            "RFC " + rfcnode.getAttribute("num")))


def fixup_signatures(doc, fragment):
    for child in fragment.childNodes:
        if child.nodeType == ELEMENT:
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
        if child.nodeName == "optional":
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


def fixup_sectionauthors(doc, fragment):
    for sectauth in find_all_elements(fragment, "sectionauthor"):
        section = sectauth.parentNode
        section.removeChild(sectauth)
        set_tagName(sectauth, "author")
        sectauth.appendChild(doc.createTextNode(
            sectauth.getAttribute("name")))
        sectauth.removeAttribute("name")
        after = section.childNodes[2]
        title = section.childNodes[1]
        if title.nodeName != "title":
            after = section.childNodes[0]
        section.insertBefore(doc.createTextNode("\n  "), after)
        section.insertBefore(sectauth, after)


def fixup_verbatims(doc):
    for verbatim in find_all_elements(doc, "verbatim"):
        child = verbatim.childNodes[0]
        if child.nodeType == TEXT \
           and string.lstrip(child.data)[:3] == ">>>":
            set_tagName(verbatim, "interactive-session")


def add_node_ids(fragment, counter=0):
    fragment.node_id = counter
    for node in fragment.childNodes:
        counter = counter + 1
        if node.nodeType == ELEMENT:
            counter = add_node_ids(node, counter)
        else:
            node.node_id = counter
    return counter + 1


REFMODINDEX_ELEMENTS = ('refmodindex', 'refbimodindex',
                        'refexmodindex', 'refstmodindex')

def fixup_refmodindexes(fragment):
    # Locate <ref*modindex>...</> co-located with <module>...</>, and
    # remove the <ref*modindex>, replacing it with index=index on the
    # <module> element.
    nodes = find_all_elements_from_set(fragment, REFMODINDEX_ELEMENTS)
    d = {}
    for node in nodes:
        parent = node.parentNode
        d[parent.node_id] = parent
    del nodes
    map(fixup_refmodindexes_chunk, d.values())


def fixup_refmodindexes_chunk(container):
    # node is probably a <para>; let's see how often it isn't:
    if container.tagName != PARA_ELEMENT:
        bwrite("--- fixup_refmodindexes_chunk(%s)\n" % container)
    module_entries = find_all_elements(container, "module")
    if not module_entries:
        return
    index_entries = find_all_elements_from_set(container, REFMODINDEX_ELEMENTS)
    removes = []
    for entry in index_entries:
        children = entry.childNodes
        if len(children) != 0:
            bwrite("--- unexpected number of children for %s node:\n"
                   % entry.tagName)
            ewrite(entry.toxml() + "\n")
            continue
        found = 0
        module_name = entry.getAttribute("module")
        for node in module_entries:
            if len(node.childNodes) != 1:
                continue
            this_name = node.childNodes[0].data
            if this_name == module_name:
                found = 1
                node.setAttribute("index", "yes")
        if found:
            removes.append(entry)
    for node in removes:
        container.removeChild(node)


def fixup_bifuncindexes(fragment):
    nodes = find_all_elements(fragment, 'bifuncindex')
    d = {}
    # make sure that each parent is only processed once:
    for node in nodes:
        parent = node.parentNode
        d[parent.node_id] = parent
    del nodes
    map(fixup_bifuncindexes_chunk, d.values())


def fixup_bifuncindexes_chunk(container):
    removes = []
    entries = find_all_child_elements(container, "bifuncindex")
    function_entries = find_all_child_elements(container, "function")
    for entry in entries:
        function_name = entry.getAttribute("name")
        found = 0
        for func_entry in function_entries:
            t2 = func_entry.childNodes[0].data
            if t2[-2:] != "()":
                continue
            t2 = t2[:-2]
            if t2 == function_name:
                func_entry.setAttribute("index", "yes")
                func_entry.setAttribute("module", "__builtin__")
                if not found:
                    found = 1
                    removes.append(entry)
    for entry in removes:
        container.removeChild(entry)


def join_adjacent_elements(container, gi):
    queue = [container]
    while queue:
        parent = queue.pop()
        i = 0
        children = parent.childNodes
        nchildren = len(children)
        while i < (nchildren - 1):
            child = children[i]
            if child.nodeName == gi:
                if children[i+1].nodeName == gi:
                    ewrite("--- merging two <%s/> elements\n" % gi)
                    child = children[i]
                    nextchild = children[i+1]
                    nextchildren = nextchild.childNodes
                    while len(nextchildren):
                        node = nextchildren[0]
                        nextchild.removeChild(node)
                        child.appendChild(node)
                    parent.removeChild(nextchild)
                    continue
            if child.nodeType == ELEMENT:
                queue.append(child)
            i = i + 1


_token_rx = re.compile(r"[a-zA-Z][a-zA-Z0-9.-]*$")

def write_esis(doc, ofp, knownempty):
    for node in doc.childNodes:
        nodeType = node.nodeType
        if nodeType == ELEMENT:
            gi = node.tagName
            if knownempty(gi):
                if node.hasChildNodes():
                    raise ValueError, \
                          "declared-empty node <%s> has children" % gi
                ofp.write("e\n")
            for k, value in node.attributes.items():
                if _token_rx.match(value):
                    dtype = "TOKEN"
                else:
                    dtype = "CDATA"
                ofp.write("A%s %s %s\n" % (k, dtype, esistools.encode(value)))
            ofp.write("(%s\n" % gi)
            write_esis(node, ofp, knownempty)
            ofp.write(")%s\n" % gi)
        elif nodeType == TEXT:
            ofp.write("-%s\n" % esistools.encode(node.data))
        elif nodeType == ENTITY_REFERENCE:
            ofp.write("&%s\n" % node.nodeName)
        else:
            raise RuntimeError, "unsupported node type: %s" % nodeType


def convert(ifp, ofp):
    events = esistools.parse(ifp)
    toktype, doc = events.getEvent()
    fragment = doc.createDocumentFragment()
    events.expandNode(fragment)

    normalize(fragment)
    simplify(doc, fragment)
    handle_labels(doc, fragment)
    handle_appendix(doc, fragment)
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
    cleanup_trailing_parens(fragment, ["function", "method", "cfunction"])
    cleanup_synopses(doc, fragment)
    fixup_descriptors(doc, fragment)
    fixup_verbatims(fragment)
    normalize(fragment)
    fixup_paras(doc, fragment)
    fixup_sectionauthors(doc, fragment)
    fixup_table_structures(doc, fragment)
    fixup_rfc_references(doc, fragment)
    fixup_signatures(doc, fragment)
    add_node_ids(fragment)
    fixup_refmodindexes(fragment)
    fixup_bifuncindexes(fragment)
    # Take care of ugly hacks in the LaTeX markup to avoid LaTeX and
    # LaTeX2HTML screwing with GNU-style long options (the '--' problem).
    join_adjacent_elements(fragment, "option")
    #
    d = {}
    for gi in events.parser.get_empties():
        d[gi] = gi
    if d.has_key("author"):
        del d["author"]
    if d.has_key("rfc"):
        del d["rfc"]
    knownempty = d.has_key
    #
    try:
        write_esis(fragment, ofp, knownempty)
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
        import StringIO
        ofp = StringIO.StringIO()
    else:
        usage()
        sys.exit(2)
    convert(ifp, ofp)
    if len(sys.argv) == 3:
        fp = open(sys.argv[2], "w")
        fp.write(ofp.getvalue())
        fp.close()
        ofp.close()


if __name__ == "__main__":
    main()
