# A parser for XML, using the derived class as static DTD.
# Author: Sjoerd Mullender.

import re
import string


version = '0.1'

# Regular expressions used for parsing

_S = '[ \t\r\n]+'
_opS = '[ \t\r\n]*'
_Name = '[a-zA-Z_:][-a-zA-Z0-9._:]*'
illegal = re.compile('[^\t\r\n -\176\240-\377]') # illegal chars in content
interesting = re.compile('[]&<]')

amp = re.compile('&')
ref = re.compile('&(' + _Name + '|#[0-9]+|#x[0-9a-fA-F]+)[^-a-zA-Z0-9._:]')
entityref = re.compile('&(?P<name>' + _Name + ')[^-a-zA-Z0-9._:]')
charref = re.compile('&#(?P<char>[0-9]+[^0-9]|x[0-9a-fA-F]+[^0-9a-fA-F])')
space = re.compile(_S + '$')
newline = re.compile('\n')

starttagopen = re.compile('<' + _Name)
endtagopen = re.compile('</')
starttagend = re.compile(_opS + '(?P<slash>/?)>')
endbracket = re.compile(_opS + '>')
tagfind = re.compile(_Name)
cdataopen = re.compile(r'<!\[CDATA\[')
cdataclose = re.compile(r'\]\]>')
# this matches one of the following:
# SYSTEM SystemLiteral
# PUBLIC PubidLiteral SystemLiteral
_SystemLiteral = '(?P<%s>\'[^\']*\'|"[^"]*")'
_PublicLiteral = '(?P<%s>"[-\'()+,./:=?;!*#@$_%% \n\ra-zA-Z0-9]*"|' \
                        "'[-()+,./:=?;!*#@$_%% \n\ra-zA-Z0-9]*')"
_ExternalId = '(?:SYSTEM|' \
                 'PUBLIC'+_S+_PublicLiteral%'pubid'+ \
              ')'+_S+_SystemLiteral%'syslit'
doctype = re.compile('<!DOCTYPE'+_S+'(?P<name>'+_Name+')'
                     '(?:'+_S+_ExternalId+')?'+_opS)
xmldecl = re.compile('<\?xml'+_S+
                     'version'+_opS+'='+_opS+'(?P<version>\'[^\']*\'|"[^"]*")'+
                     '(?:'+_S+'encoding'+_opS+'='+_opS+
                        "(?P<encoding>'[A-Za-z][-A-Za-z0-9._]*'|"
                        '"[A-Za-z][-A-Za-z0-9._]*"))?'
                     '(?:'+_S+'standalone'+_opS+'='+_opS+
                        '(?P<standalone>\'(?:yes|no)\'|"(?:yes|no)"))?'+
                     _opS+'\?>')
procopen = re.compile(r'<\?(?P<proc>' + _Name + ')' + _opS)
procclose = re.compile(_opS + r'\?>')
commentopen = re.compile('<!--')
commentclose = re.compile('-->')
doubledash = re.compile('--')
attrfind = re.compile(
    _S + '(?P<name>' + _Name + ')'
    '(' + _opS + '=' + _opS +
    '(?P<value>\'[^\']*\'|"[^"]*"|[-a-zA-Z0-9.:+*%?!()_#=~]+))')
attrtrans = string.maketrans(' \r\n\t', '    ')


# XML parser base class -- find tags and call handler functions.
# Usage: p = XMLParser(); p.feed(data); ...; p.close().
# The dtd is defined by deriving a class which defines methods with
# special names to handle tags: start_foo and end_foo to handle <foo>
# and </foo>, respectively.  The data between tags is passed to the
# parser by calling self.handle_data() with some data as argument (the
# data may be split up in arbutrary chunks).  Entity references are
# passed by calling self.handle_entityref() with the entity reference
# as argument.

