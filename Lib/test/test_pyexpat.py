# XXX TypeErrors on calling handlers, or on bad return values from a
# handler, are obscure and unhelpful.

import os
import sys
import sysconfig
import unittest
import traceback
from io import BytesIO
from test import support
from test.support import os_helper

from xml.parsers import expat
from xml.parsers.expat import errors

from test.support import sortdict


class SetAttributeTest(unittest.TestCase):
    def setUp(self):
        self.parser = expat.ParserCreate(namespace_separator='!')

    def test_buffer_text(self):
        self.assertIs(self.parser.buffer_text, False)
        for x in 0, 1, 2, 0:
            self.parser.buffer_text = x
            self.assertIs(self.parser.buffer_text, bool(x))

    def test_namespace_prefixes(self):
        self.assertIs(self.parser.namespace_prefixes, False)
        for x in 0, 1, 2, 0:
            self.parser.namespace_prefixes = x
            self.assertIs(self.parser.namespace_prefixes, bool(x))

    def test_ordered_attributes(self):
        self.assertIs(self.parser.ordered_attributes, False)
        for x in 0, 1, 2, 0:
            self.parser.ordered_attributes = x
            self.assertIs(self.parser.ordered_attributes, bool(x))

    def test_specified_attributes(self):
        self.assertIs(self.parser.specified_attributes, False)
        for x in 0, 1, 2, 0:
            self.parser.specified_attributes = x
            self.assertIs(self.parser.specified_attributes, bool(x))

    def test_invalid_attributes(self):
        with self.assertRaises(AttributeError):
            self.parser.returns_unicode = 1
        with self.assertRaises(AttributeError):
            self.parser.returns_unicode

        # Issue #25019
        self.assertRaises(TypeError, setattr, self.parser, range(0xF), 0)
        self.assertRaises(TypeError, self.parser.__setattr__, range(0xF), 0)
        self.assertRaises(TypeError, getattr, self.parser, range(0xF))


data = b'''\
<?xml version="1.0" encoding="iso-8859-1" standalone="no"?>
<?xml-stylesheet href="stylesheet.css"?>
<!-- comment data -->
<!DOCTYPE quotations SYSTEM "quotations.dtd" [
<!ELEMENT root ANY>
<!ATTLIST root attr1 CDATA #REQUIRED attr2 CDATA #IMPLIED>
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
&skipped_entity;
\xb5
</root>
'''


