#! /usr/bin/env python

"""Generate ESIS events based on a LaTeX source document and
configuration data.

The conversion is not strong enough to work with arbitrary LaTeX
documents; it has only been designed to work with the highly stylized
markup used in the standard Python documentation.  A lot of
information about specific markup is encoded in the control table
passed to the convert() function; changing this table can allow this
tool to support additional LaTeX markups.

The format of the table is largely undocumented; see the commented
headers where the table is specified in main().  There is no provision 
to load an alternate table from an external file.
"""

import errno
import getopt
import os
import re
import sys
import xml.sax
import xml.sax.saxutils

from esistools import encode


DEBUG = 0


class LaTeXFormatError(Exception):
    pass


class LaTeXStackError(LaTeXFormatError):
    def __init__(self, found, stack):
        msg = "environment close for %s doesn't match;\n  stack = %s" \
              % (found, stack)
        self.found = found
        self.stack = stack[:]
        LaTeXFormatError.__init__(self, msg)


_begin_env_rx = re.compile(r"[\\]begin{([^}]*)}")
_end_env_rx = re.compile(r"[\\]end{([^}]*)}")
_begin_macro_rx = re.compile(r"[\\]([a-zA-Z]+[*]?) ?({|\s*\n?)")
_comment_rx = re.compile("%+ ?(.*)\n[ \t]*")
_text_rx = re.compile(r"[^]~%\\{}]+")
_optional_rx = re.compile(r"\s*[[]([^]]*)[]]", re.MULTILINE)
# _parameter_rx is this complicated to allow {...} inside a parameter;
# this is useful to match tabular layout specifications like {c|p{24pt}}
_parameter_rx = re.compile("[ \n]*{(([^{}}]|{[^}]*})*)}")
_token_rx = re.compile(r"[a-zA-Z][a-zA-Z0-9.-]*$")
_start_group_rx = re.compile("[ \n]*{")
_start_optional_rx = re.compile("[ \n]*[[]")


ESCAPED_CHARS = "$%#^ {}&~"


def dbgmsg(msg):
    if DEBUG:
        sys.stderr.write(msg + "\n")

def pushing(name, point, depth):
    dbgmsg("pushing <%s> at %s" % (name, point))

def popping(name, point, depth):
    dbgmsg("popping </%s> at %s" % (name, point))


class _Stack(list):
    def append(self, entry):
        if not isinstance(entry, str):
            raise LaTeXFormatError("cannot push non-string on stack: "
                                   + `entry`)
        #dbgmsg("%s<%s>" % (" "*len(self.data), entry))
        list.append(self, entry)

    def pop(self, index=-1):
        entry = self[index]
        del self[index]
        #dbgmsg("%s</%s>" % (" " * len(self), entry))

    def __delitem__(self, index):
        entry = self[index]
        list.__delitem__(self, index)
        #dbgmsg("%s</%s>" % (" " * len(self), entry))


def new_stack():
    if DEBUG:
        return _Stack()
    else:
        return []


