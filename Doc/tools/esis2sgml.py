#! /usr/bin/env python

"""
"""
__version__ = '$Revision$'


import re
import string


_data_rx = re.compile(r"[^\\][^\\]*")

def decode(s):
    r = ''
    while s:
        m = _data_rx.match(s)
        if m:
            r = r + m.group()
            s = s[len(m.group()):]
        elif s[1] == "\\":
            r = r + "\\"
            s = s[2:]
        elif s[1] == "n":
            r = r + "\n"
            s = s[2:]
        else:
            raise ValueError, "can't handle " + `s`
    return r


def format_attrs(attrs):
    attrs = attrs.items()
    attrs.sort()
    s = ''
    for name, value in attrs:
        s = '%s %s="%s"' % (s, name, value)
    return s


def do_convert(ifp, ofp, knownempties, xml=0):
    attrs = {}
    lastopened = None
    knownempty = 0
    lastempty = 0
    while 1:
        line = ifp.readline()
        if not line:
            break

        type = line[0]
        data = line[1:]
        if data and data[-1] == "\n":
            data = data[:-1]
        if type == "-":
            data = decode(data)
            ofp.write(data)
            if "\n" in data:
                lastopened = None
            knownempty = 0
            lastempty = 0
        elif type == "(":
            if knownempty and xml:
                ofp.write("<%s%s/>" % (data, format_attrs(attrs)))
            else:
                ofp.write("<%s%s>" % (data, format_attrs(attrs)))
            attrs = {}
            lastopened = data
            lastempty = knownempty
            knownempty = 0
        elif type == ")":
            if xml:
                if not lastempty:
                    ofp.write("</%s>" % data)
            elif data not in knownempties:
                if lastopened == data:
                    ofp.write("</>")
                else:
                    ofp.write("</%s>" % data)
            lastopened = None
            lastempty = 0
        elif type == "A":
            name, type, value = string.split(data, " ", 2)
            attrs[name] = decode(value)
        elif type == "e":
            knownempty = 1


def sgml_convert(ifp, ofp, knownempties=()):
    return do_convert(ifp, ofp, knownempties, xml=0)


def xml_convert(ifp, ofp, knownempties=[]):
    return do_convert(ifp, ofp, knownempties, xml=1)


def main():
    import sys
    #
    convert = sgml_convert
    if sys.argv[1:] and sys.argv[1] in ("-x", "--xml"):
        convert = xml_convert
        del sys.argv[1]
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
    # knownempties is ignored in the XML version
    convert(ifp, ofp, knownempties=["rfc", "POSIX", "ASCII", "declaremodule",
                                    "maketitle", "makeindex", "makemodindex",
                                    "localmoduletable", "ABC", "UNIX", "Cpp",
                                    "C", "EOF", "NULL", "manpage", "input",
                                    "label"])


if __name__ == "__main__":
    main()
