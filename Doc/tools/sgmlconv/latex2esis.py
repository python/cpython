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
__version__ = '$Revision$'

import copy
import errno
import getopt
import os
import re
import string
import StringIO
import sys
import UserList

from esistools import encode
from types import ListType, StringType, TupleType

try:
    from xml.parsers.xmllib import XMLParser
except ImportError:
    from xmllib import XMLParser


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
_text_rx = re.compile(r"[^]%\\{}]+")
_optional_rx = re.compile(r"\s*[[]([^]]*)[]]")
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


class _Stack(UserList.UserList):
    StringType = type('')

    def append(self, entry):
        if type(entry) is not self.StringType:
            raise LaTeXFormatError("cannot push non-string on stack: "
                                   + `entry`)
        sys.stderr.write("%s<%s>\n" % (" "*len(self.data), entry))
        self.data.append(entry)

    def pop(self, index=-1):
        entry = self.data[index]
        del self.data[index]
        sys.stderr.write("%s</%s>\n" % (" "*len(self.data), entry))

    def __delitem__(self, index):
        entry = self.data[index]
        del self.data[index]
        sys.stderr.write("%s</%s>\n" % (" "*len(self.data), entry))


def new_stack():
    if DEBUG:
        return _Stack()
    return []


class BaseConversion:
    def __init__(self, ifp, ofp, table={}, discards=(), autoclosing=()):
        self.ofp_stack = [ofp]
        self.pop_output()
        self.table = table
        self.discards = discards
        self.autoclosing = autoclosing
        self.line = string.join(map(string.rstrip, ifp.readlines()), "\n")
        self.preamble = 1
        self.stack = new_stack()

    def push_output(self, ofp):
        self.ofp_stack.append(self.ofp)
        self.ofp = ofp
        self.write = ofp.write

    def pop_output(self):
        self.ofp = self.ofp_stack.pop()
        self.write = self.ofp.write

    def err_write(self, msg):
        if DEBUG:
            sys.stderr.write(str(msg) + "\n")

    def convert(self):
        self.subconvert()