class Conversion:
    def __init__(self, ifp, ofp, table):
        self.write = ofp.write
        self.ofp = ofp
        self.table = table
        L = [s.rstrip() for s in ifp.readlines()]
        L.append("")
        self.line = "\n".join(L)
        self.preamble = 1

    def convert(self):
        self.subconvert()

    def subconvert(self, endchar=None, depth=0):
        #
        # Parses content, including sub-structures, until the character
        # 'endchar' is found (with no open structures), or until the end
        # of the input data is endchar is None.
        #
        stack = new_stack()
        line = self.line
        while line:
            if line[0] == endchar and not stack:
                self.line = line
                return line
            m = _comment_rx.match(line)
            if m:
                text = m.group(1)
                if text:
                    self.write("(COMMENT\n- %s \n)COMMENT\n-\\n\n"
                               % encode(text))
                line = line[m.end():]
                continue
            m = _begin_env_rx.match(line)
            if m:
                name = m.group(1)
                entry = self.get_env_entry(name)
                # re-write to use the macro handler
                line = r"\%s %s" % (name, line[m.end():])
                continue
            m = _end_env_rx.match(line)
            if m:
                # end of environment
                envname = m.group(1)
                entry = self.get_entry(envname)
                while stack and envname != stack[-1] \
                      and stack[-1] in entry.endcloses:
                    self.write(")%s\n" % stack.pop())
                if stack and envname == stack[-1]:
                    self.write(")%s\n" % entry.outputname)
                    del stack[-1]
                else:
                    raise LaTeXStackError(envname, stack)
                line = line[m.end():]
                continue
            m = _begin_macro_rx.match(line)
            if m:
                # start of macro
                macroname = m.group(1)
                if macroname == "c":
                    # Ugh!  This is a combining character...
                    endpos = m.end()
                    self.combining_char("c", line[endpos])
                    line = line[endpos + 1:]
                    continue
                entry = self.get_entry(macroname)
                if entry.verbatim:
                    # magic case!
                    pos = line.find("\\end{%s}" % macroname)
                    text = line[m.end(1):pos]
                    stack.append(entry.name)
                    self.write("(%s\n" % entry.outputname)
                    self.write("-%s\n" % encode(text))
                    self.write(")%s\n" % entry.outputname)
                    stack.pop()
                    line = line[pos + len("\\end{%s}" % macroname):]
                    continue
                while stack and stack[-1] in entry.closes:
                    top = stack.pop()
                    topentry = self.get_entry(top)
                    if topentry.outputname:
                        self.write(")%s\n-\\n\n" % topentry.outputname)
                #
                if entry.outputname and entry.empty:
                    self.write("e\n")
                #
                params, optional, empty = self.start_macro(macroname)
                # rip off the macroname
                if params:
                    line = line[m.end(1):]
                elif empty:
                    line = line[m.end(1):]
                else:
                    line = line[m.end():]
                opened = 0
                implied_content = 0

                # handle attribute mappings here:
                for pentry in params:
                    if pentry.type == "attribute":
                        if pentry.optional:
                            m = _optional_rx.match(line)
                            if m and entry.outputname:
                                line = line[m.end():]
                                self.dump_attr(pentry, m.group(1))
                        elif pentry.text and entry.outputname:
                            # value supplied by conversion spec:
                            self.dump_attr(pentry, pentry.text)
                        else:
                            m = _parameter_rx.match(line)
                            if not m:
                                raise LaTeXFormatError(
                                    "could not extract parameter %s for %s: %s"
                                    % (pentry.name, macroname, `line[:100]`))
                            if entry.outputname:
                                self.dump_attr(pentry, m.group(1))
                            line = line[m.end():]
                    elif pentry.type == "child":
                        if pentry.optional:
                            m = _optional_rx.match(line)
                            if m:
                                line = line[m.end():]
                                if entry.outputname and not opened:
                                    opened = 1
                                    self.write("(%s\n" % entry.outputname)
                                    stack.append(macroname)
                                stack.append(pentry.name)
                                self.write("(%s\n" % pentry.name)
                                self.write("-%s\n" % encode(m.group(1)))
                                self.write(")%s\n" % pentry.name)
                                stack.pop()
                        else:
                            if entry.outputname and not opened:
                                opened = 1
                                self.write("(%s\n" % entry.outputname)
                                stack.append(entry.name)
                            self.write("(%s\n" % pentry.name)
                            stack.append(pentry.name)
                            self.line = skip_white(line)[1:]
                            line = self.subconvert(
                                "}", len(stack) + depth + 1)[1:]
                            self.write(")%s\n" % stack.pop())
                    elif pentry.type == "content":
                        if pentry.implied:
                            implied_content = 1
                        else:
                            if entry.outputname and not opened:
                                opened = 1
                                self.write("(%s\n" % entry.outputname)
                                stack.append(entry.name)
                            line = skip_white(line)
                            if line[0] != "{":
                                raise LaTeXFormatError(
                                    "missing content for " + macroname)
                            self.line = line[1:]
                            line = self.subconvert("}", len(stack) + depth + 1)
                            if line and line[0] == "}":
                                line = line[1:]
                    elif pentry.type == "text" and pentry.text:
                        if entry.outputname and not opened:
                            opened = 1
                            stack.append(entry.name)
                            self.write("(%s\n" % entry.outputname)
                        #dbgmsg("--- text: %s" % `pentry.text`)
                        self.write("-%s\n" % encode(pentry.text))
                    elif pentry.type == "entityref":
                        self.write("&%s\n" % pentry.name)
                if entry.outputname:
                    if not opened:
                        self.write("(%s\n" % entry.outputname)
                        stack.append(entry.name)
                    if not implied_content:
                        self.write(")%s\n" % entry.outputname)
                        stack.pop()
                continue
            if line[0] == endchar and not stack:
                self.line = line[1:]
                return self.line
            if line[0] == "}":
                # end of macro or group
                macroname = stack[-1]
                if macroname:
                    conversion = self.table[macroname]
                    if conversion.outputname:
                        # otherwise, it was just a bare group
                        self.write(")%s\n" % conversion.outputname)
                del stack[-1]
                line = line[1:]
                continue
            if line[0] == "~":
                # don't worry about the "tie" aspect of this command
                line = line[1:]
                self.write("- \n")
                continue
            if line[0] == "{":
                stack.append("")
                line = line[1:]
                continue
            if line[0] == "\\" and line[1] in ESCAPED_CHARS:
                self.write("-%s\n" % encode(line[1]))
                line = line[2:]
                continue
            if line[:2] == r"\\":
                self.write("(BREAK\n)BREAK\n")
                line = line[2:]
                continue
            if line[:2] == r"\_":
                line = "_" + line[2:]
                continue
            if line[:2] in (r"\'", r'\"'):
                # combining characters...
                self.combining_char(line[1], line[2])
                line = line[3:]
                continue
            m = _text_rx.match(line)
            if m:
                text = encode(m.group())
                self.write("-%s\n" % text)
                line = line[m.end():]
                continue
            # special case because of \item[]
            # XXX can we axe this???
            if line[0] == "]":
                self.write("-]\n")
                line = line[1:]
                continue
            # avoid infinite loops
            extra = ""
            if len(line) > 100:
                extra = "..."
            raise LaTeXFormatError("could not identify markup: %s%s"
                                   % (`line[:100]`, extra))
        while stack:
            entry = self.get_entry(stack[-1])
            if entry.closes:
                self.write(")%s\n-%s\n" % (entry.outputname, encode("\n")))
                del stack[-1]
            else:
                break
        if stack:
            raise LaTeXFormatError("elements remain on stack: "
                                   + ", ".join(stack))
        # otherwise we just ran out of input here...

    # This is a really limited table of combinations, but it will have
    # to do for now.
    _combinations = {
        ("c", "c"): 0x00E7,
        ("'", "e"): 0x00E9,
        ('"', "o"): 0x00F6,
        }

    def combining_char(self, prefix, char):
        ordinal = self._combinations[(prefix, char)]
        self.write("-\\%%%d;\n" % ordinal)

    def start_macro(self, name):
        conversion = self.get_entry(name)
        parameters = conversion.parameters
        optional = parameters and parameters[0].optional
        return parameters, optional, conversion.empty

    def get_entry(self, name):
        entry = self.table.get(name)
        if entry is None:
            dbgmsg("get_entry(%s) failing; building default entry!" % `name`)
            # not defined; build a default entry:
            entry = TableEntry(name)
            entry.has_content = 1
            entry.parameters.append(Parameter("content"))
            self.table[name] = entry
        return entry

    def get_env_entry(self, name):
        entry = self.table.get(name)
        if entry is None:
            # not defined; build a default entry:
            entry = TableEntry(name, 1)
            entry.has_content = 1
            entry.parameters.append(Parameter("content"))
            entry.parameters[-1].implied = 1
            self.table[name] = entry
        elif not entry.environment:
            raise LaTeXFormatError(
                name + " is defined as a macro; expected environment")
        return entry

    def dump_attr(self, pentry, value):
        if not (pentry.name and value):
            return
        if _token_rx.match(value):
            dtype = "TOKEN"
        else:
            dtype = "CDATA"
        self.write("A%s %s %s\n" % (pentry.name, dtype, encode(value)))


