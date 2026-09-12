"""A parser for HTML and XHTML."""

# This file is based on sgmllib.py, but the API is slightly different.

# XXX There should be a way to distinguish between PCDATA (parsed
# character data -- the normal case), RCDATA (replaceable character
# data -- only char and entity references and end tags are special)
# and CDATA (character data -- only end tags are special).


import re
import _markupbase

from html import unescape
from html.entities import html5 as html5_entities


__all__ = ['HTMLParser']

# Regular expressions used for parsing

interesting_normal = re.compile('[&<]')
incomplete = re.compile('&[a-zA-Z#]')

entityref = re.compile('&([a-zA-Z][-.a-zA-Z0-9]*)[^a-zA-Z0-9]')
charref = re.compile('&#(?:[0-9]+|[xX][0-9a-fA-F]+)[^0-9a-fA-F]')
incomplete_charref = re.compile('&#(?:[0-9]|[xX][0-9a-fA-F])')
attr_charref = re.compile(r'&(#[0-9]+|#[xX][0-9a-fA-F]+|[a-zA-Z][a-zA-Z0-9]*)[;=]?')

starttagopen = re.compile('<[a-zA-Z]')
endtagopen = re.compile('</[a-zA-Z]')
piclose = re.compile('>')
commentclose = re.compile(r'--!?>')
commentabruptclose = re.compile(r'-?>')
# Note:
#  1) if you change tagfind/attrfind remember to update locatetagend too;
#  2) if you change tagfind/attrfind and/or locatetagend the parser will
#     explode, so don't do it.
# see the HTML5 specs section "13.2.5.6 Tag open state",
# "13.2.5.8 Tag name state" and "13.2.5.33 Attribute name state".
# https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
# https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
# https://html.spec.whatwg.org/multipage/parsing.html#attribute-name-state
tagfind_tolerant = re.compile(r'([a-zA-Z][^\t\n\r\f />]*)(?:[\t\n\r\f ]|/(?!>))*')
attrfind_tolerant = re.compile(r"""
  (
    (?<=['"\t\n\r\f /])[^\t\n\r\f />][^\t\n\r\f /=>]*  # attribute name
   )
  ([\t\n\r\f ]*=[\t\n\r\f ]*        # value indicator
    ('[^']*'                        # LITA-enclosed value
    |"[^"]*"                        # LIT-enclosed value
    |(?!['"])[^>\t\n\r\f ]*         # bare value
    )
   )?
  (?:[\t\n\r\f ]|/(?!>))*           # possibly followed by a space
""", re.VERBOSE)
locatetagend = re.compile(r"""
  [a-zA-Z][^\t\n\r\f />]*           # tag name
  [\t\n\r\f /]*                     # optional whitespace before attribute name
  (?:(?<=['"\t\n\r\f /])[^\t\n\r\f />][^\t\n\r\f /=>]*  # attribute name
    (?:[\t\n\r\f ]*=[\t\n\r\f ]*    # value indicator
      (?:'[^']*'                    # LITA-enclosed value
        |"[^"]*"                    # LIT-enclosed value
        |(?!['"])[^>\t\n\r\f ]*     # bare value
       )
     )?
    [\t\n\r\f /]*                   # possibly followed by a space
   )*
   >?
""", re.VERBOSE)
# The following variables are not used, but are temporarily left for
# backward compatibility.
locatestarttagend_tolerant = re.compile(r"""
  <[a-zA-Z][^\t\n\r\f />\x00]*       # tag name
  (?:[\s/]*                          # optional whitespace before attribute name
    (?:(?<=['"\s/])[^\s/>][^\s/=>]*  # attribute name
      (?:\s*=+\s*                    # value indicator
        (?:'[^']*'                   # LITA-enclosed value
          |"[^"]*"                   # LIT-enclosed value
          |(?!['"])[^>\s]*           # bare value
         )
        \s*                          # possibly followed by a space
       )?(?:\s|/(?!>))*
     )*
   )?
  \s*                                # trailing whitespace
""", re.VERBOSE)
endendtag = re.compile('>')
endtagfind = re.compile(r'</\s*([a-zA-Z][-.a-zA-Z0-9:_]*)\s*>')

