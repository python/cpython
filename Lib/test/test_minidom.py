# test for xml.dom.minidom

from xml.dom.minidom import parse, Node, Document, parseString

import os.path
import sys
import traceback

if __name__ == "__main__":
    base = sys.argv[0]
else:
    base = __file__
tstfile = os.path.join(os.path.dirname(base), "test.xml")
del base

def confirm( test, testname="Test" ):
    if test: 
        print "Passed " + testname
    else: 
        print "Failed " + testname
        raise Exception

Node._debug=1

def testParseFromFile():
    from StringIO import StringIO
    dom=parse( StringIO( open( tstfile ).read() ) )
    print dom

def testGetElementsByTagName( ):
    dom=parse( tstfile )
    confirm( dom.getElementsByTagName( "LI" )==\
            dom.documentElement.getElementsByTagName( "LI" ) )
    dom.unlink()
    dom=None
    confirm (len( Node.allnodes )==0)

def testInsertBefore( ):
    dom=parse( tstfile )
    docel=dom.documentElement
    #docel.insertBefore( dom.createProcessingInstruction("a", "b"),
    #                        docel.childNodes[1])
                            
    #docel.insertBefore( dom.createProcessingInstruction("a", "b"),
    #                        docel.childNodes[0])

    #confirm( docel.childNodes[0].tet=="a" )
    #confirm( docel.childNodes[2].tet=="a" )
    dom.unlink()
    del dom
    del docel
    confirm( len( Node.allnodes )==0)

def testAppendChild():
    dom=parse( tstfile )
    dom.documentElement.appendChild( dom.createComment( u"Hello" ))
    confirm( dom.documentElement.childNodes[-1].nodeName=="#comment" )
    confirm( dom.documentElement.childNodes[-1].data=="Hello" )
    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testNonZero():
    dom=parse( tstfile )
    confirm( dom )# should not be zero
    dom.appendChild( dom.createComment( "foo" ) )
    confirm( not dom.childNodes[-1].childNodes )
    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testUnlink():
    dom=parse( tstfile )
    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testElement():
    dom=Document()
    dom.appendChild( dom.createElement( "abc" ) )
    confirm( dom.documentElement )
    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testAAA():
    dom=parseString( "<abc/>" )
    el=dom.documentElement
    el.setAttribute( "spam", "jam2" )
    dom.unlink()
    dom=None

def testAAB():
    dom=parseString( "<abc/>" )
    el=dom.documentElement
    el.setAttribute( "spam", "jam" )
    el.setAttribute( "spam", "jam2" )
    dom.unlink()
    dom=None

def testAddAttr():
    dom=Document()
    child=dom.appendChild( dom.createElement( "abc" ) )

    child.setAttribute( "def", "ghi" )
    confirm( child.getAttribute( "def" )=="ghi" )
    confirm( child.attributes["def"].value=="ghi" )

    child.setAttribute( "jkl", "mno" )
    confirm( child.getAttribute( "jkl" )=="mno" )
    confirm( child.attributes["jkl"].value=="mno" )

    confirm( len( child.attributes )==2 )

    child.setAttribute( "def", "newval" )
    confirm( child.getAttribute( "def" )=="newval" )
    confirm( child.attributes["def"].value=="newval" )

    confirm( len( child.attributes )==2 )

    dom.unlink()
    dom=None
    child=None

def testDeleteAttr():
    dom=Document()
    child=dom.appendChild( dom.createElement( "abc" ) )

    confirm( len( child.attributes)==0 )
    child.setAttribute( "def", "ghi" )
    confirm( len( child.attributes)==1 )
    del child.attributes["def"]
    confirm( len( child.attributes)==0 )
    dom.unlink()
    confirm( len( Node.allnodes )==0 )

def testRemoveAttr():
    dom=Document()
    child=dom.appendChild( dom.createElement( "abc" ) )

    child.setAttribute( "def", "ghi" )
    confirm( len( child.attributes)==1 )
    child.removeAttribute("def" )
    confirm( len( child.attributes)==0 )

    dom.unlink()

def testRemoveAttrNS():
    dom=Document()
    child=dom.appendChild( 
            dom.createElementNS( "http://www.python.org", "python:abc" ) )
    child.setAttributeNS( "http://www.w3.org", "xmlns:python", 
                                            "http://www.python.org" )
    child.setAttributeNS( "http://www.python.org", "python:abcattr", "foo" )
    confirm( len( child.attributes )==2 )
    child.removeAttributeNS( "http://www.python.org", "abcattr" )
    confirm( len( child.attributes )==1 )

    dom.unlink()
    dom=None
    