class XMLParser:

    # Interface -- initialize and reset this instance
    def __init__(self, verbose=0):
        self.verbose = verbose
        self.reset()

    # Interface -- reset this instance.  Loses all unprocessed data
    def reset(self):
        self.rawdata = ''
        self.stack = []
        self.nomoretags = 0
        self.literal = 0
        self.lineno = 1
        self.__at_start = 1
        self.__seen_doctype = None
        self.__seen_starttag = 0

    # For derived classes only -- enter literal mode (CDATA) till EOF
    def setnomoretags(self):
        self.nomoretags = self.literal = 1

    # For derived classes only -- enter literal mode (CDATA)
    def setliteral(self, *args):
        self.literal = 1

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

    # Interface -- translate references
    def translate_references(self, data, all = 1):
        i = 0
        while 1:
            res = amp.search(data, i)
            if res is None:
                return data
            res = ref.match(data, res.start(0))
            if res is None:
                self.syntax_error("bogus `&'")
                i =i+1
                continue
            i = res.end(0)
            if data[i - 1] != ';':
                self.syntax_error("`;' missing after entity/char reference")
                i = i-1
            str = res.group(1)
            pre = data[:res.start(0)]
            post = data[i:]
            if str[0] == '#':
                if str[1] == 'x':
                    str = chr(string.atoi(str[2:], 16))
                else:
                    str = chr(string.atoi(str[1:]))
                data = pre + str + post
                i = res.start(0)+len(str)
            elif all:
                if self.entitydefs.has_key(str):
                    data = pre + self.entitydefs[str] + post
                    i = res.start(0)    # rescan substituted text
                else:
                    self.syntax_error('reference to unknown entity')
                    # can't do it, so keep the entity ref in
                    data = pre + '&' + str + ';' + post
                    i = res.start(0) + len(str) + 2
            else:
                # just translating character references
                pass                    # i is already postioned correctly

    # Internal -- handle data as far as reasonable.  May leave state
    # and data to be processed by a subsequent call.  If 'end' is
    # true, force handling all data as if followed by EOF marker.
    def goahead(self, end):
        rawdata = self.rawdata
        i = 0
        n = len(rawdata)
        while i < n:
            if i > 0:
                self.__at_start = 0
            if self.nomoretags:
                data = rawdata[i:n]
                self.handle_data(data)
                self.lineno = self.lineno + string.count(data, '\n')
                i = n
                break
            res = interesting.search(rawdata, i)
            if res:
                    j = res.start(0)
            else:
                    j = n
            if i < j:
                if self.__at_start:
                    self.syntax_error('illegal data at start of file')
                self.__at_start = 0
                data = rawdata[i:j]
                if not self.stack and not space.match(data):
                    self.syntax_error('data not in content')
                if illegal.search(data):
                    self.syntax_error('illegal character in content')
                self.handle_data(data)
                self.lineno = self.lineno + string.count(data, '\n')
            i = j
            if i == n: break
            if rawdata[i] == '<':
                if starttagopen.match(rawdata, i):
                    if self.literal:
                        data = rawdata[i]
                        self.handle_data(data)
                        self.lineno = self.lineno + string.count(data, '\n')
                        i = i+1
                        continue
                    k = self.parse_starttag(i)
                    if k < 0: break
                    self.__seen_starttag = 1
                    self.lineno = self.lineno + string.count(rawdata[i:k], '\n')
                    i = k
                    continue
                if endtagopen.match(rawdata, i):
                    k = self.parse_endtag(i)
                    if k < 0: break
                    self.lineno = self.lineno + string.count(rawdata[i:k], '\n')
                    i =  k
                    self.literal = 0
                    continue
                if commentopen.match(rawdata, i):
                    if self.literal:
                        data = rawdata[i]
                        self.handle_data(data)
                        self.lineno = self.lineno + string.count(data, '\n')
                        i = i+1
                        continue
                    k = self.parse_comment(i)
                    if k < 0: break
                    self.lineno = self.lineno + string.count(rawdata[i:k], '\n')
                    i = k
                    continue
                if cdataopen.match(rawdata, i):
                    k = self.parse_cdata(i)
                    if k < 0: break
                    self.lineno = self.lineno + string.count(rawdata[i:i], '\n')
                    i = k
                    continue
                res = xmldecl.match(rawdata, i)
                if res:
                    if not self.__at_start:
                        self.syntax_error("<?xml?> declaration not at start of document")
                    version, encoding, standalone = res.group('version',
                                                              'encoding',
                                                              'standalone')
                    if version[1:-1] != '1.0':
                        raise RuntimeError, 'only XML version 1.0 supported'
                    if encoding: encoding = encoding[1:-1]
                    if standalone: standalone = standalone[1:-1]
                    self.handle_xml(encoding, standalone)
                    i = res.end(0)
                    continue
                res = procopen.match(rawdata, i)
                if res:
                    k = self.parse_proc(i)
                    if k < 0: break
                    self.lineno = self.lineno + string.count(rawdata[i:k], '\n')
                    i = k
                    continue
                res = doctype.match(rawdata, i)
                if res:
                    if self.literal:
                        data = rawdata[i]
                        self.handle_data(data)
                        self.lineno = self.lineno + string.count(data, '\n')
                        i = i+1
                        continue
                    if self.__seen_doctype:
                        self.syntax_error('multiple DOCTYPE elements')
                    if self.__seen_starttag:
                        self.syntax_error('DOCTYPE not at beginning of document')
                    k = self.parse_doctype(res)
                    if k < 0: break
                    self.__seen_doctype = res.group('name')
                    self.lineno = self.lineno + string.count(rawdata[i:k], '\n')
                    i = k
                    continue
            elif rawdata[i] == '&':
                res = charref.match(rawdata, i)
                if res is not None:
                    i = res.end(0)
                    if rawdata[i-1] != ';':
                        self.syntax_error("`;' missing in charref")
                        i = i-1
                    if not self.stack:
                        self.syntax_error('data not in content')
                    self.handle_charref(res.group('char')[:-1])
                    self.lineno = self.lineno + string.count(res.group(0), '\n')
                    continue
                res = entityref.match(rawdata, i)
                if res is not None:
                    i = res.end(0)
                    if rawdata[i-1] != ';':
                        self.syntax_error("`;' missing in entityref")
                        i = i-1
                    name = res.group('name')
                    if self.entitydefs.has_key(name):
                        self.rawdata = rawdata = rawdata[:res.start(0)] + self.entitydefs[name] + rawdata[i:]
                        n = len(rawdata)
                        i = res.start(0)
                    else:
                        self.syntax_error('reference to unknown entity')
                        self.unknown_entityref(name)
                    self.lineno = self.lineno + string.count(res.group(0), '\n')
                    continue
            elif rawdata[i] == ']':
                if n-i < 3:
                    break
                if cdataclose.match(rawdata, i):
                    self.syntax_error("bogus `]]>'")
                self.handle_data(rawdata[i])
                i = i+1
                continue
            else:
                raise RuntimeError, 'neither < nor & ??'
            # We get here only if incomplete matches but
            # nothing else
            break
        # end while
        if i > 0:
            self.__at_start = 0
        if end and i < n:
            data = rawdata[i]
            self.syntax_error("bogus `%s'" % data)
            if illegal.search(data):
                self.syntax_error('illegal character in content')
            self.handle_data(data)
            self.lineno = self.lineno + string.count(data, '\n')
            self.rawdata = rawdata[i+1:]
            return self.goahead(end)
        self.rawdata = rawdata[i:]
        if end:
            if not self.__seen_starttag:
                self.syntax_error('no elements in file')
            if self.stack:
                self.syntax_error('missing end tags')
                while self.stack:
                    self.finish_endtag(self.stack[-1])

    # Internal -- parse comment, return length or -1 if not terminated
    def parse_comment(self, i):
        rawdata = self.rawdata
        if rawdata[i:i+4] <> '<!--':
            raise RuntimeError, 'unexpected call to handle_comment'
        res = commentclose.search(rawdata, i+4)
        if not res:
            return -1
        if doubledash.search(rawdata, i+4, res.start(0)):
            self.syntax_error("`--' inside comment")
        if rawdata[res.start(0)-1] == '-':
            self.syntax_error('comment cannot end in three dashes')
        if illegal.search(rawdata, i+4, res.start(0)):
            self.syntax_error('illegal character in comment')
        self.handle_comment(rawdata[i+4: res.start(0)])
        return res.end(0)

    # Internal -- handle DOCTYPE tag, return length or -1 if not terminated
    def parse_doctype(self, res):
        rawdata = self.rawdata
        n = len(rawdata)
        name = res.group('name')
        pubid, syslit = res.group('pubid', 'syslit')
        if pubid is not None:
            pubid = pubid[1:-1]         # remove quotes
            pubid = string.join(string.split(pubid)) # normalize
        if syslit is not None: syslit = syslit[1:-1] # remove quotes
        j = k = res.end(0)
        if k >= n:
            return -1
        if rawdata[k] == '[':
            level = 0
            k = k+1
            dq = sq = 0
            while k < n:
                c = rawdata[k]
                if not sq and c == '"':
                    dq = not dq
                elif not dq and c == "'":
                    sq = not sq
                elif sq or dq:
                    pass
                elif level <= 0 and c == ']':
                    res = endbracket.match(rawdata, k+1)
                    if not res:
                        return -1
                    self.handle_doctype(name, pubid, syslit, rawdata[j+1:k])
                    return res.end(0)
                elif c == '<':
                    level = level + 1
                elif c == '>':
                    level = level - 1
                    if level < 0:
                        self.syntax_error("bogus `>' in DOCTYPE")
                k = k+1
        res = endbracket.search(rawdata, k)
        if not res:
            return -1
        if res.start(0) != k:
            self.syntax_error('garbage in DOCTYPE')
        self.handle_doctype(name, pubid, syslit, None)
        return res.end(0)

    # Internal -- handle CDATA tag, return length or -1 if not terminated
    def parse_cdata(self, i):
        rawdata = self.rawdata
        if rawdata[i:i+9] <> '<![CDATA[':
            raise RuntimeError, 'unexpected call to parse_cdata'
        res = cdataclose.search(rawdata, i+9)
        if not res:
            return -1
        if illegal.search(rawdata, i+9, res.start(0)):
            self.syntax_error('illegal character in CDATA')
        if not self.stack:
            self.syntax_error('CDATA not in content')
        self.handle_cdata(rawdata[i+9:res.start(0)])
        return res.end(0)

    __xml_attributes = {'version': '1.0', 'standalone': 'no', 'encoding': None}
    # Internal -- handle a processing instruction tag
    def parse_proc(self, i):
        rawdata = self.rawdata
        end = procclose.search(rawdata, i)
        if not end:
            return -1
        j = end.start(0)
        if illegal.search(rawdata, i+2, j):
            self.syntax_error('illegal character in processing instruction')
        res = tagfind.match(rawdata, i+2)
        if not res:
            raise RuntimeError, 'unexpected call to parse_proc'
        k = res.end(0)
        name = res.group(0)
        if string.find(string.lower(name), 'xml') >= 0:
            self.syntax_error('illegal processing instruction target name')
        self.handle_proc(name, rawdata[k:j])
        return end.end(0)

    # Internal -- parse attributes between i and j
    def parse_attributes(self, tag, k, j, attributes = None):
        rawdata = self.rawdata
        # Now parse the data between k and j into a tag and attrs
        attrdict = {}
        try:
            # convert attributes list to dictionary
            d = {}
            for a in attributes:
                d[a] = None
            attributes = d
        except TypeError:
            pass
        while k < j:
            res = attrfind.match(rawdata, k)
            if not res: break
            attrname, attrvalue = res.group('name', 'value')
            if attrvalue is None:
                self.syntax_error('no attribute value specified')
                attrvalue = attrname
            elif attrvalue[:1] == "'" == attrvalue[-1:] or \
                 attrvalue[:1] == '"' == attrvalue[-1:]:
                attrvalue = attrvalue[1:-1]
            else:
                self.syntax_error('attribute value not quoted')
            if attributes is not None and not attributes.has_key(attrname):
                self.syntax_error('unknown attribute %s of element %s' %
                                  (attrname, tag))
            if attrdict.has_key(attrname):
                self.syntax_error('attribute specified twice')
            attrvalue = string.translate(attrvalue, attrtrans)
            attrdict[attrname] = self.translate_references(attrvalue)
            k = res.end(0)
        if attributes is not None:
            # fill in with default attributes
            for key, val in attributes.items():
                if val is not None and not attrdict.has_key(key):
                    attrdict[key] = val
        return attrdict, k

    # Internal -- handle starttag, return length or -1 if not terminated
    def parse_starttag(self, i):
        rawdata = self.rawdata
        # i points to start of tag
        end = endbracket.search(rawdata, i+1)
        if not end:
            return -1
        j = end.start(0)
        res = tagfind.match(rawdata, i+1)
        if not res:
            raise RuntimeError, 'unexpected call to parse_starttag'
        k = res.end(0)
        tag = res.group(0)
        if not self.__seen_starttag and self.__seen_doctype:
            if tag != self.__seen_doctype:
                self.syntax_error('starttag does not match DOCTYPE')
        if self.__seen_starttag and not self.stack:
            self.syntax_error('multiple elements on top level')
        if hasattr(self, tag + '_attributes'):
            attributes = getattr(self, tag + '_attributes')
        else:
            attributes = None
        attrdict, k = self.parse_attributes(tag, k, j, attributes)
        res = starttagend.match(rawdata, k)
        if not res:
            self.syntax_error('garbage in start tag')
        self.finish_starttag(tag, attrdict)
        if res and res.group('slash') == '/':
            self.finish_endtag(tag)
        return end.end(0)

    # Internal -- parse endtag
    def parse_endtag(self, i):
        rawdata = self.rawdata
        end = endbracket.search(rawdata, i+1)
        if not end:
            return -1
        res = tagfind.match(rawdata, i+2)
        if not res:
            self.syntax_error('no name specified in end tag')
            tag = ''
            k = i+2
        else:
            tag = res.group(0)
            k = res.end(0)
        if k != end.start(0):
            self.syntax_error('garbage in end tag')
        self.finish_endtag(tag)
        return end.end(0)

    # Internal -- finish processing of start tag
    # Return -1 for unknown tag, 1 for balanced tag
    def finish_starttag(self, tag, attrs):
        self.stack.append(tag)
        methodname = 'start_' + tag
        if hasattr(self, methodname):
            method = getattr(self, methodname)
            self.handle_starttag(tag, method, attrs)
            return 1
        else:
            self.unknown_starttag(tag, attrs)
            return -1

    # Internal -- finish processing of end tag
    def finish_endtag(self, tag):
        methodname = 'end_' + tag
        if not tag:
            self.syntax_error('name-less end tag')
            found = len(self.stack) - 1
            if found < 0:
                self.unknown_endtag(tag)
                return
        else:
            if tag not in self.stack:
                self.syntax_error('unopened end tag')
                if hasattr(self, methodname):
                    method = getattr(self, methodname)
                    self.handle_endtag(tag, method)
                else:
                    self.unknown_endtag(tag)
                return
            found = len(self.stack)
            for i in range(found):
                if self.stack[i] == tag:
                    found = i
        while len(self.stack) > found:
            if found < len(self.stack) - 1:
                self.syntax_error('missing close tag for %s' % self.stack[-1])
            tag = self.stack[-1]
            if hasattr(self, methodname):
                method = getattr(self, methodname)
                self.handle_endtag(tag, method)
            else:
                self.unknown_endtag(tag)
            del self.stack[-1]

    # Overridable -- handle xml processing instruction
    def handle_xml(self, encoding, standalone):
        pass

    # Overridable -- handle DOCTYPE
    def handle_doctype(self, tag, pubid, syslit, data):
        pass

    # Overridable -- handle start tag
    def handle_starttag(self, tag, method, attrs):
        method(attrs)

    # Overridable -- handle end tag
    def handle_endtag(self, tag, method):
        method()

    # Example -- handle character reference, no need to override
    def handle_charref(self, name):
        try:
            if name[0] == 'x':
                n = string.atoi(name[1:], 16)
            else:
                n = string.atoi(name)
        except string.atoi_error:
            self.unknown_charref(name)
            return
        if not 0 <= n <= 255:
            self.unknown_charref(name)
            return
        self.handle_data(chr(n))

    # Definition of entities -- derived classes may override
    entitydefs = {'lt': '&#60;',        # must use charref
                  'gt': '&#62;',
                  'amp': '&#38;',       # must use charref
                  'quot': '&#34;',
                  'apos': '&#39;',
                  }

    # Example -- handle entity reference, no need to override
    def handle_entityref(self, name):
        table = self.entitydefs
        if table.has_key(name):
            self.handle_data(table[name])
        else:
            self.unknown_entityref(name)
            return

    # Example -- handle data, should be overridden
    def handle_data(self, data):
        pass

    # Example -- handle cdata, could be overridden
    def handle_cdata(self, data):
        pass

    # Example -- handle comment, could be overridden
    def handle_comment(self, data):
        pass

    # Example -- handle processing instructions, could be overridden
    def handle_proc(self, name, data):
        pass

    # Example -- handle relatively harmless syntax errors, could be overridden
    def syntax_error(self, message):
        raise RuntimeError, 'Syntax error at line %d: %s' % (self.lineno, message)

    # To be overridden -- handlers for unknown objects
    def unknown_starttag(self, tag, attrs): pass
    def unknown_endtag(self, tag): pass
    def unknown_charref(self, ref): pass
    def unknown_entityref(self, ref): pass


