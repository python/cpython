#! /usr/bin/env python

__version__ = '$Revision$'

import os
import re
import string
import sys


class Node:
    __rmjunk = re.compile("<#\d+#>")

    def __init__(self, link, str, seqno):
        self.links = [link]
        self.seqno = seqno
        # remove <#\d+#> left in by moving the data out of LaTeX2HTML
        str = self.__rmjunk.sub('', str)
        # now remove <tt>...</tt> markup; contents remain.
        # build up the text
        self.text = split_entry_text(str)
        self.key = split_entry_key(str)

    def __cmp__(self, other):
        """Comparison operator includes sequence number, for use with
        list.sort()."""
        return self.cmp_entry(other) or cmp(self.seqno, other.seqno)

    def cmp_entry(self, other):
        """Comparison 'operator' that ignores sequence number."""
        c = 0
        for i in range(min(len(self.key), len(other.key))):
            c = (cmp_part(self.key[i], other.key[i])
                 or cmp_part(self.text[i], other.text[i]))
            if c:
                break
        return c or cmp(self.key, other.key) or cmp(self.text, other.text)

    def __repr__(self):
        return "<Node for %s (%s)>" % (string.join(self.text, '!'), self.seqno)

    def __str__(self):
        return string.join(self.key, '!')

    def dump(self):
        return "%s\1%s###%s\n" \
               % (string.join(self.links, "\1"),
                  string.join(self.text, '!'),
                  self.seqno)


def cmp_part(s1, s2):
    result = cmp(s1, s2)
    if result == 0:
        return 0
    l1 = string.lower(s1)
    l2 = string.lower(s2)
    minlen = min(len(s1), len(s2))
    if len(s1) < len(s2) and l1 == l2[:len(s1)]:
        result = -1
    elif len(s2) < len(s1) and l2 == l1[:len(s2)]:
        result = 1
    else:
        result = cmp(l1, l2) or cmp(s1, s2)
    return result


def split_entry(str, which):
    stuff = []
    parts = string.split(str, '!')
    parts = map(string.split, parts, ['@'] * len(parts))
    for entry in parts:
        if len(entry) != 1:
            key = entry[which]
        else:
            key = entry[0]
        stuff.append(key)
    return stuff


_rmtt = re.compile(r"(.*)<tt>(.*)</tt>(.*)$", re.IGNORECASE)
_rmparens = re.compile(r"\(\)")

def split_entry_key(str):
    parts = split_entry(str, 1)
    for i in range(len(parts)):
        m = _rmtt.match(parts[i])
        if m:
            parts[i] = string.join(m.group(1, 2, 3), '')
        else:
            parts[i] = string.lower(parts[i])
        # remove '()' from the key:
        parts[i] = _rmparens.sub('', parts[i])
    return map(trim_ignored_letters, parts)


def split_entry_text(str):
    if '<' in str:
        m = _rmtt.match(str)
        if m:
            str = string.join(m.group(1, 2, 3), '')
    return split_entry(str, 1)


def load(fp):
    nodes = []
    rx = re.compile("(.*)\1(.*)###(.*)$")
    while 1:
        line = fp.readline()
        if not line:
            break
        m = rx.match(line)
        if m:
            link, str, seqno = m.group(1, 2, 3)
            nodes.append(Node(link, str, seqno))
    return nodes


def trim_ignored_letters(s):
    # ignore $ to keep environment variables with the
    # leading letter from the name
    s = string.lower(s)
    if s[0] == "$":
        return s[1:]
    else:
        return s

def get_first_letter(s):
    return string.lower(trim_ignored_letters(s)[0])


def split_letters(nodes):
    letter_groups = []
    if nodes:
        group = []
        append = group.append
        letter = get_first_letter(nodes[0].text[0])
        letter_groups.append((letter, group))
        for node in nodes:
            nletter = get_first_letter(node.text[0])
            if letter != nletter:
                letter = nletter
                group = []
                letter_groups.append((letter, group))
                append = group.append
            append(node)
    return letter_groups


# need a function to separate the nodes into columns...
def split_columns(nodes, columns=1):
    if columns <= 1:
        return [nodes]
    # This is a rough height; we may have to increase to avoid breaks before
    # a subitem.
    colheight = len(nodes) / columns
    numlong = len(nodes) % columns
    if numlong:
        colheight = colheight + 1
    else:
        numlong = columns
    cols = []
    for i in range(numlong):
        start = i * colheight
        end = start + colheight
        cols.append(nodes[start:end])
    del nodes[:end]
    colheight = colheight - 1
    try:
        numshort = len(nodes) / colheight
    except ZeroDivisionError:
        cols = cols + (columns - len(cols)) * [[]]
    else:
        for i in range(numshort):
            start = i * colheight
            end = start + colheight
            cols.append(nodes[start:end])
    return cols