def testRemoveAttributeNode():
    dom=Document()
    child=dom.appendChild( dom.createElement( "foo" ) )
    child.setAttribute( "spam", "jam" )
    confirm( len( child.attributes )==1 )
    node=child.getAttributeNode( "spam" )
    child.removeAttributeNode( node )
    confirm( len( child.attributes )==0 )

    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testChangeAttr():
    dom=parseString( "<abc/>" )
    el=dom.documentElement
    el.setAttribute( "spam", "jam" )
    confirm( len( el.attributes )==1 )
    el.setAttribute( "spam", "bam" )
    confirm( len( el.attributes )==1 )
    el.attributes["spam"]="ham"
    confirm( len( el.attributes )==1 )
    el.setAttribute( "spam2", "bam" )
    confirm( len( el.attributes )==2 )
    el.attributes[ "spam2"]= "bam2"
    confirm( len( el.attributes )==2 )
    dom.unlink()
    dom=None
    confirm( len( Node.allnodes )==0 )

def testGetAttrList():
    pass

def testGetAttrValues(): pass

def testGetAttrLength(): pass

def testGetAttribute(): pass

def testGetAttributeNS(): pass

def testGetAttributeNode(): pass

def testGetElementsByTagNameNS(): pass

def testGetEmptyNodeListFromElementsByTagNameNS(): pass

def testElementReprAndStr():
    dom=Document()
    el=dom.appendChild( dom.createElement( "abc" ) )
    string1=repr( el )
    string2=str( el )
    confirm( string1==string2 )
    dom.unlink()

# commented out until Fredrick's fix is checked in
def _testElementReprAndStrUnicode():
    dom=Document()
    el=dom.appendChild( dom.createElement( u"abc" ) )
    string1=repr( el )
    string2=str( el )
    confirm( string1==string2 )
    dom.unlink()

# commented out until Fredrick's fix is checked in
def _testElementReprAndStrUnicodeNS():
    dom=Document()
    el=dom.appendChild(
	 dom.createElementNS( u"http://www.slashdot.org", u"slash:abc" ))
    string1=repr( el )
    string2=str( el )
    confirm( string1==string2 )
    confirm( string1.find("slash:abc" )!=-1 )
    dom.unlink()

def testAttributeRepr():
    dom=Document()
    el=dom.appendChild( dom.createElement( u"abc" ) )
    node=el.setAttribute( "abc", "def" )
    confirm( str( node ) == repr( node ) )
    dom.unlink()

def testTextNodeRepr(): pass

def testWriteXML(): pass

def testProcessingInstruction(): pass

def testProcessingInstructionRepr(): pass

def testTextRepr(): pass

def testWriteText(): pass

def testDocumentElement(): pass

def testTooManyDocumentElements(): pass

def testCreateElementNS(): pass

def testCreatAttributeNS(): pass

def testParse(): pass

def testParseString(): pass

def testComment(): pass

def testAttrListItem(): pass

def testAttrListItems(): pass

def testAttrListItemNS(): pass

def testAttrListKeys(): pass

def testAttrListKeysNS(): pass

def testAttrListValues(): pass

def testAttrListLength(): pass

def testAttrList__getitem__(): pass

def testAttrList__setitem__(): pass

def testSetAttrValueandNodeValue(): pass

def testParseElement(): pass

def testParseAttributes(): pass

def testParseElementNamespaces(): pass

def testParseAttributeNamespaces(): pass

def testParseProcessingInstructions(): pass

def testChildNodes(): pass

def testFirstChild(): pass

def testHasChildNodes(): pass

def testCloneElementShallow(): pass

def testCloneElementShallowCopiesAttributes(): pass

def testCloneElementDeep(): pass

def testCloneDocumentShallow(): pass

def testCloneDocumentDeep(): pass

def testCloneAttributeShallow(): pass

def testCloneAttributeDeep(): pass

def testClonePIShallow(): pass

def testClonePIDeep(): pass


names=globals().keys()
names.sort()

works=1

for name in names:
    if name.startswith( "test" ):
        func=globals()[name]
        try:
            func()
            print "Test Succeeded", name
            if len( Node.allnodes ):
                print "Garbage left over:"
                print Node.allnodes.items()[0:10]
            Node.allnodes={}
        except Exception, e :
            works=0
            print "Test Failed: ", name
            apply( traceback.print_exception, sys.exc_info() )
            print `e`
            Node.allnodes={}

if works:
    print "All tests succeeded"
else:
    print "\n\n\n\n************ Check for failures!"