# Produce UTF-8 output
class ParseTest(unittest.TestCase):
    class Outputter:
        def __init__(self):
            self.out = []

        def StartElementHandler(self, name, attrs):
            self.out.append('Start element: ' + repr(name) + ' ' +
                            sortdict(attrs))

        def EndElementHandler(self, name):
            self.out.append('End element: ' + repr(name))

        def CharacterDataHandler(self, data):
            data = data.strip()
            if data:
                self.out.append('Character data: ' + repr(data))

        def ProcessingInstructionHandler(self, target, data):
            self.out.append('PI: ' + repr(target) + ' ' + repr(data))

        def StartNamespaceDeclHandler(self, prefix, uri):
            self.out.append('NS decl: ' + repr(prefix) + ' ' + repr(uri))

        def EndNamespaceDeclHandler(self, prefix):
            self.out.append('End of NS decl: ' + repr(prefix))

        def StartCdataSectionHandler(self):
            self.out.append('Start of CDATA section')

        def EndCdataSectionHandler(self):
            self.out.append('End of CDATA section')

        def CommentHandler(self, text):
            self.out.append('Comment: ' + repr(text))

        def NotationDeclHandler(self, *args):
            name, base, sysid, pubid = args
            self.out.append('Notation declared: %s' %(args,))

        def UnparsedEntityDeclHandler(self, *args):
            entityName, base, systemId, publicId, notationName = args
            self.out.append('Unparsed entity decl: %s' %(args,))

        def NotStandaloneHandler(self):
            self.out.append('Not standalone')
            return 1

        def ExternalEntityRefHandler(self, *args):
            context, base, sysId, pubId = args
            self.out.append('External entity ref: %s' %(args[1:],))
            return 1

        def StartDoctypeDeclHandler(self, *args):
            self.out.append(('Start doctype', args))
            return 1

        def EndDoctypeDeclHandler(self):
            self.out.append("End doctype")
            return 1

        def EntityDeclHandler(self, *args):
            self.out.append(('Entity declaration', args))
            return 1

        def XmlDeclHandler(self, *args):
            self.out.append(('XML declaration', args))
            return 1

        def ElementDeclHandler(self, *args):
            self.out.append(('Element declaration', args))
            return 1

        def AttlistDeclHandler(self, *args):
            self.out.append(('Attribute list declaration', args))
            return 1

        def SkippedEntityHandler(self, *args):
            self.out.append(("Skipped entity", args))
            return 1

        def DefaultHandler(self, userData):
            pass

        def DefaultHandlerExpand(self, userData):
            pass

    handler_names = [
        'StartElementHandler', 'EndElementHandler', 'CharacterDataHandler',
        'ProcessingInstructionHandler', 'UnparsedEntityDeclHandler',
        'NotationDeclHandler', 'StartNamespaceDeclHandler',
        'EndNamespaceDeclHandler', 'CommentHandler',
        'StartCdataSectionHandler', 'EndCdataSectionHandler', 'DefaultHandler',
        'DefaultHandlerExpand', 'NotStandaloneHandler',
        'ExternalEntityRefHandler', 'StartDoctypeDeclHandler',
        'EndDoctypeDeclHandler', 'EntityDeclHandler', 'XmlDeclHandler',
        'ElementDeclHandler', 'AttlistDeclHandler', 'SkippedEntityHandler',
        ]

    def _hookup_callbacks(self, parser, handler):
        """
        Set each of the callbacks defined on handler and named in
        self.handler_names on the given parser.
        """
        for name in self.handler_names:
            setattr(parser, name, getattr(handler, name))

    def _verify_parse_output(self, operations):
        expected_operations = [
            ('XML declaration', ('1.0', 'iso-8859-1', 0)),
            'PI: \'xml-stylesheet\' \'href="stylesheet.css"\'',
            "Comment: ' comment data '",
            "Not standalone",
            ("Start doctype", ('quotations', 'quotations.dtd', None, 1)),
            ('Element declaration', ('root', (2, 0, None, ()))),
            ('Attribute list declaration', ('root', 'attr1', 'CDATA', None,
                1)),
            ('Attribute list declaration', ('root', 'attr2', 'CDATA', None,
                0)),
            "Notation declared: ('notation', None, 'notation.jpeg', None)",
            ('Entity declaration', ('acirc', 0, '\xe2', None, None, None, None)),
            ('Entity declaration', ('external_entity', 0, None, None,
                'entity.file', None, None)),
            "Unparsed entity decl: ('unparsed_entity', None, 'entity.file', None, 'notation')",
            "Not standalone",
            "End doctype",
            "Start element: 'root' {'attr1': 'value1', 'attr2': 'value2\u1f40'}",
            "NS decl: 'myns' 'http://www.python.org/namespace'",
            "Start element: 'http://www.python.org/namespace!subelement' {}",
            "Character data: 'Contents of subelements'",
            "End element: 'http://www.python.org/namespace!subelement'",
            "End of NS decl: 'myns'",
            "Start element: 'sub2' {}",
            'Start of CDATA section',
            "Character data: 'contents of CDATA section'",
            'End of CDATA section',
            "End element: 'sub2'",
            "External entity ref: (None, 'entity.file', None)",
            ('Skipped entity', ('skipped_entity', 0)),
            "Character data: '\xb5'",
            "End element: 'root'",
        ]
        for operation, expected_operation in zip(operations, expected_operations):
            self.assertEqual(operation, expected_operation)

    def test_parse_bytes(self):
        out = self.Outputter()
        parser = expat.ParserCreate(namespace_separator='!')
        self._hookup_callbacks(parser, out)

        parser.Parse(data, True)

        operations = out.out
        self._verify_parse_output(operations)
        # Issue #6697.
        self.assertRaises(AttributeError, getattr, parser, '\uD800')

    def test_parse_str(self):
        out = self.Outputter()
        parser = expat.ParserCreate(namespace_separator='!')
        self._hookup_callbacks(parser, out)

        parser.Parse(data.decode('iso-8859-1'), True)

        operations = out.out
        self._verify_parse_output(operations)

    def test_parse_file(self):
        # Try parsing a file
        out = self.Outputter()
        parser = expat.ParserCreate(namespace_separator='!')
        self._hookup_callbacks(parser, out)
        file = BytesIO(data)

        parser.ParseFile(file)

        operations = out.out
        self._verify_parse_output(operations)

    def test_parse_again(self):
        parser = expat.ParserCreate()
        file = BytesIO(data)
        parser.ParseFile(file)
        # Issue 6676: ensure a meaningful exception is raised when attempting
        # to parse more than one XML document per xmlparser instance,
        # a limitation of the Expat library.
        with self.assertRaises(expat.error) as cm:
            parser.ParseFile(file)
        self.assertEqual(expat.ErrorString(cm.exception.code),
                          expat.errors.XML_ERROR_FINISHED)

