# Very simple test - Parse a file and print what happens

# XXX TypeErrors on calling handlers, or on bad return values from a
# handler, are obscure and unhelpful.
        
import sys, string
import os

import pyexpat
	 	
class Outputter:
    def StartElementHandler(self, name, attrs):
	print 'Start element:\n\t', name, attrs
	
    def EndElementHandler(self, name):
	print 'End element:\n\t', name

    def CharacterDataHandler(self, data):
        data = string.strip(data)
        if data:
            print 'Character data:'
            print '\t', repr(data)

    def ProcessingInstructionHandler(self, target, data):
	print 'PI:\n\t', target, data

    def StartNamespaceDeclHandler(self, prefix, uri):
	print 'NS decl:\n\t', prefix, uri

    def EndNamespaceDeclHandler(self, prefix):
	print 'End of NS decl:\n\t', prefix

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
        
    def ExternalEntityRefHandler(self, context, base, sysId, pubId):
        print 'External entity ref:', context, base, sysId, pubId
        return 1

    def DefaultHandler(self, userData):
        pass

    def DefaultHandlerExpand(self, userData):
        pass


out = Outputter()
parser = pyexpat.ParserCreate(namespace_separator='!')
for name in ['StartElementHandler', 'EndElementHandler',
	     'CharacterDataHandler', 'ProcessingInstructionHandler',
	     'UnparsedEntityDeclHandler', 'NotationDeclHandler',
	     'StartNamespaceDeclHandler', 'EndNamespaceDeclHandler',
	     'CommentHandler', 'StartCdataSectionHandler',
	     'EndCdataSectionHandler',
             'DefaultHandler', 'DefaultHandlerExpand',
             #'NotStandaloneHandler',
	     'ExternalEntityRefHandler'
             ]:
    setattr(parser, name, getattr(out, name) )

data = """<?xml version="1.0" encoding="iso-8859-1" standalone="no"?>
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

<root>
<myns:subelement xmlns:myns="http://www.python.org/namespace">
     Contents of subelements
</myns:subelement>
<sub2><![CDATA[contents of CDATA section]]></sub2>
&external_entity;
</root>
"""

try:
    parser.Parse(data, 1)
except pyexpat.error:
    print '** Error', parser.ErrorCode, pyexpat.ErrorString( parser.ErrorCode)
    print '** Line', parser.ErrorLineNumber
    print '** Column', parser.ErrorColumnNumber
    print '** Byte', parser.ErrorByteIndex

