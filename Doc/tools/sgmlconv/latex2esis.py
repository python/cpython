#! /usr/bin/env python

"""Generate ESIS events based on a LaTeX source document and configuration
data.


"""
__version__ = '$Revision$'

import errno
import re
import string
import StringIO
import sys


class Error(Exception):
    pass

class LaTeXFormatError(Error):
    pass


_begin_env_rx = re.compile(r"[\\]begin{([^}]*)}")
_end_env_rx = re.compile(r"[\\]end{([^}]*)}")
_begin_macro_rx = re.compile("[\\\\]([a-zA-Z]+[*]?)({|\\s*\n?)")
_comment_rx = re.compile("%+ ?(.*)\n")
_text_rx = re.compile(r"[^]%\\{}]+")
_optional_rx = re.compile(r"\s*[[]([^]]*)[]]")
_parameter_rx = re.compile("[ \n]*{([^}]*)}")
_token_rx = re.compile(r"[a-zA-Z][a-zA-Z0-9.-]*$")
_start_group_rx = re.compile("[ \n]*{")
_start_optional_rx = re.compile("[ \n]*[[]")


_charmap = {}
for c in map(chr, range(256)):
    _charmap[c] = c
_charmap["\n"] = r"\n"
_charmap["\\"] = r"\\"
del c

def encode(s):
    return string.join(map(_charmap.get, s), '')


ESCAPED_CHARS = "$%#^ {}&"


def subconvert(line, ofp, table, discards, autoclosing, knownempty,
               endchar=None):
    stack = []
    while line:
        if line[0] == endchar and not stack:
            return line[1:]
        m = _comment_rx.match(line)
        if m:
            text = m.group(1)
            if text:
                ofp.write("(COMMENT\n")
                ofp.write("- %s \n" % encode(text))
                ofp.write(")COMMENT\n")
                ofp.write("-\\n\n")
            else:
                ofp.write("-\\n\n")
            line = line[m.end():]
            continue
        m = _begin_env_rx.match(line)
        if m:
            # re-write to use the macro handler
            line = r"\%s %s" % (m.group(1), line[m.end():])
            continue
        m =_end_env_rx.match(line)
        if m:
            # end of environment
            envname = m.group(1)
            if envname == "document":
                # special magic
                for n in stack[1:]:
                    if n not in autoclosing:
                        raise LaTeXFormatError("open element on stack: " + `n`)
                # should be more careful, but this is easier to code:
                stack = []
                ofp.write(")document\n")
            elif envname == stack[-1]:
                ofp.write(")%s\n" % envname)
                del stack[-1]
            else:
                raise LaTeXFormatError("environment close doesn't match")
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
                ofp.write("(verbatim\n")
                ofp.write("-%s\n" % encode(text))
                ofp.write(")verbatim\n")
                line = line[pos + len("\\end{verbatim}"):]
                continue
            numbered = 1
            if macroname[-1] == "*":
                macroname = macroname[:-1]
                numbered = 0
            if macroname in autoclosing and macroname in stack:
                while stack[-1] != macroname:
                    if stack[-1] and stack[-1] not in discards:
                        ofp.write(")%s\n-\\n\n" % stack[-1])
                    del stack[-1]
                if macroname not in discards:
                    ofp.write("-\\n\n)%s\n-\\n\n" % macroname)
                del stack[-1]
            real_ofp = ofp
            if macroname in discards:
                ofp = StringIO.StringIO()
            #
            conversion = table.get(macroname, ([], 0, 0))
            params, optional, empty = conversion
            empty = empty or knownempty(macroname)
            if empty:
                ofp.write("e\n")
            if not numbered:
                ofp.write("Anumbered TOKEN no\n")
            # rip off the macroname
            if params:
                if optional and len(params) == 1:
                    line = line = line[m.end():]
                else:
                    line = line[m.end(1):]
            elif empty:
                line = line[m.end(1):]
            else:
                line = line[m.end():]
            #
            # Very ugly special case to deal with \item[].  The catch is that
            # this needs to occur outside the for loop that handles attribute
            # parsing so we can 'continue' the outer loop.
            #
            if optional and type(params[0]) is type(()):
                # the attribute name isn't used in this special case
                stack.append(macroname)
                ofp.write("(%s\n" % macroname)
                m = _start_optional_rx.match(line)
                if m:
                    line = line[m.end():]
                    line = subconvert(line, ofp, table, discards,
                                      autoclosing, knownempty, endchar="]")
                line = "}" + line
                continue
            # handle attribute mappings here:
            for attrname in params:
                if optional:
                    optional = 0
                    if type(attrname) is type(""):
                        m = _optional_rx.match(line)
                        if m:
                            line = line[m.end():]
                            ofp.write("A%s TOKEN %s\n"
                                      % (attrname, encode(m.group(1))))
                elif type(attrname) is type(()):
                    # This is a sub-element; but don't place the
                    # element we found on the stack (\section-like)
                    stack.append(macroname)
                    ofp.write("(%s\n" % macroname)
                    macroname = attrname[0]
                    m = _start_group_rx.match(line)
                    if m:
                        line = line[m.end():]
                elif type(attrname) is type([]):
                    # A normal subelement.
                    attrname = attrname[0]
                    stack.append(macroname)
                    stack.append(attrname)
                    ofp.write("(%s\n" % macroname)
                    macroname = attrname
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
                    ofp.write("A%s %s %s\n"
                              % (attrname, dtype, encode(value)))
                    line = line[m.end():]
            stack.append(macroname)
            ofp.write("(%s\n" % macroname)
            if empty:
                line = "}" + line
            ofp = real_ofp
            continue
        if line[0] == "}":
            # end of macro
            macroname = stack[-1]
            conversion = table.get(macroname)
            if macroname \
               and macroname not in discards \
               and type(conversion) is not type(""):
                # otherwise, it was just a bare group
                ofp.write(")%s\n" % stack[-1])
            del stack[-1]
            line = line[1:]
            continue
        if line[0] == "{":
            stack.append("")
            line = line[1:]
            continue
        if line[0] == "\\" and line[1] in ESCAPED_CHARS:
            ofp.write("-%s\n" % encode(line[1]))
            line = line[2:]
            continue
        if line[:2] == r"\\":
            ofp.write("(BREAK\n)BREAK\n")
            line = line[2:]
            continue
        m = _text_rx.match(line)
        if m:
            text = encode(m.group())
            ofp.write("-%s\n" % text)
            line = line[m.end():]
            continue
        # special case because of \item[]
        if line[0] == "]":
            ofp.write("-]\n")
            line = line[1:]
            continue
        # avoid infinite loops
        extra = ""
        if len(line) > 100:
            extra = "..."
        raise LaTeXFormatError("could not identify markup: %s%s"
                               % (`line[:100]`, extra))