# The following tables are used for tracking foreign content (the content
# of "svg" and "math" elements).
# See the HTML5 specs section "13.2.6.5 The rules for parsing tokens in
# foreign content".
# https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inforeign

# Start tags (and end tags "br" and "p") which break out of foreign content.
_BREAKOUT_ELEMENTS = frozenset({
    'b', 'big', 'blockquote', 'body', 'br', 'center', 'code', 'dd', 'div',
    'dl', 'dt', 'em', 'embed', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head',
    'hr', 'i', 'img', 'li', 'listing', 'menu', 'meta', 'nobr', 'ol', 'p',
    'pre', 'ruby', 's', 'small', 'span', 'strong', 'strike', 'sub', 'sup',
    'table', 'tt', 'u', 'ul', 'var'})
# HTML elements which are immediately popped from the stack of open elements.
_VOID_ELEMENTS = frozenset({
    'area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
    'hr', 'img', 'input', 'keygen', 'link', 'meta', 'param', 'source',
    'track', 'wbr'})
# Elements in the "special" category, which stop the search for a matching
# end tag in HTML content.
# https://html.spec.whatwg.org/multipage/parsing.html#special
_SPECIAL_ELEMENTS = {
    'html': frozenset({
        'address', 'applet', 'area', 'article', 'aside', 'base', 'basefont',
        'bgsound', 'blockquote', 'body', 'br', 'button', 'caption', 'center',
        'col', 'colgroup', 'dd', 'details', 'dir', 'div', 'dl', 'dt',
        'embed', 'fieldset', 'figcaption', 'figure', 'footer', 'form',
        'frame', 'frameset', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head',
        'header', 'hgroup', 'hr', 'html', 'iframe', 'img', 'input', 'keygen',
        'li', 'link', 'listing', 'main', 'marquee', 'menu', 'meta', 'nav',
        'noembed', 'noframes', 'noscript', 'object', 'ol', 'p', 'param',
        'plaintext', 'pre', 'script', 'search', 'section', 'select',
        'source', 'style', 'summary', 'table', 'tbody', 'td', 'template',
        'textarea', 'tfoot', 'th', 'thead', 'title', 'tr', 'track', 'ul',
        'wbr', 'xmp'}),
    'math': frozenset({'mi', 'mo', 'mn', 'ms', 'mtext', 'annotation-xml'}),
    'svg': frozenset({'foreignobject', 'desc', 'title'}),
}
# https://html.spec.whatwg.org/multipage/parsing.html#mathml-text-integration-point
_MATHML_TEXT_INTEGRATION_POINTS = frozenset({'mi', 'mo', 'mn', 'ms', 'mtext'})
# SVG elements which are HTML integration points.  A MathML annotation-xml
# element is an HTML integration point depending on its "encoding" attribute.
# https://html.spec.whatwg.org/multipage/parsing.html#html-integration-point
_SVG_HTML_INTEGRATION_POINTS = frozenset({'foreignobject', 'desc', 'title'})

# Character reference processing logic specific to attribute values
# See: https://html.spec.whatwg.org/multipage/parsing.html#named-character-reference-state
def _replace_attr_charref(match):
    ref = match.group(0)
    # Numeric / hex char refs must always be unescaped
    if ref.startswith('&#'):
        return unescape(ref)
    # Named character / entity references must only be unescaped
    # if they are an exact match, and they are not followed by an equals sign
    if not ref.endswith('=') and ref[1:] in html5_entities:
        return unescape(ref)
    # Otherwise do not unescape
    return ref

def _unescape_attrvalue(s):
    return attr_charref.sub(_replace_attr_charref, s)


