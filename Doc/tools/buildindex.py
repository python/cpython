#! /usr/bin/env python

"""
"""
__version__ = '$Revision$'

import re
import string
import sys


class Node:

    __rmtt = re.compile(r"(.*)<tt>(.*)</tt>(.*)$", re.IGNORECASE)
    __rmjunk = re.compile("<#\d+#>")

    def __init__(self, link, str, seqno):
        self.links = [link]
        self.seqno = seqno
        # remove <#\d+#> left in by moving the data out of LaTeX2HTML
        str = self.__rmjunk.sub('', str)
        # now remove <tt>...</tt> markup; contents remain.
        if '<' in str:
            m = self.__rmtt.match(str)
            if m:
                kstr = string.join(m.group(1, 2, 3), '')
            else:
                kstr = str
        else:
            kstr = str
        kstr = string.lower(kstr)
        # build up the text
        self.text = []
        parts = string.split(str, '!')
        parts = map(string.split, parts, ['@'] * len(parts))
        for entry in parts:
            if len(entry) != 1:
                key, text = entry
            else:
                text = entry[0]
            self.text.append(text)
        # Building the key must be separate since any <tt> has been stripped
        # from the key, but can be avoided if both key and text sources are
        # the same.
        if kstr != str:
            self.key = []
            kparts = string.split(kstr, '!')
            kparts = map(string.split, kparts, ['@'] * len(kparts))
            for entry in kparts:
                if len(entry) != 1:
                    key, text = entry
                else:
                    key = entry[0]
                self.key.append(key)
        else:
            self.key = self.text

    def __cmp__(self, other):
        """Comparison operator includes sequence number, for use with
        list.sort()."""
        return self.cmp_entry(other) or cmp(self.seqno, other.seqno)

    def cmp_entry(self, other):
        """Comparison 'operator' that ignores sequence number."""
        for i in range(min(len(self.key), len(other.key))):
            c = (cmp(self.key[i], other.key[i])
                 or cmp(self.text[i], other.text[i]))
            if c:
                return c
        return cmp(self.key, other.key)

    def __repr__(self):
        return "<Node for %s (%s)>" % (string.join(self.text, '!'), self.seqno)

    def __str__(self):
        return string.join(self.key, '!')

    def dump(self):
        return "%s\0%s###%s\n" \
               % (string.join(self.links, "\0"),
                  string.join(self.text, '!'),
                  self.seqno)


def load(fp):
    nodes = []
    rx = re.compile(r"(.*)\0(.*)###(.*)$")
    while 1:
        line = fp.readline()
        if not line:
            break
        m = rx.match(line)
        if m:
            link, str, seqno = m.group(1, 2, 3)
            nodes.append(Node(link, str, seqno))
    return nodes


def split_letters(nodes):
    letter_groups = []
    group = []
    append = group.append
    if nodes:
        letter = nodes[0].key[0][0]
        letter_groups.append((letter, group))
        for node in nodes:
            nletter = node.key[0][0]
            if letter != nletter:
                letter = nletter
                group = []
                letter_groups.append((letter, group))
                append = group.append
            append(node)
    return letter_groups


def format_nodes(nodes):
    # Does not create multiple links to multiple targets for the same entry;
    # uses a separate entry for each target.  This is a bug.
    level = 0
    strings = ["<dl compact>"]
    append = strings.append
    prev = None
    for node in nodes:
        nlevel = len(node.key) - 1
        if nlevel > level:
            if prev is None or node.key[level] != prev.key[level]:
                append("%s\n<dl compact>" % node.text[level])
            else:
                append("<dl compact>")
            level = nlevel
        elif nlevel < level:
            append("</dl>" * (level - len(node.key) + 1))
            level = nlevel
            if prev is not None and node.key[level] != prev.key[level]:
                append("</dl>")
            else:
                append("<dl compact>")
        elif level:
            if node.key[level-1] != prev.key[level-1]:
                append("</dl>\n%s<dl compact>"
                       % node.text[level-1])
        append("%s%s</a><br>" % (node.links[0], node.text[-1]))
        for link in node.links[1:]:
            strings[-1] = strings[-1][:-4] + ","
            append(link + "[Link]</a><br>")
        prev = node
    append("</dl>" * (level + 1))
    append("")
    append("")
    return string.join(strings, "\n")


def format_letter(letter):
    if letter == '.':
        lettername = ". (dot)"
    elif letter == '_':
        lettername = "_ (underscore)"
    else:
        lettername = string.upper(letter)
    return "<hr>\n<h2><a name=\"letter-%s\">%s</a></h2>\n\n" \
           % (letter, lettername)


def format_html(nodes):
    letter_groups = split_letters(nodes)
    items = []
    for letter, nodes in letter_groups:
        s = "<b><a href=\"#letter-%s\">%s</a></b>" % (letter, letter)
        items.append(s)
    s = "<hr><center>\n%s</center>\n" % string.join(items, " |\n")
    for letter, nodes in letter_groups:
        s = s + format_letter(letter) + format_nodes(nodes)
    return s


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
##             sys.stderr.write("collapsing %s\n" % `node`)
        else:
            i = i + 1
            prev = node


def dump(nodes, fp):
    for node in nodes:
        fp.write(node.dump())


def main():
    fn = sys.argv[1]
    nodes = load(open(fn))
    nodes.sort()
    dump(nodes, open(fn + ".dump-1", "w"))
    collapse(nodes)
    dump(nodes, open(fn + ".dump-2", "w"))
    sys.stdout.write(format_html(nodes))


if __name__ == "__main__":
    main()