def convert(ifp, ofp, table):
    c = Conversion(ifp, ofp, table)
    try:
        c.convert()
    except IOError, (err, msg):
        if err != errno.EPIPE:
            raise


def skip_white(line):
    while line and line[0] in " %\n\t\r":
        line = line[1:].lstrip()
    return line



class TableEntry:
    def __init__(self, name, environment=0):
        self.name = name
        self.outputname = name
        self.environment = environment
        self.empty = not environment
        self.has_content = 0
        self.verbatim = 0
        self.auto_close = 0
        self.parameters = []
        self.closes = []
        self.endcloses = []

class Parameter:
    def __init__(self, type, name=None, optional=0):
        self.type = type
        self.name = name
        self.optional = optional
        self.text = ''
        self.implied = 0


class TableHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.__table = {}
        self.__buffer = ''
        self.__methods = {}

    def get_table(self):
        for entry in self.__table.values():
            if entry.environment and not entry.has_content:
                p = Parameter("content")
                p.implied = 1
                entry.parameters.append(p)
                entry.has_content = 1
        return self.__table

    def startElement(self, tag, attrs):
        try:
            start, end = self.__methods[tag]
        except KeyError:
            start = getattr(self, "start_" + tag, None)
            end = getattr(self, "end_" + tag, None)
            self.__methods[tag] = (start, end)
        if start:
            start(attrs)

    def endElement(self, tag):
        start, end = self.__methods[tag]
        if end:
            end()

    def endDocument(self):
        self.__methods.clear()

    def characters(self, data):
        self.__buffer += data

    def start_environment(self, attrs):
        name = attrs["name"]
        self.__current = TableEntry(name, environment=1)
        self.__current.verbatim = attrs.get("verbatim") == "yes"
        if attrs.has_key("outputname"):
            self.__current.outputname = attrs.get("outputname")
        self.__current.endcloses = attrs.get("endcloses", "").split()
    def end_environment(self):
        self.end_macro()

    def start_macro(self, attrs):
        name = attrs["name"]
        self.__current = TableEntry(name)
        self.__current.closes = attrs.get("closes", "").split()
        if attrs.has_key("outputname"):
            self.__current.outputname = attrs.get("outputname")
    def end_macro(self):
        name = self.__current.name
        if self.__table.has_key(name):
            raise ValueError("name %s already in use" % `name`)
        self.__table[name] = self.__current
        self.__current = None

    def start_attribute(self, attrs):
        name = attrs.get("name")
        optional = attrs.get("optional") == "yes"
        if name:
            p = Parameter("attribute", name, optional=optional)
        else:
            p = Parameter("attribute", optional=optional)
        self.__current.parameters.append(p)
        self.__buffer = ''
    def end_attribute(self):
        self.__current.parameters[-1].text = self.__buffer

    def start_entityref(self, attrs):
        name = attrs["name"]
        p = Parameter("entityref", name)
        self.__current.parameters.append(p)

    def start_child(self, attrs):
        name = attrs["name"]
        p = Parameter("child", name, attrs.get("optional") == "yes")
        self.__current.parameters.append(p)
        self.__current.empty = 0

    def start_content(self, attrs):
        p = Parameter("content")
        p.implied = attrs.get("implied") == "yes"
        if self.__current.environment:
            p.implied = 1
        self.__current.parameters.append(p)
        self.__current.has_content = 1
        self.__current.empty = 0

    def start_text(self, attrs):
        self.__current.empty = 0
        self.__buffer = ''
    def end_text(self):
        p = Parameter("text")
        p.text = self.__buffer
        self.__current.parameters.append(p)


def load_table(fp):
    ch = TableHandler()
    xml.sax.parse(fp, ch)
    return ch.get_table()


def main():
    global DEBUG
    #
    opts, args = getopt.getopt(sys.argv[1:], "D", ["debug"])
    for opt, arg in opts:
        if opt in ("-D", "--debug"):
            DEBUG += 1
    if len(args) == 0:
        ifp = sys.stdin
        ofp = sys.stdout
    elif len(args) == 1:
        ifp = open(args[0])
        ofp = sys.stdout
    elif len(args) == 2:
        ifp = open(args[0])
        ofp = open(args[1], "w")
    else:
        usage()
        sys.exit(2)

    table = load_table(open(os.path.join(sys.path[0], 'conversion.xml')))
    convert(ifp, ofp, table)


if __name__ == "__main__":
    main()