class Conversion(BaseConversion):
    def subconvert(self, endchar=None, depth=0):
        stack = self.stack
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
                # re-write to use the macro handler
                line = r"\%s %s" % (m.group(1), line[m.end():])
                continue
            m = _end_env_rx.match(line)
            if m:
                # end of environment
                envname = m.group(1)
                if envname == "document":
                    # special magic
                    for n in stack[1:]:
                        if n not in self.autoclosing:
                            self.err_write(stack)
                            raise LaTeXFormatError(
                                "open element on stack: " + `n`)
                    self.write(")document\n")
                elif stack and envname == stack[-1]:
                    self.write(")%s\n" % envname)
                    del stack[-1]
                    popping(envname, "a", len(stack) + depth)
                else:
                    raise LaTeXStackError(envname, stack)
                line = line[m.end():]
                continue
            m = _begin_macro_rx.match(line)
            if m:
                # start of macro
                macroname = m.group(1)
                if macroname == "verbatim":
                    # really magic case!
                    pos = string.find(line, "\\end{verbatim}")
                    text = line[m.end(1):pos]
                    self.write("(verbatim\n")
                    self.write("-%s\n" % encode(text))
                    self.write(")verbatim\n")
                    line = line[pos + len("\\end{verbatim}"):]
                    continue
                numbered = 1
                opened = 0
                if macroname[-1] == "*":
                    macroname = macroname[:-1]
                    numbered = 0
                if macroname in self.autoclosing and macroname in stack:
                    while stack[-1] != macroname:
                        top = stack.pop()
                        if top and top not in self.discards:
                            self.write(")%s\n-\\n\n" % top)
                        popping(top, "b", len(stack) + depth)
                    if macroname not in self.discards:
                        self.write("-\\n\n)%s\n-\\n\n" % macroname)
                    popping(macroname, "c", len(stack) + depth - 1)
                    del stack[-1]
                #
                if macroname in self.discards:
                    self.push_output(StringIO.StringIO())
                else:
                    self.push_output(self.ofp)
                #
                params, optional, empty, environ = self.start_macro(macroname)
                if not numbered:
                    self.write("Anumbered TOKEN no\n")
                # rip off the macroname
                if params:
                    line = line[m.end(1):]
                elif empty:
                    line = line[m.end(1):]
                else:
                    line = line[m.end():]
                #
                # Very ugly special case to deal with \item[].  The catch
                # is that this needs to occur outside the for loop that
                # handles attribute parsing so we can 'continue' the outer
                # loop.
                #
                if optional and type(params[0]) is TupleType:
                    # the attribute name isn't used in this special case
                    stack.append(macroname)
                    self.write("(%s\n" % macroname)
                    m = _start_optional_rx.match(line)
                    if m:
                        self.line = line[m.end():]
                        line = self.subconvert("]", depth + len(stack))
                    line = "}" + line
                    continue
                # handle attribute mappings here:
                for attrname in params:
                    if optional:
                        optional = 0
                        if type(attrname) is StringType:
                            m = _optional_rx.match(line)
                            if m:
                                line = line[m.end():]
                                self.write("A%s TOKEN %s\n"
                                           % (attrname, encode(m.group(1))))
                    elif type(attrname) is TupleType:
                        # This is a sub-element; but place the and attribute
                        # we found on the stack (\section-like); the
                        # content of the macro will become the content 
                        # of the attribute element, and the macro will 
                        # have to be closed some other way (such as
                        # auto-closing).
                        stack.append(macroname)
                        self.write("(%s\n" % macroname)
                        macroname = attrname[0]
                        m = _start_group_rx.match(line)
                        if m:
                            line = line[m.end():]
                    elif type(attrname) is ListType:
                        # A normal subelement: <macroname><attrname>...</>...
                        attrname = attrname[0]
                        if not opened:
                            opened = 1
                            self.write("(%s\n" % macroname)
                            pushing(macroname, "c", len(stack) + depth)
                        self.write("(%s\n" % attrname)
                        pushing(attrname, "sub-elem", len(stack) + depth + 1)
                        self.line = skip_white(line)[1:]
                        line = self.subconvert("}", len(stack) + depth + 1)[1:]
                        popping(attrname, "sub-elem", len(stack) + depth + 1)
                        self.write(")%s\n" % attrname)
                    else:
                        m = _parameter_rx.match(line)
                        if not m:
                            raise LaTeXFormatError(
                                "could not extract parameter %s for %s: %s"
                                % (attrname, macroname, `line[:100]`))
                        value = m.group(1)
                        if _token_rx.match(value):
                            dtype = "TOKEN"
                        else:
                            dtype = "CDATA"
                        self.write("A%s %s %s\n"
                                   % (attrname, dtype, encode(value)))
                        line = line[m.end():]
                if params and type(params[-1]) is StringType \
                   and (not empty) and not environ:
                    # attempt to strip off next '{'
                    m = _start_group_rx.match(line)
                    if not m:
                        raise LaTeXFormatError(
                            "non-empty element '%s' has no content: %s"
                            % (macroname, line[:12]))
                    line = line[m.end():]
                if not opened:
                    self.write("(%s\n" % macroname)
                    pushing(macroname, "d", len(stack) + depth)
                if empty:
                    line = "}" + line
                stack.append(macroname)
                self.pop_output()
                continue
            if line[0] == endchar and not stack:
                self.line = line[1:]
                return self.line
            if line[0] == "}":
                # end of macro or group
                macroname = stack[-1]
                conversion = self.table.get(macroname)
                if macroname \
                   and macroname not in self.discards \
                   and type(conversion) is not StringType:
                    # otherwise, it was just a bare group
                    self.write(")%s\n" % stack[-1])
                popping(macroname, "d", len(stack) + depth - 1)
                del stack[-1]
                line = line[1:]
                continue
            if line[0] == "{":
                pushing("", "e", len(stack) + depth)
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
            m = _text_rx.match(line)
            if m:
                text = encode(m.group())
                self.write("-%s\n" % text)
                line = line[m.end():]
                continue
            # special case because of \item[]
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
        while stack and stack[-1] in self.autoclosing:
            self.write("-\\n\n")
            self.write(")%s\n" % stack[-1])
            popping(stack.pop(), "e", len(stack) + depth - 1)
        if stack:
            raise LaTeXFormatError("elements remain on stack: "
                                   + string.join(stack, ", "))
        # otherwise we just ran out of input here...

    def start_macro(self, name):
        conversion = self.table.get(name, ([], 0, 0, 0, 0))
        params, optional, empty, environ, nocontent = conversion
        if empty:
            self.write("e\n")
        elif nocontent:
            empty = 1
        return params, optional, empty, environ


