# Copyright (C) 2001-2010 Python Software Foundation
# Contact: email-sig@python.org
# email package unit tests

import re
import time
import base64
import unittest
import textwrap
import warnings

from io import StringIO, BytesIO
from itertools import chain
from random import choice
from threading import Thread
from unittest.mock import patch

import email
import email.policy
import email.utils

from email.charset import Charset
from email.generator import Generator, DecodedGenerator, BytesGenerator
from email.header import Header, decode_header, make_header
from email.headerregistry import HeaderRegistry
from email.message import Message
from email.mime.application import MIMEApplication
from email.mime.audio import MIMEAudio
from email.mime.base import MIMEBase
from email.mime.image import MIMEImage
from email.mime.message import MIMEMessage
from email.mime.multipart import MIMEMultipart
from email.mime.nonmultipart import MIMENonMultipart
from email.mime.text import MIMEText
from email.parser import Parser, HeaderParser
from email import base64mime
from email import encoders
from email import errors
from email import iterators
from email import quoprimime
from email import utils

from test import support
from test.support import threading_helper
from test.support.os_helper import unlink
from test.test_email import openfile, TestEmailBase

# These imports are documented to work, but we are testing them using a
# different path, so we import them here just to make sure they are importable.
from email.parser import FeedParser, BytesFeedParser

NL = '\n'
EMPTYSTRING = ''
SPACE = ' '