class NamespaceSeparatorTest(unittest.TestCase):
    def test_legal(self):
        # Tests that make sure we get errors when the namespace_separator value
        # is illegal, and that we don't for good values:
        expat.ParserCreate()
        expat.ParserCreate(namespace_separator=None)
        expat.ParserCreate(namespace_separator=' ')

    def test_illegal(self):
        with self.assertRaisesRegex(TypeError,
                r"ParserCreate\(\) argument (2|'namespace_separator') "
                r"must be str or None, not int"):
            expat.ParserCreate(namespace_separator=42)

        try:
            expat.ParserCreate(namespace_separator='too long')
            self.fail()
        except ValueError as e:
            self.assertEqual(str(e),
                'namespace_separator must be at most one character, omitted, or None')

    def test_zero_length(self):
        # ParserCreate() needs to accept a namespace_separator of zero length
        # to satisfy the requirements of RDF applications that are required
        # to simply glue together the namespace URI and the localname.  Though
        # considered a wart of the RDF specifications, it needs to be supported.
        #
        # See XML-SIG mailing list thread starting with
        # http://mail.python.org/pipermail/xml-sig/2001-April/005202.html
        #
        expat.ParserCreate(namespace_separator='') # too short


class InterningTest(unittest.TestCase):
    def test(self):
        # Test the interning machinery.
        p = expat.ParserCreate()
        L = []
        def collector(name, *args):
            L.append(name)
        p.StartElementHandler = collector
        p.EndElementHandler = collector
        p.Parse(b"<e> <e/> <e></e> </e>", True)
        tag = L[0]
        self.assertEqual(len(L), 6)
        for entry in L:
            # L should have the same string repeated over and over.
            self.assertTrue(tag is entry)

    def test_issue9402(self):
        # create an ExternalEntityParserCreate with buffer text
        class ExternalOutputter:
            def __init__(self, parser):
                self.parser = parser
                self.parser_result = None

            def ExternalEntityRefHandler(self, context, base, sysId, pubId):
                external_parser = self.parser.ExternalEntityParserCreate("")
                self.parser_result = external_parser.Parse(b"", True)
                return 1

        parser = expat.ParserCreate(namespace_separator='!')
        parser.buffer_text = 1
        out = ExternalOutputter(parser)
        parser.ExternalEntityRefHandler = out.ExternalEntityRefHandler
        parser.Parse(data, True)
        self.assertEqual(out.parser_result, 1)


