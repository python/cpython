"""A parser for HTML."""

# This file is based on sgmllib.py, but the API is slightly different.

# XXX There should be a way to distinguish between PCDATA (parsed
# character data -- the normal case), RCDATA (replaceable character
# data -- only char and entity references and end tags are special)
# and CDATA (character data -- only end tags are special).


import re
import string

# Regular expressions used for parsing

interesting_normal = re.compile('[&<]')
interesting_cdata = re.compile(r'<(/|\Z)')
incomplete = re.compile('&([a-zA-Z][-.a-zA-Z0-9]*|#[0-9]*)?')

entityref = re.compile('&([a-zA-Z][-.a-zA-Z0-9]*)[^a-zA-Z0-9]')
charref = re.compile('&#([0-9]+)[^0-9]')

starttagopen = re.compile('<[a-zA-Z]')
piopen = re.compile(r'<\?')
piclose = re.compile('>')
endtagopen = re.compile('</')
declopen = re.compile('<!')
special = re.compile('<![^<>]*>')
commentopen = re.compile('<!--')
commentclose = re.compile(r'--\s*>')
tagfind = re.compile('[a-zA-Z][-.a-zA-Z0-9:_]*')
attrfind = re.compile(
    r'\s*([a-zA-Z_][-.:a-zA-Z_0-9]*)(\s*=\s*'
    r'(\'[^\']*\'|"[^"]*"|[-a-zA-Z0-9./:;+*%?!&$\(\)_#=~]*))?')

locatestarttagend = re.compile(r"""
  <[a-zA-Z][-.a-zA-Z0-9:_]*          # tag name
  (?:\s+                             # whitespace before attribute name
    (?:[a-zA-Z_][-.:a-zA-Z0-9_]*     # attribute name
      (?:\s*=\s*                     # value indicator
        (?:'[^']*'                   # LITA-enclosed value
          |\"[^\"]*\"                # LIT-enclosed value
          |[^'\">\s]+                # bare value
         )
       )?
     )
   )*
  \s*                                # trailing whitespace
""", re.VERBOSE)
endstarttag = re.compile(r"\s*/?>")
endendtag = re.compile('>')
endtagfind = re.compile('</\s*([a-zA-Z][-.a-zA-Z0-9:_]*)\s*>')

declname = re.compile(r'[a-zA-Z][-_.a-zA-Z0-9]*\s*')
declstringlit = re.compile(r'(\'[^\']*\'|"[^"]*")\s*')


class HTMLParseError(Exception):
    """Exception raised for all parse errors."""

    def __init__(self, msg, position=(None, None)):
        assert msg
        self.msg = msg
        self.lineno = position[0]
        self.offset = position[1]

    def __str__(self):
        result = self.msg
        if self.lineno is not None:
            result = result + ", at line %d" % self.lineno
        if self.offset is not None:
            result = result + ", column %d" % (self.offset + 1)
        return result


# HTML parser class -- find tags and call handler functions.
# Usage: p = HTMLParser(); p.feed(data); ...; p.close().
# The dtd is defined by deriving a class which defines methods
# with special names to handle tags: start_foo and end_foo to handle
# <foo> and </foo>, respectively, or do_foo to handle <foo> by itself.
# (Tags are converted to lower case for this purpose.)  The data
# between tags is passed to the parser by calling self.handle_data()
# with some data as argument (the data may be split up in arbitrary
# chunks).  Entity references are passed by calling
# self.handle_entityref() with the entity reference as argument.