# Test various aspects of the Message class's API
class TestMessageAPI(TestEmailBase):
    def test_get_all(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_20.txt')
        eq(msg.get_all('cc'), ['ccc@zzz.org', 'ddd@zzz.org', 'eee@zzz.org'])
        eq(msg.get_all('xx', 'n/a'), 'n/a')

    def test_getset_charset(self):
        eq = self.assertEqual
        msg = Message()
        eq(msg.get_charset(), None)
        charset = Charset('iso-8859-1')
        msg.set_charset(charset)
        eq(msg['mime-version'], '1.0')
        eq(msg.get_content_type(), 'text/plain')
        eq(msg['content-type'], 'text/plain; charset="iso-8859-1"')
        eq(msg.get_param('charset'), 'iso-8859-1')
        eq(msg['content-transfer-encoding'], 'quoted-printable')
        eq(msg.get_charset().input_charset, 'iso-8859-1')
        # Remove the charset
        msg.set_charset(None)
        eq(msg.get_charset(), None)
        eq(msg['content-type'], 'text/plain')
        # Try adding a charset when there's already MIME headers present
        msg = Message()
        msg['MIME-Version'] = '2.0'
        msg['Content-Type'] = 'text/x-weird'
        msg['Content-Transfer-Encoding'] = 'quinted-puntable'
        msg.set_charset(charset)
        eq(msg['mime-version'], '2.0')
        eq(msg['content-type'], 'text/x-weird; charset="iso-8859-1"')
        eq(msg['content-transfer-encoding'], 'quinted-puntable')

    def test_set_charset_from_string(self):
        eq = self.assertEqual
        msg = Message()
        msg.set_charset('us-ascii')
        eq(msg.get_charset().input_charset, 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')

    def test_set_payload_with_charset(self):
        msg = Message()
        charset = Charset('iso-8859-1')
        msg.set_payload('This is a string payload', charset)
        self.assertEqual(msg.get_charset().input_charset, 'iso-8859-1')

    def test_set_payload_with_8bit_data_and_charset(self):
        data = b'\xd0\x90\xd0\x91\xd0\x92'
        charset = Charset('utf-8')
        msg = Message()
        msg.set_payload(data, charset)
        self.assertEqual(msg['content-transfer-encoding'], 'base64')
        self.assertEqual(msg.get_payload(decode=True), data)
        self.assertEqual(msg.get_payload(), '0JDQkdCS\n')

    def test_set_payload_with_non_ascii_and_charset_body_encoding_none(self):
        data = b'\xd0\x90\xd0\x91\xd0\x92'
        charset = Charset('utf-8')
        charset.body_encoding = None # Disable base64 encoding
        msg = Message()
        msg.set_payload(data.decode('utf-8'), charset)
        self.assertEqual(msg['content-transfer-encoding'], '8bit')
        self.assertEqual(msg.get_payload(decode=True), data)

    def test_set_payload_with_8bit_data_and_charset_body_encoding_none(self):
        data = b'\xd0\x90\xd0\x91\xd0\x92'
        charset = Charset('utf-8')
        charset.body_encoding = None # Disable base64 encoding
        msg = Message()
        msg.set_payload(data, charset)
        self.assertEqual(msg['content-transfer-encoding'], '8bit')
        self.assertEqual(msg.get_payload(decode=True), data)

    def test_set_payload_to_list(self):
        msg = Message()
        msg.set_payload([])
        self.assertEqual(msg.get_payload(), [])

    def test_attach_when_payload_is_string(self):
        msg = Message()
        msg['Content-Type'] = 'multipart/mixed'
        msg.set_payload('string payload')
        sub_msg = MIMEMessage(Message())
        self.assertRaisesRegex(TypeError, "[Aa]ttach.*non-multipart",
                               msg.attach, sub_msg)

    def test_get_charsets(self):
        eq = self.assertEqual

        msg = self._msgobj('msg_08.txt')
        charsets = msg.get_charsets()
        eq(charsets, [None, 'us-ascii', 'iso-8859-1', 'iso-8859-2', 'koi8-r'])

        msg = self._msgobj('msg_09.txt')
        charsets = msg.get_charsets('dingbat')
        eq(charsets, ['dingbat', 'us-ascii', 'iso-8859-1', 'dingbat',
                      'koi8-r'])

        msg = self._msgobj('msg_12.txt')
        charsets = msg.get_charsets()
        eq(charsets, [None, 'us-ascii', 'iso-8859-1', None, 'iso-8859-2',
                      'iso-8859-3', 'us-ascii', 'koi8-r'])

    def test_get_filename(self):
        eq = self.assertEqual

        msg = self._msgobj('msg_04.txt')
        filenames = [p.get_filename() for p in msg.get_payload()]
        eq(filenames, ['msg.txt', 'msg.txt'])

        msg = self._msgobj('msg_07.txt')
        subpart = msg.get_payload(1)
        eq(subpart.get_filename(), 'dingusfish.gif')

    def test_get_filename_with_name_parameter(self):
        eq = self.assertEqual

        msg = self._msgobj('msg_44.txt')
        filenames = [p.get_filename() for p in msg.get_payload()]
        eq(filenames, ['msg.txt', 'msg.txt'])

    def test_get_boundary(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_07.txt')
        # No quotes!
        eq(msg.get_boundary(), 'BOUNDARY')

    def test_set_boundary(self):
        eq = self.assertEqual
        # This one has no existing boundary parameter, but the Content-Type:
        # header appears fifth.
        msg = self._msgobj('msg_01.txt')
        msg.set_boundary('BOUNDARY')
        header, value = msg.items()[4]
        eq(header.lower(), 'content-type')
        eq(value, 'text/plain; charset="us-ascii"; boundary="BOUNDARY"')
        # This one has a Content-Type: header, with a boundary, stuck in the
        # middle of its headers.  Make sure the order is preserved; it should
        # be fifth.
        msg = self._msgobj('msg_04.txt')
        msg.set_boundary('BOUNDARY')
        header, value = msg.items()[4]
        eq(header.lower(), 'content-type')
        eq(value, 'multipart/mixed; boundary="BOUNDARY"')
        # And this one has no Content-Type: header at all.
        msg = self._msgobj('msg_03.txt')
        self.assertRaises(errors.HeaderParseError,
                          msg.set_boundary, 'BOUNDARY')

    def test_make_boundary(self):
        msg = MIMEMultipart('form-data')
        # Note that when the boundary gets created is an implementation
        # detail and might change.
        self.assertEqual(msg.items()[0][1], 'multipart/form-data')
        # Trigger creation of boundary
        msg.as_string()
        self.assertEqual(msg.items()[0][1][:33],
                        'multipart/form-data; boundary="==')
        # XXX: there ought to be tests of the uniqueness of the boundary, too.

    def test_message_rfc822_only(self):
        # Issue 7970: message/rfc822 not in multipart parsed by
        # HeaderParser caused an exception when flattened.
        with openfile('msg_46.txt', encoding="utf-8") as fp:
            msgdata = fp.read()
        parser = HeaderParser()
        msg = parser.parsestr(msgdata)
        out = StringIO()
        gen = Generator(out, True, 0)
        gen.flatten(msg, False)
        self.assertEqual(out.getvalue(), msgdata)

    def test_byte_message_rfc822_only(self):
        # Make sure new bytes header parser also passes this.
        with openfile('msg_46.txt', encoding="utf-8") as fp:
            msgdata = fp.read().encode('ascii')
        parser = email.parser.BytesHeaderParser()
        msg = parser.parsebytes(msgdata)
        out = BytesIO()
        gen = email.generator.BytesGenerator(out)
        gen.flatten(msg)
        self.assertEqual(out.getvalue(), msgdata)

    def test_get_decoded_payload(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_10.txt')
        # The outer message is a multipart
        eq(msg.get_payload(decode=True), None)
        # Subpart 1 is 7bit encoded
        eq(msg.get_payload(0).get_payload(decode=True),
           b'This is a 7bit encoded message.\n')
        # Subpart 2 is quopri
        eq(msg.get_payload(1).get_payload(decode=True),
           b'\xa1This is a Quoted Printable encoded message!\n')
        # Subpart 3 is base64
        eq(msg.get_payload(2).get_payload(decode=True),
           b'This is a Base64 encoded message.')
        # Subpart 4 is base64 with a trailing newline, which
        # used to be stripped (issue 7143).
        eq(msg.get_payload(3).get_payload(decode=True),
           b'This is a Base64 encoded message.\n')
        # Subpart 5 has no Content-Transfer-Encoding: header.
        eq(msg.get_payload(4).get_payload(decode=True),
           b'This has no Content-Transfer-Encoding: header.\n')

    def test_get_decoded_uu_payload(self):
        eq = self.assertEqual
        msg = Message()
        msg.set_payload('begin 666 -\n+:&5L;&\\@=V]R;&0 \n \nend\n')
        for cte in ('x-uuencode', 'uuencode', 'uue', 'x-uue'):
            msg['content-transfer-encoding'] = cte
            eq(msg.get_payload(decode=True), b'hello world')
        # Now try some bogus data
        msg.set_payload('foo')
        eq(msg.get_payload(decode=True), b'foo')

    def test_get_payload_n_raises_on_non_multipart(self):
        msg = Message()
        self.assertRaises(TypeError, msg.get_payload, 1)

    def test_decoded_generator(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_07.txt')
        with openfile('msg_17.txt', encoding="utf-8") as fp:
            text = fp.read()
        s = StringIO()
        g = DecodedGenerator(s)
        g.flatten(msg)
        eq(s.getvalue(), text)

    def test__contains__(self):
        msg = Message()
        msg['From'] = 'Me'
        msg['to'] = 'You'
        # Check for case insensitivity
        self.assertIn('from', msg)
        self.assertIn('From', msg)
        self.assertIn('FROM', msg)
        self.assertIn('to', msg)
        self.assertIn('To', msg)
        self.assertIn('TO', msg)

    def test_as_string(self):
        msg = self._msgobj('msg_01.txt')
        with openfile('msg_01.txt', encoding="utf-8") as fp:
            text = fp.read()
        self.assertEqual(text, str(msg))
        fullrepr = msg.as_string(unixfrom=True)
        lines = fullrepr.split('\n')
        self.assertTrue(lines[0].startswith('From '))
        self.assertEqual(text, NL.join(lines[1:]))

    def test_as_string_policy(self):
        msg = self._msgobj('msg_01.txt')
        newpolicy = msg.policy.clone(linesep='\r\n')
        fullrepr = msg.as_string(policy=newpolicy)
        s = StringIO()
        g = Generator(s, policy=newpolicy)
        g.flatten(msg)
        self.assertEqual(fullrepr, s.getvalue())

    def test_nonascii_as_string_without_cte(self):
        m = textwrap.dedent("""\
            MIME-Version: 1.0
            Content-type: text/plain; charset="iso-8859-1"

            Test if non-ascii messages with no Content-Transfer-Encoding set
            can be as_string'd:
            Föö bär
            """)
        source = m.encode('iso-8859-1')
        expected = textwrap.dedent("""\
            MIME-Version: 1.0
            Content-type: text/plain; charset="iso-8859-1"
            Content-Transfer-Encoding: quoted-printable

            Test if non-ascii messages with no Content-Transfer-Encoding set
            can be as_string'd:
            F=F6=F6 b=E4r
            """)
        msg = email.message_from_bytes(source)
        self.assertEqual(msg.as_string(), expected)

    def test_nonascii_as_string_with_ascii_charset(self):
        m = textwrap.dedent("""\
            MIME-Version: 1.0
            Content-type: text/plain; charset="us-ascii"
            Content-Transfer-Encoding: 8bit

            Test if non-ascii messages with no Content-Transfer-Encoding set
            can be as_string'd:
            Föö bär
            """)
        source = m.encode('iso-8859-1')
        expected = source.decode('ascii', 'replace')
        msg = email.message_from_bytes(source)
        self.assertEqual(msg.as_string(), expected)

    def test_nonascii_as_string_without_content_type_and_cte(self):
        m = textwrap.dedent("""\
            MIME-Version: 1.0

            Test if non-ascii messages with no Content-Type nor
            Content-Transfer-Encoding set can be as_string'd:
            Föö bär
            """)
        source = m.encode('iso-8859-1')
        expected = source.decode('ascii', 'replace')
        msg = email.message_from_bytes(source)
        self.assertEqual(msg.as_string(), expected)

    def test_as_bytes(self):
        msg = self._msgobj('msg_01.txt')
        with openfile('msg_01.txt', encoding="utf-8") as fp:
            data = fp.read().encode('ascii')
        self.assertEqual(data, bytes(msg))
        fullrepr = msg.as_bytes(unixfrom=True)
        lines = fullrepr.split(b'\n')
        self.assertTrue(lines[0].startswith(b'From '))
        self.assertEqual(data, b'\n'.join(lines[1:]))

    def test_as_bytes_policy(self):
        msg = self._msgobj('msg_01.txt')
        newpolicy = msg.policy.clone(linesep='\r\n')
        fullrepr = msg.as_bytes(policy=newpolicy)
        s = BytesIO()
        g = BytesGenerator(s,policy=newpolicy)
        g.flatten(msg)
        self.assertEqual(fullrepr, s.getvalue())

    # test_headerregistry.TestContentTypeHeader.bad_params
    def test_bad_param(self):
        msg = email.message_from_string("Content-Type: blarg; baz; boo\n")
        self.assertEqual(msg.get_param('baz'), '')

    def test_missing_filename(self):
        msg = email.message_from_string("From: foo\n")
        self.assertEqual(msg.get_filename(), None)

    def test_bogus_filename(self):
        msg = email.message_from_string(
        "Content-Disposition: blarg; filename\n")
        self.assertEqual(msg.get_filename(), '')

    def test_missing_boundary(self):
        msg = email.message_from_string("From: foo\n")
        self.assertEqual(msg.get_boundary(), None)

    def test_get_params(self):
        eq = self.assertEqual
        msg = email.message_from_string(
            'X-Header: foo=one; bar=two; baz=three\n')
        eq(msg.get_params(header='x-header'),
           [('foo', 'one'), ('bar', 'two'), ('baz', 'three')])
        msg = email.message_from_string(
            'X-Header: foo; bar=one; baz=two\n')
        eq(msg.get_params(header='x-header'),
           [('foo', ''), ('bar', 'one'), ('baz', 'two')])
        eq(msg.get_params(), None)
        msg = email.message_from_string(
            'X-Header: foo; bar="one"; baz=two\n')
        eq(msg.get_params(header='x-header'),
           [('foo', ''), ('bar', 'one'), ('baz', 'two')])

    # test_headerregistry.TestContentTypeHeader.spaces_around_param_equals
    def test_get_param_liberal(self):
        msg = Message()
        msg['Content-Type'] = 'Content-Type: Multipart/mixed; boundary = "CPIMSSMTPC06p5f3tG"'
        self.assertEqual(msg.get_param('boundary'), 'CPIMSSMTPC06p5f3tG')

    def test_get_param(self):
        eq = self.assertEqual
        msg = email.message_from_string(
            "X-Header: foo=one; bar=two; baz=three\n")
        eq(msg.get_param('bar', header='x-header'), 'two')
        eq(msg.get_param('quuz', header='x-header'), None)
        eq(msg.get_param('quuz'), None)
        msg = email.message_from_string(
            'X-Header: foo; bar="one"; baz=two\n')
        eq(msg.get_param('foo', header='x-header'), '')
        eq(msg.get_param('bar', header='x-header'), 'one')
        eq(msg.get_param('baz', header='x-header'), 'two')
        # XXX: We are not RFC-2045 compliant!  We cannot parse:
        # msg["Content-Type"] = 'text/plain; weird="hey; dolly? [you] @ <\\"home\\">?"'
        # msg.get_param("weird")
        # yet.

    # test_headerregistry.TestContentTypeHeader.spaces_around_semis
    def test_get_param_funky_continuation_lines(self):
        msg = self._msgobj('msg_22.txt')
        self.assertEqual(msg.get_payload(1).get_param('name'), 'wibble.JPG')

    # test_headerregistry.TestContentTypeHeader.semis_inside_quotes
    def test_get_param_with_semis_in_quotes(self):
        msg = email.message_from_string(
            'Content-Type: image/pjpeg; name="Jim&amp;&amp;Jill"\n')
        self.assertEqual(msg.get_param('name'), 'Jim&amp;&amp;Jill')
        self.assertEqual(msg.get_param('name', unquote=False),
                         '"Jim&amp;&amp;Jill"')

    # test_headerregistry.TestContentTypeHeader.quotes_inside_rfc2231_value
    def test_get_param_with_quotes(self):
        msg = email.message_from_string(
            'Content-Type: foo; bar*0="baz\\"foobar"; bar*1="\\"baz"')
        self.assertEqual(msg.get_param('bar'), 'baz"foobar"baz')
        msg = email.message_from_string(
            "Content-Type: foo; bar*0=\"baz\\\"foobar\"; bar*1=\"\\\"baz\"")
        self.assertEqual(msg.get_param('bar'), 'baz"foobar"baz')

    def test_field_containment(self):
        msg = email.message_from_string('Header: exists')
        self.assertIn('header', msg)
        self.assertIn('Header', msg)
        self.assertIn('HEADER', msg)
        self.assertNotIn('headerx', msg)

    def test_set_param(self):
        eq = self.assertEqual
        msg = Message()
        msg.set_param('charset', 'iso-2022-jp')
        eq(msg.get_param('charset'), 'iso-2022-jp')
        msg.set_param('importance', 'high value')
        eq(msg.get_param('importance'), 'high value')
        eq(msg.get_param('importance', unquote=False), '"high value"')
        eq(msg.get_params(), [('text/plain', ''),
                              ('charset', 'iso-2022-jp'),
                              ('importance', 'high value')])
        eq(msg.get_params(unquote=False), [('text/plain', ''),
                                       ('charset', '"iso-2022-jp"'),
                                       ('importance', '"high value"')])
        msg.set_param('charset', 'iso-9999-xx', header='X-Jimmy')
        eq(msg.get_param('charset', header='X-Jimmy'), 'iso-9999-xx')

    def test_del_param(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_05.txt')
        eq(msg.get_params(),
           [('multipart/report', ''), ('report-type', 'delivery-status'),
            ('boundary', 'D1690A7AC1.996856090/mail.example.com')])
        old_val = msg.get_param("report-type")
        msg.del_param("report-type")
        eq(msg.get_params(),
           [('multipart/report', ''),
            ('boundary', 'D1690A7AC1.996856090/mail.example.com')])
        msg.set_param("report-type", old_val)
        eq(msg.get_params(),
           [('multipart/report', ''),
            ('boundary', 'D1690A7AC1.996856090/mail.example.com'),
            ('report-type', old_val)])

    def test_del_param_on_other_header(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment', filename='bud.gif')
        msg.del_param('filename', 'content-disposition')
        self.assertEqual(msg['content-disposition'], 'attachment')

    def test_del_param_on_nonexistent_header(self):
        msg = Message()
        # Deleting param on empty msg should not raise exception.
        msg.del_param('filename', 'content-disposition')

    def test_del_nonexistent_param(self):
        msg = Message()
        msg.add_header('Content-Type', 'text/plain', charset='utf-8')
        existing_header = msg['Content-Type']
        msg.del_param('foobar', header='Content-Type')
        self.assertEqual(msg['Content-Type'], existing_header)

    def test_set_type(self):
        eq = self.assertEqual
        msg = Message()
        self.assertRaises(ValueError, msg.set_type, 'text')
        msg.set_type('text/plain')
        eq(msg['content-type'], 'text/plain')
        msg.set_param('charset', 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')
        msg.set_type('text/html')
        eq(msg['content-type'], 'text/html; charset="us-ascii"')

    def test_set_type_on_other_header(self):
        msg = Message()
        msg['X-Content-Type'] = 'text/plain'
        msg.set_type('application/octet-stream', 'X-Content-Type')
        self.assertEqual(msg['x-content-type'], 'application/octet-stream')

    def test_get_content_type_missing(self):
        msg = Message()
        self.assertEqual(msg.get_content_type(), 'text/plain')

    def test_get_content_type_missing_with_default_type(self):
        msg = Message()
        msg.set_default_type('message/rfc822')
        self.assertEqual(msg.get_content_type(), 'message/rfc822')

    def test_get_content_type_from_message_implicit(self):
        msg = self._msgobj('msg_30.txt')
        self.assertEqual(msg.get_payload(0).get_content_type(),
                         'message/rfc822')

    def test_get_content_type_from_message_explicit(self):
        msg = self._msgobj('msg_28.txt')
        self.assertEqual(msg.get_payload(0).get_content_type(),
                         'message/rfc822')

    def test_get_content_type_from_message_text_plain_implicit(self):
        msg = self._msgobj('msg_03.txt')
        self.assertEqual(msg.get_content_type(), 'text/plain')

    def test_get_content_type_from_message_text_plain_explicit(self):
        msg = self._msgobj('msg_01.txt')
        self.assertEqual(msg.get_content_type(), 'text/plain')

    def test_get_content_maintype_missing(self):
        msg = Message()
        self.assertEqual(msg.get_content_maintype(), 'text')

    def test_get_content_maintype_missing_with_default_type(self):
        msg = Message()
        msg.set_default_type('message/rfc822')
        self.assertEqual(msg.get_content_maintype(), 'message')

    def test_get_content_maintype_from_message_implicit(self):
        msg = self._msgobj('msg_30.txt')
        self.assertEqual(msg.get_payload(0).get_content_maintype(), 'message')

    def test_get_content_maintype_from_message_explicit(self):
        msg = self._msgobj('msg_28.txt')
        self.assertEqual(msg.get_payload(0).get_content_maintype(), 'message')

    def test_get_content_maintype_from_message_text_plain_implicit(self):
        msg = self._msgobj('msg_03.txt')
        self.assertEqual(msg.get_content_maintype(), 'text')

    def test_get_content_maintype_from_message_text_plain_explicit(self):
        msg = self._msgobj('msg_01.txt')
        self.assertEqual(msg.get_content_maintype(), 'text')

    def test_get_content_subtype_missing(self):
        msg = Message()
        self.assertEqual(msg.get_content_subtype(), 'plain')

    def test_get_content_subtype_missing_with_default_type(self):
        msg = Message()
        msg.set_default_type('message/rfc822')
        self.assertEqual(msg.get_content_subtype(), 'rfc822')

    def test_get_content_subtype_from_message_implicit(self):
        msg = self._msgobj('msg_30.txt')
        self.assertEqual(msg.get_payload(0).get_content_subtype(), 'rfc822')

    def test_get_content_subtype_from_message_explicit(self):
        msg = self._msgobj('msg_28.txt')
        self.assertEqual(msg.get_payload(0).get_content_subtype(), 'rfc822')

    def test_get_content_subtype_from_message_text_plain_implicit(self):
        msg = self._msgobj('msg_03.txt')
        self.assertEqual(msg.get_content_subtype(), 'plain')

    def test_get_content_subtype_from_message_text_plain_explicit(self):
        msg = self._msgobj('msg_01.txt')
        self.assertEqual(msg.get_content_subtype(), 'plain')

    def test_get_content_maintype_error(self):
        msg = Message()
        msg['Content-Type'] = 'no-slash-in-this-string'
        self.assertEqual(msg.get_content_maintype(), 'text')

    def test_get_content_subtype_error(self):
        msg = Message()
        msg['Content-Type'] = 'no-slash-in-this-string'
        self.assertEqual(msg.get_content_subtype(), 'plain')

    def test_replace_header(self):
        eq = self.assertEqual
        msg = Message()
        msg.add_header('First', 'One')
        msg.add_header('Second', 'Two')
        msg.add_header('Third', 'Three')
        eq(msg.keys(), ['First', 'Second', 'Third'])
        eq(msg.values(), ['One', 'Two', 'Three'])
        msg.replace_header('Second', 'Twenty')
        eq(msg.keys(), ['First', 'Second', 'Third'])
        eq(msg.values(), ['One', 'Twenty', 'Three'])
        msg.add_header('First', 'Eleven')
        msg.replace_header('First', 'One Hundred')
        eq(msg.keys(), ['First', 'Second', 'Third', 'First'])
        eq(msg.values(), ['One Hundred', 'Twenty', 'Three', 'Eleven'])
        self.assertRaises(KeyError, msg.replace_header, 'Fourth', 'Missing')

    def test_get_content_disposition(self):
        msg = Message()
        self.assertIsNone(msg.get_content_disposition())
        msg.add_header('Content-Disposition', 'attachment',
                       filename='random.avi')
        self.assertEqual(msg.get_content_disposition(), 'attachment')
        msg.replace_header('Content-Disposition', 'inline')
        self.assertEqual(msg.get_content_disposition(), 'inline')
        msg.replace_header('Content-Disposition', 'InlinE')
        self.assertEqual(msg.get_content_disposition(), 'inline')

    # test_defect_handling:test_invalid_chars_in_base64_payload
    def test_broken_base64_payload(self):
        x = 'AwDp0P7//y6LwKEAcPa/6Q=9'
        msg = Message()
        msg['content-type'] = 'audio/x-midi'
        msg['content-transfer-encoding'] = 'base64'
        msg.set_payload(x)
        self.assertEqual(msg.get_payload(decode=True),
                         (b'\x03\x00\xe9\xd0\xfe\xff\xff.\x8b\xc0'
                          b'\xa1\x00p\xf6\xbf\xe9\x0f'))
        self.assertIsInstance(msg.defects[0],
                              errors.InvalidBase64CharactersDefect)

    def test_broken_unicode_payload(self):
        # This test improves coverage but is not a compliance test.
        # The behavior in this situation is currently undefined by the API.
        x = 'this is a br\xf6ken thing to do'
        msg = Message()
        msg['content-type'] = 'text/plain'
        msg['content-transfer-encoding'] = '8bit'
        msg.set_payload(x)
        self.assertEqual(msg.get_payload(decode=True),
                         bytes(x, 'raw-unicode-escape'))

    def test_questionable_bytes_payload(self):
        # This test improves coverage but is not a compliance test,
        # since it involves poking inside the black box.
        x = 'this is a quéstionable thing to do'.encode('utf-8')
        msg = Message()
        msg['content-type'] = 'text/plain; charset="utf-8"'
        msg['content-transfer-encoding'] = '8bit'
        msg._payload = x
        self.assertEqual(msg.get_payload(decode=True), x)

    # Issue 1078919
    def test_ascii_add_header(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment',
                       filename='bud.gif')
        self.assertEqual('attachment; filename="bud.gif"',
            msg['Content-Disposition'])

    def test_noascii_add_header(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment',
            filename="Fußballer.ppt")
        self.assertEqual(
            'attachment; filename*=utf-8\'\'Fu%C3%9Fballer.ppt',
            msg['Content-Disposition'])

    def test_nonascii_add_header_via_triple(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment',
            filename=('iso-8859-1', '', 'Fußballer.ppt'))
        self.assertEqual(
            'attachment; filename*=iso-8859-1\'\'Fu%DFballer.ppt',
            msg['Content-Disposition'])

    def test_ascii_add_header_with_tspecial(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment',
            filename="windows [filename].ppt")
        self.assertEqual(
            'attachment; filename="windows [filename].ppt"',
            msg['Content-Disposition'])

    def test_nonascii_add_header_with_tspecial(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'attachment',
            filename="Fußballer [filename].ppt")
        self.assertEqual(
            "attachment; filename*=utf-8''Fu%C3%9Fballer%20%5Bfilename%5D.ppt",
            msg['Content-Disposition'])

    def test_binary_quopri_payload(self):
        for charset in ('latin-1', 'ascii'):
            msg = Message()
            msg['content-type'] = 'text/plain; charset=%s' % charset
            msg['content-transfer-encoding'] = 'quoted-printable'
            msg.set_payload(b'foo=e6=96=87bar')
            self.assertEqual(
                msg.get_payload(decode=True),
                b'foo\xe6\x96\x87bar',
                'get_payload returns wrong result with charset %s.' % charset)

    def test_binary_base64_payload(self):
        for charset in ('latin-1', 'ascii'):
            msg = Message()
            msg['content-type'] = 'text/plain; charset=%s' % charset
            msg['content-transfer-encoding'] = 'base64'
            msg.set_payload(b'Zm9v5paHYmFy')
            self.assertEqual(
                msg.get_payload(decode=True),
                b'foo\xe6\x96\x87bar',
                'get_payload returns wrong result with charset %s.' % charset)

    def test_binary_uuencode_payload(self):
        for charset in ('latin-1', 'ascii'):
            for encoding in ('x-uuencode', 'uuencode', 'uue', 'x-uue'):
                msg = Message()
                msg['content-type'] = 'text/plain; charset=%s' % charset
                msg['content-transfer-encoding'] = encoding
                msg.set_payload(b"begin 666 -\n)9F]OYI:'8F%R\n \nend\n")
                self.assertEqual(
                    msg.get_payload(decode=True),
                    b'foo\xe6\x96\x87bar',
                    str(('get_payload returns wrong result ',
                         'with charset {0} and encoding {1}.')).\
                        format(charset, encoding))

    def test_add_header_with_name_only_param(self):
        msg = Message()
        msg.add_header('Content-Disposition', 'inline', foo_bar=None)
        self.assertEqual("inline; foo-bar", msg['Content-Disposition'])

    def test_add_header_with_no_value(self):
        msg = Message()
        msg.add_header('X-Status', None)
        self.assertEqual('', msg['X-Status'])

    # Issue 5871: reject an attempt to embed a header inside a header value
    # (header injection attack).
    def test_embedded_header_via_Header_rejected(self):
        msg = Message()
        msg['Dummy'] = Header('dummy\nX-Injected-Header: test')
        self.assertRaises(errors.HeaderParseError, msg.as_string)

    def test_embedded_header_via_string_rejected(self):
        msg = Message()
        msg['Dummy'] = 'dummy\nX-Injected-Header: test'
        self.assertRaises(errors.HeaderParseError, msg.as_string)

    def test_unicode_header_defaults_to_utf8_encoding(self):
        # Issue 14291
        m = MIMEText('abc\n')
        m['Subject'] = 'É test'
        self.assertEqual(str(m),textwrap.dedent("""\
            Content-Type: text/plain; charset="us-ascii"
            MIME-Version: 1.0
            Content-Transfer-Encoding: 7bit
            Subject: =?utf-8?q?=C3=89_test?=

            abc
            """))

    def test_unicode_body_defaults_to_utf8_encoding(self):
        # Issue 14291
        m = MIMEText('É testabc\n')
        self.assertEqual(str(m),textwrap.dedent("""\
            Content-Type: text/plain; charset="utf-8"
            MIME-Version: 1.0
            Content-Transfer-Encoding: base64

            w4kgdGVzdGFiYwo=
            """))


# Test the email.encoders module
class TestEncoders(unittest.TestCase):

    def test_EncodersEncode_base64(self):
        with openfile('python.gif', 'rb') as fp:
            bindata = fp.read()
        mimed = email.mime.image.MIMEImage(bindata)
        base64ed = mimed.get_payload()
        # the transfer-encoded body lines should all be <=76 characters
        lines = base64ed.split('\n')
        self.assertLessEqual(max([ len(x) for x in lines ]), 76)

    def test_encode_empty_payload(self):
        eq = self.assertEqual
        msg = Message()
        msg.set_charset('us-ascii')
        eq(msg['content-transfer-encoding'], '7bit')

    def test_default_cte(self):
        eq = self.assertEqual
        # 7bit data and the default us-ascii _charset
        msg = MIMEText('hello world')
        eq(msg['content-transfer-encoding'], '7bit')
        # Similar, but with 8bit data
        msg = MIMEText('hello \xf8 world')
        eq(msg['content-transfer-encoding'], 'base64')
        # And now with a different charset
        msg = MIMEText('hello \xf8 world', _charset='iso-8859-1')
        eq(msg['content-transfer-encoding'], 'quoted-printable')

    def test_encode7or8bit(self):
        # Make sure a charset whose input character set is 8bit but
        # whose output character set is 7bit gets a transfer-encoding
        # of 7bit.
        eq = self.assertEqual
        msg = MIMEText('文\n', _charset='euc-jp')
        eq(msg['content-transfer-encoding'], '7bit')
        eq(msg.as_string(), textwrap.dedent("""\
            MIME-Version: 1.0
            Content-Type: text/plain; charset="iso-2022-jp"
            Content-Transfer-Encoding: 7bit

            \x1b$BJ8\x1b(B
            """))

    def test_qp_encode_latin1(self):
        msg = MIMEText('\xe1\xf6\n', 'text', 'ISO-8859-1')
        self.assertEqual(str(msg), textwrap.dedent("""\
            MIME-Version: 1.0
            Content-Type: text/text; charset="iso-8859-1"
            Content-Transfer-Encoding: quoted-printable

            =E1=F6
            """))

    def test_qp_encode_non_latin1(self):
        # Issue 16948
        msg = MIMEText('\u017c\n', 'text', 'ISO-8859-2')
        self.assertEqual(str(msg), textwrap.dedent("""\
            MIME-Version: 1.0
            Content-Type: text/text; charset="iso-8859-2"
            Content-Transfer-Encoding: quoted-printable

            =BF
            """))


# Test long header wrapping
class TestLongHeaders(TestEmailBase):

    maxDiff = None

    def test_split_long_continuation(self):
        eq = self.ndiffAssertEqual
        msg = email.message_from_string("""\
Subject: bug demonstration
\t12345678911234567892123456789312345678941234567895123456789612345678971234567898112345678911234567892123456789112345678911234567892123456789
\tmore text

test
""")
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), """\
Subject: bug demonstration
\t12345678911234567892123456789312345678941234567895123456789612345678971234567898112345678911234567892123456789112345678911234567892123456789
\tmore text

test
""")

    def test_another_long_almost_unsplittable_header(self):
        eq = self.ndiffAssertEqual
        hstr = """\
bug demonstration
\t12345678911234567892123456789312345678941234567895123456789612345678971234567898112345678911234567892123456789112345678911234567892123456789
\tmore text"""
        h = Header(hstr, continuation_ws='\t')
        eq(h.encode(), """\
bug demonstration
\t12345678911234567892123456789312345678941234567895123456789612345678971234567898112345678911234567892123456789112345678911234567892123456789
\tmore text""")
        h = Header(hstr.replace('\t', ' '))
        eq(h.encode(), """\
bug demonstration
 12345678911234567892123456789312345678941234567895123456789612345678971234567898112345678911234567892123456789112345678911234567892123456789
 more text""")

    def test_long_nonstring(self):
        eq = self.ndiffAssertEqual
        g = Charset("iso-8859-1")
        cz = Charset("iso-8859-2")
        utf8 = Charset("utf-8")
        g_head = (b'Die Mieter treten hier ein werden mit einem Foerderband '
                  b'komfortabel den Korridor entlang, an s\xfcdl\xfcndischen '
                  b'Wandgem\xe4lden vorbei, gegen die rotierenden Klingen '
                  b'bef\xf6rdert. ')
        cz_head = (b'Finan\xe8ni metropole se hroutily pod tlakem jejich '
                   b'd\xf9vtipu.. ')
        utf8_head = ('\u6b63\u78ba\u306b\u8a00\u3046\u3068\u7ffb\u8a33\u306f'
                     '\u3055\u308c\u3066\u3044\u307e\u305b\u3093\u3002\u4e00'
                     '\u90e8\u306f\u30c9\u30a4\u30c4\u8a9e\u3067\u3059\u304c'
                     '\u3001\u3042\u3068\u306f\u3067\u305f\u3089\u3081\u3067'
                     '\u3059\u3002\u5b9f\u969b\u306b\u306f\u300cWenn ist das '
                     'Nunstuck git und Slotermeyer? Ja! Beiherhund das Oder '
                     'die Flipperwaldt gersput.\u300d\u3068\u8a00\u3063\u3066'
                     '\u3044\u307e\u3059\u3002')
        h = Header(g_head, g, header_name='Subject')
        h.append(cz_head, cz)
        h.append(utf8_head, utf8)
        msg = Message()
        msg['Subject'] = h
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), """\
Subject: =?iso-8859-1?q?Die_Mieter_treten_hier_ein_werden_mit_einem_Foerderb?=
 =?iso-8859-1?q?and_komfortabel_den_Korridor_entlang=2C_an_s=FCdl=FCndischen?=
 =?iso-8859-1?q?_Wandgem=E4lden_vorbei=2C_gegen_die_rotierenden_Klingen_bef?=
 =?iso-8859-1?q?=F6rdert=2E_?= =?iso-8859-2?q?Finan=E8ni_metropole_se_hrouti?=
 =?iso-8859-2?q?ly_pod_tlakem_jejich_d=F9vtipu=2E=2E_?= =?utf-8?b?5q2j56K6?=
 =?utf-8?b?44Gr6KiA44GG44Go57+76Kiz44Gv44GV44KM44Gm44GE44G+44Gb44KT44CC5LiA?=
 =?utf-8?b?6YOo44Gv44OJ44Kk44OE6Kqe44Gn44GZ44GM44CB44GC44Go44Gv44Gn44Gf44KJ?=
 =?utf-8?b?44KB44Gn44GZ44CC5a6f6Zqb44Gr44Gv44CMV2VubiBpc3QgZGFzIE51bnN0dWNr?=
 =?utf-8?b?IGdpdCB1bmQgU2xvdGVybWV5ZXI/IEphISBCZWloZXJodW5kIGRhcyBPZGVyIGRp?=
 =?utf-8?b?ZSBGbGlwcGVyd2FsZHQgZ2Vyc3B1dC7jgI3jgajoqIDjgaPjgabjgYTjgb7jgZk=?=
 =?utf-8?b?44CC?=

""")
        eq(h.encode(maxlinelen=76), """\
=?iso-8859-1?q?Die_Mieter_treten_hier_ein_werden_mit_einem_Foerde?=
 =?iso-8859-1?q?rband_komfortabel_den_Korridor_entlang=2C_an_s=FCdl=FCndis?=
 =?iso-8859-1?q?chen_Wandgem=E4lden_vorbei=2C_gegen_die_rotierenden_Klinge?=
 =?iso-8859-1?q?n_bef=F6rdert=2E_?= =?iso-8859-2?q?Finan=E8ni_metropole_se?=
 =?iso-8859-2?q?_hroutily_pod_tlakem_jejich_d=F9vtipu=2E=2E_?=
 =?utf-8?b?5q2j56K644Gr6KiA44GG44Go57+76Kiz44Gv44GV44KM44Gm44GE44G+44Gb?=
 =?utf-8?b?44KT44CC5LiA6YOo44Gv44OJ44Kk44OE6Kqe44Gn44GZ44GM44CB44GC44Go?=
 =?utf-8?b?44Gv44Gn44Gf44KJ44KB44Gn44GZ44CC5a6f6Zqb44Gr44Gv44CMV2VubiBp?=
 =?utf-8?b?c3QgZGFzIE51bnN0dWNrIGdpdCB1bmQgU2xvdGVybWV5ZXI/IEphISBCZWlo?=
 =?utf-8?b?ZXJodW5kIGRhcyBPZGVyIGRpZSBGbGlwcGVyd2FsZHQgZ2Vyc3B1dC7jgI0=?=
 =?utf-8?b?44Go6KiA44Gj44Gm44GE44G+44GZ44CC?=""")

    def test_long_header_encode(self):
        eq = self.ndiffAssertEqual
        h = Header('wasnipoop; giraffes="very-long-necked-animals"; '
                   'spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"',
                   header_name='X-Foobar-Spoink-Defrobnit')
        eq(h.encode(), '''\
wasnipoop; giraffes="very-long-necked-animals";
 spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"''')

    def test_long_header_encode_with_tab_continuation_is_just_a_hint(self):
        eq = self.ndiffAssertEqual
        h = Header('wasnipoop; giraffes="very-long-necked-animals"; '
                   'spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"',
                   header_name='X-Foobar-Spoink-Defrobnit',
                   continuation_ws='\t')
        eq(h.encode(), '''\
wasnipoop; giraffes="very-long-necked-animals";
 spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"''')

    def test_long_header_encode_with_tab_continuation(self):
        eq = self.ndiffAssertEqual
        h = Header('wasnipoop; giraffes="very-long-necked-animals";\t'
                   'spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"',
                   header_name='X-Foobar-Spoink-Defrobnit',
                   continuation_ws='\t')
        eq(h.encode(), '''\
wasnipoop; giraffes="very-long-necked-animals";
\tspooge="yummy"; hippos="gargantuan"; marshmallows="gooey"''')

    def test_header_encode_with_different_output_charset(self):
        h = Header('文', 'euc-jp')
        self.assertEqual(h.encode(), "=?iso-2022-jp?b?GyRCSjgbKEI=?=")

    def test_long_header_encode_with_different_output_charset(self):
        h = Header(b'test-ja \xa4\xd8\xc5\xea\xb9\xc6\xa4\xb5\xa4\xec\xa4'
            b'\xbf\xa5\xe1\xa1\xbc\xa5\xeb\xa4\xcf\xbb\xca\xb2\xf1\xbc\xd4'
            b'\xa4\xce\xbe\xb5\xc7\xa7\xa4\xf2\xc2\xd4\xa4\xc3\xa4\xc6\xa4'
            b'\xa4\xa4\xde\xa4\xb9'.decode('euc-jp'), 'euc-jp')
        res = """\
=?iso-2022-jp?b?dGVzdC1qYSAbJEIkWEVqOUYkNSRsJD8lYSE8JWskTztKMnE8VCROPjUbKEI=?=
 =?iso-2022-jp?b?GyRCRyckckJUJEMkRiQkJF4kORsoQg==?="""
        self.assertEqual(h.encode(), res)

    def test_header_splitter(self):
        eq = self.ndiffAssertEqual
        msg = MIMEText('')
        # It'd be great if we could use add_header() here, but that doesn't
        # guarantee an order of the parameters.
        msg['X-Foobar-Spoink-Defrobnit'] = (
            'wasnipoop; giraffes="very-long-necked-animals"; '
            'spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"')
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), '''\
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit
X-Foobar-Spoink-Defrobnit: wasnipoop; giraffes="very-long-necked-animals";
 spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"

''')

    def test_no_semis_header_splitter(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg['From'] = 'test@dom.ain'
        msg['References'] = SPACE.join('<%d@dom.ain>' % i for i in range(10))
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), """\
From: test@dom.ain
References: <0@dom.ain> <1@dom.ain> <2@dom.ain> <3@dom.ain> <4@dom.ain>
 <5@dom.ain> <6@dom.ain> <7@dom.ain> <8@dom.ain> <9@dom.ain>

Test""")

    def test_last_split_chunk_does_not_fit(self):
        eq = self.ndiffAssertEqual
        h = Header('Subject: the first part of this is short, but_the_second'
            '_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line'
            '_all_by_itself')
        eq(h.encode(), """\
Subject: the first part of this is short,
 but_the_second_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself""")

    def test_splittable_leading_char_followed_by_overlong_unsplittable(self):
        eq = self.ndiffAssertEqual
        h = Header(', but_the_second'
            '_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line'
            '_all_by_itself')
        eq(h.encode(), """\
,
 but_the_second_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself""")

    def test_multiple_splittable_leading_char_followed_by_overlong_unsplittable(self):
        eq = self.ndiffAssertEqual
        h = Header(', , but_the_second'
            '_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line'
            '_all_by_itself')
        eq(h.encode(), """\
, ,
 but_the_second_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself""")

    def test_trailing_splittable_on_overlong_unsplittable(self):
        eq = self.ndiffAssertEqual
        h = Header('this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself;')
        eq(h.encode(), "this_part_does_not_fit_within_maxlinelen_and_thus_should_"
            "be_on_a_line_all_by_itself;")

    def test_trailing_splittable_on_overlong_unsplittable_with_leading_splittable(self):
        eq = self.ndiffAssertEqual
        h = Header('; '
            'this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself; ')
        eq(h.encode(), """\
;
 this_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself; """)

    def test_long_header_with_multiple_sequential_split_chars(self):
        eq = self.ndiffAssertEqual
        h = Header('This is a long line that has two whitespaces  in a row.  '
            'This used to cause truncation of the header when folded')
        eq(h.encode(), """\
This is a long line that has two whitespaces  in a row.  This used to cause
 truncation of the header when folded""")

    def test_splitter_split_on_punctuation_only_if_fws_with_header(self):
        eq = self.ndiffAssertEqual
        h = Header('thisverylongheaderhas;semicolons;and,commas,but'
            'they;arenotlegal;fold,points')
        eq(h.encode(), "thisverylongheaderhas;semicolons;and,commas,butthey;"
                        "arenotlegal;fold,points")

    def test_leading_splittable_in_the_middle_just_before_overlong_last_part(self):
        eq = self.ndiffAssertEqual
        h = Header('this is a  test where we need to have more than one line '
            'before; our final line that is just too big to fit;; '
            'this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself;')
        eq(h.encode(), """\
this is a  test where we need to have more than one line before;
 our final line that is just too big to fit;;
 this_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself;""")

    def test_overlong_last_part_followed_by_split_point(self):
        eq = self.ndiffAssertEqual
        h = Header('this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself ')
        eq(h.encode(), "this_part_does_not_fit_within_maxlinelen_and_thus_"
                        "should_be_on_a_line_all_by_itself ")

    def test_multiline_with_overlong_parts_separated_by_two_split_points(self):
        eq = self.ndiffAssertEqual
        h = Header('this_is_a__test_where_we_need_to_have_more_than_one_line_'
            'before_our_final_line_; ; '
            'this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself; ')
        eq(h.encode(), """\
this_is_a__test_where_we_need_to_have_more_than_one_line_before_our_final_line_;
 ;
 this_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself; """)

    def test_multiline_with_overlong_last_part_followed_by_split_point(self):
        eq = self.ndiffAssertEqual
        h = Header('this is a test where we need to have more than one line '
            'before our final line; ; '
            'this_part_does_not_fit_within_maxlinelen_and_thus_should_'
            'be_on_a_line_all_by_itself; ')
        eq(h.encode(), """\
this is a test where we need to have more than one line before our final line;
 ;
 this_part_does_not_fit_within_maxlinelen_and_thus_should_be_on_a_line_all_by_itself; """)

    def test_long_header_with_whitespace_runs(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg['From'] = 'test@dom.ain'
        msg['References'] = SPACE.join(['<foo@dom.ain>  '] * 10)
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), """\
From: test@dom.ain
References: <foo@dom.ain>   <foo@dom.ain>   <foo@dom.ain>   <foo@dom.ain>
   <foo@dom.ain>   <foo@dom.ain>   <foo@dom.ain>   <foo@dom.ain>
   <foo@dom.ain>   <foo@dom.ain>\x20\x20

Test""")

    def test_long_run_with_semi_header_splitter(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg['From'] = 'test@dom.ain'
        msg['References'] = SPACE.join(['<foo@dom.ain>'] * 10) + '; abc'
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), """\
From: test@dom.ain
References: <foo@dom.ain> <foo@dom.ain> <foo@dom.ain> <foo@dom.ain>
 <foo@dom.ain> <foo@dom.ain> <foo@dom.ain> <foo@dom.ain> <foo@dom.ain>
 <foo@dom.ain>; abc

Test""")

    def test_splitter_split_on_punctuation_only_if_fws(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg['From'] = 'test@dom.ain'
        msg['References'] = ('thisverylongheaderhas;semicolons;and,commas,but'
            'they;arenotlegal;fold,points')
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        # XXX the space after the header should not be there.
        eq(sfp.getvalue(), """\
From: test@dom.ain
References:\x20
 thisverylongheaderhas;semicolons;and,commas,butthey;arenotlegal;fold,points

Test""")

    def test_no_split_long_header(self):
        eq = self.ndiffAssertEqual
        hstr = 'References: ' + 'x' * 80
        h = Header(hstr)
        # These come on two lines because Headers are really field value
        # classes and don't really know about their field names.
        eq(h.encode(), """\
References:
 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx""")
        h = Header('x' * 80)
        eq(h.encode(), 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx')

    def test_splitting_multiple_long_lines(self):
        eq = self.ndiffAssertEqual
        hstr = """\
from babylon.socal-raves.org (localhost [127.0.0.1]); by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81; for <mailman-admin@babylon.socal-raves.org>; Sat, 2 Feb 2002 17:00:06 -0800 (PST)
\tfrom babylon.socal-raves.org (localhost [127.0.0.1]); by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81; for <mailman-admin@babylon.socal-raves.org>; Sat, 2 Feb 2002 17:00:06 -0800 (PST)
\tfrom babylon.socal-raves.org (localhost [127.0.0.1]); by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81; for <mailman-admin@babylon.socal-raves.org>; Sat, 2 Feb 2002 17:00:06 -0800 (PST)
"""
        h = Header(hstr, continuation_ws='\t')
        eq(h.encode(), """\
from babylon.socal-raves.org (localhost [127.0.0.1]);
 by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81;
 for <mailman-admin@babylon.socal-raves.org>;
 Sat, 2 Feb 2002 17:00:06 -0800 (PST)
\tfrom babylon.socal-raves.org (localhost [127.0.0.1]);
 by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81;
 for <mailman-admin@babylon.socal-raves.org>;
 Sat, 2 Feb 2002 17:00:06 -0800 (PST)
\tfrom babylon.socal-raves.org (localhost [127.0.0.1]);
 by babylon.socal-raves.org (Postfix) with ESMTP id B570E51B81;
 for <mailman-admin@babylon.socal-raves.org>;
 Sat, 2 Feb 2002 17:00:06 -0800 (PST)""")

    def test_splitting_first_line_only_is_long(self):
        eq = self.ndiffAssertEqual
        hstr = """\
from modemcable093.139-201-24.que.mc.videotron.ca ([24.201.139.93] helo=cthulhu.gerg.ca)
\tby kronos.mems-exchange.org with esmtp (Exim 4.05)
\tid 17k4h5-00034i-00
\tfor test@mems-exchange.org; Wed, 28 Aug 2002 11:25:20 -0400"""
        h = Header(hstr, maxlinelen=78, header_name='Received',
                   continuation_ws='\t')
        eq(h.encode(), """\
from modemcable093.139-201-24.que.mc.videotron.ca ([24.201.139.93]
 helo=cthulhu.gerg.ca)
\tby kronos.mems-exchange.org with esmtp (Exim 4.05)
\tid 17k4h5-00034i-00
\tfor test@mems-exchange.org; Wed, 28 Aug 2002 11:25:20 -0400""")

    def test_long_8bit_header(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        h = Header('Britische Regierung gibt', 'iso-8859-1',
                    header_name='Subject')
        h.append('gr\xfcnes Licht f\xfcr Offshore-Windkraftprojekte')
        eq(h.encode(maxlinelen=76), """\
=?iso-8859-1?q?Britische_Regierung_gibt_gr=FCnes_Licht_f=FCr_Offs?=
 =?iso-8859-1?q?hore-Windkraftprojekte?=""")
        msg['Subject'] = h
        eq(msg.as_string(maxheaderlen=76), """\
Subject: =?iso-8859-1?q?Britische_Regierung_gibt_gr=FCnes_Licht_f=FCr_Offs?=
 =?iso-8859-1?q?hore-Windkraftprojekte?=

""")
        eq(msg.as_string(maxheaderlen=0), """\
Subject: =?iso-8859-1?q?Britische_Regierung_gibt_gr=FCnes_Licht_f=FCr_Offshore-Windkraftprojekte?=

""")

    def test_long_8bit_header_no_charset(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        header_string = ('Britische Regierung gibt gr\xfcnes Licht '
                         'f\xfcr Offshore-Windkraftprojekte '
                         '<a-very-long-address@example.com>')
        msg['Reply-To'] = header_string
        eq(msg.as_string(maxheaderlen=78), """\
Reply-To: =?utf-8?q?Britische_Regierung_gibt_gr=C3=BCnes_Licht_f=C3=BCr_Offs?=
 =?utf-8?q?hore-Windkraftprojekte_=3Ca-very-long-address=40example=2Ecom=3E?=

""")
        msg = Message()
        msg['Reply-To'] = Header(header_string,
                                 header_name='Reply-To')
        eq(msg.as_string(maxheaderlen=78), """\
Reply-To: =?utf-8?q?Britische_Regierung_gibt_gr=C3=BCnes_Licht_f=C3=BCr_Offs?=
 =?utf-8?q?hore-Windkraftprojekte_=3Ca-very-long-address=40example=2Ecom=3E?=

""")

    def test_long_to_header(self):
        eq = self.ndiffAssertEqual
        to = ('"Someone Test #A" <someone@eecs.umich.edu>,'
              '<someone@eecs.umich.edu>, '
              '"Someone Test #B" <someone@umich.edu>, '
              '"Someone Test #C" <someone@eecs.umich.edu>, '
              '"Someone Test #D" <someone@eecs.umich.edu>')
        msg = Message()
        msg['To'] = to
        eq(msg.as_string(maxheaderlen=78), '''\
To: "Someone Test #A" <someone@eecs.umich.edu>,<someone@eecs.umich.edu>,
 "Someone Test #B" <someone@umich.edu>,
 "Someone Test #C" <someone@eecs.umich.edu>,
 "Someone Test #D" <someone@eecs.umich.edu>

''')

    def test_long_line_after_append(self):
        eq = self.ndiffAssertEqual
        s = 'This is an example of string which has almost the limit of header length.'
        h = Header(s)
        h.append('Add another line.')
        eq(h.encode(maxlinelen=76), """\
This is an example of string which has almost the limit of header length.
 Add another line.""")

    def test_shorter_line_with_append(self):
        eq = self.ndiffAssertEqual
        s = 'This is a shorter line.'
        h = Header(s)
        h.append('Add another sentence. (Surprise?)')
        eq(h.encode(),
           'This is a shorter line. Add another sentence. (Surprise?)')

    def test_long_field_name(self):
        eq = self.ndiffAssertEqual
        fn = 'X-Very-Very-Very-Long-Header-Name'
        gs = ('Die Mieter treten hier ein werden mit einem Foerderband '
              'komfortabel den Korridor entlang, an s\xfcdl\xfcndischen '
              'Wandgem\xe4lden vorbei, gegen die rotierenden Klingen '
              'bef\xf6rdert. ')
        h = Header(gs, 'iso-8859-1', header_name=fn)
        # BAW: this seems broken because the first line is too long
        eq(h.encode(maxlinelen=76), """\
=?iso-8859-1?q?Die_Mieter_treten_hier_e?=
 =?iso-8859-1?q?in_werden_mit_einem_Foerderband_komfortabel_den_Korridor_e?=
 =?iso-8859-1?q?ntlang=2C_an_s=FCdl=FCndischen_Wandgem=E4lden_vorbei=2C_ge?=
 =?iso-8859-1?q?gen_die_rotierenden_Klingen_bef=F6rdert=2E_?=""")

    def test_long_received_header(self):
        h = ('from FOO.TLD (vizworld.acl.foo.tld [123.452.678.9]) '
             'by hrothgar.la.mastaler.com (tmda-ofmipd) with ESMTP; '
             'Wed, 05 Mar 2003 18:10:18 -0700')
        msg = Message()
        msg['Received-1'] = Header(h, continuation_ws='\t')
        msg['Received-2'] = h
        # This should be splitting on spaces not semicolons.
        self.ndiffAssertEqual(msg.as_string(maxheaderlen=78), """\
Received-1: from FOO.TLD (vizworld.acl.foo.tld [123.452.678.9]) by
 hrothgar.la.mastaler.com (tmda-ofmipd) with ESMTP;
 Wed, 05 Mar 2003 18:10:18 -0700
Received-2: from FOO.TLD (vizworld.acl.foo.tld [123.452.678.9]) by
 hrothgar.la.mastaler.com (tmda-ofmipd) with ESMTP;
 Wed, 05 Mar 2003 18:10:18 -0700

""")

    def test_string_headerinst_eq(self):
        h = ('<15975.17901.207240.414604@sgigritzmann1.mathematik.'
             'tu-muenchen.de> (David Bremner\'s message of '
             '"Thu, 6 Mar 2003 13:58:21 +0100")')
        msg = Message()
        msg['Received-1'] = Header(h, header_name='Received-1',
                                   continuation_ws='\t')
        msg['Received-2'] = h
        # XXX The space after the ':' should not be there.
        self.ndiffAssertEqual(msg.as_string(maxheaderlen=78), """\
Received-1:\x20
 <15975.17901.207240.414604@sgigritzmann1.mathematik.tu-muenchen.de> (David
 Bremner's message of \"Thu, 6 Mar 2003 13:58:21 +0100\")
Received-2:\x20
 <15975.17901.207240.414604@sgigritzmann1.mathematik.tu-muenchen.de> (David
 Bremner's message of \"Thu, 6 Mar 2003 13:58:21 +0100\")

""")

    def test_long_unbreakable_lines_with_continuation(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        t = """\
iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp"""
        msg['Face-1'] = t
        msg['Face-2'] = Header(t, header_name='Face-2')
        msg['Face-3'] = ' ' + t
        # XXX This splitting is all wrong.  It the first value line should be
        # snug against the field name or the space after the header not there.
        eq(msg.as_string(maxheaderlen=78), """\
Face-1:\x20
 iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp
Face-2:\x20
 iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp
Face-3:\x20
 iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp

""")

    def test_another_long_multiline_header(self):
        eq = self.ndiffAssertEqual
        m = ('Received: from siimage.com '
             '([172.25.1.3]) by zima.siliconimage.com with '
             'Microsoft SMTPSVC(5.0.2195.4905); '
             'Wed, 16 Oct 2002 07:41:11 -0700')
        msg = email.message_from_string(m)
        eq(msg.as_string(maxheaderlen=78), '''\
Received: from siimage.com ([172.25.1.3]) by zima.siliconimage.com with
 Microsoft SMTPSVC(5.0.2195.4905); Wed, 16 Oct 2002 07:41:11 -0700

''')

    def test_long_lines_with_different_header(self):
        eq = self.ndiffAssertEqual
        h = ('List-Unsubscribe: '
             '<http://lists.sourceforge.net/lists/listinfo/spamassassin-talk>,'
             '        <mailto:spamassassin-talk-request@lists.sourceforge.net'
             '?subject=unsubscribe>')
        msg = Message()
        msg['List'] = h
        msg['List'] = Header(h, header_name='List')
        eq(msg.as_string(maxheaderlen=78), """\
List: List-Unsubscribe:
 <http://lists.sourceforge.net/lists/listinfo/spamassassin-talk>,
        <mailto:spamassassin-talk-request@lists.sourceforge.net?subject=unsubscribe>
List: List-Unsubscribe:
 <http://lists.sourceforge.net/lists/listinfo/spamassassin-talk>,
        <mailto:spamassassin-talk-request@lists.sourceforge.net?subject=unsubscribe>

""")

    def test_long_rfc2047_header_with_embedded_fws(self):
        h = Header(textwrap.dedent("""\
            We're going to pretend this header is in a non-ascii character set
            \tto see if line wrapping with encoded words and embedded
               folding white space works"""),
                   charset='utf-8',
                   header_name='Test')
        self.assertEqual(h.encode()+'\n', textwrap.dedent("""\
            =?utf-8?q?We=27re_going_to_pretend_this_header_is_in_a_non-ascii_chara?=
             =?utf-8?q?cter_set?=
             =?utf-8?q?_to_see_if_line_wrapping_with_encoded_words_and_embedded?=
             =?utf-8?q?_folding_white_space_works?=""")+'\n')



# Test mangling of "From " lines in the body of a message
class TestFromMangling(unittest.TestCase):
    def setUp(self):
        self.msg = Message()
        self.msg['From'] = 'aaa@bbb.org'
        self.msg.set_payload("""\
From the desk of A.A.A.:
Blah blah blah
""")

    def test_mangled_from(self):
        s = StringIO()
        g = Generator(s, mangle_from_=True)
        g.flatten(self.msg)
        self.assertEqual(s.getvalue(), """\
From: aaa@bbb.org

>From the desk of A.A.A.:
Blah blah blah
""")

    def test_dont_mangle_from(self):
        s = StringIO()
        g = Generator(s, mangle_from_=False)
        g.flatten(self.msg)
        self.assertEqual(s.getvalue(), """\
From: aaa@bbb.org

From the desk of A.A.A.:
Blah blah blah
""")

    def test_mangle_from_in_preamble_and_epilog(self):
        s = StringIO()
        g = Generator(s, mangle_from_=True)
        msg = email.message_from_string(textwrap.dedent("""\
            From: foo@bar.com
            Mime-Version: 1.0
            Content-Type: multipart/mixed; boundary=XXX

            From somewhere unknown

            --XXX
            Content-Type: text/plain

            foo

            --XXX--

            From somewhere unknowable
            """))
        g.flatten(msg)
        self.assertEqual(len([1 for x in s.getvalue().split('\n')
                                  if x.startswith('>From ')]), 2)

    def test_mangled_from_with_bad_bytes(self):
        source = textwrap.dedent("""\
            Content-Type: text/plain; charset="utf-8"
            MIME-Version: 1.0
            Content-Transfer-Encoding: 8bit
            From: aaa@bbb.org

        """).encode('utf-8')
        msg = email.message_from_bytes(source + b'From R\xc3\xb6lli\n')
        b = BytesIO()
        g = BytesGenerator(b, mangle_from_=True)
        g.flatten(msg)
        self.assertEqual(b.getvalue(), source + b'>From R\xc3\xb6lli\n')

    def test_multipart_with_bad_bytes_in_cte(self):
        # bpo30835
        source = textwrap.dedent("""\
            From: aperson@example.com
            Content-Type: multipart/mixed; boundary="1"
            Content-Transfer-Encoding: \xc8
        """).encode('utf-8')
        msg = email.message_from_bytes(source)


# Test the basic MIMEAudio class
class TestMIMEAudio(unittest.TestCase):
    def _make_audio(self, ext):
        with openfile(f'sndhdr.{ext}', 'rb') as fp:
            self._audiodata = fp.read()
        self._au = MIMEAudio(self._audiodata)

    def test_guess_minor_type(self):
        for ext, subtype in {
            'aifc': 'x-aiff',
            'aiff': 'x-aiff',
            'wav': 'x-wav',
            'au': 'basic',
        }.items():
            self._make_audio(ext)
            subtype = ext if subtype is None else subtype
            self.assertEqual(self._au.get_content_type(), f'audio/{subtype}')

    def test_encoding(self):
        self._make_audio('au')
        payload = self._au.get_payload()
        self.assertEqual(base64.decodebytes(bytes(payload, 'ascii')),
                         self._audiodata)

    def test_checkSetMinor(self):
        self._make_audio('au')
        au = MIMEAudio(self._audiodata, 'fish')
        self.assertEqual(au.get_content_type(), 'audio/fish')

    def test_add_header(self):
        self._make_audio('au')
        eq = self.assertEqual
        self._au.add_header('Content-Disposition', 'attachment',
                            filename='sndhdr.au')
        eq(self._au['content-disposition'],
           'attachment; filename="sndhdr.au"')
        eq(self._au.get_params(header='content-disposition'),
           [('attachment', ''), ('filename', 'sndhdr.au')])
        eq(self._au.get_param('filename', header='content-disposition'),
           'sndhdr.au')
        missing = []
        eq(self._au.get_param('attachment', header='content-disposition'), '')
        self.assertIs(self._au.get_param(
            'foo', failobj=missing,
            header='content-disposition'), missing)
        # Try some missing stuff
        self.assertIs(self._au.get_param('foobar', missing), missing)
        self.assertIs(self._au.get_param('attachment', missing,
                                         header='foobar'), missing)



# Test the basic MIMEImage class
class TestMIMEImage(unittest.TestCase):
    def _make_image(self, ext):
        with openfile(f'python.{ext}', 'rb') as fp:
            self._imgdata = fp.read()
        self._im = MIMEImage(self._imgdata)

    def test_guess_minor_type(self):
        for ext, subtype in {
            'bmp': None,
            'exr': None,
            'gif': None,
            'jpg': 'jpeg',
            'pbm': None,
            'pgm': None,
            'png': None,
            'ppm': None,
            'ras': 'rast',
            'sgi': 'rgb',
            'tiff': None,
            'webp': None,
            'xbm': None,
        }.items():
            self._make_image(ext)
            subtype = ext if subtype is None else subtype
            self.assertEqual(self._im.get_content_type(), f'image/{subtype}')

    def test_encoding(self):
        self._make_image('gif')
        payload = self._im.get_payload()
        self.assertEqual(base64.decodebytes(bytes(payload, 'ascii')),
                         self._imgdata)

    def test_checkSetMinor(self):
        self._make_image('gif')
        im = MIMEImage(self._imgdata, 'fish')
        self.assertEqual(im.get_content_type(), 'image/fish')

    def test_add_header(self):
        self._make_image('gif')
        eq = self.assertEqual
        self._im.add_header('Content-Disposition', 'attachment',
                            filename='dingusfish.gif')
        eq(self._im['content-disposition'],
           'attachment; filename="dingusfish.gif"')
        eq(self._im.get_params(header='content-disposition'),
           [('attachment', ''), ('filename', 'dingusfish.gif')])
        eq(self._im.get_param('filename', header='content-disposition'),
           'dingusfish.gif')
        missing = []
        eq(self._im.get_param('attachment', header='content-disposition'), '')
        self.assertIs(self._im.get_param('foo', failobj=missing,
                                         header='content-disposition'), missing)
        # Try some missing stuff
        self.assertIs(self._im.get_param('foobar', missing), missing)
        self.assertIs(self._im.get_param('attachment', missing,
                                         header='foobar'), missing)


# Test the basic MIMEApplication class
class TestMIMEApplication(unittest.TestCase):
    def test_headers(self):
        eq = self.assertEqual
        msg = MIMEApplication(b'\xfa\xfb\xfc\xfd\xfe\xff')
        eq(msg.get_content_type(), 'application/octet-stream')
        eq(msg['content-transfer-encoding'], 'base64')

    def test_body(self):
        eq = self.assertEqual
        bytesdata = b'\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytesdata)
        # whitespace in the cte encoded block is RFC-irrelevant.
        eq(msg.get_payload().strip(), '+vv8/f7/')
        eq(msg.get_payload(decode=True), bytesdata)

    def test_binary_body_with_encode_7or8bit(self):
        # Issue 17171.
        bytesdata = b'\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytesdata, _encoder=encoders.encode_7or8bit)
        # Treated as a string, this will be invalid code points.
        self.assertEqual(msg.get_payload(), '\uFFFD' * len(bytesdata))
        self.assertEqual(msg.get_payload(decode=True), bytesdata)
        self.assertEqual(msg['Content-Transfer-Encoding'], '8bit')
        s = BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        wireform = s.getvalue()
        msg2 = email.message_from_bytes(wireform)
        self.assertEqual(msg.get_payload(), '\uFFFD' * len(bytesdata))
        self.assertEqual(msg2.get_payload(decode=True), bytesdata)
        self.assertEqual(msg2['Content-Transfer-Encoding'], '8bit')

    def test_binary_body_with_encode_noop(self):
        # Issue 16564: This does not produce an RFC valid message, since to be
        # valid it should have a CTE of binary.  But the below works in
        # Python2, and is documented as working this way.
        bytesdata = b'\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytesdata, _encoder=encoders.encode_noop)
        # Treated as a string, this will be invalid code points.
        self.assertEqual(msg.get_payload(), '\uFFFD' * len(bytesdata))
        self.assertEqual(msg.get_payload(decode=True), bytesdata)
        s = BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        wireform = s.getvalue()
        msg2 = email.message_from_bytes(wireform)
        self.assertEqual(msg.get_payload(), '\uFFFD' * len(bytesdata))
        self.assertEqual(msg2.get_payload(decode=True), bytesdata)

    def test_binary_body_with_unicode_linend_encode_noop(self):
        # Issue 19003: This is a variation on #16564.
        bytesdata = b'\x0b\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytesdata, _encoder=encoders.encode_noop)
        self.assertEqual(msg.get_payload(decode=True), bytesdata)
        s = BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        wireform = s.getvalue()
        msg2 = email.message_from_bytes(wireform)
        self.assertEqual(msg2.get_payload(decode=True), bytesdata)

    def test_binary_body_with_encode_quopri(self):
        # Issue 14360.
        bytesdata = b'\xfa\xfb\xfc\xfd\xfe\xff '
        msg = MIMEApplication(bytesdata, _encoder=encoders.encode_quopri)
        self.assertEqual(msg.get_payload(), '=FA=FB=FC=FD=FE=FF=20')
        self.assertEqual(msg.get_payload(decode=True), bytesdata)
        self.assertEqual(msg['Content-Transfer-Encoding'], 'quoted-printable')
        s = BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        wireform = s.getvalue()
        msg2 = email.message_from_bytes(wireform)
        self.assertEqual(msg.get_payload(), '=FA=FB=FC=FD=FE=FF=20')
        self.assertEqual(msg2.get_payload(decode=True), bytesdata)
        self.assertEqual(msg2['Content-Transfer-Encoding'], 'quoted-printable')

    def test_binary_body_with_encode_base64(self):
        bytesdata = b'\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytesdata, _encoder=encoders.encode_base64)
        self.assertEqual(msg.get_payload(), '+vv8/f7/\n')
        self.assertEqual(msg.get_payload(decode=True), bytesdata)
        s = BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        wireform = s.getvalue()
        msg2 = email.message_from_bytes(wireform)
        self.assertEqual(msg.get_payload(), '+vv8/f7/\n')
        self.assertEqual(msg2.get_payload(decode=True), bytesdata)


# Test the basic MIMEText class
class TestMIMEText(unittest.TestCase):
    def setUp(self):
        self._msg = MIMEText('hello there')

    def test_types(self):
        eq = self.assertEqual
        eq(self._msg.get_content_type(), 'text/plain')
        eq(self._msg.get_param('charset'), 'us-ascii')
        missing = []
        self.assertIs(self._msg.get_param('foobar', missing), missing)
        self.assertIs(self._msg.get_param('charset', missing, header='foobar'),
                      missing)

    def test_payload(self):
        self.assertEqual(self._msg.get_payload(), 'hello there')
        self.assertFalse(self._msg.is_multipart())

    def test_charset(self):
        eq = self.assertEqual
        msg = MIMEText('hello there', _charset='us-ascii')
        eq(msg.get_charset().input_charset, 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')
        # Also accept a Charset instance
        charset = Charset('utf-8')
        charset.body_encoding = None
        msg = MIMEText('hello there', _charset=charset)
        eq(msg.get_charset().input_charset, 'utf-8')
        eq(msg['content-type'], 'text/plain; charset="utf-8"')
        eq(msg.get_payload(), 'hello there')

    def test_7bit_input(self):
        eq = self.assertEqual
        msg = MIMEText('hello there', _charset='us-ascii')
        eq(msg.get_charset().input_charset, 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')

    def test_7bit_input_no_charset(self):
        eq = self.assertEqual
        msg = MIMEText('hello there')
        eq(msg.get_charset(), 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')
        self.assertIn('hello there', msg.as_string())

    def test_utf8_input(self):
        teststr = '\u043a\u0438\u0440\u0438\u043b\u0438\u0446\u0430'
        eq = self.assertEqual
        msg = MIMEText(teststr, _charset='utf-8')
        eq(msg.get_charset().output_charset, 'utf-8')
        eq(msg['content-type'], 'text/plain; charset="utf-8"')
        eq(msg.get_payload(decode=True), teststr.encode('utf-8'))

    @unittest.skip("can't fix because of backward compat in email5, "
        "will fix in email6")
    def test_utf8_input_no_charset(self):
        teststr = '\u043a\u0438\u0440\u0438\u043b\u0438\u0446\u0430'
        self.assertRaises(UnicodeEncodeError, MIMEText, teststr)



# Test complicated multipart/* messages
class TestMultipart(TestEmailBase):
    def setUp(self):
        with openfile('python.gif', 'rb') as fp:
            data = fp.read()
        container = MIMEBase('multipart', 'mixed', boundary='BOUNDARY')
        image = MIMEImage(data, name='dingusfish.gif')
        image.add_header('content-disposition', 'attachment',
                         filename='dingusfish.gif')
        intro = MIMEText('''\
Hi there,

This is the dingus fish.
''')
        container.attach(intro)
        container.attach(image)
        container['From'] = 'Barry <barry@digicool.com>'
        container['To'] = 'Dingus Lovers <cravindogs@cravindogs.com>'
        container['Subject'] = 'Here is your dingus fish'

        now = 987809702.54848599
        timetuple = time.localtime(now)
        if timetuple[-1] == 0:
            tzsecs = time.timezone
        else:
            tzsecs = time.altzone
        if tzsecs > 0:
            sign = '-'
        else:
            sign = '+'
        tzoffset = ' %s%04d' % (sign, tzsecs / 36)
        container['Date'] = time.strftime(
            '%a, %d %b %Y %H:%M:%S',
            time.localtime(now)) + tzoffset
        self._msg = container
        self._im = image
        self._txt = intro

    def test_hierarchy(self):
        # convenience
        eq = self.assertEqual
        raises = self.assertRaises
        # tests
        m = self._msg
        self.assertTrue(m.is_multipart())
        eq(m.get_content_type(), 'multipart/mixed')
        eq(len(m.get_payload()), 2)
        raises(IndexError, m.get_payload, 2)
        m0 = m.get_payload(0)
        m1 = m.get_payload(1)
        self.assertIs(m0, self._txt)
        self.assertIs(m1, self._im)
        eq(m.get_payload(), [m0, m1])
        self.assertFalse(m0.is_multipart())
        self.assertFalse(m1.is_multipart())

    def test_empty_multipart_idempotent(self):
        text = """\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain


--BOUNDARY


--BOUNDARY--
"""
        msg = Parser().parsestr(text)
        self.ndiffAssertEqual(text, msg.as_string())

    def test_no_parts_in_a_multipart_with_none_epilogue(self):
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.set_boundary('BOUNDARY')
        self.ndiffAssertEqual(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY

--BOUNDARY--
''')

    def test_no_parts_in_a_multipart_with_empty_epilogue(self):
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.preamble = ''
        outer.epilogue = ''
        outer.set_boundary('BOUNDARY')
        self.ndiffAssertEqual(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain


--BOUNDARY

--BOUNDARY--
''')

    def test_one_part_in_a_multipart(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.set_boundary('BOUNDARY')
        msg = MIMEText('hello world')
        outer.attach(msg)
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--
''')

    def test_seq_parts_in_a_multipart_with_empty_preamble(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.preamble = ''
        msg = MIMEText('hello world')
        outer.attach(msg)
        outer.set_boundary('BOUNDARY')
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain


--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--
''')


    def test_seq_parts_in_a_multipart_with_none_preamble(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.preamble = None
        msg = MIMEText('hello world')
        outer.attach(msg)
        outer.set_boundary('BOUNDARY')
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--
''')


    def test_seq_parts_in_a_multipart_with_none_epilogue(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.epilogue = None
        msg = MIMEText('hello world')
        outer.attach(msg)
        outer.set_boundary('BOUNDARY')
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--
''')


    def test_seq_parts_in_a_multipart_with_empty_epilogue(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.epilogue = ''
        msg = MIMEText('hello world')
        outer.attach(msg)
        outer.set_boundary('BOUNDARY')
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--
''')


    def test_seq_parts_in_a_multipart_with_nl_epilogue(self):
        eq = self.ndiffAssertEqual
        outer = MIMEBase('multipart', 'mixed')
        outer['Subject'] = 'A subject'
        outer['To'] = 'aperson@dom.ain'
        outer['From'] = 'bperson@dom.ain'
        outer.epilogue = '\n'
        msg = MIMEText('hello world')
        outer.attach(msg)
        outer.set_boundary('BOUNDARY')
        eq(outer.as_string(), '''\
Content-Type: multipart/mixed; boundary="BOUNDARY"
MIME-Version: 1.0
Subject: A subject
To: aperson@dom.ain
From: bperson@dom.ain

--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

hello world
--BOUNDARY--

''')

    def test_message_external_body(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_36.txt')
        eq(len(msg.get_payload()), 2)
        msg1 = msg.get_payload(1)
        eq(msg1.get_content_type(), 'multipart/alternative')
        eq(len(msg1.get_payload()), 2)
        for subpart in msg1.get_payload():
            eq(subpart.get_content_type(), 'message/external-body')
            eq(len(subpart.get_payload()), 1)
            subsubpart = subpart.get_payload(0)
            eq(subsubpart.get_content_type(), 'text/plain')

    def test_double_boundary(self):
        # msg_37.txt is a multipart that contains two dash-boundary's in a
        # row.  Our interpretation of RFC 2046 calls for ignoring the second
        # and subsequent boundaries.
        msg = self._msgobj('msg_37.txt')
        self.assertEqual(len(msg.get_payload()), 3)

    def test_nested_inner_contains_outer_boundary(self):
        eq = self.ndiffAssertEqual
        # msg_38.txt has an inner part that contains outer boundaries.  My
        # interpretation of RFC 2046 (based on sections 5.1 and 5.1.2) say
        # these are illegal and should be interpreted as unterminated inner
        # parts.
        msg = self._msgobj('msg_38.txt')
        sfp = StringIO()
        iterators._structure(msg, sfp)
        eq(sfp.getvalue(), """\
multipart/mixed
    multipart/mixed
        multipart/alternative
            text/plain
        text/plain
    text/plain
    text/plain
""")

    def test_nested_with_same_boundary(self):
        eq = self.ndiffAssertEqual
        # msg 39.txt is similarly evil in that it's got inner parts that use
        # the same boundary as outer parts.  Again, I believe the way this is
        # parsed is closest to the spirit of RFC 2046
        msg = self._msgobj('msg_39.txt')
        sfp = StringIO()
        iterators._structure(msg, sfp)
        eq(sfp.getvalue(), """\
multipart/mixed
    multipart/mixed
        multipart/alternative
        application/octet-stream
        application/octet-stream
    text/plain
""")

    def test_boundary_in_non_multipart(self):
        msg = self._msgobj('msg_40.txt')
        self.assertEqual(msg.as_string(), '''\
MIME-Version: 1.0
Content-Type: text/html; boundary="--961284236552522269"

----961284236552522269
Content-Type: text/html;
Content-Transfer-Encoding: 7Bit

<html></html>

----961284236552522269--
''')

    def test_boundary_with_leading_space(self):
        eq = self.assertEqual
        msg = email.message_from_string('''\
MIME-Version: 1.0
Content-Type: multipart/mixed; boundary="    XXXX"

--    XXXX
Content-Type: text/plain


--    XXXX
Content-Type: text/plain

--    XXXX--
''')
        self.assertTrue(msg.is_multipart())
        eq(msg.get_boundary(), '    XXXX')
        eq(len(msg.get_payload()), 2)

    def test_boundary_without_trailing_newline(self):
        m = Parser().parsestr("""\
Content-Type: multipart/mixed; boundary="===============0012394164=="
MIME-Version: 1.0

--===============0012394164==
Content-Type: image/file1.jpg
MIME-Version: 1.0
Content-Transfer-Encoding: base64

YXNkZg==
--===============0012394164==--""")
        self.assertEqual(m.get_payload(0).get_payload(), 'YXNkZg==')

    def test_mimebase_default_policy(self):
        m = MIMEBase('multipart', 'mixed')
        self.assertIs(m.policy, email.policy.compat32)

    def test_mimebase_custom_policy(self):
        m = MIMEBase('multipart', 'mixed', policy=email.policy.default)
        self.assertIs(m.policy, email.policy.default)

# Test some badly formatted messages
class TestNonConformant(TestEmailBase):

    def test_parse_missing_minor_type(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_14.txt')
        eq(msg.get_content_type(), 'text/plain')
        eq(msg.get_content_maintype(), 'text')
        eq(msg.get_content_subtype(), 'plain')

    # test_defect_handling
    def test_same_boundary_inner_outer(self):
        msg = self._msgobj('msg_15.txt')
        # XXX We can probably eventually do better
        inner = msg.get_payload(0)
        self.assertTrue(hasattr(inner, 'defects'))
        self.assertEqual(len(inner.defects), 1)
        self.assertIsInstance(inner.defects[0],
                              errors.StartBoundaryNotFoundDefect)

    # test_defect_handling
    def test_multipart_no_boundary(self):
        msg = self._msgobj('msg_25.txt')
        self.assertIsInstance(msg.get_payload(), str)
        self.assertEqual(len(msg.defects), 2)
        self.assertIsInstance(msg.defects[0],
                              errors.NoBoundaryInMultipartDefect)
        self.assertIsInstance(msg.defects[1],
                              errors.MultipartInvariantViolationDefect)

    multipart_msg = textwrap.dedent("""\
        Date: Wed, 14 Nov 2007 12:56:23 GMT
        From: foo@bar.invalid
        To: foo@bar.invalid
        Subject: Content-Transfer-Encoding: base64 and multipart
        MIME-Version: 1.0
        Content-Type: multipart/mixed;
            boundary="===============3344438784458119861=="{}

        --===============3344438784458119861==
        Content-Type: text/plain

        Test message

        --===============3344438784458119861==
        Content-Type: application/octet-stream
        Content-Transfer-Encoding: base64

        YWJj

        --===============3344438784458119861==--
        """)

    # test_defect_handling
    def test_multipart_invalid_cte(self):
        msg = self._str_msg(
            self.multipart_msg.format("\nContent-Transfer-Encoding: base64"))
        self.assertEqual(len(msg.defects), 1)
        self.assertIsInstance(msg.defects[0],
            errors.InvalidMultipartContentTransferEncodingDefect)

    # test_defect_handling
    def test_multipart_no_cte_no_defect(self):
        msg = self._str_msg(self.multipart_msg.format(''))
        self.assertEqual(len(msg.defects), 0)

    # test_defect_handling
    def test_multipart_valid_cte_no_defect(self):
        for cte in ('7bit', '8bit', 'BINary'):
            msg = self._str_msg(
                self.multipart_msg.format(
                    "\nContent-Transfer-Encoding: {}".format(cte)))
            self.assertEqual(len(msg.defects), 0)

    # test_headerregistry.TestContentTyopeHeader invalid_1 and invalid_2.
    def test_invalid_content_type(self):
        eq = self.assertEqual
        neq = self.ndiffAssertEqual
        msg = Message()
        # RFC 2045, $5.2 says invalid yields text/plain
        msg['Content-Type'] = 'text'
        eq(msg.get_content_maintype(), 'text')
        eq(msg.get_content_subtype(), 'plain')
        eq(msg.get_content_type(), 'text/plain')
        # Clear the old value and try something /really/ invalid
        del msg['content-type']
        msg['Content-Type'] = 'foo'
        eq(msg.get_content_maintype(), 'text')
        eq(msg.get_content_subtype(), 'plain')
        eq(msg.get_content_type(), 'text/plain')
        # Still, make sure that the message is idempotently generated
        s = StringIO()
        g = Generator(s)
        g.flatten(msg)
        neq(s.getvalue(), 'Content-Type: foo\n\n')

    def test_no_start_boundary(self):
        eq = self.ndiffAssertEqual
        msg = self._msgobj('msg_31.txt')
        eq(msg.get_payload(), """\
--BOUNDARY
Content-Type: text/plain

message 1

--BOUNDARY
Content-Type: text/plain

message 2

--BOUNDARY--
""")

    def test_no_separating_blank_line(self):
        eq = self.ndiffAssertEqual
        msg = self._msgobj('msg_35.txt')
        eq(msg.as_string(), """\
From: aperson@dom.ain
To: bperson@dom.ain
Subject: here's something interesting

counter to RFC 2822, there's no separating newline here
""")

    # test_defect_handling
    def test_lying_multipart(self):
        msg = self._msgobj('msg_41.txt')
        self.assertTrue(hasattr(msg, 'defects'))
        self.assertEqual(len(msg.defects), 2)
        self.assertIsInstance(msg.defects[0],
                              errors.NoBoundaryInMultipartDefect)
        self.assertIsInstance(msg.defects[1],
                              errors.MultipartInvariantViolationDefect)

    # test_defect_handling
    def test_missing_start_boundary(self):
        outer = self._msgobj('msg_42.txt')
        # The message structure is:
        #
        # multipart/mixed
        #    text/plain
        #    message/rfc822
        #        multipart/mixed [*]
        #
        # [*] This message is missing its start boundary
        bad = outer.get_payload(1).get_payload(0)
        self.assertEqual(len(bad.defects), 1)
        self.assertIsInstance(bad.defects[0],
                              errors.StartBoundaryNotFoundDefect)

    # test_defect_handling
    def test_first_line_is_continuation_header(self):
        eq = self.assertEqual
        m = ' Line 1\nSubject: test\n\nbody'
        msg = email.message_from_string(m)
        eq(msg.keys(), ['Subject'])
        eq(msg.get_payload(), 'body')
        eq(len(msg.defects), 1)
        self.assertDefectsEqual(msg.defects,
                                 [errors.FirstHeaderLineIsContinuationDefect])
        eq(msg.defects[0].line, ' Line 1\n')

    # test_defect_handling
    def test_missing_header_body_separator(self):
        # Our heuristic if we see a line that doesn't look like a header (no
        # leading whitespace but no ':') is to assume that the blank line that
        # separates the header from the body is missing, and to stop parsing
        # headers and start parsing the body.
        msg = self._str_msg('Subject: test\nnot a header\nTo: abc\n\nb\n')
        self.assertEqual(msg.keys(), ['Subject'])
        self.assertEqual(msg.get_payload(), 'not a header\nTo: abc\n\nb\n')
        self.assertDefectsEqual(msg.defects,
                                [errors.MissingHeaderBodySeparatorDefect])


# Test RFC 2047 header encoding and decoding
class TestRFC2047(TestEmailBase):
    def test_rfc2047_multiline(self):
        eq = self.assertEqual
        s = """Re: =?mac-iceland?q?r=8Aksm=9Arg=8Cs?= baz
 foo bar =?mac-iceland?q?r=8Aksm=9Arg=8Cs?="""
        dh = decode_header(s)
        eq(dh, [
            (b'Re: ', None),
            (b'r\x8aksm\x9arg\x8cs', 'mac-iceland'),
            (b' baz foo bar ', None),
            (b'r\x8aksm\x9arg\x8cs', 'mac-iceland')])
        header = make_header(dh)
        eq(str(header),
           'Re: r\xe4ksm\xf6rg\xe5s baz foo bar r\xe4ksm\xf6rg\xe5s')
        self.ndiffAssertEqual(header.encode(maxlinelen=76), """\
Re: =?mac-iceland?q?r=8Aksm=9Arg=8Cs?= baz foo bar =?mac-iceland?q?r=8Aksm?=
 =?mac-iceland?q?=9Arg=8Cs?=""")

    def test_whitespace_keeper_unicode(self):
        eq = self.assertEqual
        s = '=?ISO-8859-1?Q?Andr=E9?= Pirard <pirard@dom.ain>'
        dh = decode_header(s)
        eq(dh, [(b'Andr\xe9', 'iso-8859-1'),
                (b' Pirard <pirard@dom.ain>', None)])
        header = str(make_header(dh))
        eq(header, 'Andr\xe9 Pirard <pirard@dom.ain>')

    def test_whitespace_keeper_unicode_2(self):
        eq = self.assertEqual
        s = 'The =?iso-8859-1?b?cXVpY2sgYnJvd24gZm94?= jumped over the =?iso-8859-1?b?bGF6eSBkb2c=?='
        dh = decode_header(s)
        eq(dh, [(b'The ', None), (b'quick brown fox', 'iso-8859-1'),
                (b' jumped over the ', None), (b'lazy dog', 'iso-8859-1')])
        hu = str(make_header(dh))
        eq(hu, 'The quick brown fox jumped over the lazy dog')

    def test_rfc2047_missing_whitespace(self):
        s = 'Sm=?ISO-8859-1?B?9g==?=rg=?ISO-8859-1?B?5Q==?=sbord'
        dh = decode_header(s)
        self.assertEqual(dh, [(b'Sm', None), (b'\xf6', 'iso-8859-1'),
                              (b'rg', None), (b'\xe5', 'iso-8859-1'),
                              (b'sbord', None)])

    def test_rfc2047_with_whitespace(self):
        s = 'Sm =?ISO-8859-1?B?9g==?= rg =?ISO-8859-1?B?5Q==?= sbord'
        dh = decode_header(s)
        self.assertEqual(dh, [(b'Sm ', None), (b'\xf6', 'iso-8859-1'),
                              (b' rg ', None), (b'\xe5', 'iso-8859-1'),
                              (b' sbord', None)])

    def test_rfc2047_B_bad_padding(self):
        s = '=?iso-8859-1?B?%s?='
        data = [                                # only test complete bytes
            ('dm==', b'v'), ('dm=', b'v'), ('dm', b'v'),
            ('dmk=', b'vi'), ('dmk', b'vi')
          ]
        for q, a in data:
            dh = decode_header(s % q)
            self.assertEqual(dh, [(a, 'iso-8859-1')])

    def test_rfc2047_Q_invalid_digits(self):
        # issue 10004.
        s = '=?iso-8859-1?Q?andr=e9=zz?='
        self.assertEqual(decode_header(s),
                        [(b'andr\xe9=zz', 'iso-8859-1')])

    def test_rfc2047_rfc2047_1(self):
        # 1st testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'a', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_2(self):
        # 2nd testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a?= b)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'a', 'iso-8859-1'), (b' b)', None)])

    def test_rfc2047_rfc2047_3(self):
        # 3rd testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a?= =?ISO-8859-1?Q?b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'ab', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_4(self):
        # 4th testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a?=  =?ISO-8859-1?Q?b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'ab', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_5a(self):
        # 5th testcase at end of rfc2047 newline is \r\n
        s = '(=?ISO-8859-1?Q?a?=\r\n    =?ISO-8859-1?Q?b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'ab', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_5b(self):
        # 5th testcase at end of rfc2047 newline is \n
        s = '(=?ISO-8859-1?Q?a?=\n    =?ISO-8859-1?Q?b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'ab', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_6(self):
        # 6th testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a_b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'a b', 'iso-8859-1'), (b')', None)])

    def test_rfc2047_rfc2047_7(self):
        # 7th testcase at end of rfc2047
        s = '(=?ISO-8859-1?Q?a?= =?ISO-8859-2?Q?_b?=)'
        self.assertEqual(decode_header(s),
            [(b'(', None), (b'a', 'iso-8859-1'), (b' b', 'iso-8859-2'),
             (b')', None)])
        self.assertEqual(make_header(decode_header(s)).encode(), s.lower())
        self.assertEqual(str(make_header(decode_header(s))), '(a b)')

    def test_multiline_header(self):
        s = '=?windows-1252?q?=22M=FCller_T=22?=\r\n <T.Mueller@xxx.com>'
        self.assertEqual(decode_header(s),
            [(b'"M\xfcller T"', 'windows-1252'),
             (b'<T.Mueller@xxx.com>', None)])
        self.assertEqual(make_header(decode_header(s)).encode(),
                         ''.join(s.splitlines()))
        self.assertEqual(str(make_header(decode_header(s))),
                         '"Müller T" <T.Mueller@xxx.com>')


# Test the MIMEMessage class
class TestMIMEMessage(TestEmailBase):
    def setUp(self):
        with openfile('msg_11.txt', encoding="utf-8") as fp:
            self._text = fp.read()

    def test_type_error(self):
        self.assertRaises(TypeError, MIMEMessage, 'a plain string')

    def test_valid_argument(self):
        eq = self.assertEqual
        subject = 'A sub-message'
        m = Message()
        m['Subject'] = subject
        r = MIMEMessage(m)
        eq(r.get_content_type(), 'message/rfc822')
        payload = r.get_payload()
        self.assertIsInstance(payload, list)
        eq(len(payload), 1)
        subpart = payload[0]
        self.assertIs(subpart, m)
        eq(subpart['subject'], subject)

    def test_bad_multipart(self):
        msg1 = Message()
        msg1['Subject'] = 'subpart 1'
        msg2 = Message()
        msg2['Subject'] = 'subpart 2'
        r = MIMEMessage(msg1)
        self.assertRaises(errors.MultipartConversionError, r.attach, msg2)

    def test_generate(self):
        # First craft the message to be encapsulated
        m = Message()
        m['Subject'] = 'An enclosed message'
        m.set_payload('Here is the body of the message.\n')
        r = MIMEMessage(m)
        r['Subject'] = 'The enclosing message'
        s = StringIO()
        g = Generator(s)
        g.flatten(r)
        self.assertEqual(s.getvalue(), """\
Content-Type: message/rfc822
MIME-Version: 1.0
Subject: The enclosing message

Subject: An enclosed message

Here is the body of the message.
""")

    def test_parse_message_rfc822(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_11.txt')
        eq(msg.get_content_type(), 'message/rfc822')
        payload = msg.get_payload()
        self.assertIsInstance(payload, list)
        eq(len(payload), 1)
        submsg = payload[0]
        self.assertIsInstance(submsg, Message)
        eq(submsg['subject'], 'An enclosed message')
        eq(submsg.get_payload(), 'Here is the body of the message.\n')

    def test_dsn(self):
        eq = self.assertEqual
        # msg 16 is a Delivery Status Notification, see RFC 1894
        msg = self._msgobj('msg_16.txt')
        eq(msg.get_content_type(), 'multipart/report')
        self.assertTrue(msg.is_multipart())
        eq(len(msg.get_payload()), 3)
        # Subpart 1 is a text/plain, human readable section
        subpart = msg.get_payload(0)
        eq(subpart.get_content_type(), 'text/plain')
        eq(subpart.get_payload(), """\
This report relates to a message you sent with the following header fields:

  Message-id: <002001c144a6$8752e060$56104586@oxy.edu>
  Date: Sun, 23 Sep 2001 20:10:55 -0700
  From: "Ian T. Henry" <henryi@oxy.edu>
  To: SoCal Raves <scr@socal-raves.org>
  Subject: [scr] yeah for Ians!!

Your message cannot be delivered to the following recipients:

  Recipient address: jangel1@cougar.noc.ucla.edu
  Reason: recipient reached disk quota

""")
        # Subpart 2 contains the machine parsable DSN information.  It
        # consists of two blocks of headers, represented by two nested Message
        # objects.
        subpart = msg.get_payload(1)
        eq(subpart.get_content_type(), 'message/delivery-status')
        eq(len(subpart.get_payload()), 2)
        # message/delivery-status should treat each block as a bunch of
        # headers, i.e. a bunch of Message objects.
        dsn1 = subpart.get_payload(0)
        self.assertIsInstance(dsn1, Message)
        eq(dsn1['original-envelope-id'], '0GK500B4HD0888@cougar.noc.ucla.edu')
        eq(dsn1.get_param('dns', header='reporting-mta'), '')
        # Try a missing one <wink>
        eq(dsn1.get_param('nsd', header='reporting-mta'), None)
        dsn2 = subpart.get_payload(1)
        self.assertIsInstance(dsn2, Message)
        eq(dsn2['action'], 'failed')
        eq(dsn2.get_params(header='original-recipient'),
           [('rfc822', ''), ('jangel1@cougar.noc.ucla.edu', '')])
        eq(dsn2.get_param('rfc822', header='final-recipient'), '')
        # Subpart 3 is the original message
        subpart = msg.get_payload(2)
        eq(subpart.get_content_type(), 'message/rfc822')
        payload = subpart.get_payload()
        self.assertIsInstance(payload, list)
        eq(len(payload), 1)
        subsubpart = payload[0]
        self.assertIsInstance(subsubpart, Message)
        eq(subsubpart.get_content_type(), 'text/plain')
        eq(subsubpart['message-id'],
           '<002001c144a6$8752e060$56104586@oxy.edu>')

    def test_epilogue(self):
        eq = self.ndiffAssertEqual
        with openfile('msg_21.txt', encoding="utf-8") as fp:
            text = fp.read()
        msg = Message()
        msg['From'] = 'aperson@dom.ain'
        msg['To'] = 'bperson@dom.ain'
        msg['Subject'] = 'Test'
        msg.preamble = 'MIME message'
        msg.epilogue = 'End of MIME message\n'
        msg1 = MIMEText('One')
        msg2 = MIMEText('Two')
        msg.add_header('Content-Type', 'multipart/mixed', boundary='BOUNDARY')
        msg.attach(msg1)
        msg.attach(msg2)
        sfp = StringIO()
        g = Generator(sfp)
        g.flatten(msg)
        eq(sfp.getvalue(), text)

    def test_no_nl_preamble(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg['From'] = 'aperson@dom.ain'
        msg['To'] = 'bperson@dom.ain'
        msg['Subject'] = 'Test'
        msg.preamble = 'MIME message'
        msg.epilogue = ''
        msg1 = MIMEText('One')
        msg2 = MIMEText('Two')
        msg.add_header('Content-Type', 'multipart/mixed', boundary='BOUNDARY')
        msg.attach(msg1)
        msg.attach(msg2)
        eq(msg.as_string(), """\
From: aperson@dom.ain
To: bperson@dom.ain
Subject: Test
Content-Type: multipart/mixed; boundary="BOUNDARY"

MIME message
--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

One
--BOUNDARY
Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

Two
--BOUNDARY--
""")

    def test_default_type(self):
        eq = self.assertEqual
        with openfile('msg_30.txt', encoding="utf-8") as fp:
            msg = email.message_from_file(fp)
        container1 = msg.get_payload(0)
        eq(container1.get_default_type(), 'message/rfc822')
        eq(container1.get_content_type(), 'message/rfc822')
        container2 = msg.get_payload(1)
        eq(container2.get_default_type(), 'message/rfc822')
        eq(container2.get_content_type(), 'message/rfc822')
        container1a = container1.get_payload(0)
        eq(container1a.get_default_type(), 'text/plain')
        eq(container1a.get_content_type(), 'text/plain')
        container2a = container2.get_payload(0)
        eq(container2a.get_default_type(), 'text/plain')
        eq(container2a.get_content_type(), 'text/plain')

    def test_default_type_with_explicit_container_type(self):
        eq = self.assertEqual
        with openfile('msg_28.txt', encoding="utf-8") as fp:
            msg = email.message_from_file(fp)
        container1 = msg.get_payload(0)
        eq(container1.get_default_type(), 'message/rfc822')
        eq(container1.get_content_type(), 'message/rfc822')
        container2 = msg.get_payload(1)
        eq(container2.get_default_type(), 'message/rfc822')
        eq(container2.get_content_type(), 'message/rfc822')
        container1a = container1.get_payload(0)
        eq(container1a.get_default_type(), 'text/plain')
        eq(container1a.get_content_type(), 'text/plain')
        container2a = container2.get_payload(0)
        eq(container2a.get_default_type(), 'text/plain')
        eq(container2a.get_content_type(), 'text/plain')

    def test_default_type_non_parsed(self):
        eq = self.assertEqual
        neq = self.ndiffAssertEqual
        # Set up container
        container = MIMEMultipart('digest', 'BOUNDARY')
        container.epilogue = ''
        # Set up subparts
        subpart1a = MIMEText('message 1\n')
        subpart2a = MIMEText('message 2\n')
        subpart1 = MIMEMessage(subpart1a)
        subpart2 = MIMEMessage(subpart2a)
        container.attach(subpart1)
        container.attach(subpart2)
        eq(subpart1.get_content_type(), 'message/rfc822')
        eq(subpart1.get_default_type(), 'message/rfc822')
        eq(subpart2.get_content_type(), 'message/rfc822')
        eq(subpart2.get_default_type(), 'message/rfc822')
        neq(container.as_string(0), '''\
Content-Type: multipart/digest; boundary="BOUNDARY"
MIME-Version: 1.0

--BOUNDARY
Content-Type: message/rfc822
MIME-Version: 1.0

Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

message 1

--BOUNDARY
Content-Type: message/rfc822
MIME-Version: 1.0

Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

message 2

--BOUNDARY--
''')
        del subpart1['content-type']
        del subpart1['mime-version']
        del subpart2['content-type']
        del subpart2['mime-version']
        eq(subpart1.get_content_type(), 'message/rfc822')
        eq(subpart1.get_default_type(), 'message/rfc822')
        eq(subpart2.get_content_type(), 'message/rfc822')
        eq(subpart2.get_default_type(), 'message/rfc822')
        neq(container.as_string(0), '''\
Content-Type: multipart/digest; boundary="BOUNDARY"
MIME-Version: 1.0

--BOUNDARY

Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

message 1

--BOUNDARY

Content-Type: text/plain; charset="us-ascii"
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit

message 2

--BOUNDARY--
''')

    def test_mime_attachments_in_constructor(self):
        eq = self.assertEqual
        text1 = MIMEText('')
        text2 = MIMEText('')
        msg = MIMEMultipart(_subparts=(text1, text2))
        eq(len(msg.get_payload()), 2)
        eq(msg.get_payload(0), text1)
        eq(msg.get_payload(1), text2)

    def test_default_multipart_constructor(self):
        msg = MIMEMultipart()
        self.assertTrue(msg.is_multipart())

    def test_multipart_default_policy(self):
        msg = MIMEMultipart()
        msg['To'] = 'a@b.com'
        msg['To'] = 'c@d.com'
        self.assertEqual(msg.get_all('to'), ['a@b.com', 'c@d.com'])

    def test_multipart_custom_policy(self):
        msg = MIMEMultipart(policy=email.policy.default)
        msg['To'] = 'a@b.com'
        with self.assertRaises(ValueError) as cm:
            msg['To'] = 'c@d.com'
        self.assertEqual(str(cm.exception),
                         'There may be at most 1 To headers in a message')


# Test the NonMultipart class
class TestNonMultipart(TestEmailBase):
    def test_nonmultipart_is_not_multipart(self):
        msg = MIMENonMultipart('text', 'plain')
        self.assertFalse(msg.is_multipart())

    def test_attach_raises_exception(self):
        msg = Message()
        msg['Subject'] = 'subpart 1'
        r = MIMENonMultipart('text', 'plain')
        self.assertRaises(errors.MultipartConversionError, r.attach, msg)


# A general test of parser->model->generator idempotency.  IOW, read a message
# in, parse it into a message object tree, then without touching the tree,
# regenerate the plain text.  The original text and the transformed text
# should be identical.  Note: that we ignore the Unix-From since that may
# contain a changed date.
class TestIdempotent(TestEmailBase):

    linesep = '\n'

    def _msgobj(self, filename):
        with openfile(filename, encoding="utf-8") as fp:
            data = fp.read()
        msg = email.message_from_string(data)
        return msg, data

    def _idempotent(self, msg, text, unixfrom=False):
        eq = self.ndiffAssertEqual
        s = StringIO()
        g = Generator(s, maxheaderlen=0)
        g.flatten(msg, unixfrom=unixfrom)
        eq(text, s.getvalue())

    def test_parse_text_message(self):
        eq = self.assertEqual
        msg, text = self._msgobj('msg_01.txt')
        eq(msg.get_content_type(), 'text/plain')
        eq(msg.get_content_maintype(), 'text')
        eq(msg.get_content_subtype(), 'plain')
        eq(msg.get_params()[1], ('charset', 'us-ascii'))
        eq(msg.get_param('charset'), 'us-ascii')
        eq(msg.preamble, None)
        eq(msg.epilogue, None)
        self._idempotent(msg, text)

    def test_parse_untyped_message(self):
        eq = self.assertEqual
        msg, text = self._msgobj('msg_03.txt')
        eq(msg.get_content_type(), 'text/plain')
        eq(msg.get_params(), None)
        eq(msg.get_param('charset'), None)
        self._idempotent(msg, text)

    def test_simple_multipart(self):
        msg, text = self._msgobj('msg_04.txt')
        self._idempotent(msg, text)

    def test_MIME_digest(self):
        msg, text = self._msgobj('msg_02.txt')
        self._idempotent(msg, text)

    def test_long_header(self):
        msg, text = self._msgobj('msg_27.txt')
        self._idempotent(msg, text)

    def test_MIME_digest_with_part_headers(self):
        msg, text = self._msgobj('msg_28.txt')
        self._idempotent(msg, text)

    def test_mixed_with_image(self):
        msg, text = self._msgobj('msg_06.txt')
        self._idempotent(msg, text)

    def test_multipart_report(self):
        msg, text = self._msgobj('msg_05.txt')
        self._idempotent(msg, text)

    def test_dsn(self):
        msg, text = self._msgobj('msg_16.txt')
        self._idempotent(msg, text)

    def test_preamble_epilogue(self):
        msg, text = self._msgobj('msg_21.txt')
        self._idempotent(msg, text)

    def test_multipart_one_part(self):
        msg, text = self._msgobj('msg_23.txt')
        self._idempotent(msg, text)

    def test_multipart_no_parts(self):
        msg, text = self._msgobj('msg_24.txt')
        self._idempotent(msg, text)

    def test_no_start_boundary(self):
        msg, text = self._msgobj('msg_31.txt')
        self._idempotent(msg, text)

    def test_rfc2231_charset(self):
        msg, text = self._msgobj('msg_32.txt')
        self._idempotent(msg, text)

    def test_more_rfc2231_parameters(self):
        msg, text = self._msgobj('msg_33.txt')
        self._idempotent(msg, text)

    def test_text_plain_in_a_multipart_digest(self):
        msg, text = self._msgobj('msg_34.txt')
        self._idempotent(msg, text)

    def test_nested_multipart_mixeds(self):
        msg, text = self._msgobj('msg_12a.txt')
        self._idempotent(msg, text)

    def test_message_external_body_idempotent(self):
        msg, text = self._msgobj('msg_36.txt')
        self._idempotent(msg, text)

    def test_message_delivery_status(self):
        msg, text = self._msgobj('msg_43.txt')
        self._idempotent(msg, text, unixfrom=True)

    def test_message_signed_idempotent(self):
        msg, text = self._msgobj('msg_45.txt')
        self._idempotent(msg, text)

    def test_content_type(self):
        eq = self.assertEqual
        # Get a message object and reset the seek pointer for other tests
        msg, text = self._msgobj('msg_05.txt')
        eq(msg.get_content_type(), 'multipart/report')
        # Test the Content-Type: parameters
        params = {}
        for pk, pv in msg.get_params():
            params[pk] = pv
        eq(params['report-type'], 'delivery-status')
        eq(params['boundary'], 'D1690A7AC1.996856090/mail.example.com')
        eq(msg.preamble, 'This is a MIME-encapsulated message.' + self.linesep)
        eq(msg.epilogue, self.linesep)
        eq(len(msg.get_payload()), 3)
        # Make sure the subparts are what we expect
        msg1 = msg.get_payload(0)
        eq(msg1.get_content_type(), 'text/plain')
        eq(msg1.get_payload(), 'Yadda yadda yadda' + self.linesep)
        msg2 = msg.get_payload(1)
        eq(msg2.get_content_type(), 'text/plain')
        eq(msg2.get_payload(), 'Yadda yadda yadda' + self.linesep)
        msg3 = msg.get_payload(2)
        eq(msg3.get_content_type(), 'message/rfc822')
        self.assertIsInstance(msg3, Message)
        payload = msg3.get_payload()
        self.assertIsInstance(payload, list)
        eq(len(payload), 1)
        msg4 = payload[0]
        self.assertIsInstance(msg4, Message)
        eq(msg4.get_payload(), 'Yadda yadda yadda' + self.linesep)

    def test_parser(self):
        eq = self.assertEqual
        msg, text = self._msgobj('msg_06.txt')
        # Check some of the outer headers
        eq(msg.get_content_type(), 'message/rfc822')
        # Make sure the payload is a list of exactly one sub-Message, and that
        # that submessage has a type of text/plain
        payload = msg.get_payload()
        self.assertIsInstance(payload, list)
        eq(len(payload), 1)
        msg1 = payload[0]
        self.assertIsInstance(msg1, Message)
        eq(msg1.get_content_type(), 'text/plain')
        self.assertIsInstance(msg1.get_payload(), str)
        eq(msg1.get_payload(), self.linesep)



# Test various other bits of the package's functionality
class TestMiscellaneous(TestEmailBase):
    def test_message_from_string(self):
        with openfile('msg_01.txt', encoding="utf-8") as fp:
            text = fp.read()
        msg = email.message_from_string(text)
        s = StringIO()
        # Don't wrap/continue long headers since we're trying to test
        # idempotency.
        g = Generator(s, maxheaderlen=0)
        g.flatten(msg)
        self.assertEqual(text, s.getvalue())

    def test_message_from_file(self):
        with openfile('msg_01.txt', encoding="utf-8") as fp:
            text = fp.read()
            fp.seek(0)
            msg = email.message_from_file(fp)
            s = StringIO()
            # Don't wrap/continue long headers since we're trying to test
            # idempotency.
            g = Generator(s, maxheaderlen=0)
            g.flatten(msg)
            self.assertEqual(text, s.getvalue())

    def test_message_from_string_with_class(self):
        with openfile('msg_01.txt', encoding="utf-8") as fp:
            text = fp.read()

        # Create a subclass
        class MyMessage(Message):
            pass

        msg = email.message_from_string(text, MyMessage)
        self.assertIsInstance(msg, MyMessage)
        # Try something more complicated
        with openfile('msg_02.txt', encoding="utf-8") as fp:
            text = fp.read()
        msg = email.message_from_string(text, MyMessage)
        for subpart in msg.walk():
            self.assertIsInstance(subpart, MyMessage)

    def test_message_from_file_with_class(self):
        # Create a subclass
        class MyMessage(Message):
            pass

        with openfile('msg_01.txt', encoding="utf-8") as fp:
            msg = email.message_from_file(fp, MyMessage)
        self.assertIsInstance(msg, MyMessage)
        # Try something more complicated
        with openfile('msg_02.txt', encoding="utf-8") as fp:
            msg = email.message_from_file(fp, MyMessage)
        for subpart in msg.walk():
            self.assertIsInstance(subpart, MyMessage)

    def test_custom_message_does_not_require_arguments(self):
        class MyMessage(Message):
            def __init__(self):
                super().__init__()
        msg = self._str_msg("Subject: test\n\ntest", MyMessage)
        self.assertIsInstance(msg, MyMessage)

    def test__all__(self):
        module = __import__('email')
        self.assertEqual(sorted(module.__all__), [
            'base64mime', 'charset', 'encoders', 'errors', 'feedparser',
            'generator', 'header', 'iterators', 'message',
            'message_from_binary_file', 'message_from_bytes',
            'message_from_file', 'message_from_string', 'mime', 'parser',
            'quoprimime', 'utils',
            ])

    def test_formatdate(self):
        now = time.time()
        self.assertEqual(utils.parsedate(utils.formatdate(now))[:6],
                         time.gmtime(now)[:6])

    def test_formatdate_localtime(self):
        now = time.time()
        self.assertEqual(
            utils.parsedate(utils.formatdate(now, localtime=True))[:6],
            time.localtime(now)[:6])

    def test_formatdate_usegmt(self):
        now = time.time()
        self.assertEqual(
            utils.formatdate(now, localtime=False),
            time.strftime('%a, %d %b %Y %H:%M:%S -0000', time.gmtime(now)))
        self.assertEqual(
            utils.formatdate(now, localtime=False, usegmt=True),
            time.strftime('%a, %d %b %Y %H:%M:%S GMT', time.gmtime(now)))

    # parsedate and parsedate_tz will become deprecated interfaces someday
    def test_parsedate_returns_None_for_invalid_strings(self):
        # See also test_parsedate_to_datetime_with_invalid_raises_valueerror
        # in test_utils.
        invalid_dates = [
            '',
            ' ',
            '0',
            'A Complete Waste of Time',
            'Wed, 3 Apr 2002 12.34.56.78+0800',
            '17 June , 2022',
            'Friday, -Nov-82 16:14:55 EST',
            'Friday, Nov--82 16:14:55 EST',
            'Friday, 19-Nov- 16:14:55 EST',
        ]
        for dtstr in invalid_dates:
            with self.subTest(dtstr=dtstr):
                self.assertIsNone(utils.parsedate(dtstr))
                self.assertIsNone(utils.parsedate_tz(dtstr))
        # Not a part of the spec but, but this has historically worked:
        self.assertIsNone(utils.parsedate(None))
        self.assertIsNone(utils.parsedate_tz(None))

    def test_parsedate_compact(self):
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14:58:26 +0800'),
                         (2002, 4, 3, 14, 58, 26, 0, 1, -1, 28800))
        # The FWS after the comma is optional
        self.assertEqual(utils.parsedate_tz('Wed,3 Apr 2002 14:58:26 +0800'),
                         (2002, 4, 3, 14, 58, 26, 0, 1, -1, 28800))
        # The comma is optional
        self.assertEqual(utils.parsedate_tz('Wed 3 Apr 2002 14:58:26 +0800'),
                         (2002, 4, 3, 14, 58, 26, 0, 1, -1, 28800))

    def test_parsedate_no_dayofweek(self):
        eq = self.assertEqual
        eq(utils.parsedate_tz('5 Feb 2003 13:47:26 -0800'),
           (2003, 2, 5, 13, 47, 26, 0, 1, -1, -28800))
        eq(utils.parsedate_tz('February 5, 2003 13:47:26 -0800'),
           (2003, 2, 5, 13, 47, 26, 0, 1, -1, -28800))

    def test_parsedate_no_space_before_positive_offset(self):
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14:58:26+0800'),
           (2002, 4, 3, 14, 58, 26, 0, 1, -1, 28800))

    def test_parsedate_no_space_before_negative_offset(self):
        # Issue 1155362: we already handled '+' for this case.
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14:58:26-0800'),
           (2002, 4, 3, 14, 58, 26, 0, 1, -1, -28800))

    def test_parsedate_accepts_time_with_dots(self):
        eq = self.assertEqual
        eq(utils.parsedate_tz('5 Feb 2003 13.47.26 -0800'),
           (2003, 2, 5, 13, 47, 26, 0, 1, -1, -28800))
        eq(utils.parsedate_tz('5 Feb 2003 13.47 -0800'),
           (2003, 2, 5, 13, 47, 0, 0, 1, -1, -28800))

    def test_parsedate_rfc_850(self):
        self.assertEqual(utils.parsedate_tz('Friday, 19-Nov-82 16:14:55 EST'),
           (1982, 11, 19, 16, 14, 55, 0, 1, -1, -18000))

    def test_parsedate_no_seconds(self):
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14:58 +0800'),
                         (2002, 4, 3, 14, 58, 0, 0, 1, -1, 28800))

    def test_parsedate_dot_time_delimiter(self):
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14.58.26 +0800'),
                         (2002, 4, 3, 14, 58, 26, 0, 1, -1, 28800))
        self.assertEqual(utils.parsedate_tz('Wed, 3 Apr 2002 14.58 +0800'),
                         (2002, 4, 3, 14, 58, 0, 0, 1, -1, 28800))

    def test_parsedate_acceptable_to_time_functions(self):
        eq = self.assertEqual
        timetup = utils.parsedate('5 Feb 2003 13:47:26 -0800')
        t = int(time.mktime(timetup))
        eq(time.localtime(t)[:6], timetup[:6])
        eq(int(time.strftime('%Y', timetup)), 2003)
        timetup = utils.parsedate_tz('5 Feb 2003 13:47:26 -0800')
        t = int(time.mktime(timetup[:9]))
        eq(time.localtime(t)[:6], timetup[:6])
        eq(int(time.strftime('%Y', timetup[:9])), 2003)

    def test_mktime_tz(self):
        self.assertEqual(utils.mktime_tz((1970, 1, 1, 0, 0, 0,
                                          -1, -1, -1, 0)), 0)
        self.assertEqual(utils.mktime_tz((1970, 1, 1, 0, 0, 0,
                                          -1, -1, -1, 1234)), -1234)

    def test_parsedate_y2k(self):
        """Test for parsing a date with a two-digit year.

        Parsing a date with a two-digit year should return the correct
        four-digit year. RFC822 allows two-digit years, but RFC2822 (which
        obsoletes RFC822) requires four-digit years.

        """
        self.assertEqual(utils.parsedate_tz('25 Feb 03 13:47:26 -0800'),
                         utils.parsedate_tz('25 Feb 2003 13:47:26 -0800'))
        self.assertEqual(utils.parsedate_tz('25 Feb 71 13:47:26 -0800'),
                         utils.parsedate_tz('25 Feb 1971 13:47:26 -0800'))

    def test_parseaddr_empty(self):
        self.assertEqual(utils.parseaddr('<>'), ('', ''))
        self.assertEqual(utils.formataddr(utils.parseaddr('<>')), '')

    def test_parseaddr_multiple_domains(self):
        self.assertEqual(
            utils.parseaddr('a@b@c'),
            ('', '')
        )
        self.assertEqual(
            utils.parseaddr('a@b.c@c'),
            ('', '')
        )
        self.assertEqual(
            utils.parseaddr('a@172.17.0.1@c'),
            ('', '')
        )

    def test_noquote_dump(self):
        self.assertEqual(
            utils.formataddr(('A Silly Person', 'person@dom.ain')),
            'A Silly Person <person@dom.ain>')

    def test_escape_dump(self):
        self.assertEqual(
            utils.formataddr(('A (Very) Silly Person', 'person@dom.ain')),
            r'"A (Very) Silly Person" <person@dom.ain>')
        self.assertEqual(
            utils.parseaddr(r'"A \(Very\) Silly Person" <person@dom.ain>'),
            ('A (Very) Silly Person', 'person@dom.ain'))
        a = r'A \(Special\) Person'
        b = 'person@dom.ain'
        self.assertEqual(utils.parseaddr(utils.formataddr((a, b))), (a, b))

    def test_escape_backslashes(self):
        self.assertEqual(
            utils.formataddr((r'Arthur \Backslash\ Foobar', 'person@dom.ain')),
            r'"Arthur \\Backslash\\ Foobar" <person@dom.ain>')
        a = r'Arthur \Backslash\ Foobar'
        b = 'person@dom.ain'
        self.assertEqual(utils.parseaddr(utils.formataddr((a, b))), (a, b))

    def test_quotes_unicode_names(self):
        # issue 1690608.  email.utils.formataddr() should be rfc2047 aware.
        name = "H\u00e4ns W\u00fcrst"
        addr = 'person@dom.ain'
        utf8_base64 = "=?utf-8?b?SMOkbnMgV8O8cnN0?= <person@dom.ain>"
        latin1_quopri = "=?iso-8859-1?q?H=E4ns_W=FCrst?= <person@dom.ain>"
        self.assertEqual(utils.formataddr((name, addr)), utf8_base64)
        self.assertEqual(utils.formataddr((name, addr), 'iso-8859-1'),
            latin1_quopri)

    def test_accepts_any_charset_like_object(self):
        # issue 1690608.  email.utils.formataddr() should be rfc2047 aware.
        name = "H\u00e4ns W\u00fcrst"
        addr = 'person@dom.ain'
        utf8_base64 = "=?utf-8?b?SMOkbnMgV8O8cnN0?= <person@dom.ain>"
        foobar = "FOOBAR"
        class CharsetMock:
            def header_encode(self, string):
                return foobar
        mock = CharsetMock()
        mock_expected = "%s <%s>" % (foobar, addr)
        self.assertEqual(utils.formataddr((name, addr), mock), mock_expected)
        self.assertEqual(utils.formataddr((name, addr), Charset('utf-8')),
            utf8_base64)

    def test_invalid_charset_like_object_raises_error(self):
        # issue 1690608.  email.utils.formataddr() should be rfc2047 aware.
        name = "H\u00e4ns W\u00fcrst"
        addr = 'person@dom.ain'
        # An object without a header_encode method:
        bad_charset = object()
        self.assertRaises(AttributeError, utils.formataddr, (name, addr),
            bad_charset)

    def test_unicode_address_raises_error(self):
        # issue 1690608.  email.utils.formataddr() should be rfc2047 aware.
        addr = 'pers\u00f6n@dom.in'
        self.assertRaises(UnicodeError, utils.formataddr, (None, addr))
        self.assertRaises(UnicodeError, utils.formataddr, ("Name", addr))

    def test_name_with_dot(self):
        x = 'John X. Doe <jxd@example.com>'
        y = '"John X. Doe" <jxd@example.com>'
        a, b = ('John X. Doe', 'jxd@example.com')
        self.assertEqual(utils.parseaddr(x), (a, b))
        self.assertEqual(utils.parseaddr(y), (a, b))
        # formataddr() quotes the name if there's a dot in it
        self.assertEqual(utils.formataddr((a, b)), y)

    def test_parseaddr_preserves_quoted_pairs_in_addresses(self):
        # issue 10005.  Note that in the third test the second pair of
        # backslashes is not actually a quoted pair because it is not inside a
        # comment or quoted string: the address being parsed has a quoted
        # string containing a quoted backslash, followed by 'example' and two
        # backslashes, followed by another quoted string containing a space and
        # the word 'example'.  parseaddr copies those two backslashes
        # literally.  Per rfc5322 this is not technically correct since a \ may
        # not appear in an address outside of a quoted string.  It is probably
        # a sensible Postel interpretation, though.
        eq = self.assertEqual
        eq(utils.parseaddr('""example" example"@example.com'),
          ('', '""example" example"@example.com'))
        eq(utils.parseaddr('"\\"example\\" example"@example.com'),
          ('', '"\\"example\\" example"@example.com'))
        eq(utils.parseaddr('"\\\\"example\\\\" example"@example.com'),
          ('', '"\\\\"example\\\\" example"@example.com'))

    def test_parseaddr_preserves_spaces_in_local_part(self):
        # issue 9286.  A normal RFC5322 local part should not contain any
        # folding white space, but legacy local parts can (they are a sequence
        # of atoms, not dotatoms).  On the other hand we strip whitespace from
        # before the @ and around dots, on the assumption that the whitespace
        # around the punctuation is a mistake in what would otherwise be
        # an RFC5322 local part.  Leading whitespace is, usual, stripped as well.
        self.assertEqual(('', "merwok wok@xample.com"),
            utils.parseaddr("merwok wok@xample.com"))
        self.assertEqual(('', "merwok  wok@xample.com"),
            utils.parseaddr("merwok  wok@xample.com"))
        self.assertEqual(('', "merwok  wok@xample.com"),
            utils.parseaddr(" merwok  wok  @xample.com"))
        self.assertEqual(('', 'merwok"wok"  wok@xample.com'),
            utils.parseaddr('merwok"wok"  wok@xample.com'))
        self.assertEqual(('', 'merwok.wok.wok@xample.com'),
            utils.parseaddr('merwok. wok .  wok@xample.com'))

    def test_formataddr_does_not_quote_parens_in_quoted_string(self):
        addr = ("'foo@example.com' (foo@example.com)",
                'foo@example.com')
        addrstr = ('"\'foo@example.com\' '
                            '(foo@example.com)" <foo@example.com>')
        self.assertEqual(utils.parseaddr(addrstr), addr)
        self.assertEqual(utils.formataddr(addr), addrstr)


    def test_multiline_from_comment(self):
        x = """\
Foo
\tBar <foo@example.com>"""
        self.assertEqual(utils.parseaddr(x), ('Foo Bar', 'foo@example.com'))

    def test_quote_dump(self):
        self.assertEqual(
            utils.formataddr(('A Silly; Person', 'person@dom.ain')),
            r'"A Silly; Person" <person@dom.ain>')

    def test_charset_richcomparisons(self):
        eq = self.assertEqual
        ne = self.assertNotEqual
        cset1 = Charset()
        cset2 = Charset()
        eq(cset1, 'us-ascii')
        eq(cset1, 'US-ASCII')
        eq(cset1, 'Us-AsCiI')
        eq('us-ascii', cset1)
        eq('US-ASCII', cset1)
        eq('Us-AsCiI', cset1)
        ne(cset1, 'usascii')
        ne(cset1, 'USASCII')
        ne(cset1, 'UsAsCiI')
        ne('usascii', cset1)
        ne('USASCII', cset1)
        ne('UsAsCiI', cset1)
        eq(cset1, cset2)
        eq(cset2, cset1)

    def test_getaddresses(self):
        eq = self.assertEqual
        eq(utils.getaddresses(['aperson@dom.ain (Al Person)',
                               'Bud Person <bperson@dom.ain>']),
           [('Al Person', 'aperson@dom.ain'),
            ('Bud Person', 'bperson@dom.ain')])

    def test_getaddresses_comma_in_name(self):
        """GH-106669 regression test."""
        self.assertEqual(
            utils.getaddresses(
                [
                    '"Bud, Person" <bperson@dom.ain>',
                    'aperson@dom.ain (Al Person)',
                    '"Mariusz Felisiak" <to@example.com>',
                ]
            ),
            [
                ('Bud, Person', 'bperson@dom.ain'),
                ('Al Person', 'aperson@dom.ain'),
                ('Mariusz Felisiak', 'to@example.com'),
            ],
        )

    def test_parsing_errors(self):
        """Test for parsing errors from CVE-2023-27043 and CVE-2019-16056"""
        alice = 'alice@example.org'
        bob = 'bob@example.com'
        empty = ('', '')

        # Test utils.getaddresses() and utils.parseaddr() on malformed email
        # addresses: default behavior (strict=True) rejects malformed address,
        # and strict=False which tolerates malformed address.
        for invalid_separator, expected_non_strict in (
            ('(', [(f'<{bob}>', alice)]),
            (')', [('', alice), empty, ('', bob)]),
            ('<', [('', alice), empty, ('', bob), empty]),
            ('>', [('', alice), empty, ('', bob)]),
            ('[', [('', f'{alice}[<{bob}>]')]),
            (']', [('', alice), empty, ('', bob)]),
            ('@', [empty, empty, ('', bob)]),
            (';', [('', alice), empty, ('', bob)]),
            (':', [('', alice), ('', bob)]),
            ('.', [('', alice + '.'), ('', bob)]),
            ('"', [('', alice), ('', f'<{bob}>')]),
        ):
            address = f'{alice}{invalid_separator}<{bob}>'
            with self.subTest(address=address):
                self.assertEqual(utils.getaddresses([address]),
                                 [empty])
                self.assertEqual(utils.getaddresses([address], strict=False),
                                 expected_non_strict)

                self.assertEqual(utils.parseaddr([address]),
                                 empty)
                self.assertEqual(utils.parseaddr([address], strict=False),
                                 ('', address))

        # Comma (',') is treated differently depending on strict parameter.
        # Comma without quotes.
        address = f'{alice},<{bob}>'
        self.assertEqual(utils.getaddresses([address]),
                         [('', alice), ('', bob)])
        self.assertEqual(utils.getaddresses([address], strict=False),
                         [('', alice), ('', bob)])
        self.assertEqual(utils.parseaddr([address]),
                         empty)
        self.assertEqual(utils.parseaddr([address], strict=False),
                         ('', address))

        # Real name between quotes containing comma.
        address = '"Alice, alice@example.org" <bob@example.com>'
        expected_strict = ('Alice, alice@example.org', 'bob@example.com')
        self.assertEqual(utils.getaddresses([address]), [expected_strict])
        self.assertEqual(utils.getaddresses([address], strict=False), [expected_strict])
        self.assertEqual(utils.parseaddr([address]), expected_strict)
        self.assertEqual(utils.parseaddr([address], strict=False),
                         ('', address))

        # Valid parenthesis in comments.
        address = 'alice@example.org (Alice)'
        expected_strict = ('Alice', 'alice@example.org')
        self.assertEqual(utils.getaddresses([address]), [expected_strict])
        self.assertEqual(utils.getaddresses([address], strict=False), [expected_strict])
        self.assertEqual(utils.parseaddr([address]), expected_strict)
        self.assertEqual(utils.parseaddr([address], strict=False),
                         ('', address))

        # Invalid parenthesis in comments.
        address = 'alice@example.org )Alice('
        self.assertEqual(utils.getaddresses([address]), [empty])
        self.assertEqual(utils.getaddresses([address], strict=False),
                         [('', 'alice@example.org'), ('', ''), ('', 'Alice')])
        self.assertEqual(utils.parseaddr([address]), empty)
        self.assertEqual(utils.parseaddr([address], strict=False),
                         ('', address))

        # Two addresses with quotes separated by comma.
        address = '"Jane Doe" <jane@example.net>, "John Doe" <john@example.net>'
        self.assertEqual(utils.getaddresses([address]),
                         [('Jane Doe', 'jane@example.net'),
                          ('John Doe', 'john@example.net')])
        self.assertEqual(utils.getaddresses([address], strict=False),
                         [('Jane Doe', 'jane@example.net'),
                          ('John Doe', 'john@example.net')])
        self.assertEqual(utils.parseaddr([address]), empty)
        self.assertEqual(utils.parseaddr([address], strict=False),
                         ('', address))

        # Test email.utils.supports_strict_parsing attribute
        self.assertEqual(email.utils.supports_strict_parsing, True)

    def test_getaddresses_nasty(self):
        for addresses, expected in (
            (['"Sürname, Firstname" <to@example.com>'],
             [('Sürname, Firstname', 'to@example.com')]),

            (['foo: ;'],
             [('', '')]),

            (['foo: ;', '"Jason R. Mastaler" <jason@dom.ain>'],
             [('', ''), ('Jason R. Mastaler', 'jason@dom.ain')]),

            ([r'Pete(A nice \) chap) <pete(his account)@silly.test(his host)>'],
             [('Pete (A nice ) chap his account his host)', 'pete@silly.test')]),

            (['(Empty list)(start)Undisclosed recipients  :(nobody(I know))'],
             [('', '')]),

            (['Mary <@machine.tld:mary@example.net>, , jdoe@test   . example'],
             [('Mary', 'mary@example.net'), ('', ''), ('', 'jdoe@test.example')]),

            (['John Doe <jdoe@machine(comment).  example>'],
             [('John Doe (comment)', 'jdoe@machine.example')]),

            (['"Mary Smith: Personal Account" <smith@home.example>'],
             [('Mary Smith: Personal Account', 'smith@home.example')]),

            (['Undisclosed recipients:;'],
             [('', '')]),

            ([r'<boss@nil.test>, "Giant; \"Big\" Box" <bob@example.net>'],
             [('', 'boss@nil.test'), ('Giant; "Big" Box', 'bob@example.net')]),
        ):
            with self.subTest(addresses=addresses):
                self.assertEqual(utils.getaddresses(addresses),
                                 expected)
                self.assertEqual(utils.getaddresses(addresses, strict=False),
                                 expected)

        addresses = ['[]*-- =~$']
        self.assertEqual(utils.getaddresses(addresses),
                         [('', '')])
        self.assertEqual(utils.getaddresses(addresses, strict=False),
                         [('', ''), ('', ''), ('', '*--')])

    def test_getaddresses_embedded_comment(self):
        """Test proper handling of a nested comment"""
        eq = self.assertEqual
        addrs = utils.getaddresses(['User ((nested comment)) <foo@bar.com>'])
        eq(addrs[0][1], 'foo@bar.com')

    def test_getaddresses_header_obj(self):
        """Test the handling of a Header object."""
        addrs = utils.getaddresses([Header('Al Person <aperson@dom.ain>')])
        self.assertEqual(addrs[0][1], 'aperson@dom.ain')

    @threading_helper.requires_working_threading()
    @support.requires_resource('cpu')
    def test_make_msgid_collisions(self):
        # Test make_msgid uniqueness, even with multiple threads
        class MsgidsThread(Thread):
            def run(self):
                # generate msgids for 3 seconds
                self.msgids = []
                append = self.msgids.append
                make_msgid = utils.make_msgid
                clock = time.monotonic
                tfin = clock() + 3.0
                while clock() < tfin:
                    append(make_msgid(domain='testdomain-string'))

        threads = [MsgidsThread() for i in range(5)]
        with threading_helper.start_threads(threads):
            pass
        all_ids = sum([t.msgids for t in threads], [])
        self.assertEqual(len(set(all_ids)), len(all_ids))

    def test_utils_quote_unquote(self):
        eq = self.assertEqual
        msg = Message()
        msg.add_header('content-disposition', 'attachment',
                       filename='foo\\wacky"name')
        eq(msg.get_filename(), 'foo\\wacky"name')

    def test_get_body_encoding_with_bogus_charset(self):
        charset = Charset('not a charset')
        self.assertEqual(charset.get_body_encoding(), 'base64')

    def test_get_body_encoding_with_uppercase_charset(self):
        eq = self.assertEqual
        msg = Message()
        msg['Content-Type'] = 'text/plain; charset=UTF-8'
        eq(msg['content-type'], 'text/plain; charset=UTF-8')
        charsets = msg.get_charsets()
        eq(len(charsets), 1)
        eq(charsets[0], 'utf-8')
        charset = Charset(charsets[0])
        eq(charset.get_body_encoding(), 'base64')
        msg.set_payload(b'hello world', charset=charset)
        eq(msg.get_payload(), 'aGVsbG8gd29ybGQ=\n')
        eq(msg.get_payload(decode=True), b'hello world')
        eq(msg['content-transfer-encoding'], 'base64')
        # Try another one
        msg = Message()
        msg['Content-Type'] = 'text/plain; charset="US-ASCII"'
        charsets = msg.get_charsets()
        eq(len(charsets), 1)
        eq(charsets[0], 'us-ascii')
        charset = Charset(charsets[0])
        eq(charset.get_body_encoding(), encoders.encode_7or8bit)
        msg.set_payload('hello world', charset=charset)
        eq(msg.get_payload(), 'hello world')
        eq(msg['content-transfer-encoding'], '7bit')

    def test_charsets_case_insensitive(self):
        lc = Charset('us-ascii')
        uc = Charset('US-ASCII')
        self.assertEqual(lc.get_body_encoding(), uc.get_body_encoding())

    def test_partial_falls_inside_message_delivery_status(self):
        eq = self.ndiffAssertEqual
        # The Parser interface provides chunks of data to FeedParser in 8192
        # byte gulps.  SF bug #1076485 found one of those chunks inside
        # message/delivery-status header block, which triggered an
        # unreadline() of NeedMoreData.
        msg = self._msgobj('msg_43.txt')
        sfp = StringIO()
        iterators._structure(msg, sfp)
        eq(sfp.getvalue(), """\
multipart/report
    text/plain
    message/delivery-status
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
        text/plain
    text/rfc822-headers
""")

    def test_make_msgid_domain(self):
        self.assertEqual(
            email.utils.make_msgid(domain='testdomain-string')[-19:],
            '@testdomain-string>')

    def test_make_msgid_idstring(self):
        self.assertEqual(
            email.utils.make_msgid(idstring='test-idstring',
                domain='testdomain-string')[-33:],
            '.test-idstring@testdomain-string>')

    def test_make_msgid_default_domain(self):
        with patch('socket.getfqdn') as mock_getfqdn:
            mock_getfqdn.return_value = domain = 'pythontest.example.com'
            self.assertTrue(
                email.utils.make_msgid().endswith(
                    '@' + domain + '>'))

    def test_Generator_linend(self):
        # Issue 14645.
        with openfile('msg_26.txt', encoding="utf-8", newline='\n') as f:
            msgtxt = f.read()
        msgtxt_nl = msgtxt.replace('\r\n', '\n')
        msg = email.message_from_string(msgtxt)
        s = StringIO()
        g = email.generator.Generator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), msgtxt_nl)

    def test_BytesGenerator_linend(self):
        # Issue 14645.
        with openfile('msg_26.txt', encoding="utf-8", newline='\n') as f:
            msgtxt = f.read()
        msgtxt_nl = msgtxt.replace('\r\n', '\n')
        msg = email.message_from_string(msgtxt_nl)
        s = BytesIO()
        g = email.generator.BytesGenerator(s)
        g.flatten(msg, linesep='\r\n')
        self.assertEqual(s.getvalue().decode('ascii'), msgtxt)

    def test_BytesGenerator_linend_with_non_ascii(self):
        # Issue 14645.
        with openfile('msg_26.txt', 'rb') as f:
            msgtxt = f.read()
        msgtxt = msgtxt.replace(b'with attachment', b'fo\xf6')
        msgtxt_nl = msgtxt.replace(b'\r\n', b'\n')
        msg = email.message_from_bytes(msgtxt_nl)
        s = BytesIO()
        g = email.generator.BytesGenerator(s)
        g.flatten(msg, linesep='\r\n')
        self.assertEqual(s.getvalue(), msgtxt)

    def test_mime_classes_policy_argument(self):
        with openfile('sndhdr.au', 'rb') as fp:
            audiodata = fp.read()
        with openfile('python.gif', 'rb') as fp:
            bindata = fp.read()
        classes = [
            (MIMEApplication, ('',)),
            (MIMEAudio, (audiodata,)),
            (MIMEImage, (bindata,)),
            (MIMEMessage, (Message(),)),
            (MIMENonMultipart, ('multipart', 'mixed')),
            (MIMEText, ('',)),
        ]
        for cls, constructor in classes:
            with self.subTest(cls=cls.__name__, policy='compat32'):
                m = cls(*constructor)
                self.assertIs(m.policy, email.policy.compat32)
            with self.subTest(cls=cls.__name__, policy='default'):
                m = cls(*constructor, policy=email.policy.default)
                self.assertIs(m.policy, email.policy.default)

    def test_iter_escaped_chars(self):
        self.assertEqual(list(utils._iter_escaped_chars(r'a\\b\"c\\"d')),
                         [(0, 'a'),
                          (2, '\\\\'),
                          (3, 'b'),
                          (5, '\\"'),
                          (6, 'c'),
                          (8, '\\\\'),
                          (9, '"'),
                          (10, 'd')])
        self.assertEqual(list(utils._iter_escaped_chars('a\\')),
                         [(0, 'a'), (1, '\\')])

    def test_strip_quoted_realnames(self):
        def check(addr, expected):
            self.assertEqual(utils._strip_quoted_realnames(addr), expected)

        check('"Jane Doe" <jane@example.net>, "John Doe" <john@example.net>',
              ' <jane@example.net>,  <john@example.net>')
        check(r'"Jane \"Doe\"." <jane@example.net>',
              ' <jane@example.net>')

        # special cases
        check(r'before"name"after', 'beforeafter')
        check(r'before"name"', 'before')
        check(r'b"name"', 'b')  # single char
        check(r'"name"after', 'after')
        check(r'"name"a', 'a')  # single char
        check(r'"name"', '')

        # no change
        for addr in (
            'Jane Doe <jane@example.net>, John Doe <john@example.net>',
            'lone " quote',
        ):
            self.assertEqual(utils._strip_quoted_realnames(addr), addr)


    def test_check_parenthesis(self):
        addr = 'alice@example.net'
        self.assertTrue(utils._check_parenthesis(f'{addr} (Alice)'))
        self.assertFalse(utils._check_parenthesis(f'{addr} )Alice('))
        self.assertFalse(utils._check_parenthesis(f'{addr} (Alice))'))
        self.assertFalse(utils._check_parenthesis(f'{addr} ((Alice)'))

        # Ignore real name between quotes
        self.assertTrue(utils._check_parenthesis(f'")Alice((" {addr}'))


# Test the iterator/generators
class TestIterators(TestEmailBase):
    def test_body_line_iterator(self):
        eq = self.assertEqual
        neq = self.ndiffAssertEqual
        # First a simple non-multipart message
        msg = self._msgobj('msg_01.txt')
        it = iterators.body_line_iterator(msg)
        lines = list(it)
        eq(len(lines), 6)
        neq(EMPTYSTRING.join(lines), msg.get_payload())
        # Now a more complicated multipart
        msg = self._msgobj('msg_02.txt')
        it = iterators.body_line_iterator(msg)
        lines = list(it)
        eq(len(lines), 43)
        with openfile('msg_19.txt', encoding="utf-8") as fp:
            neq(EMPTYSTRING.join(lines), fp.read())

    def test_typed_subpart_iterator(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_04.txt')
        it = iterators.typed_subpart_iterator(msg, 'text')
        lines = []
        subparts = 0
        for subpart in it:
            subparts += 1
            lines.append(subpart.get_payload())
        eq(subparts, 2)
        eq(EMPTYSTRING.join(lines), """\
a simple kind of mirror
to reflect upon our own
a simple kind of mirror
to reflect upon our own
""")

    def test_typed_subpart_iterator_default_type(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_03.txt')
        it = iterators.typed_subpart_iterator(msg, 'text', 'plain')
        lines = []
        subparts = 0
        for subpart in it:
            subparts += 1
            lines.append(subpart.get_payload())
        eq(subparts, 1)
        eq(EMPTYSTRING.join(lines), """\

Hi,

Do you like this message?

-Me
""")

    def test_pushCR_LF(self):
        '''FeedParser BufferedSubFile.push() assumed it received complete
           line endings.  A CR ending one push() followed by a LF starting
           the next push() added an empty line.
        '''
        imt = [
            ("a\r \n",  2),
            ("b",       0),
            ("c\n",     1),
            ("",        0),
            ("d\r\n",   1),
            ("e\r",     0),
            ("\nf",     1),
            ("\r\n",    1),
          ]
        from email.feedparser import BufferedSubFile, NeedMoreData
        bsf = BufferedSubFile()
        om = []
        nt = 0
        for il, n in imt:
            bsf.push(il)
            nt += n
            n1 = 0
            for ol in iter(bsf.readline, NeedMoreData):
                om.append(ol)
                n1 += 1
            self.assertEqual(n, n1)
        self.assertEqual(len(om), nt)
        self.assertEqual(''.join([il for il, n in imt]), ''.join(om))

    def test_push_random(self):
        from email.feedparser import BufferedSubFile, NeedMoreData

        n = 10000
        chunksize = 5
        chars = 'abcd \t\r\n'

        s = ''.join(choice(chars) for i in range(n)) + '\n'
        target = s.splitlines(True)

        bsf = BufferedSubFile()
        lines = []
        for i in range(0, len(s), chunksize):
            chunk = s[i:i+chunksize]
            bsf.push(chunk)
            lines.extend(iter(bsf.readline, NeedMoreData))
        self.assertEqual(lines, target)


class TestFeedParsers(TestEmailBase):

    def parse(self, chunks):
        feedparser = FeedParser()
        for chunk in chunks:
            feedparser.feed(chunk)
        return feedparser.close()

    def test_empty_header_name_handled(self):
        # Issue 19996
        msg = self.parse("First: val\n: bad\nSecond: val")
        self.assertEqual(msg['First'], 'val')
        self.assertEqual(msg['Second'], 'val')

    def test_newlines(self):
        m = self.parse(['a:\nb:\rc:\r\nd:\n'])
        self.assertEqual(m.keys(), ['a', 'b', 'c', 'd'])
        m = self.parse(['a:\nb:\rc:\r\nd:'])
        self.assertEqual(m.keys(), ['a', 'b', 'c', 'd'])
        m = self.parse(['a:\rb', 'c:\n'])
        self.assertEqual(m.keys(), ['a', 'bc'])
        m = self.parse(['a:\r', 'b:\n'])
        self.assertEqual(m.keys(), ['a', 'b'])
        m = self.parse(['a:\r', '\nb:\n'])
        self.assertEqual(m.keys(), ['a', 'b'])

        # Only CR and LF should break header fields
        m = self.parse(['a:\x85b:\u2028c:\n'])
        self.assertEqual(m.items(), [('a', '\x85b:\u2028c:')])
        m = self.parse(['a:\r', 'b:\x85', 'c:\n'])
        self.assertEqual(m.items(), [('a', ''), ('b', '\x85c:')])

    def test_long_lines(self):
        # Expected peak memory use on 32-bit platform: 6*N*M bytes.
        M, N = 1000, 20000
        m = self.parse(['a:b\n\n'] + ['x'*M] * N)
        self.assertEqual(m.items(), [('a', 'b')])
        self.assertEqual(m.get_payload(), 'x'*M*N)
        m = self.parse(['a:b\r\r'] + ['x'*M] * N)
        self.assertEqual(m.items(), [('a', 'b')])
        self.assertEqual(m.get_payload(), 'x'*M*N)
        m = self.parse(['a:b\r\r'] + ['x'*M+'\x85'] * N)
        self.assertEqual(m.items(), [('a', 'b')])
        self.assertEqual(m.get_payload(), ('x'*M+'\x85')*N)
        m = self.parse(['a:\r', 'b: '] + ['x'*M] * N)
        self.assertEqual(m.items(), [('a', ''), ('b', 'x'*M*N)])


class TestParsers(TestEmailBase):

    def test_header_parser(self):
        eq = self.assertEqual
        # Parse only the headers of a complex multipart MIME document
        with openfile('msg_02.txt', encoding="utf-8") as fp:
            msg = HeaderParser().parse(fp)
        eq(msg['from'], 'ppp-request@zzz.org')
        eq(msg['to'], 'ppp@zzz.org')
        eq(msg.get_content_type(), 'multipart/mixed')
        self.assertFalse(msg.is_multipart())
        self.assertIsInstance(msg.get_payload(), str)

    def test_bytes_header_parser(self):
        eq = self.assertEqual
        # Parse only the headers of a complex multipart MIME document
        with openfile('msg_02.txt', 'rb') as fp:
            msg = email.parser.BytesHeaderParser().parse(fp)
        eq(msg['from'], 'ppp-request@zzz.org')
        eq(msg['to'], 'ppp@zzz.org')
        eq(msg.get_content_type(), 'multipart/mixed')
        self.assertFalse(msg.is_multipart())
        self.assertIsInstance(msg.get_payload(), str)
        self.assertIsInstance(msg.get_payload(decode=True), bytes)

    def test_header_parser_multipart_is_valid(self):
        # Don't flag valid multipart emails as having defects
        with openfile('msg_47.txt', encoding="utf-8") as fp:
            msgdata = fp.read()

        parser = email.parser.Parser(policy=email.policy.default)
        parsed_msg = parser.parsestr(msgdata, headersonly=True)

        self.assertEqual(parsed_msg.defects, [])

    def test_bytes_parser_does_not_close_file(self):
        with openfile('msg_02.txt', 'rb') as fp:
            email.parser.BytesParser().parse(fp)
            self.assertFalse(fp.closed)

    def test_bytes_parser_on_exception_does_not_close_file(self):
        with openfile('msg_15.txt', 'rb') as fp:
            bytesParser = email.parser.BytesParser
            self.assertRaises(email.errors.StartBoundaryNotFoundDefect,
                              bytesParser(policy=email.policy.strict).parse,
                              fp)
            self.assertFalse(fp.closed)

    def test_parser_does_not_close_file(self):
        with openfile('msg_02.txt', encoding="utf-8") as fp:
            email.parser.Parser().parse(fp)
            self.assertFalse(fp.closed)

    def test_parser_on_exception_does_not_close_file(self):
        with openfile('msg_15.txt', encoding="utf-8") as fp:
            parser = email.parser.Parser
            self.assertRaises(email.errors.StartBoundaryNotFoundDefect,
                              parser(policy=email.policy.strict).parse, fp)
            self.assertFalse(fp.closed)

    def test_whitespace_continuation(self):
        eq = self.assertEqual
        # This message contains a line after the Subject: header that has only
        # whitespace, but it is not empty!
        msg = email.message_from_string("""\
From: aperson@dom.ain
To: bperson@dom.ain
Subject: the next line has a space on it
\x20
Date: Mon, 8 Apr 2002 15:09:19 -0400
Message-ID: spam

Here's the message body
""")
        eq(msg['subject'], 'the next line has a space on it\n ')
        eq(msg['message-id'], 'spam')
        eq(msg.get_payload(), "Here's the message body\n")

    def test_whitespace_continuation_last_header(self):
        eq = self.assertEqual
        # Like the previous test, but the subject line is the last
        # header.
        msg = email.message_from_string("""\
From: aperson@dom.ain
To: bperson@dom.ain
Date: Mon, 8 Apr 2002 15:09:19 -0400
Message-ID: spam
Subject: the next line has a space on it
\x20

Here's the message body
""")
        eq(msg['subject'], 'the next line has a space on it\n ')
        eq(msg['message-id'], 'spam')
        eq(msg.get_payload(), "Here's the message body\n")

    def test_crlf_separation(self):
        eq = self.assertEqual
        with openfile('msg_26.txt', encoding="utf-8", newline='\n') as fp:
            msg = Parser().parse(fp)
        eq(len(msg.get_payload()), 2)
        part1 = msg.get_payload(0)
        eq(part1.get_content_type(), 'text/plain')
        eq(part1.get_payload(), 'Simple email with attachment.\r\n\r\n')
        part2 = msg.get_payload(1)
        eq(part2.get_content_type(), 'application/riscos')

    def test_crlf_flatten(self):
        # Using newline='\n' preserves the crlfs in this input file.
        with openfile('msg_26.txt', encoding="utf-8", newline='\n') as fp:
            text = fp.read()
        msg = email.message_from_string(text)
        s = StringIO()
        g = Generator(s)
        g.flatten(msg, linesep='\r\n')
        self.assertEqual(s.getvalue(), text)

    maxDiff = None

    def test_multipart_digest_with_extra_mime_headers(self):
        eq = self.assertEqual
        neq = self.ndiffAssertEqual
        with openfile('msg_28.txt', encoding="utf-8") as fp:
            msg = email.message_from_file(fp)
        # Structure is:
        # multipart/digest
        #   message/rfc822
        #     text/plain
        #   message/rfc822
        #     text/plain
        eq(msg.is_multipart(), 1)
        eq(len(msg.get_payload()), 2)
        part1 = msg.get_payload(0)
        eq(part1.get_content_type(), 'message/rfc822')
        eq(part1.is_multipart(), 1)
        eq(len(part1.get_payload()), 1)
        part1a = part1.get_payload(0)
        eq(part1a.is_multipart(), 0)
        eq(part1a.get_content_type(), 'text/plain')
        neq(part1a.get_payload(), 'message 1\n')
        # next message/rfc822
        part2 = msg.get_payload(1)
        eq(part2.get_content_type(), 'message/rfc822')
        eq(part2.is_multipart(), 1)
        eq(len(part2.get_payload()), 1)
        part2a = part2.get_payload(0)
        eq(part2a.is_multipart(), 0)
        eq(part2a.get_content_type(), 'text/plain')
        neq(part2a.get_payload(), 'message 2\n')

    def test_three_lines(self):
        # A bug report by Andrew McNamara
        lines = ['From: Andrew Person <aperson@dom.ain',
                 'Subject: Test',
                 'Date: Tue, 20 Aug 2002 16:43:45 +1000']
        msg = email.message_from_string(NL.join(lines))
        self.assertEqual(msg['date'], 'Tue, 20 Aug 2002 16:43:45 +1000')

    def test_strip_line_feed_and_carriage_return_in_headers(self):
        eq = self.assertEqual
        # For [ 1002475 ] email message parser doesn't handle \r\n correctly
        value1 = 'text'
        value2 = 'more text'
        m = 'Header: %s\r\nNext-Header: %s\r\n\r\nBody\r\n\r\n' % (
            value1, value2)
        msg = email.message_from_string(m)
        eq(msg.get('Header'), value1)
        eq(msg.get('Next-Header'), value2)

    def test_rfc2822_header_syntax(self):
        eq = self.assertEqual
        m = '>From: foo\nFrom: bar\n!"#QUX;~: zoo\n\nbody'
        msg = email.message_from_string(m)
        eq(len(msg), 3)
        eq(sorted(field for field in msg), ['!"#QUX;~', '>From', 'From'])
        eq(msg.get_payload(), 'body')

    def test_rfc2822_space_not_allowed_in_header(self):
        eq = self.assertEqual
        m = '>From foo@example.com 11:25:53\nFrom: bar\n!"#QUX;~: zoo\n\nbody'
        msg = email.message_from_string(m)
        eq(len(msg.keys()), 0)

    def test_rfc2822_one_character_header(self):
        eq = self.assertEqual
        m = 'A: first header\nB: second header\nCC: third header\n\nbody'
        msg = email.message_from_string(m)
        headers = msg.keys()
        headers.sort()
        eq(headers, ['A', 'B', 'CC'])
        eq(msg.get_payload(), 'body')

    def test_CRLFLF_at_end_of_part(self):
        # issue 5610: feedparser should not eat two chars from body part ending
        # with "\r\n\n".
        m = (
            "From: foo@bar.com\n"
            "To: baz\n"
            "Mime-Version: 1.0\n"
            "Content-Type: multipart/mixed; boundary=BOUNDARY\n"
            "\n"
            "--BOUNDARY\n"
            "Content-Type: text/plain\n"
            "\n"
            "body ending with CRLF newline\r\n"
            "\n"
            "--BOUNDARY--\n"
          )
        msg = email.message_from_string(m)
        self.assertTrue(msg.get_payload(0).get_payload().endswith('\r\n'))


class Test8BitBytesHandling(TestEmailBase):
    # In Python3 all input is string, but that doesn't work if the actual input
    # uses an 8bit transfer encoding.  To hack around that, in email 5.1 we
    # decode byte streams using the surrogateescape error handler, and
    # reconvert to binary at appropriate places if we detect surrogates.  This
    # doesn't allow us to transform headers with 8bit bytes (they get munged),
    # but it does allow us to parse and preserve them, and to decode body
    # parts that use an 8bit CTE.

    bodytest_msg = textwrap.dedent("""\
        From: foo@bar.com
        To: baz
        Mime-Version: 1.0
        Content-Type: text/plain; charset={charset}
        Content-Transfer-Encoding: {cte}

        {bodyline}
        """)

    def test_known_8bit_CTE(self):
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='8bit',
                                     bodyline='pöstal').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(), "pöstal\n")
        self.assertEqual(msg.get_payload(decode=True),
                         "pöstal\n".encode('utf-8'))

    def test_unknown_8bit_CTE(self):
        m = self.bodytest_msg.format(charset='notavalidcharset',
                                     cte='8bit',
                                     bodyline='pöstal').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(), "p\uFFFD\uFFFDstal\n")
        self.assertEqual(msg.get_payload(decode=True),
                         "pöstal\n".encode('utf-8'))

    def test_8bit_in_quopri_body(self):
        # This is non-RFC compliant data...without 'decode' the library code
        # decodes the body using the charset from the headers, and because the
        # source byte really is utf-8 this works.  This is likely to fail
        # against real dirty data (ie: produce mojibake), but the data is
        # invalid anyway so it is as good a guess as any.  But this means that
        # this test just confirms the current behavior; that behavior is not
        # necessarily the best possible behavior.  With 'decode' it is
        # returning the raw bytes, so that test should be of correct behavior,
        # or at least produce the same result that email4 did.
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='quoted-printable',
                                     bodyline='p=C3=B6stál').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(), 'p=C3=B6stál\n')
        self.assertEqual(msg.get_payload(decode=True),
                         'pöstál\n'.encode('utf-8'))

    def test_invalid_8bit_in_non_8bit_cte_uses_replace(self):
        # This is similar to the previous test, but proves that if the 8bit
        # byte is undecodeable in the specified charset, it gets replaced
        # by the unicode 'unknown' character.  Again, this may or may not
        # be the ideal behavior.  Note that if decode=False none of the
        # decoders will get involved, so this is the only test we need
        # for this behavior.
        m = self.bodytest_msg.format(charset='ascii',
                                     cte='quoted-printable',
                                     bodyline='p=C3=B6stál').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(), 'p=C3=B6st\uFFFD\uFFFDl\n')
        self.assertEqual(msg.get_payload(decode=True),
                        'pöstál\n'.encode('utf-8'))

    # test_defect_handling:test_invalid_chars_in_base64_payload
    def test_8bit_in_base64_body(self):
        # If we get 8bit bytes in a base64 body, we can just ignore them
        # as being outside the base64 alphabet and decode anyway.  But
        # we register a defect.
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='base64',
                                     bodyline='cMO2c3RhbAá=').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(decode=True),
                         'pöstal'.encode('utf-8'))
        self.assertIsInstance(msg.defects[0],
                              errors.InvalidBase64CharactersDefect)

    def test_8bit_in_uuencode_body(self):
        # Sticking an 8bit byte in a uuencode block makes it undecodable by
        # normal means, so the block is returned undecoded, but as bytes.
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='uuencode',
                                     bodyline='<,.V<W1A; á ').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(decode=True),
                         '<,.V<W1A; á \n'.encode('utf-8'))


    headertest_headers = (
        ('From: foo@bar.com', ('From', 'foo@bar.com')),
        ('To: báz', ('To', '=?unknown-8bit?q?b=C3=A1z?=')),
        ('Subject: Maintenant je vous présente mon collègue, le pouf célèbre\n'
            '\tJean de Baddie',
            ('Subject', '=?unknown-8bit?q?Maintenant_je_vous_pr=C3=A9sente_mon_'
                'coll=C3=A8gue=2C_le_pouf_c=C3=A9l=C3=A8bre?=\n'
                ' =?unknown-8bit?q?_Jean_de_Baddie?=')),
        ('From: göst', ('From', '=?unknown-8bit?b?Z8O2c3Q=?=')),
        )
    headertest_msg = ('\n'.join([src for (src, _) in headertest_headers]) +
        '\nYes, they are flying.\n').encode('utf-8')

    def test_get_8bit_header(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertEqual(str(msg.get('to')), 'b\uFFFD\uFFFDz')
        self.assertEqual(str(msg['to']), 'b\uFFFD\uFFFDz')

    def test_print_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertEqual(str(msg),
                         textwrap.dedent("""\
                            From: {}
                            To: {}
                            Subject: {}
                            From: {}

                            Yes, they are flying.
                            """).format(*[expected[1] for (_, expected) in
                                        self.headertest_headers]))

    def test_values_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual([str(x) for x in msg.values()],
                              ['foo@bar.com',
                               'b\uFFFD\uFFFDz',
                               'Maintenant je vous pr\uFFFD\uFFFDsente mon '
                                   'coll\uFFFD\uFFFDgue, le pouf '
                                   'c\uFFFD\uFFFDl\uFFFD\uFFFDbre\n'
                                   '\tJean de Baddie',
                               "g\uFFFD\uFFFDst"])

    def test_items_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual([(str(x), str(y)) for (x, y) in msg.items()],
                              [('From', 'foo@bar.com'),
                               ('To', 'b\uFFFD\uFFFDz'),
                               ('Subject', 'Maintenant je vous '
                                  'pr\uFFFD\uFFFDsente '
                                  'mon coll\uFFFD\uFFFDgue, le pouf '
                                  'c\uFFFD\uFFFDl\uFFFD\uFFFDbre\n'
                                  '\tJean de Baddie'),
                               ('From', 'g\uFFFD\uFFFDst')])

    def test_get_all_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual([str(x) for x in msg.get_all('from')],
                              ['foo@bar.com',
                               'g\uFFFD\uFFFDst'])

    def test_get_content_type_with_8bit(self):
        msg = email.message_from_bytes(textwrap.dedent("""\
            Content-Type: text/pl\xA7in; charset=utf-8
            """).encode('latin-1'))
        self.assertEqual(msg.get_content_type(), "text/pl\uFFFDin")
        self.assertEqual(msg.get_content_maintype(), "text")
        self.assertEqual(msg.get_content_subtype(), "pl\uFFFDin")

    # test_headerregistry.TestContentTypeHeader.non_ascii_in_params
    def test_get_params_with_8bit(self):
        msg = email.message_from_bytes(
            'X-Header: foo=\xa7ne; b\xa7r=two; baz=three\n'.encode('latin-1'))
        self.assertEqual(msg.get_params(header='x-header'),
           [('foo', '\uFFFDne'), ('b\uFFFDr', 'two'), ('baz', 'three')])
        self.assertEqual(msg.get_param('Foo', header='x-header'), '\uFFFdne')
        # XXX: someday you might be able to get 'b\xa7r', for now you can't.
        self.assertEqual(msg.get_param('b\xa7r', header='x-header'), None)

    # test_headerregistry.TestContentTypeHeader.non_ascii_in_rfc2231_value
    def test_get_rfc2231_params_with_8bit(self):
        msg = email.message_from_bytes(textwrap.dedent("""\
            Content-Type: text/plain; charset=us-ascii;
             title*=us-ascii'en'This%20is%20not%20f\xa7n"""
             ).encode('latin-1'))
        self.assertEqual(msg.get_param('title'),
            ('us-ascii', 'en', 'This is not f\uFFFDn'))

    def test_set_rfc2231_params_with_8bit(self):
        msg = email.message_from_bytes(textwrap.dedent("""\
            Content-Type: text/plain; charset=us-ascii;
             title*=us-ascii'en'This%20is%20not%20f\xa7n"""
             ).encode('latin-1'))
        msg.set_param('title', 'test')
        self.assertEqual(msg.get_param('title'), 'test')

    def test_del_rfc2231_params_with_8bit(self):
        msg = email.message_from_bytes(textwrap.dedent("""\
            Content-Type: text/plain; charset=us-ascii;
             title*=us-ascii'en'This%20is%20not%20f\xa7n"""
             ).encode('latin-1'))
        msg.del_param('title')
        self.assertEqual(msg.get_param('title'), None)
        self.assertEqual(msg.get_content_maintype(), 'text')

    def test_get_payload_with_8bit_cte_header(self):
        msg = email.message_from_bytes(textwrap.dedent("""\
            Content-Transfer-Encoding: b\xa7se64
            Content-Type: text/plain; charset=latin-1

            payload
            """).encode('latin-1'))
        self.assertEqual(msg.get_payload(), 'payload\n')
        self.assertEqual(msg.get_payload(decode=True), b'payload\n')

    non_latin_bin_msg = textwrap.dedent("""\
        From: foo@bar.com
        To: báz
        Subject: Maintenant je vous présente mon collègue, le pouf célèbre
        \tJean de Baddie
        Mime-Version: 1.0
        Content-Type: text/plain; charset="utf-8"
        Content-Transfer-Encoding: 8bit

        Да, они летят.
        """).encode('utf-8')

    def test_bytes_generator(self):
        msg = email.message_from_bytes(self.non_latin_bin_msg)
        out = BytesIO()
        email.generator.BytesGenerator(out).flatten(msg)
        self.assertEqual(out.getvalue(), self.non_latin_bin_msg)

    def test_bytes_generator_handles_None_body(self):
        #Issue 11019
        msg = email.message.Message()
        out = BytesIO()
        email.generator.BytesGenerator(out).flatten(msg)
        self.assertEqual(out.getvalue(), b"\n")

    non_latin_bin_msg_as7bit_wrapped = textwrap.dedent("""\
        From: foo@bar.com
        To: =?unknown-8bit?q?b=C3=A1z?=
        Subject: =?unknown-8bit?q?Maintenant_je_vous_pr=C3=A9sente_mon_coll=C3=A8gue?=
         =?unknown-8bit?q?=2C_le_pouf_c=C3=A9l=C3=A8bre?=
         =?unknown-8bit?q?_Jean_de_Baddie?=
        Mime-Version: 1.0
        Content-Type: text/plain; charset="utf-8"
        Content-Transfer-Encoding: base64

        0JTQsCwg0L7QvdC4INC70LXRgtGP0YIuCg==
        """)

    def test_generator_handles_8bit(self):
        msg = email.message_from_bytes(self.non_latin_bin_msg)
        out = StringIO()
        email.generator.Generator(out).flatten(msg)
        self.assertEqual(out.getvalue(), self.non_latin_bin_msg_as7bit_wrapped)

    def test_str_generator_should_not_mutate_msg_when_handling_8bit(self):
        msg = email.message_from_bytes(self.non_latin_bin_msg)
        out = BytesIO()
        BytesGenerator(out).flatten(msg)
        orig_value = out.getvalue()
        Generator(StringIO()).flatten(msg) # Should not mutate msg!
        out = BytesIO()
        BytesGenerator(out).flatten(msg)
        self.assertEqual(out.getvalue(), orig_value)

    def test_bytes_generator_with_unix_from(self):
        # The unixfrom contains a current date, so we can't check it
        # literally.  Just make sure the first word is 'From' and the
        # rest of the message matches the input.
        msg = email.message_from_bytes(self.non_latin_bin_msg)
        out = BytesIO()
        email.generator.BytesGenerator(out).flatten(msg, unixfrom=True)
        lines = out.getvalue().split(b'\n')
        self.assertEqual(lines[0].split()[0], b'From')
        self.assertEqual(b'\n'.join(lines[1:]), self.non_latin_bin_msg)

    non_latin_bin_msg_as7bit = non_latin_bin_msg_as7bit_wrapped.split('\n')
    non_latin_bin_msg_as7bit[2:4] = [
        'Subject: =?unknown-8bit?q?Maintenant_je_vous_pr=C3=A9sente_mon_'
         'coll=C3=A8gue=2C_le_pouf_c=C3=A9l=C3=A8bre?=']
    non_latin_bin_msg_as7bit = '\n'.join(non_latin_bin_msg_as7bit)

    def test_message_from_binary_file(self):
        fn = 'test.msg'
        self.addCleanup(unlink, fn)
        with open(fn, 'wb') as testfile:
            testfile.write(self.non_latin_bin_msg)
        with open(fn, 'rb') as testfile:
            m = email.parser.BytesParser().parse(testfile)
        self.assertEqual(str(m), self.non_latin_bin_msg_as7bit)

    latin_bin_msg = textwrap.dedent("""\
        From: foo@bar.com
        To: Dinsdale
        Subject: Nudge nudge, wink, wink
        Mime-Version: 1.0
        Content-Type: text/plain; charset="latin-1"
        Content-Transfer-Encoding: 8bit

        oh là là, know what I mean, know what I mean?
        """).encode('latin-1')

    latin_bin_msg_as7bit = textwrap.dedent("""\
        From: foo@bar.com
        To: Dinsdale
        Subject: Nudge nudge, wink, wink
        Mime-Version: 1.0
        Content-Type: text/plain; charset="iso-8859-1"
        Content-Transfer-Encoding: quoted-printable

        oh l=E0 l=E0, know what I mean, know what I mean?
        """)

    def test_string_generator_reencodes_to_quopri_when_appropriate(self):
        m = email.message_from_bytes(self.latin_bin_msg)
        self.assertEqual(str(m), self.latin_bin_msg_as7bit)

    def test_decoded_generator_emits_unicode_body(self):
        m = email.message_from_bytes(self.latin_bin_msg)
        out = StringIO()
        email.generator.DecodedGenerator(out).flatten(m)
        #DecodedHeader output contains an extra blank line compared
        #to the input message.  RDM: not sure if this is a bug or not,
        #but it is not specific to the 8bit->7bit conversion.
        self.assertEqual(out.getvalue(),
            self.latin_bin_msg.decode('latin-1')+'\n')

    def test_bytes_feedparser(self):
        bfp = email.feedparser.BytesFeedParser()
        for i in range(0, len(self.latin_bin_msg), 10):
            bfp.feed(self.latin_bin_msg[i:i+10])
        m = bfp.close()
        self.assertEqual(str(m), self.latin_bin_msg_as7bit)

    def test_crlf_flatten(self):
        with openfile('msg_26.txt', 'rb') as fp:
            text = fp.read()
        msg = email.message_from_bytes(text)
        s = BytesIO()
        g = email.generator.BytesGenerator(s)
        g.flatten(msg, linesep='\r\n')
        self.assertEqual(s.getvalue(), text)

    def test_8bit_multipart(self):
        # Issue 11605
        source = textwrap.dedent("""\
            Date: Fri, 18 Mar 2011 17:15:43 +0100
            To: foo@example.com
            From: foodwatch-Newsletter <bar@example.com>
            Subject: Aktuelles zu Japan, Klonfleisch und Smiley-System
            Message-ID: <76a486bee62b0d200f33dc2ca08220ad@localhost.localdomain>
            MIME-Version: 1.0
            Content-Type: multipart/alternative;
                    boundary="b1_76a486bee62b0d200f33dc2ca08220ad"

            --b1_76a486bee62b0d200f33dc2ca08220ad
            Content-Type: text/plain; charset="utf-8"
            Content-Transfer-Encoding: 8bit

            Guten Tag, ,

            mit großer Betroffenheit verfolgen auch wir im foodwatch-Team die
            Nachrichten aus Japan.


            --b1_76a486bee62b0d200f33dc2ca08220ad
            Content-Type: text/html; charset="utf-8"
            Content-Transfer-Encoding: 8bit

            <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
                "http://www.w3.org/TR/html4/loose.dtd">
            <html lang="de">
            <head>
                    <title>foodwatch - Newsletter</title>
            </head>
            <body>
              <p>mit gro&szlig;er Betroffenheit verfolgen auch wir im foodwatch-Team
                 die Nachrichten aus Japan.</p>
            </body>
            </html>
            --b1_76a486bee62b0d200f33dc2ca08220ad--

            """).encode('utf-8')
        msg = email.message_from_bytes(source)
        s = BytesIO()
        g = email.generator.BytesGenerator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), source)

    def test_bytes_generator_b_encoding_linesep(self):
        # Issue 14062: b encoding was tacking on an extra \n.
        m = Message()
        # This has enough non-ascii that it should always end up b encoded.
        m['Subject'] = Header('žluťoučký kůň')
        s = BytesIO()
        g = email.generator.BytesGenerator(s)
        g.flatten(m, linesep='\r\n')
        self.assertEqual(
            s.getvalue(),
            b'Subject: =?utf-8?b?xb5sdcWlb3XEjWvDvSBrxa/FiA==?=\r\n\r\n')

    def test_generator_b_encoding_linesep(self):
        # Since this broke in ByteGenerator, test Generator for completeness.
        m = Message()
        # This has enough non-ascii that it should always end up b encoded.
        m['Subject'] = Header('žluťoučký kůň')
        s = StringIO()
        g = email.generator.Generator(s)
        g.flatten(m, linesep='\r\n')
        self.assertEqual(
            s.getvalue(),
            'Subject: =?utf-8?b?xb5sdcWlb3XEjWvDvSBrxa/FiA==?=\r\n\r\n')

    maxDiff = None


class BaseTestBytesGeneratorIdempotent:

    maxDiff = None

    def _msgobj(self, filename):
        with openfile(filename, 'rb') as fp:
            data = fp.read()
        data = self.normalize_linesep_regex.sub(self.blinesep, data)
        msg = email.message_from_bytes(data)
        return msg, data

    def _idempotent(self, msg, data, unixfrom=False):
        b = BytesIO()
        g = email.generator.BytesGenerator(b, maxheaderlen=0)
        g.flatten(msg, unixfrom=unixfrom, linesep=self.linesep)
        self.assertEqual(data, b.getvalue())


class TestBytesGeneratorIdempotentNL(BaseTestBytesGeneratorIdempotent,
                                    TestIdempotent):
    linesep = '\n'
    blinesep = b'\n'
    normalize_linesep_regex = re.compile(br'\r\n')


class TestBytesGeneratorIdempotentCRLF(BaseTestBytesGeneratorIdempotent,
                                       TestIdempotent):
    linesep = '\r\n'
    blinesep = b'\r\n'
    normalize_linesep_regex = re.compile(br'(?<!\r)\n')


class TestBase64(unittest.TestCase):
    def test_len(self):
        eq = self.assertEqual
        eq(base64mime.header_length('hello'),
           len(base64mime.body_encode(b'hello', eol='')))
        for size in range(15):
            if   size == 0 : bsize = 0
            elif size <= 3 : bsize = 4
            elif size <= 6 : bsize = 8
            elif size <= 9 : bsize = 12
            elif size <= 12: bsize = 16
            else           : bsize = 20
            eq(base64mime.header_length('x' * size), bsize)

    def test_decode(self):
        eq = self.assertEqual
        eq(base64mime.decode(''), b'')
        eq(base64mime.decode('aGVsbG8='), b'hello')

    def test_encode(self):
        eq = self.assertEqual
        eq(base64mime.body_encode(b''), '')
        eq(base64mime.body_encode(b'hello'), 'aGVsbG8=\n')
        # Test the binary flag
        eq(base64mime.body_encode(b'hello\n'), 'aGVsbG8K\n')
        # Test the maxlinelen arg
        eq(base64mime.body_encode(b'xxxx ' * 20, maxlinelen=40), """\
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg
eHh4eCB4eHh4IA==
""")
        # Test the eol argument
        eq(base64mime.body_encode(b'xxxx ' * 20, maxlinelen=40, eol='\r\n'),
           """\
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg\r
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg\r
eHh4eCB4eHh4IHh4eHggeHh4eCB4eHh4IHh4eHgg\r
eHh4eCB4eHh4IA==\r
""")

    def test_header_encode(self):
        eq = self.assertEqual
        he = base64mime.header_encode
        eq(he('hello'), '=?iso-8859-1?b?aGVsbG8=?=')
        eq(he('hello\r\nworld'), '=?iso-8859-1?b?aGVsbG8NCndvcmxk?=')
        eq(he('hello\nworld'), '=?iso-8859-1?b?aGVsbG8Kd29ybGQ=?=')
        # Test the charset option
        eq(he('hello', charset='iso-8859-2'), '=?iso-8859-2?b?aGVsbG8=?=')
        eq(he('hello\nworld'), '=?iso-8859-1?b?aGVsbG8Kd29ybGQ=?=')


class TestQuopri(unittest.TestCase):
    def setUp(self):
        # Set of characters (as byte integers) that don't need to be encoded
        # in headers.
        self.hlit = list(chain(
            range(ord('a'), ord('z') + 1),
            range(ord('A'), ord('Z') + 1),
            range(ord('0'), ord('9') + 1),
            (c for c in b'!*+-/')))
        # Set of characters (as byte integers) that do need to be encoded in
        # headers.
        self.hnon = [c for c in range(256) if c not in self.hlit]
        assert len(self.hlit) + len(self.hnon) == 256
        # Set of characters (as byte integers) that don't need to be encoded
        # in bodies.
        self.blit = list(range(ord(' '), ord('~') + 1))
        self.blit.append(ord('\t'))
        self.blit.remove(ord('='))
        # Set of characters (as byte integers) that do need to be encoded in
        # bodies.
        self.bnon = [c for c in range(256) if c not in self.blit]
        assert len(self.blit) + len(self.bnon) == 256

    def test_quopri_header_check(self):
        for c in self.hlit:
            self.assertFalse(quoprimime.header_check(c),
                        'Should not be header quopri encoded: %s' % chr(c))
        for c in self.hnon:
            self.assertTrue(quoprimime.header_check(c),
                            'Should be header quopri encoded: %s' % chr(c))

    def test_quopri_body_check(self):
        for c in self.blit:
            self.assertFalse(quoprimime.body_check(c),
                        'Should not be body quopri encoded: %s' % chr(c))
        for c in self.bnon:
            self.assertTrue(quoprimime.body_check(c),
                            'Should be body quopri encoded: %s' % chr(c))

    def test_header_quopri_len(self):
        eq = self.assertEqual
        eq(quoprimime.header_length(b'hello'), 5)
        # RFC 2047 chrome is not included in header_length().
        eq(len(quoprimime.header_encode(b'hello', charset='xxx')),
           quoprimime.header_length(b'hello') +
           # =?xxx?q?...?= means 10 extra characters
           10)
        eq(quoprimime.header_length(b'h@e@l@l@o@'), 20)
        # RFC 2047 chrome is not included in header_length().
        eq(len(quoprimime.header_encode(b'h@e@l@l@o@', charset='xxx')),
           quoprimime.header_length(b'h@e@l@l@o@') +
           # =?xxx?q?...?= means 10 extra characters
           10)
        for c in self.hlit:
            eq(quoprimime.header_length(bytes([c])), 1,
               'expected length 1 for %r' % chr(c))
        for c in self.hnon:
            # Space is special; it's encoded to _
            if c == ord(' '):
                continue
            eq(quoprimime.header_length(bytes([c])), 3,
               'expected length 3 for %r' % chr(c))
        eq(quoprimime.header_length(b' '), 1)

    def test_body_quopri_len(self):
        eq = self.assertEqual
        for c in self.blit:
            eq(quoprimime.body_length(bytes([c])), 1)
        for c in self.bnon:
            eq(quoprimime.body_length(bytes([c])), 3)

    def test_quote_unquote_idempotent(self):
        for x in range(256):
            c = chr(x)
            self.assertEqual(quoprimime.unquote(quoprimime.quote(c)), c)

    def _test_header_encode(self, header, expected_encoded_header, charset=None):
        if charset is None:
            encoded_header = quoprimime.header_encode(header)
        else:
            encoded_header = quoprimime.header_encode(header, charset)
        self.assertEqual(encoded_header, expected_encoded_header)

    def test_header_encode_null(self):
        self._test_header_encode(b'', '')

    def test_header_encode_one_word(self):
        self._test_header_encode(b'hello', '=?iso-8859-1?q?hello?=')

    def test_header_encode_two_lines(self):
        self._test_header_encode(b'hello\nworld',
                                '=?iso-8859-1?q?hello=0Aworld?=')

    def test_header_encode_non_ascii(self):
        self._test_header_encode(b'hello\xc7there',
                                '=?iso-8859-1?q?hello=C7there?=')

    def test_header_encode_alt_charset(self):
        self._test_header_encode(b'hello', '=?iso-8859-2?q?hello?=',
                charset='iso-8859-2')

    def _test_header_decode(self, encoded_header, expected_decoded_header):
        decoded_header = quoprimime.header_decode(encoded_header)
        self.assertEqual(decoded_header, expected_decoded_header)

    def test_header_decode_null(self):
        self._test_header_decode('', '')

    def test_header_decode_one_word(self):
        self._test_header_decode('hello', 'hello')

    def test_header_decode_two_lines(self):
        self._test_header_decode('hello=0Aworld', 'hello\nworld')

    def test_header_decode_non_ascii(self):
        self._test_header_decode('hello=C7there', 'hello\xc7there')

    def test_header_decode_re_bug_18380(self):
        # Issue 18380: Call re.sub with a positional argument for flags in the wrong position
        self.assertEqual(quoprimime.header_decode('=30' * 257), '0' * 257)

    def _test_decode(self, encoded, expected_decoded, eol=None):
        if eol is None:
            decoded = quoprimime.decode(encoded)
        else:
            decoded = quoprimime.decode(encoded, eol=eol)
        self.assertEqual(decoded, expected_decoded)

    def test_decode_null_word(self):
        self._test_decode('', '')

    def test_decode_null_line_null_word(self):
        self._test_decode('\r\n', '\n')

    def test_decode_one_word(self):
        self._test_decode('hello', 'hello')

    def test_decode_one_word_eol(self):
        self._test_decode('hello', 'hello', eol='X')

    def test_decode_one_line(self):
        self._test_decode('hello\r\n', 'hello\n')

    def test_decode_one_line_lf(self):
        self._test_decode('hello\n', 'hello\n')

    def test_decode_one_line_cr(self):
        self._test_decode('hello\r', 'hello\n')

    def test_decode_one_line_nl(self):
        self._test_decode('hello\n', 'helloX', eol='X')

    def test_decode_one_line_crnl(self):
        self._test_decode('hello\r\n', 'helloX', eol='X')

    def test_decode_one_line_one_word(self):
        self._test_decode('hello\r\nworld', 'hello\nworld')

    def test_decode_one_line_one_word_eol(self):
        self._test_decode('hello\r\nworld', 'helloXworld', eol='X')

    def test_decode_two_lines(self):
        self._test_decode('hello\r\nworld\r\n', 'hello\nworld\n')

    def test_decode_two_lines_eol(self):
        self._test_decode('hello\r\nworld\r\n', 'helloXworldX', eol='X')

    def test_decode_one_long_line(self):
        self._test_decode('Spam' * 250, 'Spam' * 250)

    def test_decode_one_space(self):
        self._test_decode(' ', '')

    def test_decode_multiple_spaces(self):
        self._test_decode(' ' * 5, '')

    def test_decode_one_line_trailing_spaces(self):
        self._test_decode('hello    \r\n', 'hello\n')

    def test_decode_two_lines_trailing_spaces(self):
        self._test_decode('hello    \r\nworld   \r\n', 'hello\nworld\n')

    def test_decode_quoted_word(self):
        self._test_decode('=22quoted=20words=22', '"quoted words"')

    def test_decode_uppercase_quoting(self):
        self._test_decode('ab=CD=EF', 'ab\xcd\xef')

    def test_decode_lowercase_quoting(self):
        self._test_decode('ab=cd=ef', 'ab\xcd\xef')

    def test_decode_soft_line_break(self):
        self._test_decode('soft line=\r\nbreak', 'soft linebreak')

    def test_decode_false_quoting(self):
        self._test_decode('A=1,B=A ==> A+B==2', 'A=1,B=A ==> A+B==2')

    def _test_encode(self, body, expected_encoded_body, maxlinelen=None, eol=None):
        kwargs = {}
        if maxlinelen is None:
            # Use body_encode's default.
            maxlinelen = 76
        else:
            kwargs['maxlinelen'] = maxlinelen
        if eol is None:
            # Use body_encode's default.
            eol = '\n'
        else:
            kwargs['eol'] = eol
        encoded_body = quoprimime.body_encode(body, **kwargs)
        self.assertEqual(encoded_body, expected_encoded_body)
        if eol == '\n' or eol == '\r\n':
            # We know how to split the result back into lines, so maxlinelen
            # can be checked.
            for line in encoded_body.splitlines():
                self.assertLessEqual(len(line), maxlinelen)

    def test_encode_null(self):
        self._test_encode('', '')

    def test_encode_null_lines(self):
        self._test_encode('\n\n', '\n\n')

    def test_encode_one_line(self):
        self._test_encode('hello\n', 'hello\n')

    def test_encode_one_line_crlf(self):
        self._test_encode('hello\r\n', 'hello\n')

    def test_encode_one_line_eol(self):
        self._test_encode('hello\n', 'hello\r\n', eol='\r\n')

    def test_encode_one_line_eol_after_non_ascii(self):
        # issue 20206; see changeset 0cf700464177 for why the encode/decode.
        self._test_encode('hello\u03c5\n'.encode('utf-8').decode('latin1'),
                          'hello=CF=85\r\n', eol='\r\n')

    def test_encode_one_space(self):
        self._test_encode(' ', '=20')

    def test_encode_one_line_one_space(self):
        self._test_encode(' \n', '=20\n')

# XXX: body_encode() expect strings, but uses ord(char) from these strings
# to index into a 256-entry list.  For code points above 255, this will fail.
# Should there be a check for 8-bit only ord() values in body, or at least
# a comment about the expected input?

    def test_encode_two_lines_one_space(self):
        self._test_encode(' \n \n', '=20\n=20\n')

    def test_encode_one_word_trailing_spaces(self):
        self._test_encode('hello   ', 'hello  =20')

    def test_encode_one_line_trailing_spaces(self):
        self._test_encode('hello   \n', 'hello  =20\n')

    def test_encode_one_word_trailing_tab(self):
        self._test_encode('hello  \t', 'hello  =09')

    def test_encode_one_line_trailing_tab(self):
        self._test_encode('hello  \t\n', 'hello  =09\n')

    def test_encode_trailing_space_before_maxlinelen(self):
        self._test_encode('abcd \n1234', 'abcd =\n\n1234', maxlinelen=6)

    def test_encode_trailing_space_at_maxlinelen(self):
        self._test_encode('abcd \n1234', 'abcd=\n=20\n1234', maxlinelen=5)

    def test_encode_trailing_space_beyond_maxlinelen(self):
        self._test_encode('abcd \n1234', 'abc=\nd=20\n1234', maxlinelen=4)

    def test_encode_whitespace_lines(self):
        self._test_encode(' \n' * 5, '=20\n' * 5)

    def test_encode_quoted_equals(self):
        self._test_encode('a = b', 'a =3D b')

    def test_encode_one_long_string(self):
        self._test_encode('x' * 100, 'x' * 75 + '=\n' + 'x' * 25)

    def test_encode_one_long_line(self):
        self._test_encode('x' * 100 + '\n', 'x' * 75 + '=\n' + 'x' * 25 + '\n')

    def test_encode_one_very_long_line(self):
        self._test_encode('x' * 200 + '\n',
                2 * ('x' * 75 + '=\n') + 'x' * 50 + '\n')

    def test_encode_shortest_maxlinelen(self):
        self._test_encode('=' * 5, '=3D=\n' * 4 + '=3D', maxlinelen=4)

    def test_encode_maxlinelen_too_small(self):
        self.assertRaises(ValueError, self._test_encode, '', '', maxlinelen=3)

    def test_encode(self):
        eq = self.assertEqual
        eq(quoprimime.body_encode(''), '')
        eq(quoprimime.body_encode('hello'), 'hello')
        # Test the binary flag
        eq(quoprimime.body_encode('hello\r\nworld'), 'hello\nworld')
        # Test the maxlinelen arg
        eq(quoprimime.body_encode('xxxx ' * 20, maxlinelen=40), """\
xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx=
 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxx=
x xxxx xxxx xxxx xxxx=20""")
        # Test the eol argument
        eq(quoprimime.body_encode('xxxx ' * 20, maxlinelen=40, eol='\r\n'),
           """\
xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx=\r
 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxx=\r
x xxxx xxxx xxxx xxxx=20""")
        eq(quoprimime.body_encode("""\
one line

two line"""), """\
one line

two line""")



# Test the Charset class
class TestCharset(unittest.TestCase):
    def tearDown(self):
        from email import charset as CharsetModule
        try:
            del CharsetModule.CHARSETS['fake']
        except KeyError:
            pass

    def test_codec_encodeable(self):
        eq = self.assertEqual
        # Make sure us-ascii = no Unicode conversion
        c = Charset('us-ascii')
        eq(c.header_encode('Hello World!'), 'Hello World!')
        # Test 8-bit idempotency with us-ascii
        s = '\xa4\xa2\xa4\xa4\xa4\xa6\xa4\xa8\xa4\xaa'
        self.assertRaises(UnicodeError, c.header_encode, s)
        c = Charset('utf-8')
        eq(c.header_encode(s), '=?utf-8?b?wqTCosKkwqTCpMKmwqTCqMKkwqo=?=')

    def test_body_encode(self):
        eq = self.assertEqual
        # Try a charset with QP body encoding
        c = Charset('iso-8859-1')
        eq('hello w=F6rld', c.body_encode('hello w\xf6rld'))
        # Try a charset with Base64 body encoding
        c = Charset('utf-8')
        eq('aGVsbG8gd29ybGQ=\n', c.body_encode(b'hello world'))
        # Try a charset with None body encoding
        c = Charset('us-ascii')
        eq('hello world', c.body_encode('hello world'))
        # Try the convert argument, where input codec != output codec
        c = Charset('euc-jp')
        # With apologies to Tokio Kikuchi ;)
        # XXX FIXME
##         try:
##             eq('\x1b$B5FCO;~IW\x1b(B',
##                c.body_encode('\xb5\xc6\xc3\xcf\xbb\xfe\xc9\xd7'))
##             eq('\xb5\xc6\xc3\xcf\xbb\xfe\xc9\xd7',
##                c.body_encode('\xb5\xc6\xc3\xcf\xbb\xfe\xc9\xd7', False))
##         except LookupError:
##             # We probably don't have the Japanese codecs installed
##             pass
        # Testing SF bug #625509, which we have to fake, since there are no
        # built-in encodings where the header encoding is QP but the body
        # encoding is not.
        from email import charset as CharsetModule
        CharsetModule.add_charset('fake', CharsetModule.QP, None, 'utf-8')
        c = Charset('fake')
        eq('hello world', c.body_encode('hello world'))

    def test_unicode_charset_name(self):
        charset = Charset('us-ascii')
        self.assertEqual(str(charset), 'us-ascii')
        self.assertRaises(errors.CharsetError, Charset, 'asc\xffii')



# Test multilingual MIME headers.
class TestHeader(TestEmailBase):
    def test_simple(self):
        eq = self.ndiffAssertEqual
        h = Header('Hello World!')
        eq(h.encode(), 'Hello World!')
        h.append(' Goodbye World!')
        eq(h.encode(), 'Hello World!  Goodbye World!')

    def test_simple_surprise(self):
        eq = self.ndiffAssertEqual
        h = Header('Hello World!')
        eq(h.encode(), 'Hello World!')
        h.append('Goodbye World!')
        eq(h.encode(), 'Hello World! Goodbye World!')

    def test_header_needs_no_decoding(self):
        h = 'no decoding needed'
        self.assertEqual(decode_header(h), [(h, None)])

    def test_long(self):
        h = Header("I am the very model of a modern Major-General; I've information vegetable, animal, and mineral; I know the kings of England, and I quote the fights historical from Marathon to Waterloo, in order categorical; I'm very well acquainted, too, with matters mathematical; I understand equations, both the simple and quadratical; about binomial theorem I'm teeming with a lot o' news, with many cheerful facts about the square of the hypotenuse.",
                   maxlinelen=76)
        for l in h.encode(splitchars=' ').split('\n '):
            self.assertLessEqual(len(l), 76)

    def test_multilingual(self):
        eq = self.ndiffAssertEqual
        g = Charset("iso-8859-1")
        cz = Charset("iso-8859-2")
        utf8 = Charset("utf-8")
        g_head = (b'Die Mieter treten hier ein werden mit einem '
                  b'Foerderband komfortabel den Korridor entlang, '
                  b'an s\xfcdl\xfcndischen Wandgem\xe4lden vorbei, '
                  b'gegen die rotierenden Klingen bef\xf6rdert. ')
        cz_head = (b'Finan\xe8ni metropole se hroutily pod tlakem jejich '
                   b'd\xf9vtipu.. ')
        utf8_head = ('\u6b63\u78ba\u306b\u8a00\u3046\u3068\u7ffb\u8a33\u306f'
                     '\u3055\u308c\u3066\u3044\u307e\u305b\u3093\u3002\u4e00'
                     '\u90e8\u306f\u30c9\u30a4\u30c4\u8a9e\u3067\u3059\u304c'
                     '\u3001\u3042\u3068\u306f\u3067\u305f\u3089\u3081\u3067'
                     '\u3059\u3002\u5b9f\u969b\u306b\u306f\u300cWenn ist das '
                     'Nunstuck git und Slotermeyer? Ja! Beiherhund das Oder '
                     'die Flipperwaldt gersput.\u300d\u3068\u8a00\u3063\u3066'
                     '\u3044\u307e\u3059\u3002')
        h = Header(g_head, g)
        h.append(cz_head, cz)
        h.append(utf8_head, utf8)
        enc = h.encode(maxlinelen=76)
        eq(enc, """\
=?iso-8859-1?q?Die_Mieter_treten_hier_ein_werden_mit_einem_Foerderband_kom?=
 =?iso-8859-1?q?fortabel_den_Korridor_entlang=2C_an_s=FCdl=FCndischen_Wand?=
 =?iso-8859-1?q?gem=E4lden_vorbei=2C_gegen_die_rotierenden_Klingen_bef=F6r?=
 =?iso-8859-1?q?dert=2E_?= =?iso-8859-2?q?Finan=E8ni_metropole_se_hroutily?=
 =?iso-8859-2?q?_pod_tlakem_jejich_d=F9vtipu=2E=2E_?= =?utf-8?b?5q2j56K6?=
 =?utf-8?b?44Gr6KiA44GG44Go57+76Kiz44Gv44GV44KM44Gm44GE44G+44Gb44KT44CC?=
 =?utf-8?b?5LiA6YOo44Gv44OJ44Kk44OE6Kqe44Gn44GZ44GM44CB44GC44Go44Gv44Gn?=
 =?utf-8?b?44Gf44KJ44KB44Gn44GZ44CC5a6f6Zqb44Gr44Gv44CMV2VubiBpc3QgZGFz?=
 =?utf-8?b?IE51bnN0dWNrIGdpdCB1bmQgU2xvdGVybWV5ZXI/IEphISBCZWloZXJodW5k?=
 =?utf-8?b?IGRhcyBPZGVyIGRpZSBGbGlwcGVyd2FsZHQgZ2Vyc3B1dC7jgI3jgajoqIA=?=
 =?utf-8?b?44Gj44Gm44GE44G+44GZ44CC?=""")
        decoded = decode_header(enc)
        eq(len(decoded), 3)
        eq(decoded[0], (g_head, 'iso-8859-1'))
        eq(decoded[1], (cz_head, 'iso-8859-2'))
        eq(decoded[2], (utf8_head.encode('utf-8'), 'utf-8'))
        ustr = str(h)
        eq(ustr,
           (b'Die Mieter treten hier ein werden mit einem Foerderband '
            b'komfortabel den Korridor entlang, an s\xc3\xbcdl\xc3\xbcndischen '
            b'Wandgem\xc3\xa4lden vorbei, gegen die rotierenden Klingen '
            b'bef\xc3\xb6rdert. Finan\xc4\x8dni metropole se hroutily pod '
            b'tlakem jejich d\xc5\xafvtipu.. \xe6\xad\xa3\xe7\xa2\xba\xe3\x81'
            b'\xab\xe8\xa8\x80\xe3\x81\x86\xe3\x81\xa8\xe7\xbf\xbb\xe8\xa8\xb3'
            b'\xe3\x81\xaf\xe3\x81\x95\xe3\x82\x8c\xe3\x81\xa6\xe3\x81\x84\xe3'
            b'\x81\xbe\xe3\x81\x9b\xe3\x82\x93\xe3\x80\x82\xe4\xb8\x80\xe9\x83'
            b'\xa8\xe3\x81\xaf\xe3\x83\x89\xe3\x82\xa4\xe3\x83\x84\xe8\xaa\x9e'
            b'\xe3\x81\xa7\xe3\x81\x99\xe3\x81\x8c\xe3\x80\x81\xe3\x81\x82\xe3'
            b'\x81\xa8\xe3\x81\xaf\xe3\x81\xa7\xe3\x81\x9f\xe3\x82\x89\xe3\x82'
            b'\x81\xe3\x81\xa7\xe3\x81\x99\xe3\x80\x82\xe5\xae\x9f\xe9\x9a\x9b'
            b'\xe3\x81\xab\xe3\x81\xaf\xe3\x80\x8cWenn ist das Nunstuck git '
            b'und Slotermeyer? Ja! Beiherhund das Oder die Flipperwaldt '
            b'gersput.\xe3\x80\x8d\xe3\x81\xa8\xe8\xa8\x80\xe3\x81\xa3\xe3\x81'
            b'\xa6\xe3\x81\x84\xe3\x81\xbe\xe3\x81\x99\xe3\x80\x82'
            ).decode('utf-8'))
        # Test make_header()
        newh = make_header(decode_header(enc))
        eq(newh, h)

    def test_empty_header_encode(self):
        h = Header()
        self.assertEqual(h.encode(), '')

    def test_header_ctor_default_args(self):
        eq = self.ndiffAssertEqual
        h = Header()
        eq(h, '')
        h.append('foo', Charset('iso-8859-1'))
        eq(h, 'foo')

    def test_explicit_maxlinelen(self):
        eq = self.ndiffAssertEqual
        hstr = ('A very long line that must get split to something other '
                'than at the 76th character boundary to test the non-default '
                'behavior')
        h = Header(hstr)
        eq(h.encode(), '''\
A very long line that must get split to something other than at the 76th
 character boundary to test the non-default behavior''')
        eq(str(h), hstr)
        h = Header(hstr, header_name='Subject')
        eq(h.encode(), '''\
A very long line that must get split to something other than at the
 76th character boundary to test the non-default behavior''')
        eq(str(h), hstr)
        h = Header(hstr, maxlinelen=1024, header_name='Subject')
        eq(h.encode(), hstr)
        eq(str(h), hstr)

    def test_quopri_splittable(self):
        eq = self.ndiffAssertEqual
        h = Header(charset='iso-8859-1', maxlinelen=20)
        x = 'xxxx ' * 20
        h.append(x)
        s = h.encode()
        eq(s, """\
=?iso-8859-1?q?xxx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_x?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?x_?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?xx?=
 =?iso-8859-1?q?_?=""")
        eq(x, str(make_header(decode_header(s))))
        h = Header(charset='iso-8859-1', maxlinelen=40)
        h.append('xxxx ' * 20)
        s = h.encode()
        eq(s, """\
=?iso-8859-1?q?xxxx_xxxx_xxxx_xxxx_xxx?=
 =?iso-8859-1?q?x_xxxx_xxxx_xxxx_xxxx_?=
 =?iso-8859-1?q?xxxx_xxxx_xxxx_xxxx_xx?=
 =?iso-8859-1?q?xx_xxxx_xxxx_xxxx_xxxx?=
 =?iso-8859-1?q?_xxxx_xxxx_?=""")
        eq(x, str(make_header(decode_header(s))))

    def test_base64_splittable(self):
        eq = self.ndiffAssertEqual
        h = Header(charset='koi8-r', maxlinelen=20)
        x = 'xxxx ' * 20
        h.append(x)
        s = h.encode()
        eq(s, """\
=?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IHh4?=
 =?koi8-r?b?eHgg?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?eCB4?=
 =?koi8-r?b?eHh4?=
 =?koi8-r?b?IA==?=""")
        eq(x, str(make_header(decode_header(s))))
        h = Header(charset='koi8-r', maxlinelen=40)
        h.append(x)
        s = h.encode()
        eq(s, """\
=?koi8-r?b?eHh4eCB4eHh4IHh4eHggeHh4?=
 =?koi8-r?b?eCB4eHh4IHh4eHggeHh4eCB4?=
 =?koi8-r?b?eHh4IHh4eHggeHh4eCB4eHh4?=
 =?koi8-r?b?IHh4eHggeHh4eCB4eHh4IHh4?=
 =?koi8-r?b?eHggeHh4eCB4eHh4IHh4eHgg?=
 =?koi8-r?b?eHh4eCB4eHh4IA==?=""")
        eq(x, str(make_header(decode_header(s))))

    def test_us_ascii_header(self):
        eq = self.assertEqual
        s = 'hello'
        x = decode_header(s)
        eq(x, [('hello', None)])
        h = make_header(x)
        eq(s, h.encode())

    def test_string_charset(self):
        eq = self.assertEqual
        h = Header()
        h.append('hello', 'iso-8859-1')
        eq(h, 'hello')

##    def test_unicode_error(self):
##        raises = self.assertRaises
##        raises(UnicodeError, Header, u'[P\xf6stal]', 'us-ascii')
##        raises(UnicodeError, Header, '[P\xf6stal]', 'us-ascii')
##        h = Header()
##        raises(UnicodeError, h.append, u'[P\xf6stal]', 'us-ascii')
##        raises(UnicodeError, h.append, '[P\xf6stal]', 'us-ascii')
##        raises(UnicodeError, Header, u'\u83ca\u5730\u6642\u592b', 'iso-8859-1')

    def test_utf8_shortest(self):
        eq = self.assertEqual
        h = Header('p\xf6stal', 'utf-8')
        eq(h.encode(), '=?utf-8?q?p=C3=B6stal?=')
        h = Header('\u83ca\u5730\u6642\u592b', 'utf-8')
        eq(h.encode(), '=?utf-8?b?6I+K5Zyw5pmC5aSr?=')

    def test_bad_8bit_header(self):
        raises = self.assertRaises
        eq = self.assertEqual
        x = b'Ynwp4dUEbay Auction Semiar- No Charge \x96 Earn Big'
        raises(UnicodeError, Header, x)
        h = Header()
        raises(UnicodeError, h.append, x)
        e = x.decode('utf-8', 'replace')
        eq(str(Header(x, errors='replace')), e)
        h.append(x, errors='replace')
        eq(str(h), e)

    def test_escaped_8bit_header(self):
        x = b'Ynwp4dUEbay Auction Semiar- No Charge \x96 Earn Big'
        e = x.decode('ascii', 'surrogateescape')
        h = Header(e, charset=email.charset.UNKNOWN8BIT)
        self.assertEqual(str(h),
                        'Ynwp4dUEbay Auction Semiar- No Charge \uFFFD Earn Big')
        self.assertEqual(email.header.decode_header(h), [(x, 'unknown-8bit')])

    def test_header_handles_binary_unknown8bit(self):
        x = b'Ynwp4dUEbay Auction Semiar- No Charge \x96 Earn Big'
        h = Header(x, charset=email.charset.UNKNOWN8BIT)
        self.assertEqual(str(h),
                        'Ynwp4dUEbay Auction Semiar- No Charge \uFFFD Earn Big')
        self.assertEqual(email.header.decode_header(h), [(x, 'unknown-8bit')])

    def test_make_header_handles_binary_unknown8bit(self):
        x = b'Ynwp4dUEbay Auction Semiar- No Charge \x96 Earn Big'
        h = Header(x, charset=email.charset.UNKNOWN8BIT)
        h2 = email.header.make_header(email.header.decode_header(h))
        self.assertEqual(str(h2),
                        'Ynwp4dUEbay Auction Semiar- No Charge \uFFFD Earn Big')
        self.assertEqual(email.header.decode_header(h2), [(x, 'unknown-8bit')])

    def test_modify_returned_list_does_not_change_header(self):
        h = Header('test')
        chunks = email.header.decode_header(h)
        chunks.append(('ascii', 'test2'))
        self.assertEqual(str(h), 'test')

    def test_encoded_adjacent_nonencoded(self):
        eq = self.assertEqual
        h = Header()
        h.append('hello', 'iso-8859-1')
        h.append('world')
        s = h.encode()
        eq(s, '=?iso-8859-1?q?hello?= world')
        h = make_header(decode_header(s))
        eq(h.encode(), s)

    def test_whitespace_keeper(self):
        eq = self.assertEqual
        s = 'Subject: =?koi8-r?b?8NLP18XSy8EgzsEgxsnOwczYztk=?= =?koi8-r?q?=CA?= zz.'
        parts = decode_header(s)
        eq(parts, [(b'Subject: ', None), (b'\xf0\xd2\xcf\xd7\xc5\xd2\xcb\xc1 \xce\xc1 \xc6\xc9\xce\xc1\xcc\xd8\xce\xd9\xca', 'koi8-r'), (b' zz.', None)])
        hdr = make_header(parts)
        eq(hdr.encode(),
           'Subject: =?koi8-r?b?8NLP18XSy8EgzsEgxsnOwczYztnK?= zz.')

    def test_broken_base64_header(self):
        raises = self.assertRaises
        s = 'Subject: =?EUC-KR?B?CSixpLDtKSC/7Liuvsax4iC6uLmwMcijIKHaILzSwd/H0SC8+LCjwLsgv7W/+Mj3I ?='
        raises(errors.HeaderParseError, decode_header, s)

    def test_shift_jis_charset(self):
        h = Header('文', charset='shift_jis')
        self.assertEqual(h.encode(), '=?iso-2022-jp?b?GyRCSjgbKEI=?=')

    def test_flatten_header_with_no_value(self):
        # Issue 11401 (regression from email 4.x)  Note that the space after
        # the header doesn't reflect the input, but this is also the way
        # email 4.x behaved.  At some point it would be nice to fix that.
        msg = email.message_from_string("EmptyHeader:")
        self.assertEqual(str(msg), "EmptyHeader: \n\n")

    def test_encode_preserves_leading_ws_on_value(self):
        msg = Message()
        msg['SomeHeader'] = '   value with leading ws'
        self.assertEqual(str(msg), "SomeHeader:    value with leading ws\n\n")

    def test_whitespace_header(self):
        self.assertEqual(Header(' ').encode(), ' ')



# Test RFC 2231 header parameters (en/de)coding
class TestRFC2231(TestEmailBase):

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_with_double_quotes
    # test_headerregistry.TestContentTypeHeader.rfc2231_single_quote_inside_double_quotes
    def test_get_param(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_29.txt')
        eq(msg.get_param('title'),
           ('us-ascii', 'en', 'This is even more ***fun*** isn\'t it!'))
        eq(msg.get_param('title', unquote=False),
           ('us-ascii', 'en', '"This is even more ***fun*** isn\'t it!"'))

    def test_set_param(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        msg.set_param('title', 'This is even more ***fun*** isn\'t it!',
                      charset='us-ascii')
        eq(msg.get_param('title'),
           ('us-ascii', '', 'This is even more ***fun*** isn\'t it!'))
        msg.set_param('title', 'This is even more ***fun*** isn\'t it!',
                      charset='us-ascii', language='en')
        eq(msg.get_param('title'),
           ('us-ascii', 'en', 'This is even more ***fun*** isn\'t it!'))
        msg = self._msgobj('msg_01.txt')
        msg.set_param('title', 'This is even more ***fun*** isn\'t it!',
                      charset='us-ascii', language='en')
        eq(msg.as_string(maxheaderlen=78), """\
Return-Path: <bbb@zzz.org>
Delivered-To: bbb@zzz.org
Received: by mail.zzz.org (Postfix, from userid 889)
\tid 27CEAD38CC; Fri,  4 May 2001 14:05:44 -0400 (EDT)
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit
Message-ID: <15090.61304.110929.45684@aaa.zzz.org>
From: bbb@ddd.com (John X. Doe)
To: bbb@zzz.org
Subject: This is a test message
Date: Fri, 4 May 2001 14:05:44 -0400
Content-Type: text/plain; charset=us-ascii;
 title*=us-ascii'en'This%20is%20even%20more%20%2A%2A%2Afun%2A%2A%2A%20isn%27t%20it%21


Hi,

Do you like this message?

-Me
""")

    def test_set_param_requote(self):
        msg = Message()
        msg.set_param('title', 'foo')
        self.assertEqual(msg['content-type'], 'text/plain; title="foo"')
        msg.set_param('title', 'bar', requote=False)
        self.assertEqual(msg['content-type'], 'text/plain; title=bar')
        # tspecial is still quoted.
        msg.set_param('title', "(bar)bell", requote=False)
        self.assertEqual(msg['content-type'], 'text/plain; title="(bar)bell"')

    def test_del_param(self):
        eq = self.ndiffAssertEqual
        msg = self._msgobj('msg_01.txt')
        msg.set_param('foo', 'bar', charset='us-ascii', language='en')
        msg.set_param('title', 'This is even more ***fun*** isn\'t it!',
            charset='us-ascii', language='en')
        msg.del_param('foo', header='Content-Type')
        eq(msg.as_string(maxheaderlen=78), """\
Return-Path: <bbb@zzz.org>
Delivered-To: bbb@zzz.org
Received: by mail.zzz.org (Postfix, from userid 889)
\tid 27CEAD38CC; Fri,  4 May 2001 14:05:44 -0400 (EDT)
MIME-Version: 1.0
Content-Transfer-Encoding: 7bit
Message-ID: <15090.61304.110929.45684@aaa.zzz.org>
From: bbb@ddd.com (John X. Doe)
To: bbb@zzz.org
Subject: This is a test message
Date: Fri, 4 May 2001 14:05:44 -0400
Content-Type: text/plain; charset="us-ascii";
 title*=us-ascii'en'This%20is%20even%20more%20%2A%2A%2Afun%2A%2A%2A%20isn%27t%20it%21


Hi,

Do you like this message?

-Me
""")

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_charset
    # I changed the charset name, though, because the one in the file isn't
    # a legal charset name.  Should add a test for an illegal charset.
    def test_rfc2231_get_content_charset(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_32.txt')
        eq(msg.get_content_charset(), 'us-ascii')

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_no_double_quotes
    def test_rfc2231_parse_rfc_quoting(self):
        m = textwrap.dedent('''\
            Content-Disposition: inline;
            \tfilename*0*=''This%20is%20even%20more%20;
            \tfilename*1*=%2A%2A%2Afun%2A%2A%2A%20;
            \tfilename*2="is it not.pdf"

            ''')
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf')
        self.assertEqual(m, msg.as_string())

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_with_double_quotes
    def test_rfc2231_parse_extra_quoting(self):
        m = textwrap.dedent('''\
            Content-Disposition: inline;
            \tfilename*0*="''This%20is%20even%20more%20";
            \tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
            \tfilename*2="is it not.pdf"

            ''')
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf')
        self.assertEqual(m, msg.as_string())

    # test_headerregistry.TestContentTypeHeader.rfc2231_no_language_or_charset
    # but new test uses *0* because otherwise lang/charset is not valid.
    # test_headerregistry.TestContentTypeHeader.rfc2231_segmented_normal_values
    def test_rfc2231_no_language_or_charset(self):
        m = '''\
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEMP_nsmail.htm"
Content-Type: text/html; NAME*0=file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEM; NAME*1=P_nsmail.htm

'''
        msg = email.message_from_string(m)
        param = msg.get_param('NAME')
        self.assertNotIsInstance(param, tuple)
        self.assertEqual(
            param,
            'file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEMP_nsmail.htm')

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_no_charset
    def test_rfc2231_no_language_or_charset_in_filename(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="''This%20is%20even%20more%20";
\tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf')

    # Duplicate of previous test?
    def test_rfc2231_no_language_or_charset_in_filename_encoded(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="''This%20is%20even%20more%20";
\tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf')

    # test_headerregistry.TestContentTypeHeader.rfc2231_partly_encoded,
    # but the test below is wrong (the first part should be decoded).
    def test_rfc2231_partly_encoded(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0="''This%20is%20even%20more%20";
\tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(
            msg.get_filename(),
            'This%20is%20even%20more%20***fun*** is it not.pdf')

    def test_rfc2231_partly_nonencoded(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0="This%20is%20even%20more%20";
\tfilename*1="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(
            msg.get_filename(),
            'This%20is%20even%20more%20%2A%2A%2Afun%2A%2A%2A%20is it not.pdf')

    def test_rfc2231_no_language_or_charset_in_boundary(self):
        m = '''\
Content-Type: multipart/alternative;
\tboundary*0*="''This%20is%20even%20more%20";
\tboundary*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tboundary*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_boundary(),
                         'This is even more ***fun*** is it not.pdf')

    def test_rfc2231_no_language_or_charset_in_charset(self):
        # This is a nonsensical charset value, but tests the code anyway
        m = '''\
Content-Type: text/plain;
\tcharset*0*="This%20is%20even%20more%20";
\tcharset*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tcharset*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_content_charset(),
                         'this is even more ***fun*** is it not.pdf')

    # test_headerregistry.TestContentTypeHeader.rfc2231_unknown_charset_treated_as_ascii
    def test_rfc2231_bad_encoding_in_filename(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="bogus'xx'This%20is%20even%20more%20";
\tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2="is it not.pdf"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf')

    def test_rfc2231_bad_encoding_in_charset(self):
        m = """\
Content-Type: text/plain; charset*=bogus''utf-8%E2%80%9D

"""
        msg = email.message_from_string(m)
        # This should return None because non-ascii characters in the charset
        # are not allowed.
        self.assertEqual(msg.get_content_charset(), None)

    def test_rfc2231_bad_character_in_charset(self):
        m = """\
Content-Type: text/plain; charset*=ascii''utf-8%E2%80%9D

"""
        msg = email.message_from_string(m)
        # This should return None because non-ascii characters in the charset
        # are not allowed.
        self.assertEqual(msg.get_content_charset(), None)

    def test_rfc2231_bad_character_in_filename(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="ascii'xx'This%20is%20even%20more%20";
\tfilename*1*="%2A%2A%2Afun%2A%2A%2A%20";
\tfilename*2*="is it not.pdf%E2"

'''
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(),
                         'This is even more ***fun*** is it not.pdf\ufffd')

    def test_rfc2231_unknown_encoding(self):
        m = """\
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename*=X-UNKNOWN''myfile.txt

"""
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(), 'myfile.txt')

    def test_rfc2231_bad_character_in_encoding(self):
        m = """\
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename*=utf-8\udce2\udc80\udc9d''myfile.txt

"""
        msg = email.message_from_string(m)
        self.assertEqual(msg.get_filename(), 'myfile.txt')

    def test_rfc2231_single_tick_in_filename_extended(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo;
\tname*0*=\"Frank's\"; name*1*=\" Document\"

"""
        msg = email.message_from_string(m)
        charset, language, s = msg.get_param('name')
        eq(charset, None)
        eq(language, None)
        eq(s, "Frank's Document")

    # test_headerregistry.TestContentTypeHeader.rfc2231_single_quote_inside_double_quotes
    def test_rfc2231_single_tick_in_filename(self):
        m = """\
Content-Type: application/x-foo; name*0=\"Frank's\"; name*1=\" Document\"

"""
        msg = email.message_from_string(m)
        param = msg.get_param('name')
        self.assertNotIsInstance(param, tuple)
        self.assertEqual(param, "Frank's Document")

    def test_rfc2231_missing_tick(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="'This%20is%20broken";
'''
        msg = email.message_from_string(m)
        self.assertEqual(
            msg.get_filename(),
            "'This is broken")

    def test_rfc2231_missing_tick_with_encoded_non_ascii(self):
        m = '''\
Content-Disposition: inline;
\tfilename*0*="'This%20is%E2broken";
'''
        msg = email.message_from_string(m)
        self.assertEqual(
            msg.get_filename(),
            "'This is\ufffdbroken")

    # test_headerregistry.TestContentTypeHeader.rfc2231_single_quote_in_value_with_charset_and_lang
    def test_rfc2231_tick_attack_extended(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo;
\tname*0*=\"us-ascii'en-us'Frank's\"; name*1*=\" Document\"

"""
        msg = email.message_from_string(m)
        charset, language, s = msg.get_param('name')
        eq(charset, 'us-ascii')
        eq(language, 'en-us')
        eq(s, "Frank's Document")

    # test_headerregistry.TestContentTypeHeader.rfc2231_single_quote_in_non_encoded_value
    def test_rfc2231_tick_attack(self):
        m = """\
Content-Type: application/x-foo;
\tname*0=\"us-ascii'en-us'Frank's\"; name*1=\" Document\"

"""
        msg = email.message_from_string(m)
        param = msg.get_param('name')
        self.assertNotIsInstance(param, tuple)
        self.assertEqual(param, "us-ascii'en-us'Frank's Document")

    # test_headerregistry.TestContentTypeHeader.rfc2231_single_quotes_inside_quotes
    def test_rfc2231_no_extended_values(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo; name=\"Frank's Document\"

"""
        msg = email.message_from_string(m)
        eq(msg.get_param('name'), "Frank's Document")

    # test_headerregistry.TestContentTypeHeader.rfc2231_encoded_then_unencoded_segments
    def test_rfc2231_encoded_then_unencoded_segments(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo;
\tname*0*=\"us-ascii'en-us'My\";
\tname*1=\" Document\";
\tname*2*=\" For You\"

"""
        msg = email.message_from_string(m)
        charset, language, s = msg.get_param('name')
        eq(charset, 'us-ascii')
        eq(language, 'en-us')
        eq(s, 'My Document For You')

    # test_headerregistry.TestContentTypeHeader.rfc2231_unencoded_then_encoded_segments
    # test_headerregistry.TestContentTypeHeader.rfc2231_quoted_unencoded_then_encoded_segments
    def test_rfc2231_unencoded_then_encoded_segments(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo;
\tname*0=\"us-ascii'en-us'My\";
\tname*1*=\" Document\";
\tname*2*=\" For You\"

"""
        msg = email.message_from_string(m)
        charset, language, s = msg.get_param('name')
        eq(charset, 'us-ascii')
        eq(language, 'en-us')
        eq(s, 'My Document For You')

    def test_should_not_hang_on_invalid_ew_messages(self):
        messages = ["""From: user@host.com
To: user@host.com
Bad-Header:
 =?us-ascii?Q?LCSwrV11+IB0rSbSker+M9vWR7wEDSuGqmHD89Gt=ea0nJFSaiz4vX3XMJPT4vrE?=
 =?us-ascii?Q?xGUZeOnp0o22pLBB7CYLH74Js=wOlK6Tfru2U47qR?=
 =?us-ascii?Q?72OfyEY2p2=2FrA9xNFyvH+fBTCmazxwzF8nGkK6D?=

Hello!
""", """From: ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ <xxx@xxx>
To: "xxx" <xxx@xxx>
Subject:   ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½
MIME-Version: 1.0
Content-Type: text/plain; charset="windows-1251";
Content-Transfer-Encoding: 8bit

ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
"""]
        for m in messages:
            with self.subTest(m=m):
                msg = email.message_from_string(m)


# Tests to ensure that signed parts of an email are completely preserved, as
# required by RFC1847 section 2.1.  Note that these are incomplete, because the
# email package does not currently always preserve the body.  See issue 1670765.
class TestSigned(TestEmailBase):

    def _msg_and_obj(self, filename):
        with openfile(filename, encoding="utf-8") as fp:
            original = fp.read()
            msg = email.message_from_string(original)
        return original, msg

    def _signed_parts_eq(self, original, result):
        # Extract the first mime part of each message
        import re
        repart = re.compile(r'^--([^\n]+)\n(.*?)\n--\1$', re.S | re.M)
        inpart = repart.search(original).group(2)
        outpart = repart.search(result).group(2)
        self.assertEqual(outpart, inpart)

    def test_long_headers_as_string(self):
        original, msg = self._msg_and_obj('msg_45.txt')
        result = msg.as_string()
        self._signed_parts_eq(original, result)

    def test_long_headers_as_string_maxheaderlen(self):
        original, msg = self._msg_and_obj('msg_45.txt')
        result = msg.as_string(maxheaderlen=60)
        self._signed_parts_eq(original, result)

    def test_long_headers_flatten(self):
        original, msg = self._msg_and_obj('msg_45.txt')
        fp = StringIO()
        Generator(fp).flatten(msg)
        result = fp.getvalue()
        self._signed_parts_eq(original, result)

class TestHeaderRegistry(TestEmailBase):
    # See issue gh-93010.
    def test_HeaderRegistry(self):
        reg = HeaderRegistry()
        a = reg('Content-Disposition', 'attachment; 0*00="foo"')
        self.assertIsInstance(a.defects[0], errors.InvalidHeaderDefect)

if __name__ == '__main__':
    unittest.main()