DL_LEVEL_INDENT = "  "

def format_column(nodes):
    strings = ["<dl compact>"]
    append = strings.append
    level = 0
    previous = []
    for node in nodes:
        current = node.text
        count = 0
        for i in range(min(len(current), len(previous))):
            if previous[i] != current[i]:
                break
            count = i + 1
        if count > level:
            append("<dl compact>" * (count - level) + "\n")
            level = count
        elif level > count:
            append("\n")
            append(level * DL_LEVEL_INDENT)
            append("</dl>" * (level - count))
            level = count
        # else: level == count
        for i in range(count, len(current) - 1):
            term = node.text[i]
            level = level + 1
            append("\n<dt>%s\n<dd>\n%s<dl compact>"
                   % (term, level * DL_LEVEL_INDENT))
        append("\n%s<dt>%s%s</a>"
               % (level * DL_LEVEL_INDENT, node.links[0], node.text[-1]))
        for link in node.links[1:]:
            append(",\n%s    %s[Link]</a>" % (level * DL_LEVEL_INDENT, link))
        previous = current
    append("\n")
    append("</dl>" * (level + 1))
    return string.join(strings, '')


def format_nodes(nodes, columns=1):
    strings = []
    append = strings.append
    if columns > 1:
        colnos = range(columns)
        colheight = len(nodes) / columns
        if len(nodes) % columns:
            colheight = colheight + 1
        colwidth = 100 / columns
        append('<table width="100%"><tr valign="top">')
        for col in split_columns(nodes, columns):
            append('<td width="%d%%">\n' % colwidth)
            append(format_column(col))
            append("\n</td>")
        append("\n</tr></table>")
    else:
        append(format_column(nodes))
    append("\n<p>\n")
    return string.join(strings, '')


def format_letter(letter):
    if letter == '.':
        lettername = ". (dot)"
    elif letter == '_':
        lettername = "_ (underscore)"
    else:
        lettername = string.upper(letter)
    return "\n<hr>\n<h2><a name=\"letter-%s\">%s</a></h2>\n\n" \
           % (letter, lettername)


def format_html_letters(nodes, columns=1):
    letter_groups = split_letters(nodes)
    items = []
    for letter, nodes in letter_groups:
        s = "<b><a href=\"#letter-%s\">%s</a></b>" % (letter, letter)
        items.append(s)
    s = ["<hr><center>\n%s</center>\n" % string.join(items, " |\n")]
    for letter, nodes in letter_groups:
        s.append(format_letter(letter))
        s.append(format_nodes(nodes, columns))
    return string.join(s, '')

def format_html(nodes, columns):
    return format_nodes(nodes, columns)


def collapse(nodes):
    """Collapse sequences of nodes with matching keys into a single node.
    Destructive."""
    if len(nodes) < 2:
        return
    prev = nodes[0]
    i = 1
    while i < len(nodes):
        node = nodes[i]
        if not node.cmp_entry(prev):
            prev.links.append(node.links[0])
            del nodes[i]
        else:
            i = i + 1
            prev = node


def dump(nodes, fp):
    for node in nodes:
        fp.write(node.dump())


def main():
    import getopt
    ifn = "-"
    ofn = "-"
    columns = 1
    letters = 0
    opts, args = getopt.getopt(sys.argv[1:], "c:lo:",
                               ["columns=", "letters", "output="])
    for opt, val in opts:
        if opt in ("-o", "--output"):
            ofn = val
        elif opt in ("-c", "--columns"):
            columns = string.atoi(val)
        elif opt in ("-l", "--letters"):
            letters = 1
    if not args:
        args = [ifn]
    nodes = []
    for fn in args:
        nodes = nodes + load(open(fn))
    num_nodes = len(nodes)
    nodes.sort()
    collapse(nodes)
    if letters:
        html = format_html_letters(nodes, columns)
    else:
        html = format_html(nodes, columns)
    program = os.path.basename(sys.argv[0])
    if ofn == "-":
        sys.stdout.write(html)
        sys.stderr.write("\n%s: %d index nodes" % (program, num_nodes))
    else:
        open(ofn, "w").write(html)
        print
        print "%s: %d index nodes" % (program, num_nodes)


if __name__ == "__main__":
    main()