class NewConversion(BaseConversion):
    def __init__(self, ifp, ofp, table={}):
        BaseConversion.__init__(self, ifp, ofp, table)
        self.discards = []

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
                entry = self.get_entry(macroname)
                if entry.verbatim:
                    # magic case!
                    pos = string.find(line, "\\end{%s}" % macroname)
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
                if entry.outputname:
                    if entry.empty:
                        self.write("e\n")
                    self.push_output(self.ofp)
                else:
                    self.push_output(StringIO.StringIO())
                #
                params, optional, empty, environ = self.start_macro(macroname)
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
                            if m:
                                line = line[m.end():]
                                self.dump_attr(pentry, m.group(1))
                        elif pentry.text:
                            # value supplied by conversion spec:
                            self.dump_attr(pentry, pentry.text)
                        else:
                            m = _parameter_rx.match(line)
                            if not m:
                                raise LaTeXFormatError(
                                    "could not extract parameter %s for %s: %s"
                                    % (pentry.name, macroname, `line[:100]`))
                            self.dump_attr(pentry, m.group(1))
##                            if entry.name == "label":
##                                sys.stderr.write("[%s]" % m.group(1))
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
                    elif pentry.type == "text":
                        if pentry.text:
                            if entry.outputname and not opened:
                                opened = 1
                                stack.append(entry.name)
                                self.write("(%s\n" % entry.outputname)
                            self.write("-%s\n" % encode(pentry.text))
                if entry.outputname:
                    if not opened:
                        self.write("(%s\n" % entry.outputname)
                        stack.append(entry.name)
                    if not implied_content:
                        self.write(")%s\n" % entry.outputname)
                        stack.pop()
                self.pop_output()
                continue
            if line[0] == endchar and not stack:
                self.line = line[1:]
                return self.line
            if line[0] == "}":
                # end of macro or group
                macroname = stack[-1]
                if macroname:
                    conversion = self.table.get(macroname)
                    if conversion.outputname:
                        # otherwise, it was just a bare group
                        self.write(")%s\n" % conversion.outputname)
                del stack[-1]
                line = line[1:]
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
                                   + string.join(stack, ", "))
        # otherwise we just ran out of input here...

    def start_macro(self, name):
        conversion = self.get_entry(name)
        parameters = conversion.parameters
        optional = parameters and parameters[0].optional
##         empty = not len(parameters)
##         if empty:
##             self.write("e\n")
##         elif conversion.empty:
##             empty = 1
        return parameters, optional, conversion.empty, conversion.environment

    def get_entry(self, name):
        entry = self.table.get(name)
        if entry is None:
            self.err_write("get_entry(%s) failing; building default entry!"
                           % `name`)
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


def old_convert(ifp, ofp, table={}, discards=(), autoclosing=()):
    c = Conversion(ifp, ofp, table, discards, autoclosing)
    try:
        c.convert()
    except IOError, (err, msg):
        if err != errno.EPIPE:
            raise


def new_convert(ifp, ofp, table={}, discards=(), autoclosing=()):
    c = NewConversion(ifp, ofp, table)
    try:
        c.convert()
    except IOError, (err, msg):
        if err != errno.EPIPE:
            raise


def skip_white(line):
    while line and line[0] in " %\n\t\r":
        line = string.lstrip(line[1:])
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


class TableParser(XMLParser):
    def __init__(self):
        self.__table = {}
        self.__current = None
        self.__buffer = ''
        XMLParser.__init__(self)

    def get_table(self):
        for entry in self.__table.values():
            if entry.environment and not entry.has_content:
                p = Parameter("content")
                p.implied = 1
                entry.parameters.append(p)
                entry.has_content = 1
        return self.__table

    def start_environment(self, attrs):
        name = attrs["name"]
        self.__current = TableEntry(name, environment=1)
        self.__current.verbatim = attrs.get("verbatim") == "yes"
        if attrs.has_key("outputname"):
            self.__current.outputname = attrs.get("outputname")
        self.__current.endcloses = string.split(attrs.get("endcloses", ""))
    def end_environment(self):
        self.end_macro()

    def start_macro(self, attrs):
        name = attrs["name"]
        self.__current = TableEntry(name)
        self.__current.closes = string.split(attrs.get("closes", ""))
        if attrs.has_key("outputname"):
            self.__current.outputname = attrs.get("outputname")
    def end_macro(self):
