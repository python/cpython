# Very simple test - Parse a file and print what happens

# XXX TypeErrors on calling handlers, or on bad return values from a
# handler, are obscure and unhelpful.
        
from xml.parsers import expat
                
class Outputter:
    def StartElementHandler(self, name, attrs):
        print 'Start element:\n\t', repr(name), attrs
        
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
        print 'External entity ref:', args
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
