# Very simple test - Parse a file and print what happens

# XXX TypeErrors on calling handlers, or on bad return values from a
# handler, are obscure and unhelpful.

import pyexpat
from xml.parsers import expat

from test.test_support import sortdict, TestFailed

class Outputter:
    def StartElementHandler(self, name, attrs):
        print 'Start element:\n\t', repr(name), sortdict(attrs)

    def EndElementHandler(self, name):
        print 'End element:\n\t', repr(name)

    def CharacterDataHandler(self, data):
        data = data.strip()
        if data:
            print 'Character data:'
            print '\t', repr(data)

    def ProcessingInstructionHandler(self, target, data):
        print 'PI:\n\t', repr(target), repr(data)

    def StartNamespaceDeclHandler(self, prefix, uri):
        print 'NS decl:\n\t', repr(prefix), repr(uri)

    def EndNamespaceDeclHandler(self, prefix):
        print 'End of NS decl:\n\t', repr(prefix)

    def StartCdataSectionHandler(self):
        print 'Start of CDATA section'

    def EndCdataSectionHandler(self):
        print 'End of CDATA section'

    def CommentHandler(self, text):
        print 'Comment:\n\t', repr(text)

    def NotationDeclHandler(self, *args):
        name, base, sysid, pubid = args
        print 'Notation declared:', args

    def UnparsedEntityDeclHandler(self, *args):
        entityName, base, systemId, publicId, notationName = args
        print 'Unparsed entity decl:\n\t', args

    def NotStandaloneHandler(self, userData):
        print 'Not standalone'
        return 1

    def ExternalEntityRefHandler(self, *args):
        context, base, sysId, pubId = args
        print 'External entity ref:', args[1:]
        return 1

    def DefaultHandler(self, userData):
        pass

    def DefaultHandlerExpand(self, userData):
        pass


def confirm(ok):
    if ok:
        print "OK."
    else:
        print "Not OK."

out = Outputter()
parser = expat.ParserCreate(namespace_separator='!')

# Test getting/setting returns_unicode
parser.returns_unicode = 0; confirm(parser.returns_unicode == 0)
parser.returns_unicode = 1; confirm(parser.returns_unicode == 1)
parser.returns_unicode = 2; confirm(parser.returns_unicode == 1)
parser.returns_unicode = 0; confirm(parser.returns_unicode == 0)

# Test getting/setting ordered_attributes
parser.ordered_attributes = 0; confirm(parser.ordered_attributes == 0)
parser.ordered_attributes = 1; confirm(parser.ordered_attributes == 1)
parser.ordered_attributes = 2; confirm(parser.ordered_attributes == 1)
parser.ordered_attributes = 0; confirm(parser.ordered_attributes == 0)

# Test getting/setting specified_attributes
parser.specified_attributes = 0; confirm(parser.specified_attributes == 0)
parser.specified_attributes = 1; confirm(parser.specified_attributes == 1)
parser.specified_attributes = 2; confirm(parser.specified_attributes == 1)
parser.specified_attributes = 0; confirm(parser.specified_attributes == 0)

HANDLER_NAMES = [
    'StartElementHandler', 'EndElementHandler',
    'CharacterDataHandler', 'ProcessingInstructionHandler',
    'UnparsedEntityDeclHandler', 'NotationDeclHandler',
    'StartNamespaceDeclHandler', 'EndNamespaceDeclHandler',
    'CommentHandler', 'StartCdataSectionHandler',
    'EndCdataSectionHandler',
    'DefaultHandler', 'DefaultHandlerExpand',
    #'NotStandaloneHandler',
    'ExternalEntityRefHandler'
    ]
for name in HANDLER_NAMES:
    setattr(parser, name, getattr(out, name))

data = '''\
<?xml version="1.0" encoding="iso-8859-1" standalone="no"?>
<?xml-stylesheet href="stylesheet.css"?>
<!-- comment data -->
<!DOCTYPE quotations SYSTEM "quotations.dtd" [
<!ELEMENT root ANY>
<!NOTATION notation SYSTEM "notation.jpeg">
<!ENTITY acirc "&#226;">
<!ENTITY external_entity SYSTEM "entity.file">
<!ENTITY unparsed_entity SYSTEM "entity.file" NDATA notation>
%unparsed_entity;
]>

<root attr1="value1" attr2="value2&#8000;">
<myns:subelement xmlns:myns="http://www.python.org/namespace">
     Contents of subelements
</myns:subelement>
<sub2><![CDATA[contents of CDATA section]]></sub2>
&external_entity;
</root>
'''

