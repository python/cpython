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

import errno
import re
import string
import StringIO
import sys

from esistools import encode
from types import ListType, StringType, TupleType


DEBUG = 0


class Error(Exception):
    pass

class LaTeXFormatError(Error):
    pass


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
    dbgmsg("%s<%s> at %s" % (" "*depth, name, point))

def popping(name, point, depth):
    dbgmsg("%s</%s> at %s" % (" "*depth, name, point))


class Conversion:
    def __init__(self, ifp, ofp, table=None, discards=(), autoclosing=()):
        self.ofp_stack = [ofp]
        self.pop_output()
        self.table = table
        self.discards = discards
        self.autoclosing = autoclosing
        self.line = string.join(map(string.rstrip, ifp.readlines()), "\n")
        self.err_write = sys.stderr.write
        self.preamble = 1

    def push_output(self, ofp):
        self.ofp_stack.append(self.ofp)
        self.ofp = ofp
        self.write = ofp.write

    def pop_output(self):
        self.ofp = self.ofp_stack.pop()
        self.write = self.ofp.write

    def subconvert(self, endchar=None, depth=0):
        stack = []
        line = self.line
        if DEBUG and endchar:
            self.err_write(
                "subconvert(%s)\n  line = %s\n" % (`endchar`, `line[:20]`))
        while line:
            if line[0] == endchar and not stack:
                if DEBUG:
                    self.err_write("subconvert() --> %s\n" % `line[1:21]`)
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
                            raise LaTeXFormatError(
                                "open element on stack: " + `n`)
                    # should be more careful, but this is easier to code:
                    stack = []
                    self.write(")document\n")
                elif stack and envname == stack[-1]:
                    self.write(")%s\n" % envname)
                    del stack[-1]
                    popping(envname, "a", len(stack) + depth)
                else:
                    self.err_write("stack: %s\n" % `stack`)
                    raise LaTeXFormatError(
                        "environment close for %s doesn't match" % envname)
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
                    pushing(macroname, "a", depth + len(stack))
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
                        pushing(macroname, "b", len(stack) + depth)
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
                if DEBUG:
                    self.err_write("subconvert() --> %s\n" % `line[1:21]`)
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

    def convert(self):
        self.subconvert()

    def start_macro(self, name):
        conversion = self.table.get(name, ([], 0, 0, 0, 0))
        params, optional, empty, environ, nocontent = conversion
        if empty:
            self.write("e\n")
        elif nocontent:
            empty = 1
        return params, optional, empty, environ


def convert(ifp, ofp, table={}, discards=(), autoclosing=()):
    c = Conversion(ifp, ofp, table, discards, autoclosing)
    try:
        c.convert()
    except IOError, (err, msg):
        if err != errno.EPIPE:
            raise


def skip_white(line):
    while line and line[0] in " %\n\t":
        line = string.lstrip(line[1:])
    return line


def main():
    if len(sys.argv) == 2:
        ifp = open(sys.argv[1])
        ofp = sys.stdout
    elif len(sys.argv) == 3:
        ifp = open(sys.argv[1])
        ofp = open(sys.argv[2], "w")
    else:
        usage()
        sys.exit(2)
    convert(ifp, ofp, {
        # entries have the form:
        # name: ([attribute names], is1stOptional, isEmpty, isEnv, nocontent)
        # attribute names can be:
        #   "string" -- normal attribute
        #   ("string",) -- sub-element with content of macro; like for \section
        #   ["string"] -- sub-element
        "appendix": ([], 0, 1, 0, 0),
        "bifuncindex": (["name"], 0, 1, 0, 0),
        "catcode": ([], 0, 1, 0, 0),
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
        "fi": ([], 0, 1, 0, 0),
        "ifhtml": ([], 0, 1, 0, 0),
        "makeindex": ([], 0, 1, 0, 0),
        "makemodindex": ([], 0, 1, 0, 0),
        "maketitle": ([], 0, 1, 0, 0),
        "noindent": ([], 0, 1, 0, 0),
        "protect": ([], 0, 1, 0, 0),
        "tableofcontents": ([], 0, 1, 0, 0),
        },
            discards=["fi", "ifhtml", "makeindex", "makemodindex", "maketitle",
                      "noindent", "tableofcontents"],
            autoclosing=["chapter", "section", "subsection", "subsubsection",
                         "paragraph", "subparagraph", ])


if __name__ == "__main__":
    main()