class TestXMLParser(XMLParser):

    def __init__(self, verbose=0):
        self.testdata = ""
        XMLParser.__init__(self, verbose)

    def handle_xml(self, encoding, standalone):
        self.flush()
        print 'xml: encoding =',encoding,'standalone =',standalone

    def handle_doctype(self, tag, pubid, syslit, data):
        self.flush()
        print 'DOCTYPE:',tag, `data`

    def handle_entity(self, name, strval, pubid, syslit, ndata):
        self.flush()
        print 'ENTITY:',`data`

    def handle_data(self, data):
        self.testdata = self.testdata + data
        if len(`self.testdata`) >= 70:
            self.flush()

    def flush(self):
        data = self.testdata
        if data:
            self.testdata = ""
            print 'data:', `data`

    def handle_cdata(self, data):
        self.flush()
        print 'cdata:', `data`

    def handle_proc(self, name, data):
        self.flush()
        print 'processing:',name,`data`

    def handle_comment(self, data):
        self.flush()
        r = `data`
        if len(r) > 68:
            r = r[:32] + '...' + r[-32:]
        print 'comment:', r

    def syntax_error(self, message):
        print 'error at line %d:' % self.lineno, message

    def unknown_starttag(self, tag, attrs):
        self.flush()
        if not attrs:
            print 'start tag: <' + tag + '>'
        else:
            print 'start tag: <' + tag,
            for name, value in attrs.items():
                print name + '=' + '"' + value + '"',
            print '>'

    def unknown_endtag(self, tag):
        self.flush()
        print 'end tag: </' + tag + '>'

    def unknown_entityref(self, ref):
        self.flush()
        print '*** unknown entity ref: &' + ref + ';'

    def unknown_charref(self, ref):
        self.flush()
        print '*** unknown char ref: &#' + ref + ';'

    def close(self):
        XMLParser.close(self)
        self.flush()

def test(args = None):
    import sys

    if not args:
        args = sys.argv[1:]

    if args and args[0] == '-s':
        args = args[1:]
        klass = XMLParser
    else:
        klass = TestXMLParser

    if args:
        file = args[0]
    else:
        file = 'test.xml'

    if file == '-':
        f = sys.stdin
    else:
        try:
            f = open(file, 'r')
        except IOError, msg:
            print file, ":", msg
            sys.exit(1)

    data = f.read()
    if f is not sys.stdin:
        f.close()

    x = klass()
    try:
        for c in data:
            x.feed(c)
        x.close()
    except RuntimeError, msg:
        print msg
        sys.exit(1)


if __name__ == '__main__':
    test()