# Produce UTF-8 output
parser.returns_unicode = 0
try:
    parser.Parse(data, 1)
except expat.error:
    print '** Error', parser.ErrorCode, expat.ErrorString(parser.ErrorCode)
    print '** Line', parser.ErrorLineNumber
    print '** Column', parser.ErrorColumnNumber
    print '** Byte', parser.ErrorByteIndex

# Try the parse again, this time producing Unicode output
parser = expat.ParserCreate(namespace_separator='!')
parser.returns_unicode = 1

for name in HANDLER_NAMES:
    setattr(parser, name, getattr(out, name))
try:
    parser.Parse(data, 1)
except expat.error:
    print '** Error', parser.ErrorCode, expat.ErrorString(parser.ErrorCode)
    print '** Line', parser.ErrorLineNumber
    print '** Column', parser.ErrorColumnNumber
    print '** Byte', parser.ErrorByteIndex

# Try parsing a file
parser = expat.ParserCreate(namespace_separator='!')
parser.returns_unicode = 1

for name in HANDLER_NAMES:
    setattr(parser, name, getattr(out, name))
import StringIO
file = StringIO.StringIO(data)
try:
    parser.ParseFile(file)
except expat.error:
    print '** Error', parser.ErrorCode, expat.ErrorString(parser.ErrorCode)
    print '** Line', parser.ErrorLineNumber
    print '** Column', parser.ErrorColumnNumber
    print '** Byte', parser.ErrorByteIndex


# Tests that make sure we get errors when the namespace_separator value
# is illegal, and that we don't for good values:
print
print "Testing constructor for proper handling of namespace_separator values:"
expat.ParserCreate()
expat.ParserCreate(namespace_separator=None)
expat.ParserCreate(namespace_separator=' ')
print "Legal values tested o.k."
try:
    expat.ParserCreate(namespace_separator=42)
except TypeError, e:
    print "Caught expected TypeError:"
    print e
else:
    print "Failed to catch expected TypeError."

try:
    expat.ParserCreate(namespace_separator='too long')
except ValueError, e:
    print "Caught expected ValueError:"
    print e
else:
    print "Failed to catch expected ValueError."

# ParserCreate() needs to accept a namespace_separator of zero length
# to satisfy the requirements of RDF applications that are required
# to simply glue together the namespace URI and the localname.  Though
# considered a wart of the RDF specifications, it needs to be supported.
#
# See XML-SIG mailing list thread starting with
# http://mail.python.org/pipermail/xml-sig/2001-April/005202.html
#
expat.ParserCreate(namespace_separator='') # too short

# Test the interning machinery.
p = expat.ParserCreate()
L = []
def collector(name, *args):
    L.append(name)
p.StartElementHandler = collector
p.EndElementHandler = collector
p.Parse("<e> <e/> <e></e> </e>", 1)
tag = L[0]
if len(L) != 6:
    print "L should only contain 6 entries; found", len(L)
for entry in L:
    if tag is not entry:
        print "expected L to contain many references to the same string",
        print "(it didn't)"
        print "L =", repr(L)
        break

# Tests of the buffer_text attribute.
import sys

class TextCollector:
    def __init__(self, parser):
        self.stuff = []

    def check(self, expected, label):
        require(self.stuff == expected,
                "%s\nstuff    = %r\nexpected = %r"
                % (label, self.stuff, map(unicode, expected)))

    def CharacterDataHandler(self, text):
        self.stuff.append(text)

    def StartElementHandler(self, name, attrs):
        self.stuff.append("<%s>" % name)
        bt = attrs.get("buffer-text")
        if bt == "yes":
            parser.buffer_text = 1
        elif bt == "no":
            parser.buffer_text = 0

    def EndElementHandler(self, name):
        self.stuff.append("</%s>" % name)

    def CommentHandler(self, data):
        self.stuff.append("<!--%s-->" % data)

def require(cond, label):
    # similar to confirm(), but no extraneous output
    if not cond:
        raise TestFailed(label)