##        if self.__current.parameters and not self.__current.outputname:
##            raise ValueError, "markup with parameters must have an output name"
        self.__table[self.__current.name] = self.__current
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
        self.__buffer = ''
    def end_text(self):
        p = Parameter("text")
        p.text = self.__buffer
        self.__current.parameters.append(p)

    def handle_data(self, data):
        self.__buffer = self.__buffer + data


def load_table(fp):
    parser = TableParser()
    parser.feed(fp.read())
    parser.close()
    return parser.get_table()


def main():
    global DEBUG
    #
    convert = new_convert
    newstyle = 1
    opts, args = getopt.getopt(sys.argv[1:], "Dn", ["debug", "new"])
    for opt, arg in opts:
        if opt in ("-n", "--new"):
            convert = new_convert
            newstyle = 1
        elif opt in ("-o", "--old"):
            convert = old_convert
            newstyle = 0
        elif opt in ("-D", "--debug"):
            DEBUG = DEBUG + 1
    if len(args) == 0:
        ifp = sys.stdin
        ofp = sys.stdout
    elif len(args) == 1:
        ifp = open(args)
        ofp = sys.stdout
    elif len(args) == 2:
        ifp = open(args[0])
        ofp = open(args[1], "w")
    else:
        usage()
        sys.exit(2)
    table = {
        # entries have the form:
        # name: ([attribute names], is1stOptional, isEmpty, isEnv, nocontent)
        # attribute names can be:
        #   "string" -- normal attribute
        #   ("string",) -- sub-element with content of macro; like for \section
        #   ["string"] -- sub-element
        "bifuncindex": (["name"], 0, 1, 0, 0),
        "cfuncdesc": (["type", "name", ("args",)], 0, 0, 1, 0),
        "chapter": ([("title",)], 0, 0, 0, 0),
        "chapter*": ([("title",)], 0, 0, 0, 0),
        "classdesc": (["name", ("args",)], 0, 0, 1, 0),
        "ctypedesc": (["name"], 0, 0, 1, 0),
        "cvardesc":  (["type", "name"], 0, 0, 1, 0),
        "datadesc":  (["name"], 0, 0, 1, 0),
        "declaremodule": (["id", "type", "name"], 1, 1, 0, 0),
        "deprecated": (["release"], 0, 0, 0, 0),
        "documentclass": (["classname"], 0, 1, 0, 0),
        "excdesc": (["name"], 0, 0, 1, 0),
        "funcdesc": (["name", ("args",)], 0, 0, 1, 0),
        "funcdescni": (["name", ("args",)], 0, 0, 1, 0),
        "funcline": (["name"], 0, 0, 0, 0),
        "funclineni": (["name"], 0, 0, 0, 0),
        "geq": ([], 0, 1, 0, 0),
        "hline": ([], 0, 1, 0, 0),
        "include": (["source"], 0, 1, 0, 0),
        "indexii": (["ie1", "ie2"], 0, 1, 0, 0),
        "indexiii": (["ie1", "ie2", "ie3"], 0, 1, 0, 0),
        "indexiv": (["ie1", "ie2", "ie3", "ie4"], 0, 1, 0, 0),
        "indexname": ([], 0, 0, 0, 0),
        "input": (["source"], 0, 1, 0, 0),
        "item": ([("leader",)], 1, 0, 0, 0),
        "label": (["id"], 0, 1, 0, 0),
        "labelwidth": ([], 0, 1, 0, 0),
        "large": ([], 0, 1, 0, 0),
        "LaTeX": ([], 0, 1, 0, 0),
        "leftmargin": ([], 0, 1, 0, 0),
        "leq": ([], 0, 1, 0, 0),
        "lineii": ([["entry"], ["entry"]], 0, 0, 0, 1),
        "lineiii": ([["entry"], ["entry"], ["entry"]], 0, 0, 0, 1),
        "lineiv": ([["entry"], ["entry"], ["entry"], ["entry"]], 0, 0, 0, 1),
        "localmoduletable": ([], 0, 1, 0, 0),
        "makeindex": ([], 0, 1, 0, 0), 
        "makemodindex": ([], 0, 1, 0, 0), 
        "maketitle": ([], 0, 1, 0, 0),
        "manpage": (["name", "section"], 0, 1, 0, 0),
        "memberdesc": (["class", "name"], 1, 0, 1, 0),
        "memberdescni": (["class", "name"], 1, 0, 1, 0),
        "methoddesc": (["class", "name", ("args",)], 1, 0, 1, 0),
        "methoddescni": (["class", "name", ("args",)], 1, 0, 1, 0),
        "methodline": (["class", "name"], 1, 0, 0, 0),
        "methodlineni": (["class", "name"], 1, 0, 0, 0),
        "moduleauthor": (["name", "email"], 0, 1, 0, 0),
        "opcodedesc": (["name", "var"], 0, 0, 1, 0),
        "par": ([], 0, 1, 0, 0),
        "paragraph": ([("title",)], 0, 0, 0, 0),
        "refbimodindex": (["name"], 0, 1, 0, 0),
        "refexmodindex": (["name"], 0, 1, 0, 0),
        "refmodindex": (["name"], 0, 1, 0, 0),
        "refstmodindex": (["name"], 0, 1, 0, 0),
        "refmodule": (["ref"], 1, 0, 0, 0),
        "renewcommand": (["macro"], 0, 0, 0, 0),
        "rfc": (["num"], 0, 1, 0, 0),
        "section": ([("title",)], 0, 0, 0, 0),
        "sectionauthor": (["name", "email"], 0, 1, 0, 0),
        "seemodule": (["ref", "name"], 1, 0, 0, 0),
        "stindex": (["type"], 0, 1, 0, 0),
        "subparagraph": ([("title",)], 0, 0, 0, 0),
        "subsection": ([("title",)], 0, 0, 0, 0),
        "subsubsection": ([("title",)], 0, 0, 0, 0),
        "list": (["bullet", "init"], 0, 0, 1, 0),
        "tableii": (["colspec", "style",
                     ["entry"], ["entry"]], 0, 0, 1, 0),
        "tableiii": (["colspec", "style",
                      ["entry"], ["entry"], ["entry"]], 0, 0, 1, 0),
        "tableiv": (["colspec", "style",
                     ["entry"], ["entry"], ["entry"], ["entry"]], 0, 0, 1, 0),
        "version": ([], 0, 1, 0, 0),
        "versionadded": (["version"], 0, 1, 0, 0),
        "versionchanged": (["version"], 0, 1, 0, 0),
        "withsubitem": (["text"], 0, 0, 0, 0),
        #
        "ABC": ([], 0, 1, 0, 0),
        "ASCII": ([], 0, 1, 0, 0),
        "C": ([], 0, 1, 0, 0),
        "Cpp": ([], 0, 1, 0, 0),
        "EOF": ([], 0, 1, 0, 0),
        "e": ([], 0, 1, 0, 0),
        "ldots": ([], 0, 1, 0, 0),
        "NULL": ([], 0, 1, 0, 0),
        "POSIX": ([], 0, 1, 0, 0),
        "UNIX": ([], 0, 1, 0, 0),
        #
        # Things that will actually be going away!
        #
        "appendix": ([], 0, 1, 0, 0),
        "catcode": ([], 0, 1, 0, 0),
        "fi": ([], 0, 1, 0, 0),
        "ifhtml": ([], 0, 1, 0, 0),
        "makeindex": ([], 0, 1, 0, 0),
        "makemodindex": ([], 0, 1, 0, 0),
        "maketitle": ([], 0, 1, 0, 0),
        "noindent": ([], 0, 1, 0, 0),
        "protect": ([], 0, 1, 0, 0),
        "tableofcontents": ([], 0, 1, 0, 0),
        }
    if newstyle:
        table = load_table(open(os.path.join(sys.path[0], 'conversion.xml')))
    convert(ifp, ofp, table,
            discards=["fi", "ifhtml", "makeindex", "makemodindex", "maketitle",
                      "noindent", "tableofcontents"],
            autoclosing=["chapter", "section", "subsection", "subsubsection",
                         "paragraph", "subparagraph", ])


if __name__ == "__main__":
    main()