class BufferTextTest(unittest.TestCase):
    def setUp(self):
        self.stuff = []
        self.parser = expat.ParserCreate()
        self.parser.buffer_text = 1
        self.parser.CharacterDataHandler = self.CharacterDataHandler

    def check(self, expected, label):
        self.assertEqual(self.stuff, expected,
                "%s\nstuff    = %r\nexpected = %r"
                % (label, self.stuff, map(str, expected)))

    def CharacterDataHandler(self, text):
        self.stuff.append(text)

    def StartElementHandler(self, name, attrs):
        self.stuff.append("<%s>" % name)
        bt = attrs.get("buffer-text")
        if bt == "yes":
            self.parser.buffer_text = 1
        elif bt == "no":
            self.parser.buffer_text = 0

    def EndElementHandler(self, name):
        self.stuff.append("</%s>" % name)

    def CommentHandler(self, data):
        self.stuff.append("<!--%s-->" % data)

    def setHandlers(self, handlers=[]):
        for name in handlers:
            setattr(self.parser, name, getattr(self, name))

    def test_default_to_disabled(self):
        parser = expat.ParserCreate()
        self.assertFalse(parser.buffer_text)

    def test_buffering_enabled(self):
        # Make sure buffering is turned on
        self.assertTrue(self.parser.buffer_text)
        self.parser.Parse(b"<a>1<b/>2<c/>3</a>", True)
        self.assertEqual(self.stuff, ['123'],
                         "buffered text not properly collapsed")

    def test1(self):
        # XXX This test exposes more detail of Expat's text chunking than we
        # XXX like, but it tests what we need to concisely.
        self.setHandlers(["StartElementHandler"])
        self.parser.Parse(b"<a>1<b buffer-text='no'/>2\n3<c buffer-text='yes'/>4\n5</a>", True)
        self.assertEqual(self.stuff,
                         ["<a>", "1", "<b>", "2", "\n", "3", "<c>", "4\n5"],
                         "buffering control not reacting as expected")

    def test2(self):
        self.parser.Parse(b"<a>1<b/>&lt;2&gt;<c/>&#32;\n&#x20;3</a>", True)
        self.assertEqual(self.stuff, ["1<2> \n 3"],
                         "buffered text not properly collapsed")

    def test3(self):
        self.setHandlers(["StartElementHandler"])
        self.parser.Parse(b"<a>1<b/>2<c/>3</a>", True)
        self.assertEqual(self.stuff, ["<a>", "1", "<b>", "2", "<c>", "3"],
                         "buffered text not properly split")

    def test4(self):
        self.setHandlers(["StartElementHandler", "EndElementHandler"])
        self.parser.CharacterDataHandler = None
        self.parser.Parse(b"<a>1<b/>2<c/>3</a>", True)
        self.assertEqual(self.stuff,
                         ["<a>", "<b>", "</b>", "<c>", "</c>", "</a>"])

    def test5(self):
        self.setHandlers(["StartElementHandler", "EndElementHandler"])
        self.parser.Parse(b"<a>1<b></b>2<c/>3</a>", True)
        self.assertEqual(self.stuff,
            ["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "3", "</a>"])

    def test6(self):
        self.setHandlers(["CommentHandler", "EndElementHandler",
                    "StartElementHandler"])
        self.parser.Parse(b"<a>1<b/>2<c></c>345</a> ", True)
        self.assertEqual(self.stuff,
            ["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "345", "</a>"],
            "buffered text not properly split")

    def test7(self):
        self.setHandlers(["CommentHandler", "EndElementHandler",
                    "StartElementHandler"])
        self.parser.Parse(b"<a>1<b/>2<c></c>3<!--abc-->4<!--def-->5</a> ", True)
        self.assertEqual(self.stuff,
                         ["<a>", "1", "<b>", "</b>", "2", "<c>", "</c>", "3",
                          "<!--abc-->", "4", "<!--def-->", "5", "</a>"],
                         "buffered text not properly split")


# Test handling of exception from callback:
class HandlerExceptionTest(unittest.TestCase):
    def StartElementHandler(self, name, attrs):
        raise RuntimeError(f'StartElementHandler: <{name}>')

    def check_traceback_entry(self, entry, filename, funcname):
        self.assertEqual(os.path.basename(entry.filename), filename)
        self.assertEqual(entry.name, funcname)

    @support.cpython_only
    def test_exception(self):
        # gh-66652: test _PyTraceback_Add() used by pyexpat.c to inject frames

        # Change the current directory to the Python source code directory
        # if it is available.
        src_dir = sysconfig.get_config_var('abs_builddir')
        if src_dir:
            have_source = os.path.isdir(src_dir)
        else:
            have_source = False
        if have_source:
            with os_helper.change_cwd(src_dir):
                self._test_exception(have_source)
        else:
            self._test_exception(have_source)

    def _test_exception(self, have_source):
        # Use path relative to the current directory which should be the Python
        # source code directory (if it is available).
        PYEXPAT_C = os.path.join('Modules', 'pyexpat.c')

        parser = expat.ParserCreate()
        parser.StartElementHandler = self.StartElementHandler
        try:
            parser.Parse(b"<a><b><c/></b></a>", True)

            self.fail("the parser did not raise RuntimeError")
        except RuntimeError as exc:
            self.assertEqual(exc.args[0], 'StartElementHandler: <a>', exc)
            entries = traceback.extract_tb(exc.__traceback__)

        self.assertEqual(len(entries), 3, entries)
        self.check_traceback_entry(entries[0],
                                   "test_pyexpat.py", "_test_exception")
        self.check_traceback_entry(entries[1],
                                   os.path.basename(PYEXPAT_C),
                                   "StartElement")
        self.check_traceback_entry(entries[2],
                                   "test_pyexpat.py", "StartElementHandler")

        # Check that the traceback contains the relevant line in
        # Modules/pyexpat.c. Skip the test if Modules/pyexpat.c is not
        # available.
        if have_source and os.path.exists(PYEXPAT_C):
            self.assertIn('call_with_frame("StartElement"',
                          entries[1].line)


# Test Current* members:
class PositionTest(unittest.TestCase):
    def StartElementHandler(self, name, attrs):
        self.check_pos('s')

    def EndElementHandler(self, name):
        self.check_pos('e')

    def check_pos(self, event):
        pos = (event,
               self.parser.CurrentByteIndex,
               self.parser.CurrentLineNumber,
               self.parser.CurrentColumnNumber)
        self.assertTrue(self.upto < len(self.expected_list),
                        'too many parser events')
        expected = self.expected_list[self.upto]
        self.assertEqual(pos, expected,
                'Expected position %s, got position %s' %(pos, expected))
        self.upto += 1

    def test(self):
        self.parser = expat.ParserCreate()
        self.parser.StartElementHandler = self.StartElementHandler
        self.parser.EndElementHandler = self.EndElementHandler
        self.upto = 0
        self.expected_list = [('s', 0, 1, 0), ('s', 5, 2, 1), ('s', 11, 3, 2),
                              ('e', 15, 3, 6), ('e', 17, 4, 1), ('e', 22, 5, 0)]

        xml = b'<a>\n <b>\n  <c/>\n </b>\n</a>'
        self.parser.Parse(xml, True)


class sf1296433Test(unittest.TestCase):
    def test_parse_only_xml_data(self):
        # https://bugs.python.org/issue1296433
        #
        xml = "<?xml version='1.0' encoding='iso8859'?><s>%s</s>" % ('a' * 1025)
        # this one doesn't crash
        #xml = "<?xml version='1.0'?><s>%s</s>" % ('a' * 10000)

        class SpecificException(Exception):
            pass

        def handler(text):
            raise SpecificException

        parser = expat.ParserCreate()
        parser.CharacterDataHandler = handler

        self.assertRaises(SpecificException, parser.Parse, xml.encode('iso8859'))

class ChardataBufferTest(unittest.TestCase):
    """
    test setting of chardata buffer size
    """

    def test_1025_bytes(self):
        self.assertEqual(self.small_buffer_test(1025), 2)

    def test_1000_bytes(self):
        self.assertEqual(self.small_buffer_test(1000), 1)

    def test_wrong_size(self):
        parser = expat.ParserCreate()
        parser.buffer_text = 1
        with self.assertRaises(ValueError):
            parser.buffer_size = -1
        with self.assertRaises(ValueError):
            parser.buffer_size = 0
        with self.assertRaises((ValueError, OverflowError)):
            parser.buffer_size = sys.maxsize + 1
        with self.assertRaises(TypeError):
            parser.buffer_size = 512.0

    def test_unchanged_size(self):
        xml1 = b"<?xml version='1.0' encoding='iso8859'?><s>" + b'a' * 512
        xml2 = b'a'*512 + b'</s>'
        parser = expat.ParserCreate()
        parser.CharacterDataHandler = self.counting_handler
        parser.buffer_size = 512
        parser.buffer_text = 1

        # Feed 512 bytes of character data: the handler should be called
        # once.
        self.n = 0
        parser.Parse(xml1)
        self.assertEqual(self.n, 1)

        # Reassign to buffer_size, but assign the same size.
        parser.buffer_size = parser.buffer_size
        self.assertEqual(self.n, 1)

        # Try parsing rest of the document
        parser.Parse(xml2)
        self.assertEqual(self.n, 2)


    def test_disabling_buffer(self):
        xml1 = b"<?xml version='1.0' encoding='iso8859'?><a>" + b'a' * 512
        xml2 = b'b' * 1024
        xml3 = b'c' * 1024 + b'</a>';
        parser = expat.ParserCreate()
        parser.CharacterDataHandler = self.counting_handler
        parser.buffer_text = 1
        parser.buffer_size = 1024
        self.assertEqual(parser.buffer_size, 1024)

        # Parse one chunk of XML
        self.n = 0
        parser.Parse(xml1, False)
        self.assertEqual(parser.buffer_size, 1024)
        self.assertEqual(self.n, 1)

        # Turn off buffering and parse the next chunk.
        parser.buffer_text = 0
        self.assertFalse(parser.buffer_text)
        self.assertEqual(parser.buffer_size, 1024)
        for i in range(10):
            parser.Parse(xml2, False)
        self.assertEqual(self.n, 11)

        parser.buffer_text = 1
        self.assertTrue(parser.buffer_text)
        self.assertEqual(parser.buffer_size, 1024)
        parser.Parse(xml3, True)
        self.assertEqual(self.n, 12)

    def counting_handler(self, text):
        self.n += 1

    def small_buffer_test(self, buffer_len):
        xml = b"<?xml version='1.0' encoding='iso8859'?><s>" + b'a' * buffer_len + b'</s>'
        parser = expat.ParserCreate()
        parser.CharacterDataHandler = self.counting_handler
        parser.buffer_size = 1024
        parser.buffer_text = 1

        self.n = 0
        parser.Parse(xml)
        return self.n

    def test_change_size_1(self):
        xml1 = b"<?xml version='1.0' encoding='iso8859'?><a><s>" + b'a' * 1024
        xml2 = b'aaa</s><s>' + b'a' * 1025 + b'</s></a>'
        parser = expat.ParserCreate()
        parser.CharacterDataHandler = self.counting_handler
        parser.buffer_text = 1
        parser.buffer_size = 1024
        self.assertEqual(parser.buffer_size, 1024)

        self.n = 0
        parser.Parse(xml1, False)
        parser.buffer_size *= 2
        self.assertEqual(parser.buffer_size, 2048)
        parser.Parse(xml2, True)
        self.assertEqual(self.n, 2)

    def test_change_size_2(self):
        xml1 = b"<?xml version='1.0' encoding='iso8859'?><a>a<s>" + b'a' * 1023
        xml2 = b'aaa</s><s>' + b'a' * 1025 + b'</s></a>'
        parser = expat.ParserCreate()
        parser.CharacterDataHandler = self.counting_handler
        parser.buffer_text = 1
        parser.buffer_size = 2048
        self.assertEqual(parser.buffer_size, 2048)

        self.n=0
        parser.Parse(xml1, False)
        parser.buffer_size = parser.buffer_size // 2
        self.assertEqual(parser.buffer_size, 1024)
        parser.Parse(xml2, True)
        self.assertEqual(self.n, 4)

class MalformedInputTest(unittest.TestCase):
    def test1(self):
        xml = b"\0\r\n"
        parser = expat.ParserCreate()
        try:
            parser.Parse(xml, True)
            self.fail()
        except expat.ExpatError as e:
            self.assertEqual(str(e), 'unclosed token: line 2, column 0')

    def test2(self):
        # \xc2\x85 is UTF-8 encoded U+0085 (NEXT LINE)
        xml = b"<?xml version\xc2\x85='1.0'?>\r\n"
        parser = expat.ParserCreate()
        err_pattern = r'XML declaration not well-formed: line 1, column \d+'
        with self.assertRaisesRegex(expat.ExpatError, err_pattern):
            parser.Parse(xml, True)

class ErrorMessageTest(unittest.TestCase):
    def test_codes(self):
        # verify mapping of errors.codes and errors.messages
        self.assertEqual(errors.XML_ERROR_SYNTAX,
                         errors.messages[errors.codes[errors.XML_ERROR_SYNTAX]])

    def test_expaterror(self):
        xml = b'<'
        parser = expat.ParserCreate()
        try:
            parser.Parse(xml, True)
            self.fail()
        except expat.ExpatError as e:
            self.assertEqual(e.code,
                             errors.codes[errors.XML_ERROR_UNCLOSED_TOKEN])


class ForeignDTDTests(unittest.TestCase):
    """
    Tests for the UseForeignDTD method of expat parser objects.
    """
    def test_use_foreign_dtd(self):
        """
        If UseForeignDTD is passed True and a document without an external
        entity reference is parsed, ExternalEntityRefHandler is first called
        with None for the public and system ids.
        """
        handler_call_args = []
        def resolve_entity(context, base, system_id, public_id):
            handler_call_args.append((public_id, system_id))
            return 1

        parser = expat.ParserCreate()
        parser.UseForeignDTD(True)
        parser.SetParamEntityParsing(expat.XML_PARAM_ENTITY_PARSING_ALWAYS)
        parser.ExternalEntityRefHandler = resolve_entity
        parser.Parse(b"<?xml version='1.0'?><element/>")
        self.assertEqual(handler_call_args, [(None, None)])

        # test UseForeignDTD() is equal to UseForeignDTD(True)
        handler_call_args[:] = []

        parser = expat.ParserCreate()
        parser.UseForeignDTD()
        parser.SetParamEntityParsing(expat.XML_PARAM_ENTITY_PARSING_ALWAYS)
        parser.ExternalEntityRefHandler = resolve_entity
        parser.Parse(b"<?xml version='1.0'?><element/>")
        self.assertEqual(handler_call_args, [(None, None)])

    def test_ignore_use_foreign_dtd(self):
        """
        If UseForeignDTD is passed True and a document with an external
        entity reference is parsed, ExternalEntityRefHandler is called with
        the public and system ids from the document.
        """
        handler_call_args = []
        def resolve_entity(context, base, system_id, public_id):
            handler_call_args.append((public_id, system_id))
            return 1

        parser = expat.ParserCreate()
        parser.UseForeignDTD(True)
        parser.SetParamEntityParsing(expat.XML_PARAM_ENTITY_PARSING_ALWAYS)
        parser.ExternalEntityRefHandler = resolve_entity
        parser.Parse(
            b"<?xml version='1.0'?><!DOCTYPE foo PUBLIC 'bar' 'baz'><element/>")
        self.assertEqual(handler_call_args, [("bar", "baz")])


class ReparseDeferralTest(unittest.TestCase):
    def test_getter_setter_round_trip(self):
        parser = expat.ParserCreate()
        enabled = (expat.version_info >= (2, 6, 0))

        self.assertIs(parser.GetReparseDeferralEnabled(), enabled)
        parser.SetReparseDeferralEnabled(False)
        self.assertIs(parser.GetReparseDeferralEnabled(), False)
        parser.SetReparseDeferralEnabled(True)
        self.assertIs(parser.GetReparseDeferralEnabled(), enabled)

    def test_reparse_deferral_enabled(self):
        if expat.version_info < (2, 6, 0):
            self.skipTest(f'Expat {expat.version_info} does not '
                          'support reparse deferral')

        started = []

        def start_element(name, _):
            started.append(name)

        parser = expat.ParserCreate()
        parser.StartElementHandler = start_element
        self.assertTrue(parser.GetReparseDeferralEnabled())

        for chunk in (b'<doc', b'/>'):
            parser.Parse(chunk, False)

        # The key test: Have handlers already fired?  Expecting: no.
        self.assertEqual(started, [])

        parser.Parse(b'', True)

        self.assertEqual(started, ['doc'])

    def test_reparse_deferral_disabled(self):
        started = []

        def start_element(name, _):
            started.append(name)

        parser = expat.ParserCreate()
        parser.StartElementHandler = start_element
        if expat.version_info >= (2, 6, 0):
            parser.SetReparseDeferralEnabled(False)
        self.assertFalse(parser.GetReparseDeferralEnabled())

        for chunk in (b'<doc', b'/>'):
            parser.Parse(chunk, False)

        # The key test: Have handlers already fired?  Expecting: yes.
        self.assertEqual(started, ['doc'])


if __name__ == "__main__":
    unittest.main()