def setup(handlers=[]):
    parser = expat.ParserCreate()
    require(not parser.buffer_text,
            "buffer_text not disabled by default")
    parser.buffer_text = 1
    handler = TextCollector(parser)
    parser.CharacterDataHandler = handler.CharacterDataHandler
    for name in handlers:
        setattr(parser, name, getattr(handler, name))
    return parser, handler

parser, handler = setup()
require(parser.buffer_text,
        "text buffering either not acknowledged or not enabled")
parser.Parse("<a>1<b/>2<c/>3</a>", 1)
handler.check(["123"],
              "buffered text not properly collapsed")

# XXX This test exposes more detail of Expat's text chunking than we
# XXX like, but it tests what we need to concisely.
parser, handler = setup(["StartElementHandler"])
parser.Parse("<a>1<b buffer-text='no'/>2\n3<c buffer-text='yes'/>4\n5</a>", 1)
handler.check(["<a>", "1", "<b>", "2", "\n", "3", "<c>", "4\n5"],
              "buffering control not reacting as expected")

parser, handler = setup()
parser.Parse("<a>1<b/>&lt;2&gt;<c/>&#32;\n&#x20;3</a>", 1)
handler.check(["1<2> \n 3"],
              "buffered text not properly collapsed")

parser, handler = setup(["StartElementHandler"])
parser.Parse("<a>1<b/>2<c/>3</a>", 1)
handler.check(["<a>", "1", "<b>", "2", "<c>", "3"],
              "buffered text not properly split")

parser, handler = setup(["StartElementHandler", "EndElementHandler"])
parser.CharacterDataHandler = None
parser.Parse("<a>1<b/>2<c/>3</a>", 1)
handler.check(["<a>", "<b>", "</b>", "<c>", "</c>", "</a>"],
              "huh?")

parser, handler = setup(["StartElementHandler", "EndElementHandler"])
parser.Parse("<a>1<b></b>2<c/>3</a>", 1)
handler.check(["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "3", "</a>"],
              "huh?")

parser, handler = setup(["CommentHandler", "EndElementHandler",
                         "StartElementHandler"])
parser.Parse("<a>1<b/>2<c></c>345</a> ", 1)
handler.check(["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "345", "</a>"],
              "buffered text not properly split")

parser, handler = setup(["CommentHandler", "EndElementHandler",
                         "StartElementHandler"])
parser.Parse("<a>1<b/>2<c></c>3<!--abc-->4<!--def-->5</a> ", 1)
handler.check(["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "3",
               "<!--abc-->", "4", "<!--def-->", "5", "</a>"],
              "buffered text not properly split")

# Test handling of exception from callback:
def StartElementHandler(name, attrs):
    raise RuntimeError(name)

parser = expat.ParserCreate()
parser.StartElementHandler = StartElementHandler

try:
    parser.Parse("<a><b><c/></b></a>", 1)
except RuntimeError, e:
    if e.args[0] != "a":
        print "Expected RuntimeError for element 'a'; found %r" % e.args[0]
else:
    print "Expected RuntimeError for 'a'"

# Test Current* members:
class PositionTest:

    def __init__(self, expected_list, parser):
        self.parser = parser
        self.parser.StartElementHandler = self.StartElementHandler
        self.parser.EndElementHandler = self.EndElementHandler
        self.expected_list = expected_list
        self.upto = 0

    def StartElementHandler(self, name, attrs):
        self.check_pos('s')

    def EndElementHandler(self, name):
        self.check_pos('e')

    def check_pos(self, event):
        pos = (event,
               self.parser.CurrentByteIndex,
               self.parser.CurrentLineNumber,
               self.parser.CurrentColumnNumber)
        require(self.upto < len(self.expected_list),
                'too many parser events')
        expected = self.expected_list[self.upto]
        require(pos == expected,
                'expected position %s, got %s' % (expected, pos))
        self.upto += 1


parser = expat.ParserCreate()
handler = PositionTest([('s', 0, 1, 0), ('s', 5, 2, 1), ('s', 11, 3, 2),
                        ('e', 15, 3, 6), ('e', 17, 4, 1), ('e', 22, 5, 0)],
                       parser)
parser.Parse('''<a>
 <b>
  <c/>
 </b>
</a>''', 1)