class HTMLParser:

    CDATA_CONTENT_ELEMENTS = ("script", "style")


    # Interface -- initialize and reset this instance
    def __init__(self):
        self.reset()

    # Interface -- reset this instance.  Loses all unprocessed data
    def reset(self):
        self.rawdata = ''
        self.stack = []
        self.lasttag = '???'
        self.lineno = 1
        self.offset = 0
        self.interesting = interesting_normal

    # Interface -- feed some data to the parser.  Call this as
    # often as you want, with as little or as much text as you
    # want (may include '\n').  (This just saves the text, all the
    # processing is done by goahead().)
    def feed(self, data):
        self.rawdata = self.rawdata + data
        self.goahead(0)

    # Interface -- handle the remaining data
    def close(self):
        self.goahead(1)

    # Internal -- update line number and offset.  This should be
    # called for each piece of data exactly once, in order -- in other
    # words the concatenation of all the input strings to this
    # function should be exactly the entire input.
    def updatepos(self, i, j):
        if i >= j:
            return j
        rawdata = self.rawdata
        nlines = string.count(rawdata, "\n", i, j)
        if nlines:
            self.lineno = self.lineno + nlines
            pos = string.rindex(rawdata, "\n", i, j) # Should not fail
            self.offset = j-(pos+1)
        else:
            self.offset = self.offset + j-i
        return j

    # Interface -- return current line number and offset.
    def getpos(self):
        return self.lineno, self.offset

    __starttag_text = None

    # Interface -- return full source of start tag: "<...>"
    def get_starttag_text(self):
        return self.__starttag_text

    def set_cdata_mode(self):
        self.interesting = interesting_cdata

    def clear_cdata_mode(self):
        self.interesting = interesting_normal

    # Internal -- handle data as far as reasonable.  May leave state
    # and data to be processed by a subsequent call.  If 'end' is
    # true, force handling all data as if followed by EOF marker.
    def goahead(self, end):
        rawdata = self.rawdata
        i = 0
        n = len(rawdata)
        while i < n:
            match = self.interesting.search(rawdata, i) # < or &
            if match:
                j = match.start()
            else:
                j = n
            if i < j: self.handle_data(rawdata[i:j])
            i = self.updatepos(i, j)
            if i == n: break
            if rawdata[i] == '<':
                if starttagopen.match(rawdata, i): # < + letter
                    k = self.parse_starttag(i)
                elif endtagopen.match(rawdata, i): # </
                    k = self.parse_endtag(i)
                    if k >= 0:
                        self.clear_cdata_mode()
                elif commentopen.match(rawdata, i): # <!--
                    k = self.parse_comment(i)
                elif piopen.match(rawdata, i): # <?
                    k = self.parse_pi(i)
                elif declopen.match(rawdata, i): # <!
                    k = self.parse_declaration(i)
                else:
                    if i < n-1:
                        raise HTMLParseError(
                            "invalid '<' construct: %s" % `rawdata[i:i+2]`,
                            self.getpos())
                    k = -1
                if k < 0:
                    if end:
                        raise HTMLParseError("EOF in middle of construct",
                                             self.getpos())
                    break
                i = self.updatepos(i, k)
            elif rawdata[i] == '&':
                match = charref.match(rawdata, i)
                if match:
                    name = match.group(1)
                    self.handle_charref(name)
                    k = match.end()
                    if rawdata[k-1] != ';':
                        k = k-1
                    i = self.updatepos(i, k)
                    continue
                match = entityref.match(rawdata, i)
                if match:
                    name = match.group(1)
                    self.handle_entityref(name)
                    k = match.end()
                    if rawdata[k-1] != ';':
                        k = k-1
                    i = self.updatepos(i, k)
                    continue
                if incomplete.match(rawdata, i):
                    if end:
                        raise HTMLParseError(
                            "EOF in middle of entity or char ref",
                            self.getpos())
                    return -1 # incomplete
                raise HTMLParseError("'&' not part of entity or char ref",
                                     self.getpos())
            else:
                assert 0, "interesting.search() lied"
        # end while
        if end and i < n:
            self.handle_data(rawdata[i:n])
            i = self.updatepos(i, n)
        self.rawdata = rawdata[i:]

    # Internal -- parse comment, return end or -1 if not terminated
    def parse_comment(self, i):
        rawdata = self.rawdata
        assert rawdata[i:i+4] == '<!--', 'unexpected call to parse_comment()'
        match = commentclose.search(rawdata, i+4)
        if not match:
            return -1
        j = match.start()
        self.handle_comment(rawdata[i+4: j])
        j = match.end()
        return j

    # Internal -- parse declaration.
    def parse_declaration(self, i):
        # This is some sort of declaration; in "HTML as
        # deployed," this should only be the document type
        # declaration ("<!DOCTYPE html...>").
        rawdata = self.rawdata
        j = i + 2
        assert rawdata[i:j] == "<!", "unexpected call to parse_declaration"
        if rawdata[j:j+1] in ("-", ""):
            # Start of comment followed by buffer boundary,
            # or just a buffer boundary.
            return -1
        # in practice, this should look like: ((name|stringlit) S*)+ '>'
        n = len(rawdata)
        while j < n:
            c = rawdata[j]
            if c == ">":
                # end of declaration syntax
                self.handle_decl(rawdata[i+2:j])
                return j + 1
            if c in "\"'":
                m = declstringlit.match(rawdata, j)
                if not m:
                    return -1 # incomplete
                j = m.end()
            elif c in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ":
                m = declname.match(rawdata, j)
                if not m:
                    return -1 # incomplete
                j = m.end()
            else:
                raise HTMLParseError(
                    "unexpected char in declaration: %s" % `rawdata[j]`,
                    self.getpos())
        return -1 # incomplete

    # Internal -- parse processing instr, return end or -1 if not terminated
    def parse_pi(self, i):
        rawdata = self.rawdata
        assert rawdata[i:i+2] == '<?', 'unexpected call to parse_pi()'
        match = piclose.search(rawdata, i+2) # >
        if not match:
            return -1
        j = match.start()
        self.handle_pi(rawdata[i+2: j])
        j = match.end()
        return j

    # Internal -- handle starttag, return end or -1 if not terminated
    def parse_starttag(self, i):
        self.__starttag_text = None
        endpos = self.check_for_whole_start_tag(i)
        if endpos < 0:
            return endpos
        rawdata = self.rawdata
        self.__starttag_text = rawdata[i:endpos]

        # Now parse the data between i+1 and j into a tag and attrs
        attrs = []
        match = tagfind.match(rawdata, i+1)
        assert match, 'unexpected call to parse_starttag()'
        k = match.end()
        self.lasttag = tag = string.lower(rawdata[i+1:k])

        while k < endpos:
            m = attrfind.match(rawdata, k)
            if not m:
                break
            attrname, rest, attrvalue = m.group(1, 2, 3)
            if not rest:
                attrvalue = None
            elif attrvalue[:1] == '\'' == attrvalue[-1:] or \
                 attrvalue[:1] == '"' == attrvalue[-1:]:
                attrvalue = attrvalue[1:-1]
                attrvalue = self.unescape(attrvalue)
            attrs.append((string.lower(attrname), attrvalue))
            k = m.end()

        end = string.strip(rawdata[k:endpos])
        if end not in (">", "/>"):
            lineno, offset = self.getpos()
            if "\n" in self.__starttag_text:
                lineno = lineno + string.count(self.__starttag_text, "\n")
                offset = len(self.__starttag_text) \
                         - string.rfind(self.__starttag_text, "\n")
            else:
                offset = offset + len(self.__starttag_text)
            raise HTMLParseError("junk characters in start tag: %s"
                                 % `rawdata[k:endpos][:20]`,
                                 (lineno, offset))
        if end[-2:] == '/>':
            # XHTML-style empty tag: <span attr="value" />
            self.handle_startendtag(tag, attrs)
        else:
            self.handle_starttag(tag, attrs)
            if tag in self.CDATA_CONTENT_ELEMENTS:
                self.set_cdata_mode()
        return endpos

    # Internal -- check to see if we have a complete starttag; return end
    # or -1 if incomplete.
    def check_for_whole_start_tag(self, i):
        rawdata = self.rawdata
        m = locatestarttagend.match(rawdata, i)
        if m:
            j = m.end()
            next = rawdata[j:j+1]
            if next == ">":
                return j + 1
            if next == "/":
                s = rawdata[j:j+2]
                if s == "/>":
                    return j + 2
                if s == "/":
                    # buffer boundary
                    return -1
                # else bogus input
                self.updatepos(i, j + 1)
                raise HTMLParseError("malformed empty start tag",
                                     self.getpos())
            if next == "":
                # end of input
                return -1
            if next in ("abcdefghijklmnopqrstuvwxyz=/"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
                # end of input in or before attribute value, or we have the
                # '/' from a '/>' ending
                return -1
            self.updatepos(i, j)
            raise HTMLParseError("malformed start tag", self.getpos())
        raise AssertionError("we should not gt here!")

    # Internal -- parse endtag, return end or -1 if incomplete
    def parse_endtag(self, i):
        rawdata = self.rawdata
        assert rawdata[i:i+2] == "</", "unexpected call to parse_endtag"
        match = endendtag.search(rawdata, i+1) # >
        if not match:
            return -1
        j = match.end()
        match = endtagfind.match(rawdata, i) # </ + tag + >
        if not match:
            raise HTMLParseError("bad end tag: %s" % `rawdata[i:j]`,
                                 self.getpos())
        tag = match.group(1)
        self.handle_endtag(string.lower(tag))
        return j

    # Overridable -- finish processing of start+end tag: <tag.../>
    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)
        self.handle_endtag(tag)

    # Overridable -- handle start tag
    def handle_starttag(self, tag, attrs):
        pass

    # Overridable -- handle end tag
    def handle_endtag(self, tag):
        pass

    # Overridable -- handle character reference
    def handle_charref(self, name):
        pass

    # Overridable -- handle entity reference
    def handle_entityref(self, name):
        pass

    # Overridable -- handle data
    def handle_data(self, data):
        pass

    # Overridable -- handle comment
    def handle_comment(self, data):
        pass

    # Overridable -- handle declaration
    def handle_decl(self, decl):
        pass

    # Overridable -- handle processing instruction
    def handle_pi(self, data):
        pass

    # Internal -- helper to remove special character quoting
    def unescape(self, s):
        if '&' not in s:
            return s
        s = string.replace(s, "&lt;", "<")
        s = string.replace(s, "&gt;", ">")
        s = string.replace(s, "&apos;", "'")
        s = string.replace(s, "&quot;", '"')
        s = string.replace(s, "&amp;", "&") # Must be last
        return s