class HTMLParser(_markupbase.ParserBase):
    """Find tags and other markup and call handler functions.

    Usage:
        p = HTMLParser()
        p.feed(data)
        ...
        p.close()

    Start tags are handled by calling self.handle_starttag() or
    self.handle_startendtag(); end tags by self.handle_endtag().  The
    data between tags is passed from the parser to the derived class
    by calling self.handle_data() with the data as argument (the data
    may be split up in arbitrary chunks).  If convert_charrefs is
    True the character references are converted automatically to the
    corresponding Unicode character (and self.handle_data() is no
    longer split in chunks), otherwise they are passed by calling
    self.handle_entityref() or self.handle_charref() with the string
    containing respectively the named or numeric reference as the
    argument.
    """

    # See the HTML5 specs section "13.4 Parsing HTML fragments".
    # https://html.spec.whatwg.org/multipage/parsing.html#parsing-html-fragments
    # CDATA_CONTENT_ELEMENTS are parsed in RAWTEXT mode
    CDATA_CONTENT_ELEMENTS = ("script", "style", "xmp", "iframe", "noembed", "noframes")
    RCDATA_CONTENT_ELEMENTS = ("textarea", "title")

    def __init__(self, *, convert_charrefs=True, scripting=False,
                 support_cdata=None):
        """Initialize and reset this instance.

        If convert_charrefs is true (the default), all character references
        are automatically converted to the corresponding Unicode characters.

        If *scripting* is false (the default), the content of the
        ``noscript`` element is parsed normally; if it's true,
        it's returned as is without being parsed.

        If *support_cdata* is None (the default), a CDATA section
        "<![CDATA[...]]>" is only recognized in foreign content (the
        content of "svg" and "math" elements), where RAWTEXT and RCDATA
        elements (such as "script" or "title") are also parsed as normal
        elements.  If *support_cdata* is true, a CDATA section is
        recognized in any context; if it's false -- in no context.  In
        both cases foreign content is not detected.
        """
        super().__init__()
        self.convert_charrefs = convert_charrefs
        self.scripting = scripting
        self.support_cdata = support_cdata
        self.reset()

    def reset(self):
        """Reset this instance.  Loses all unprocessed data."""
        self.rawdata = ''
        self.lasttag = '???'
        self.interesting = interesting_normal
        self.cdata_elem = None
        self._open_elements = []
        self._support_cdata = bool(self.support_cdata)
        self._escapable = True
        self._pending = []
        self._pending_len = 0
        self._parse_threshold = 1
        super().reset()

    def feed(self, data):
        r"""Feed data to the parser.

        Call this as often as you want, with as little or as much text
        as you want (may include '\n').
        """
        # Accumulate new data in a list and only join and parse it once
        # enough has piled up.  Rescanning an unparsed buffer (e.g. an
        # unterminated tag) and concatenating onto it on every call would
        # both be quadratic in the input size.
        self._pending_len += len(data)
        if self._pending_len < self._parse_threshold:
            self._pending.append(data)
        else:
            if not self._pending:
                self.rawdata += data
            else:
                self._pending.append(data)
                self.rawdata += ''.join(self._pending)
                self._pending.clear()
            self._pending_len = 0
            n = len(self.rawdata)
            self.goahead(0)
            if len(self.rawdata) < n:
                # Some data was parsed; resume on the next call.
                self._parse_threshold = 1
            else:
                # Nothing was parsed; wait until the buffer doubles.
                self._parse_threshold = len(self.rawdata)

    def close(self):
        """Handle any buffered data."""
        if self._pending:
            self.rawdata += ''.join(self._pending)
            self._pending.clear()
            self._pending_len = 0
        self.goahead(1)

    __starttag_text = None

    def get_starttag_text(self):
        """Return full source of start tag: '<...>'."""
        return self.__starttag_text

    def set_cdata_mode(self, elem, *, escapable=False):
        self.cdata_elem = elem.lower()
        self._escapable = escapable
        if self.cdata_elem == 'plaintext':
            self.interesting = re.compile(r'\z')
        elif escapable and not self.convert_charrefs:
            self.interesting = re.compile(r'&|</%s(?=[\t\n\r\f />])' % self.cdata_elem,
                                          re.IGNORECASE|re.ASCII)
        else:
            self.interesting = re.compile(r'</%s(?=[\t\n\r\f />])' % self.cdata_elem,
                                          re.IGNORECASE|re.ASCII)

    def clear_cdata_mode(self):
        self.interesting = interesting_normal
        self.cdata_elem = None
        self._escapable = True

    def _set_support_cdata(self, flag=True):
        """Enable or disable support of the CDATA sections.
        If enabled, "<![CDATA[" starts a CDATA section which ends with "]]>".
        If disabled, "<![CDATA[" starts a bogus comment which ends with ">".

        Calling this method disables automatic detection of foreign
        content, as if the parser was created with support_cdata=flag.
        It can be called in custom handle_starttag() and handle_endtag()
        methods, with value that depends on the adjusted current node.
        See https://html.spec.whatwg.org/multipage/parsing.html#markup-declaration-open-state
        for details.
        """
        self.support_cdata = bool(flag)
        self._open_elements = []
        self._support_cdata = bool(flag)

    # Internal -- update the stack of open elements for a start tag and
    # return whether it was processed by the rules for HTML content.
    # The stack contains (namespace, name, html_integration_point) triples
    # and only tracks foreign elements and their descendants.
    # This approximates the tree construction dispatcher and the rules for
    # parsing tokens in foreign content.
    # https://html.spec.whatwg.org/multipage/parsing.html#tree-construction-dispatcher
    # https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inforeign
    def _handle_starttag_context(self, tag, attrs, selfclosing):
        stack = self._open_elements
        if self._dispatch_html(tag):
            if tag in ('svg', 'math'):
                if not selfclosing:
                    stack.append((tag, tag, False))
            elif stack and not selfclosing and tag not in _VOID_ELEMENTS:
                stack.append(('html', tag, False))
            html = True
        elif (tag in _BREAKOUT_ELEMENTS or
              (tag == 'font' and
               any(name in ('color', 'face', 'size') for name, _ in attrs))):
            self._exit_foreign_content()
            # Reprocess following the rules for HTML content.
            if stack and not selfclosing and tag not in _VOID_ELEMENTS:
                stack.append(('html', tag, False))
            html = True
        else:
            ns = stack[-1][0]
            if not selfclosing:
                if ns == 'svg':
                    html_ip = tag in _SVG_HTML_INTEGRATION_POINTS
                    if tag == 'foreignobject':
                        # The adjusted element name does not match
                        # the lowercased end tag name in HTML content.
                        tag = 'foreignObject'
                else:
                    html_ip = (tag == 'annotation-xml' and
                               self._get_attr(attrs, 'encoding', '').lower()
                               in ('text/html', 'application/xhtml+xml'))
                stack.append((ns, tag, html_ip))
            html = False
        self._support_cdata = bool(stack) and stack[-1][0] != 'html'
        return html

    # Internal -- return whether a start tag is processed by the rules
    # for HTML content.
    def _dispatch_html(self, tag):
        stack = self._open_elements
        if not stack:
            return True
        ns, name, html_ip = stack[-1]
        return (ns == 'html' or
                html_ip or
                (ns == 'math' and name in _MATHML_TEXT_INTEGRATION_POINTS and
                 tag not in ('mglyph', 'malignmark')) or
                (ns == 'math' and name == 'annotation-xml' and tag == 'svg'))

    @staticmethod
    def _get_attr(attrs, name, default=None):
        for attrname, attrvalue in attrs:
            if attrname == name:
                return attrvalue if attrvalue is not None else default
        return default

    # Internal -- pop foreign elements from the stack of open elements
    # until the current node is a MathML text integration point, an HTML
    # integration point, or an element in HTML content.
    def _exit_foreign_content(self):
        stack = self._open_elements
        while stack:
            ns, name, html_ip = stack[-1]
            if (ns == 'html' or html_ip or
                (ns == 'math' and name in _MATHML_TEXT_INTEGRATION_POINTS)):
                break
            stack.pop()

    # Internal -- update the stack of open elements for an end tag.
    def _handle_endtag_context(self, tag):
        stack = self._open_elements
        if stack:
            if stack[-1][0] == 'html':
                self._pop_matching_element(tag)
            elif tag in ('br', 'p'):
                # An end tag "br" or "p" breaks out of foreign content.
                self._exit_foreign_content()
            else:
                # The rules for parsing an end tag in foreign content.
                for i in reversed(range(len(stack))):
                    ns, name, html_ip = stack[i]
                    if ns == 'html':
                        self._pop_matching_element(tag)
                        break
                    if name is not None and name.lower() == tag:
                        del stack[i:]
                        break
        self._support_cdata = bool(stack) and stack[-1][0] != 'html'

    # Internal -- pop the matching element and all elements above it from
    # the stack of open elements.  The search stops at a "special" element.
    # This approximates "any other end tag" in the "in body" insertion mode.
    # https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
    def _pop_matching_element(self, tag):
        stack = self._open_elements
        for i in reversed(range(len(stack))):
            ns, name, html_ip = stack[i]
            if name == tag:
                del stack[i:]
                break
            if name is not None and name.lower() in _SPECIAL_ELEMENTS[ns]:
                break

    # Internal -- handle data as far as reasonable.  May leave state
    # and data to be processed by a subsequent call.  If 'end' is
    # true, force handling all data as if followed by EOF marker.
    def goahead(self, end):
        rawdata = self.rawdata
        i = 0
        n = len(rawdata)
        while i < n:
            if self.convert_charrefs and not self.cdata_elem:
                j = rawdata.find('<', i)
                if j < 0:
                    # if we can't find the next <, either we are at the end
                    # or there's more text incoming.  If the latter is True,
                    # we can't pass the text to handle_data in case we have
                    # a charref cut in half at end.  Try to determine if
                    # this is the case before proceeding by looking for an
                    # & near the end and see if it's followed by a space or ;.
                    amppos = rawdata.rfind('&', max(i, n-34))
                    if (amppos >= 0 and
                        not re.compile(r'[\t\n\r\f ;]').search(rawdata, amppos)):
                        break  # wait till we get all the text
                    j = n
            else:
                match = self.interesting.search(rawdata, i)  # < or &
                if match:
                    j = match.start()
                else:
                    if self.cdata_elem:
                        break
                    j = n
            if i < j:
                if self.convert_charrefs and self._escapable:
                    self.handle_data(unescape(rawdata[i:j]))
                else:
                    self.handle_data(rawdata[i:j])
            i = self.updatepos(i, j)
            if i == n: break
            startswith = rawdata.startswith
            if startswith('<', i):
                if starttagopen.match(rawdata, i): # < + letter
                    k = self.parse_starttag(i)
                elif startswith("</", i):
                    k = self.parse_endtag(i)
                elif startswith("<!--", i):
                    k = self.parse_comment(i)
                elif startswith("<?", i):
                    k = self.parse_pi(i)
                elif startswith("<!", i):
                    k = self.parse_html_declaration(i)
                elif (i + 1) < n or end:
                    self.handle_data("<")
                    k = i + 1
                else:
                    break
                if k < 0:
                    if not end:
                        break
                    if starttagopen.match(rawdata, i):  # < + letter
                        pass
                    elif startswith("</", i):
                        if i + 2 == n:
                            self.handle_data("</")
                        elif endtagopen.match(rawdata, i):  # </ + letter
                            pass
                        else:
                            # bogus comment
                            self.handle_comment(rawdata[i+2:])
                    elif startswith("<!--", i):
                        j = n
                        for suffix in ("--!", "--", "-"):
                            if rawdata.endswith(suffix, i+4):
                                j -= len(suffix)
                                break
                        self.handle_comment(rawdata[i+4:j])
                    elif startswith("<![CDATA[", i) and self._support_cdata:
                        self.unknown_decl(rawdata[i+3:])
                    elif rawdata[i:i+9].lower() == '<!doctype':
                        self.handle_decl(rawdata[i+2:])
                    elif startswith("<!", i):
                        # bogus comment
                        self.handle_comment(rawdata[i+2:])
                    elif startswith("<?", i):
                        self.handle_pi(rawdata[i+2:])
                    else:
                        raise AssertionError("we should not get here!")
                    k = n
                i = self.updatepos(i, k)
            elif startswith("&#", i):
                match = charref.match(rawdata, i)
                if match:
                    name = match.group()[2:-1]
                    self.handle_charref(name)
                    k = match.end()
                    if not startswith(';', k-1):
                        k = k - 1
                    i = self.updatepos(i, k)
                    continue
                match = incomplete_charref.match(rawdata, i)
                if match:
                    if end:
                        self.handle_charref(rawdata[i+2:])
                        i = self.updatepos(i, n)
                        break
                    # incomplete
                    break
                elif i + 3 < n:  # larger than "&#x"
                    # not the end of the buffer, and can't be confused
                    # with some other construct
                    self.handle_data("&#")
                    i = self.updatepos(i, i + 2)
                else:
                    break
            elif startswith('&', i):
                match = entityref.match(rawdata, i)
                if match:
                    name = match.group(1)
                    self.handle_entityref(name)
                    k = match.end()
                    if not startswith(';', k-1):
                        k = k - 1
                    i = self.updatepos(i, k)
                    continue
                match = incomplete.match(rawdata, i)
                if match:
                    if end:
                        self.handle_entityref(rawdata[i+1:])
                        i = self.updatepos(i, n)
                        break
                    # incomplete
                    break
                elif i + 1 < n:
                    # not the end of the buffer, and can't be confused
                    # with some other construct
                    self.handle_data("&")
                    i = self.updatepos(i, i + 1)
                else:
                    break
            else:
                assert 0, "interesting.search() lied"
        # end while
        if end and i < n:
            if self.convert_charrefs and self._escapable:
                self.handle_data(unescape(rawdata[i:n]))
            else:
                self.handle_data(rawdata[i:n])
            i = self.updatepos(i, n)
        self.rawdata = rawdata[i:]

    # Internal -- parse html declarations, return length or -1 if not terminated
    # See w3.org/TR/html5/tokenization.html#markup-declaration-open-state
    # See also parse_declaration in _markupbase
    def parse_html_declaration(self, i):
        rawdata = self.rawdata
        assert rawdata[i:i+2] == '<!', ('unexpected call to '
                                        'parse_html_declaration()')
        if rawdata[i:i+4] == '<!--':
            # this case is actually already handled in goahead()
            return self.parse_comment(i)
        elif rawdata[i:i+9] == '<![CDATA[' and self._support_cdata:
            j = rawdata.find(']]>', i+9)
            if j < 0:
                return -1
            self.unknown_decl(rawdata[i+3: j])
            return j + 3
        elif rawdata[i:i+9].lower() == '<!doctype':
            # find the closing >
            gtpos = rawdata.find('>', i+9)
            if gtpos == -1:
                return -1
            self.handle_decl(rawdata[i+2:gtpos])
            return gtpos+1
        else:
            return self.parse_bogus_comment(i)

    # Internal -- parse comment, return length or -1 if not terminated
    # see https://html.spec.whatwg.org/multipage/parsing.html#comment-start-state
    def parse_comment(self, i, report=True):
        rawdata = self.rawdata
        assert rawdata.startswith('<!--', i), 'unexpected call to parse_comment()'
        # An empty comment is abruptly closed by the first ">" or "->",
        # taking priority over a later "-->" or "--!>" close.
        match = commentabruptclose.match(rawdata, i+4)
        if not match:
            match = commentclose.search(rawdata, i+4)
            if not match:
                return -1
        if report:
            j = match.start()
            self.handle_comment(rawdata[i+4: j])
        return match.end()

    # Internal -- parse bogus comment, return length or -1 if not terminated
    # see https://html.spec.whatwg.org/multipage/parsing.html#bogus-comment-state
    def parse_bogus_comment(self, i, report=1):
        rawdata = self.rawdata
        assert rawdata[i:i+2] in ('<!', '</'), ('unexpected call to '
                                                'parse_bogus_comment()')
        pos = rawdata.find('>', i+2)
        if pos == -1:
            return -1
        if report:
            self.handle_comment(rawdata[i+2:pos])
        return pos + 1

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
        # See the HTML5 specs section "13.2.5.8 Tag name state"
        # https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
        self.__starttag_text = None
        endpos = self.check_for_whole_start_tag(i)
        if endpos < 0:
            return endpos
        rawdata = self.rawdata
        self.__starttag_text = rawdata[i:endpos]

        # Now parse the data between i+1 and j into a tag and attrs
        attrs = []
        match = tagfind_tolerant.match(rawdata, i+1)
        assert match, 'unexpected call to parse_starttag()'
        k = match.end()
        self.lasttag = tag = match.group(1).lower()
        while k < endpos:
            m = attrfind_tolerant.match(rawdata, k)
            if not m:
                break
            attrname, rest, attrvalue = m.group(1, 2, 3)
            if not rest:
                attrvalue = None
            elif attrvalue[:1] == '\'' == attrvalue[-1:] or \
                 attrvalue[:1] == '"' == attrvalue[-1:]:
                attrvalue = attrvalue[1:-1]
            if attrvalue:
                attrvalue = _unescape_attrvalue(attrvalue)
            attrs.append((attrname.lower(), attrvalue))
            k = m.end()

        end = rawdata[k:endpos].strip()
        if end not in (">", "/>"):
            self.handle_data(rawdata[i:endpos])
            return endpos
        if (self.support_cdata is None and
                (self._open_elements or tag in ('svg', 'math'))):
            html = self._handle_starttag_context(tag, attrs, end == "/>")
        else:
            html = True
        if end.endswith('/>'):
            # XHTML-style empty tag: <span attr="value" />
            self.handle_startendtag(tag, attrs)
        else:
            self.handle_starttag(tag, attrs)
            # In foreign content these elements are not special.
            if html:
                if (tag in self.CDATA_CONTENT_ELEMENTS or
                    (self.scripting and tag == "noscript") or
                    tag == "plaintext"):
                    self.set_cdata_mode(tag, escapable=False)
                elif tag in self.RCDATA_CONTENT_ELEMENTS:
                    self.set_cdata_mode(tag, escapable=True)
        return endpos

    # Internal -- check to see if we have a complete starttag; return end
    # or -1 if incomplete.
    def check_for_whole_start_tag(self, i):
        rawdata = self.rawdata
        match = locatetagend.match(rawdata, i+1)
        assert match
        j = match.end()
        if rawdata[j-1] != ">":
            return -1
        return j

    # Internal -- parse endtag, return end or -1 if incomplete
    def parse_endtag(self, i):
        # See the HTML5 specs section "13.2.5.7 End tag open state"
        # https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
        rawdata = self.rawdata
        assert rawdata[i:i+2] == "</", "unexpected call to parse_endtag"
        if rawdata.find('>', i+2) < 0:  # fast check
            return -1
        if not endtagopen.match(rawdata, i):  # </ + letter
            if rawdata[i+2:i+3] == '>':  # </> is ignored
                # "missing-end-tag-name" parser error
                return i+3
            else:
                return self.parse_bogus_comment(i)

        match = locatetagend.match(rawdata, i+2)
        assert match
        j = match.end()
        if rawdata[j-1] != ">":
            return -1

        # find the name: "13.2.5.8 Tag name state"
        # https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
        match = tagfind_tolerant.match(rawdata, i+2)
        assert match
        tag = match.group(1).lower()
        if self.support_cdata is None and self._open_elements:
            self._handle_endtag_context(tag)
        self.handle_endtag(tag)
        self.clear_cdata_mode()
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

    def unknown_decl(self, data):
        pass