def convert(ifp, ofp, table={}, discards=(), autoclosing=(), knownempties=()):
    d = {}
    for gi in knownempties:
        d[gi] = gi
    try:
        subconvert(ifp.read(), ofp, table, discards, autoclosing, d.has_key)
    except IOError, (err, msg):
        if err != errno.EPIPE:
            raise


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
        # entries are name
        #          -> ([list of attribute names], first_is_optional, empty)
        "cfuncdesc": (["type", "name", ("args",)], 0, 0),
        "chapter": ([("title",)], 0, 0),
        "chapter*": ([("title",)], 0, 0),
        "classdesc": (["name", ("constructor-args",)], 0, 0),
        "ctypedesc": (["name"], 0, 0),
        "cvardesc":  (["type", "name"], 0, 0),
        "datadesc":  (["name"], 0, 0),
        "declaremodule": (["id", "type", "name"], 1, 1),
        "deprecated": (["release"], 0, 1),
        "documentclass": (["classname"], 0, 1),
        "excdesc": (["name"], 0, 0),
        "funcdesc": (["name", ("args",)], 0, 0),
        "funcdescni": (["name", ("args",)], 0, 0),
        "indexii": (["ie1", "ie2"], 0, 1),
        "indexiii": (["ie1", "ie2", "ie3"], 0, 1),
        "indexiv": (["ie1", "ie2", "ie3", "ie4"], 0, 1),
        "input": (["source"], 0, 1),
        "item": ([("leader",)], 1, 0),
        "label": (["id"], 0, 1),
        "manpage": (["name", "section"], 0, 1),
        "memberdesc": (["class", "name"], 1, 0),
        "methoddesc": (["class", "name", ("args",)], 1, 0),
        "methoddescni": (["class", "name", ("args",)], 1, 0),
        "opcodedesc": (["name", "var"], 0, 0),
        "par": ([], 0, 1),
        "paragraph": ([("title",)], 0, 0),
        "rfc": (["number"], 0, 1),
        "section": ([("title",)], 0, 0),
        "seemodule": (["ref", "name"], 1, 0),
        "subparagraph": ([("title",)], 0, 0),
        "subsection": ([("title",)], 0, 0),
        "subsubsection": ([("title",)], 0, 0),
        "tableii": (["colspec", "style", "head1", "head2"], 0, 0),
        "tableiii": (["colspec", "style", "head1", "head2", "head3"], 0, 0),
        "tableiv": (["colspec", "style", "head1", "head2", "head3", "head4"],
                    0, 0),
        "versionadded": (["version"], 0, 1),
        "versionchanged": (["version"], 0, 1),
        #
        "ABC": ([], 0, 1),
        "ASCII": ([], 0, 1),
        "C": ([], 0, 1),
        "Cpp": ([], 0, 1),
        "EOF": ([], 0, 1),
        "e": ([], 0, 1),
        "ldots": ([], 0, 1),
        "NULL": ([], 0, 1),
        "POSIX": ([], 0, 1),
        "UNIX": ([], 0, 1),
        #
        # Things that will actually be going away!
        #
        "fi": ([], 0, 1),
        "ifhtml": ([], 0, 1),
        "makeindex": ([], 0, 1),
        "makemodindex": ([], 0, 1),
        "maketitle": ([], 0, 1),
        "noindent": ([], 0, 1),
        "tableofcontents": ([], 0, 1),
        },
            discards=["fi", "ifhtml", "makeindex", "makemodindex", "maketitle",
                      "noindent", "tableofcontents"],
            autoclosing=["chapter", "section", "subsection", "subsubsection",
                         "paragraph", "subparagraph", ],
            knownempties=["appendix",
                          "maketitle", "makeindex", "makemodindex",
                          "localmoduletable"])


if __name__ == "__main__":
    main()
