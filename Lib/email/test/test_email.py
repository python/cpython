# Copyright (C) 2001-2010 Python Software Foundation
# Contact: email-sig@python.org
# email package unit tests

import os
import sys
import time
import base64
import difflib
import unittest
import warnings
import textwrap

from io import StringIO, BytesIO
from itertools import chain

import email

from email.charset import Charset
from email.header import Header, decode_header, make_header
from email.parser import Parser, HeaderParser
from email.generator import Generator, DecodedGenerator
from email.message import Message
from email.mime.application import MIMEApplication
from email.mime.audio import MIMEAudio
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.base import MIMEBase
from email.mime.message import MIMEMessage
from email.mime.multipart import MIMEMultipart
from email import utils
from email import errors
from email import encoders
from email import iterators
from email import base64mime
from email import quoprimime

from test.support import findfile, run_unittest, unlink
from email.test import __file__ as landmark


NL = '\n'
EMPTYSTRING = ''
SPACE = ' '



def openfile(filename, *args, **kws):
    path = os.path.join(os.path.dirname(landmark), 'data', filename)
    return open(path, *args, **kws)



# Base test class
class TestEmailBase(unittest.TestCase):
    def ndiffAssertEqual(self, first, second):
        """Like assertEqual except use ndiff for readable output."""
        if first != second:
            sfirst = str(first)
            ssecond = str(second)
            rfirst = [repr(line) for line in sfirst.splitlines()]
            rsecond = [repr(line) for line in ssecond.splitlines()]
            diff = difflib.ndiff(rfirst, rsecond)
            raise self.failureException(NL + NL.join(diff))

    def _msgobj(self, filename):
        with openfile(findfile(filename)) as fp:
            return email.message_from_file(fp)



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

    def test_message_rfc822_only(self):
        # Issue 7970: message/rfc822 not in multipart parsed by
        # HeaderParser caused an exception when flattened.
        with openfile(findfile('msg_46.txt')) as fp:
            msgdata = fp.read()
        parser = HeaderParser()
        msg = parser.parsestr(msgdata)
        out = StringIO()
        gen = Generator(out, True, 0)
        gen.flatten(msg, False)
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

    def test_decoded_generator(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_07.txt')
        with openfile('msg_17.txt') as fp:
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
        self.assertTrue('from' in msg)
        self.assertTrue('From' in msg)
        self.assertTrue('FROM' in msg)
        self.assertTrue('to' in msg)
        self.assertTrue('To' in msg)
        self.assertTrue('TO' in msg)

    def test_as_string(self):
        eq = self.ndiffAssertEqual
        msg = self._msgobj('msg_01.txt')
        with openfile('msg_01.txt') as fp:
            text = fp.read()
        eq(text, str(msg))
        fullrepr = msg.as_string(unixfrom=True)
        lines = fullrepr.split('\n')
        self.assertTrue(lines[0].startswith('From '))
        eq(text, NL.join(lines[1:]))

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

    def test_get_param_funky_continuation_lines(self):
        msg = self._msgobj('msg_22.txt')
        self.assertEqual(msg.get_payload(1).get_param('name'), 'wibble.JPG')

    def test_get_param_with_semis_in_quotes(self):
        msg = email.message_from_string(
            'Content-Type: image/pjpeg; name="Jim&amp;&amp;Jill"\n')
        self.assertEqual(msg.get_param('name'), 'Jim&amp;&amp;Jill')
        self.assertEqual(msg.get_param('name', unquote=False),
                         '"Jim&amp;&amp;Jill"')

    def test_get_param_with_quotes(self):
        msg = email.message_from_string(
            'Content-Type: foo; bar*0="baz\\"foobar"; bar*1="\\"baz"')
        self.assertEqual(msg.get_param('bar'), 'baz"foobar"baz')
        msg = email.message_from_string(
            "Content-Type: foo; bar*0=\"baz\\\"foobar\"; bar*1=\"\\\"baz\"")
        self.assertEqual(msg.get_param('bar'), 'baz"foobar"baz')

    def test_field_containment(self):
        unless = self.assertTrue
        msg = email.message_from_string('Header: exists')
        unless('header' in msg)
        unless('Header' in msg)
        unless('HEADER' in msg)
        self.assertFalse('headerx' in msg)

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

    def test_broken_base64_payload(self):
        x = 'AwDp0P7//y6LwKEAcPa/6Q=9'
        msg = Message()
        msg['content-type'] = 'audio/x-midi'
        msg['content-transfer-encoding'] = 'base64'
        msg.set_payload(x)
        self.assertEqual(msg.get_payload(decode=True),
                         bytes(x, 'raw-unicode-escape'))



# Test the email.encoders module
class TestEncoders(unittest.TestCase):
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
        eq(msg['content-transfer-encoding'], '8bit')
        # And now with a different charset
        msg = MIMEText('hello \xf8 world', _charset='iso-8859-1')
        eq(msg['content-transfer-encoding'], 'quoted-printable')

    def test_encode7or8bit(self):
        # Make sure a charset whose input character set is 8bit but
        # whose output character set is 7bit gets a transfer-encoding
        # of 7bit.
        eq = self.assertEqual
        msg = MIMEText('文', _charset='euc-jp')
        eq(msg['content-transfer-encoding'], '7bit')


# Test long header wrapping
class TestLongHeaders(TestEmailBase):
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
        self.assertRaises(UnicodeEncodeError, msg.as_string)
        msg = Message()
        msg['Reply-To'] = Header(header_string, 'utf-8',
                                 header_name='Reply-To')
        eq(msg.as_string(maxheaderlen=78), """\
Reply-To: =?utf-8?q?Britische_Regierung_gibt_gr=C3=BCnes_Licht_f=C3=BCr_Offs?=
 =?utf-8?q?hore-Windkraftprojekte_=3Ca-very-long-address=40example=2Ecom=3E?=

""")

    def test_long_to_header(self):
        eq = self.ndiffAssertEqual
        to = ('"Someone Test #A" <someone@eecs.umich.edu>,'
              '<someone@eecs.umich.edu>,'
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
Received-1: from FOO.TLD (vizworld.acl.foo.tld [123.452.678.9]) by hrothgar.la.mastaler.com (tmda-ofmipd) with ESMTP;
 Wed, 05 Mar 2003 18:10:18 -0700
Received-2: from FOO.TLD (vizworld.acl.foo.tld [123.452.678.9]) by hrothgar.la.mastaler.com (tmda-ofmipd) with ESMTP;
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
        # XXX This should be splitting on spaces not commas.
        self.ndiffAssertEqual(msg.as_string(maxheaderlen=78), """\
Received-1: <15975.17901.207240.414604@sgigritzmann1.mathematik.tu-muenchen.de> (David Bremner's message of \"Thu,
 6 Mar 2003 13:58:21 +0100\")
Received-2: <15975.17901.207240.414604@sgigritzmann1.mathematik.tu-muenchen.de> (David Bremner's message of \"Thu,
 6 Mar 2003 13:58:21 +0100\")

""")

    def test_long_unbreakable_lines_with_continuation(self):
        eq = self.ndiffAssertEqual
        msg = Message()
        t = """\
iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp"""
        msg['Face-1'] = t
        msg['Face-2'] = Header(t, header_name='Face-2')
        # XXX This splitting is all wrong.  It the first value line should be
        # snug against the field name.
        eq(msg.as_string(maxheaderlen=78), """\
Face-1:\x20
 iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAGFBMVEUAAAAkHiJeRUIcGBi9
 locQDQ4zJykFBAXJfWDjAAACYUlEQVR4nF2TQY/jIAyFc6lydlG5x8Nyp1Y69wj1PN2I5gzp
Face-2:\x20
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
Received: from siimage.com ([172.25.1.3]) by zima.siliconimage.com with Microsoft SMTPSVC(5.0.2195.4905);
 Wed, 16 Oct 2002 07:41:11 -0700

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
List: List-Unsubscribe: <http://lists.sourceforge.net/lists/listinfo/spamassassin-talk>,
        <mailto:spamassassin-talk-request@lists.sourceforge.net?subject=unsubscribe>
List: List-Unsubscribe: <http://lists.sourceforge.net/lists/listinfo/spamassassin-talk>,
        <mailto:spamassassin-talk-request@lists.sourceforge.net?subject=unsubscribe>

""")



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



# Test the basic MIMEAudio class
class TestMIMEAudio(unittest.TestCase):
    def setUp(self):
        # Make sure we pick up the audiotest.au that lives in email/test/data.
        # In Python, there's an audiotest.au living in Lib/test but that isn't
        # included in some binary distros that don't include the test
        # package.  The trailing empty string on the .join() is significant
        # since findfile() will do a dirname().
        datadir = os.path.join(os.path.dirname(landmark), 'data', '')
        with open(findfile('audiotest.au', datadir), 'rb') as fp:
            self._audiodata = fp.read()
        self._au = MIMEAudio(self._audiodata)

    def test_guess_minor_type(self):
        self.assertEqual(self._au.get_content_type(), 'audio/basic')

    def test_encoding(self):
        payload = self._au.get_payload()
        self.assertEqual(base64.decodebytes(bytes(payload, 'ascii')),
                self._audiodata)

    def test_checkSetMinor(self):
        au = MIMEAudio(self._audiodata, 'fish')
        self.assertEqual(au.get_content_type(), 'audio/fish')

    def test_add_header(self):
        eq = self.assertEqual
        unless = self.assertTrue
        self._au.add_header('Content-Disposition', 'attachment',
                            filename='audiotest.au')
        eq(self._au['content-disposition'],
           'attachment; filename="audiotest.au"')
        eq(self._au.get_params(header='content-disposition'),
           [('attachment', ''), ('filename', 'audiotest.au')])
        eq(self._au.get_param('filename', header='content-disposition'),
           'audiotest.au')
        missing = []
        eq(self._au.get_param('attachment', header='content-disposition'), '')
        unless(self._au.get_param('foo', failobj=missing,
                                  header='content-disposition') is missing)
        # Try some missing stuff
        unless(self._au.get_param('foobar', missing) is missing)
        unless(self._au.get_param('attachment', missing,
                                  header='foobar') is missing)



# Test the basic MIMEImage class
class TestMIMEImage(unittest.TestCase):
    def setUp(self):
        with openfile('PyBanner048.gif', 'rb') as fp:
            self._imgdata = fp.read()
        self._im = MIMEImage(self._imgdata)

    def test_guess_minor_type(self):
        self.assertEqual(self._im.get_content_type(), 'image/gif')

    def test_encoding(self):
        payload = self._im.get_payload()
        self.assertEqual(base64.decodebytes(bytes(payload, 'ascii')),
                self._imgdata)

    def test_checkSetMinor(self):
        im = MIMEImage(self._imgdata, 'fish')
        self.assertEqual(im.get_content_type(), 'image/fish')

    def test_add_header(self):
        eq = self.assertEqual
        unless = self.assertTrue
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
        unless(self._im.get_param('foo', failobj=missing,
                                  header='content-disposition') is missing)
        # Try some missing stuff
        unless(self._im.get_param('foobar', missing) is missing)
        unless(self._im.get_param('attachment', missing,
                                  header='foobar') is missing)



# Test the basic MIMEApplication class
class TestMIMEApplication(unittest.TestCase):
    def test_headers(self):
        eq = self.assertEqual
        msg = MIMEApplication(b'\xfa\xfb\xfc\xfd\xfe\xff')
        eq(msg.get_content_type(), 'application/octet-stream')
        eq(msg['content-transfer-encoding'], 'base64')

    def test_body(self):
        eq = self.assertEqual
        bytes = b'\xfa\xfb\xfc\xfd\xfe\xff'
        msg = MIMEApplication(bytes)
        eq(msg.get_payload(), '+vv8/f7/')
        eq(msg.get_payload(decode=True), bytes)



# Test the basic MIMEText class
class TestMIMEText(unittest.TestCase):
    def setUp(self):
        self._msg = MIMEText('hello there')

    def test_types(self):
        eq = self.assertEqual
        unless = self.assertTrue
        eq(self._msg.get_content_type(), 'text/plain')
        eq(self._msg.get_param('charset'), 'us-ascii')
        missing = []
        unless(self._msg.get_param('foobar', missing) is missing)
        unless(self._msg.get_param('charset', missing, header='foobar')
               is missing)

    def test_payload(self):
        self.assertEqual(self._msg.get_payload(), 'hello there')
        self.assertTrue(not self._msg.is_multipart())

    def test_charset(self):
        eq = self.assertEqual
        msg = MIMEText('hello there', _charset='us-ascii')
        eq(msg.get_charset().input_charset, 'us-ascii')
        eq(msg['content-type'], 'text/plain; charset="us-ascii"')

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
        self.assertTrue('hello there' in msg.as_string())

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
        with openfile('PyBanner048.gif', 'rb') as fp:
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
        unless = self.assertTrue
        raises = self.assertRaises
        # tests
        m = self._msg
        unless(m.is_multipart())
        eq(m.get_content_type(), 'multipart/mixed')
        eq(len(m.get_payload()), 2)
        raises(IndexError, m.get_payload, 2)
        m0 = m.get_payload(0)
        m1 = m.get_payload(1)
        unless(m0 is self._txt)
        unless(m1 is self._im)
        eq(m.get_payload(), [m0, m1])
        unless(not m0.is_multipart())
        unless(not m1.is_multipart())

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

--BOUNDARY--''')

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
--BOUNDARY--''')

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
--BOUNDARY--''')


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
--BOUNDARY--''')


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
--BOUNDARY--''')


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
        self.assertEquals(m.get_payload(0).get_payload(), 'YXNkZg==')



# Test some badly formatted messages
class TestNonConformant(TestEmailBase):
    def test_parse_missing_minor_type(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_14.txt')
        eq(msg.get_content_type(), 'text/plain')
        eq(msg.get_content_maintype(), 'text')
        eq(msg.get_content_subtype(), 'plain')

    def test_same_boundary_inner_outer(self):
        unless = self.assertTrue
        msg = self._msgobj('msg_15.txt')
        # XXX We can probably eventually do better
        inner = msg.get_payload(0)
        unless(hasattr(inner, 'defects'))
        self.assertEqual(len(inner.defects), 1)
        unless(isinstance(inner.defects[0],
                          errors.StartBoundaryNotFoundDefect))

    def test_multipart_no_boundary(self):
        unless = self.assertTrue
        msg = self._msgobj('msg_25.txt')
        unless(isinstance(msg.get_payload(), str))
        self.assertEqual(len(msg.defects), 2)
        unless(isinstance(msg.defects[0], errors.NoBoundaryInMultipartDefect))
        unless(isinstance(msg.defects[1],
                          errors.MultipartInvariantViolationDefect))

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

    def test_lying_multipart(self):
        unless = self.assertTrue
        msg = self._msgobj('msg_41.txt')
        unless(hasattr(msg, 'defects'))
        self.assertEqual(len(msg.defects), 2)
        unless(isinstance(msg.defects[0], errors.NoBoundaryInMultipartDefect))
        unless(isinstance(msg.defects[1],
                          errors.MultipartInvariantViolationDefect))

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
        self.assertTrue(isinstance(bad.defects[0],
                                   errors.StartBoundaryNotFoundDefect))

    def test_first_line_is_continuation_header(self):
        eq = self.assertEqual
        m = ' Line 1\nLine 2\nLine 3'
        msg = email.message_from_string(m)
        eq(msg.keys(), [])
        eq(msg.get_payload(), 'Line 2\nLine 3')
        eq(len(msg.defects), 1)
        self.assertTrue(isinstance(msg.defects[0],
                                   errors.FirstHeaderLineIsContinuationDefect))
        eq(msg.defects[0].line, ' Line 1\n')



# Test RFC 2047 header encoding and decoding
class TestRFC2047(TestEmailBase):
    def test_rfc2047_multiline(self):
        eq = self.assertEqual
        s = """Re: =?mac-iceland?q?r=8Aksm=9Arg=8Cs?= baz
 foo bar =?mac-iceland?q?r=8Aksm=9Arg=8Cs?="""
        dh = decode_header(s)
        eq(dh, [
            (b'Re:', None),
            (b'r\x8aksm\x9arg\x8cs', 'mac-iceland'),
            (b'baz foo bar', None),
            (b'r\x8aksm\x9arg\x8cs', 'mac-iceland')])
        header = make_header(dh)
        eq(str(header),
           'Re: r\xe4ksm\xf6rg\xe5s baz foo bar r\xe4ksm\xf6rg\xe5s')
        self.ndiffAssertEqual(header.encode(maxlinelen=76), """\
Re: =?mac-iceland?q?r=8Aksm=9Arg=8Cs?= baz foo bar =?mac-iceland?q?r=8Aksm?=
 =?mac-iceland?q?=9Arg=8Cs?=""")

    def test_whitespace_eater_unicode(self):
        eq = self.assertEqual
        s = '=?ISO-8859-1?Q?Andr=E9?= Pirard <pirard@dom.ain>'
        dh = decode_header(s)
        eq(dh, [(b'Andr\xe9', 'iso-8859-1'),
                (b'Pirard <pirard@dom.ain>', None)])
        header = str(make_header(dh))
        eq(header, 'Andr\xe9 Pirard <pirard@dom.ain>')

    def test_whitespace_eater_unicode_2(self):
        eq = self.assertEqual
        s = 'The =?iso-8859-1?b?cXVpY2sgYnJvd24gZm94?= jumped over the =?iso-8859-1?b?bGF6eSBkb2c=?='
        dh = decode_header(s)
        eq(dh, [(b'The', None), (b'quick brown fox', 'iso-8859-1'),
                (b'jumped over the', None), (b'lazy dog', 'iso-8859-1')])
        hu = str(make_header(dh))
        eq(hu, 'The quick brown fox jumped over the lazy dog')

    def test_rfc2047_missing_whitespace(self):
        s = 'Sm=?ISO-8859-1?B?9g==?=rg=?ISO-8859-1?B?5Q==?=sbord'
        dh = decode_header(s)
        self.assertEqual(dh, [(s, None)])

    def test_rfc2047_with_whitespace(self):
        s = 'Sm =?ISO-8859-1?B?9g==?= rg =?ISO-8859-1?B?5Q==?= sbord'
        dh = decode_header(s)
        self.assertEqual(dh, [(b'Sm', None), (b'\xf6', 'iso-8859-1'),
                              (b'rg', None), (b'\xe5', 'iso-8859-1'),
                              (b'sbord', None)])

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
        s = '=?iso-8659-1?Q?andr=e9=zz?='
        self.assertEqual(decode_header(s),
                        [(b'andr\xe9=zz', 'iso-8659-1')])


# Test the MIMEMessage class
class TestMIMEMessage(TestEmailBase):
    def setUp(self):
        with openfile('msg_11.txt') as fp:
            self._text = fp.read()

    def test_type_error(self):
        self.assertRaises(TypeError, MIMEMessage, 'a plain string')

    def test_valid_argument(self):
        eq = self.assertEqual
        unless = self.assertTrue
        subject = 'A sub-message'
        m = Message()
        m['Subject'] = subject
        r = MIMEMessage(m)
        eq(r.get_content_type(), 'message/rfc822')
        payload = r.get_payload()
        unless(isinstance(payload, list))
        eq(len(payload), 1)
        subpart = payload[0]
        unless(subpart is m)
        eq(subpart['subject'], subject)

    def test_bad_multipart(self):
        eq = self.assertEqual
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
        unless = self.assertTrue
        msg = self._msgobj('msg_11.txt')
        eq(msg.get_content_type(), 'message/rfc822')
        payload = msg.get_payload()
        unless(isinstance(payload, list))
        eq(len(payload), 1)
        submsg = payload[0]
        self.assertTrue(isinstance(submsg, Message))
        eq(submsg['subject'], 'An enclosed message')
        eq(submsg.get_payload(), 'Here is the body of the message.\n')

    def test_dsn(self):
        eq = self.assertEqual
        unless = self.assertTrue
        # msg 16 is a Delivery Status Notification, see RFC 1894
        msg = self._msgobj('msg_16.txt')
        eq(msg.get_content_type(), 'multipart/report')
        unless(msg.is_multipart())
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
        unless(isinstance(dsn1, Message))
        eq(dsn1['original-envelope-id'], '0GK500B4HD0888@cougar.noc.ucla.edu')
        eq(dsn1.get_param('dns', header='reporting-mta'), '')
        # Try a missing one <wink>
        eq(dsn1.get_param('nsd', header='reporting-mta'), None)
        dsn2 = subpart.get_payload(1)
        unless(isinstance(dsn2, Message))
        eq(dsn2['action'], 'failed')
        eq(dsn2.get_params(header='original-recipient'),
           [('rfc822', ''), ('jangel1@cougar.noc.ucla.edu', '')])
        eq(dsn2.get_param('rfc822', header='final-recipient'), '')
        # Subpart 3 is the original message
        subpart = msg.get_payload(2)
        eq(subpart.get_content_type(), 'message/rfc822')
        payload = subpart.get_payload()
        unless(isinstance(payload, list))
        eq(len(payload), 1)
        subsubpart = payload[0]
        unless(isinstance(subsubpart, Message))
        eq(subsubpart.get_content_type(), 'text/plain')
        eq(subsubpart['message-id'],
           '<002001c144a6$8752e060$56104586@oxy.edu>')

    def test_epilogue(self):
        eq = self.ndiffAssertEqual
        with openfile('msg_21.txt') as fp:
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
        with openfile('msg_30.txt') as fp:
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
        with openfile('msg_28.txt') as fp:
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


# A general test of parser->model->generator idempotency.  IOW, read a message
# in, parse it into a message object tree, then without touching the tree,
# regenerate the plain text.  The original text and the transformed text
# should be identical.  Note: that we ignore the Unix-From since that may
# contain a changed date.
class TestIdempotent(TestEmailBase):
    def _msgobj(self, filename):
        with openfile(filename) as fp:
            data = fp.read()
        msg = email.message_from_string(data)
        return msg, data

    def _idempotent(self, msg, text):
        eq = self.ndiffAssertEqual
        s = StringIO()
        g = Generator(s, maxheaderlen=0)
        g.flatten(msg)
        eq(text, s.getvalue())

    def test_parse_text_message(self):
        eq = self.assertEquals
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
        eq = self.assertEquals
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

    def test_message_signed_idempotent(self):
        msg, text = self._msgobj('msg_45.txt')
        self._idempotent(msg, text)

    def test_content_type(self):
        eq = self.assertEquals
        unless = self.assertTrue
        # Get a message object and reset the seek pointer for other tests
        msg, text = self._msgobj('msg_05.txt')
        eq(msg.get_content_type(), 'multipart/report')
        # Test the Content-Type: parameters
        params = {}
        for pk, pv in msg.get_params():
            params[pk] = pv
        eq(params['report-type'], 'delivery-status')
        eq(params['boundary'], 'D1690A7AC1.996856090/mail.example.com')
        eq(msg.preamble, 'This is a MIME-encapsulated message.\n')
        eq(msg.epilogue, '\n')
        eq(len(msg.get_payload()), 3)
        # Make sure the subparts are what we expect
        msg1 = msg.get_payload(0)
        eq(msg1.get_content_type(), 'text/plain')
        eq(msg1.get_payload(), 'Yadda yadda yadda\n')
        msg2 = msg.get_payload(1)
        eq(msg2.get_content_type(), 'text/plain')
        eq(msg2.get_payload(), 'Yadda yadda yadda\n')
        msg3 = msg.get_payload(2)
        eq(msg3.get_content_type(), 'message/rfc822')
        self.assertTrue(isinstance(msg3, Message))
        payload = msg3.get_payload()
        unless(isinstance(payload, list))
        eq(len(payload), 1)
        msg4 = payload[0]
        unless(isinstance(msg4, Message))
        eq(msg4.get_payload(), 'Yadda yadda yadda\n')

    def test_parser(self):
        eq = self.assertEquals
        unless = self.assertTrue
        msg, text = self._msgobj('msg_06.txt')
        # Check some of the outer headers
        eq(msg.get_content_type(), 'message/rfc822')
        # Make sure the payload is a list of exactly one sub-Message, and that
        # that submessage has a type of text/plain
        payload = msg.get_payload()
        unless(isinstance(payload, list))
        eq(len(payload), 1)
        msg1 = payload[0]
        self.assertTrue(isinstance(msg1, Message))
        eq(msg1.get_content_type(), 'text/plain')
        self.assertTrue(isinstance(msg1.get_payload(), str))
        eq(msg1.get_payload(), '\n')



# Test various other bits of the package's functionality
class TestMiscellaneous(TestEmailBase):
    def test_message_from_string(self):
        with openfile('msg_01.txt') as fp:
            text = fp.read()
        msg = email.message_from_string(text)
        s = StringIO()
        # Don't wrap/continue long headers since we're trying to test
        # idempotency.
        g = Generator(s, maxheaderlen=0)
        g.flatten(msg)
        self.assertEqual(text, s.getvalue())

    def test_message_from_file(self):
        with openfile('msg_01.txt') as fp:
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
        unless = self.assertTrue
        with openfile('msg_01.txt') as fp:
            text = fp.read()

        # Create a subclass
        class MyMessage(Message):
            pass

        msg = email.message_from_string(text, MyMessage)
        unless(isinstance(msg, MyMessage))
        # Try something more complicated
        with openfile('msg_02.txt') as fp:
            text = fp.read()
        msg = email.message_from_string(text, MyMessage)
        for subpart in msg.walk():
            unless(isinstance(subpart, MyMessage))

    def test_message_from_file_with_class(self):
        unless = self.assertTrue
        # Create a subclass
        class MyMessage(Message):
            pass

        with openfile('msg_01.txt') as fp:
            msg = email.message_from_file(fp, MyMessage)
        unless(isinstance(msg, MyMessage))
        # Try something more complicated
        with openfile('msg_02.txt') as fp:
            msg = email.message_from_file(fp, MyMessage)
        for subpart in msg.walk():
            unless(isinstance(subpart, MyMessage))

    def test__all__(self):
        module = __import__('email')
        # Can't use sorted() here due to Python 2.3 compatibility
        all = module.__all__[:]
        all.sort()
        self.assertEqual(all, [
            'base64mime', 'charset', 'encoders', 'errors', 'generator',
            'header', 'iterators', 'message', 'message_from_binary_file',
            'message_from_bytes', 'message_from_file',
            'message_from_string', 'mime', 'parser',
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

    def test_parsedate_none(self):
        self.assertEqual(utils.parsedate(''), None)

    def test_parsedate_compact(self):
        # The FWS after the comma is optional
        self.assertEqual(utils.parsedate('Wed,3 Apr 2002 14:58:26 +0800'),
                         utils.parsedate('Wed, 3 Apr 2002 14:58:26 +0800'))

    def test_parsedate_no_dayofweek(self):
        eq = self.assertEqual
        eq(utils.parsedate_tz('25 Feb 2003 13:47:26 -0800'),
           (2003, 2, 25, 13, 47, 26, 0, 1, -1, -28800))

    def test_parsedate_compact_no_dayofweek(self):
        eq = self.assertEqual
        eq(utils.parsedate_tz('5 Feb 2003 13:47:26 -0800'),
           (2003, 2, 5, 13, 47, 26, 0, 1, -1, -28800))

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

    def test_noquote_dump(self):
        self.assertEqual(
            utils.formataddr(('A Silly Person', 'person@dom.ain')),
            'A Silly Person <person@dom.ain>')

    def test_escape_dump(self):
        self.assertEqual(
            utils.formataddr(('A (Very) Silly Person', 'person@dom.ain')),
            r'"A \(Very\) Silly Person" <person@dom.ain>')
        a = r'A \(Special\) Person'
        b = 'person@dom.ain'
        self.assertEqual(utils.parseaddr(utils.formataddr((a, b))), (a, b))

    def test_escape_backslashes(self):
        self.assertEqual(
            utils.formataddr(('Arthur \Backslash\ Foobar', 'person@dom.ain')),
            r'"Arthur \\Backslash\\ Foobar" <person@dom.ain>')
        a = r'Arthur \Backslash\ Foobar'
        b = 'person@dom.ain'
        self.assertEqual(utils.parseaddr(utils.formataddr((a, b))), (a, b))

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

    def test_getaddresses_nasty(self):
        eq = self.assertEqual
        eq(utils.getaddresses(['foo: ;']), [('', '')])
        eq(utils.getaddresses(
           ['[]*-- =~$']),
           [('', ''), ('', ''), ('', '*--')])
        eq(utils.getaddresses(
           ['foo: ;', '"Jason R. Mastaler" <jason@dom.ain>']),
           [('', ''), ('Jason R. Mastaler', 'jason@dom.ain')])

    def test_getaddresses_embedded_comment(self):
        """Test proper handling of a nested comment"""
        eq = self.assertEqual
        addrs = utils.getaddresses(['User ((nested comment)) <foo@bar.com>'])
        eq(addrs[0][1], 'foo@bar.com')

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
        with openfile('msg_19.txt') as fp:
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
            while True:
                ol = bsf.readline()
                if ol == NeedMoreData:
                    break
                om.append(ol)
                n1 += 1
            self.assertTrue(n == n1)
        self.assertTrue(len(om) == nt)
        self.assertTrue(''.join([il for il, n in imt]) == ''.join(om))



class TestParsers(TestEmailBase):
    def test_header_parser(self):
        eq = self.assertEqual
        # Parse only the headers of a complex multipart MIME document
        with openfile('msg_02.txt') as fp:
            msg = HeaderParser().parse(fp)
        eq(msg['from'], 'ppp-request@zzz.org')
        eq(msg['to'], 'ppp@zzz.org')
        eq(msg.get_content_type(), 'multipart/mixed')
        self.assertFalse(msg.is_multipart())
        self.assertTrue(isinstance(msg.get_payload(), str))

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
        with openfile('msg_26.txt', newline='\n') as fp:
            msg = Parser().parse(fp)
        eq(len(msg.get_payload()), 2)
        part1 = msg.get_payload(0)
        eq(part1.get_content_type(), 'text/plain')
        eq(part1.get_payload(), 'Simple email with attachment.\r\n\r\n')
        part2 = msg.get_payload(1)
        eq(part2.get_content_type(), 'application/riscos')

    def test_crlf_flatten(self):
        # Using newline='\n' preserves the crlfs in this input file.
        with openfile('msg_26.txt', newline='\n') as fp:
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
        with openfile('msg_28.txt') as fp:
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


class Test8BitBytesHandling(unittest.TestCase):
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
        self.assertEqual(msg.get_payload(), "p��stal\n")
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
        self.assertEqual(msg.get_payload(), 'p=C3=B6st��l\n')
        self.assertEqual(msg.get_payload(decode=True),
                        'pöstál\n'.encode('utf-8'))

    def test_8bit_in_base64_body(self):
        # Sticking an 8bit byte in a base64 block makes it undecodable by
        # normal means, so the block is returned undecoded, but as bytes.
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='base64',
                                     bodyline='cMO2c3RhbAá=').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(decode=True),
                         'cMO2c3RhbAá=\n'.encode('utf-8'))

    def test_8bit_in_uuencode_body(self):
        # Sticking an 8bit byte in a uuencode block makes it undecodable by
        # normal means, so the block is returned undecoded, but as bytes.
        m = self.bodytest_msg.format(charset='utf-8',
                                     cte='uuencode',
                                     bodyline='<,.V<W1A; á ').encode('utf-8')
        msg = email.message_from_bytes(m)
        self.assertEqual(msg.get_payload(decode=True),
                         '<,.V<W1A; á \n'.encode('utf-8'))


    headertest_msg = textwrap.dedent("""\
        From: foo@bar.com
        To: báz
        Subject: Maintenant je vous présente mon collègue, le pouf célèbre
        \tJean de Baddie
        From: göst

        Yes, they are flying.
        """).encode('utf-8')

    def test_get_8bit_header(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertEqual(msg.get('to'), 'b??z')
        self.assertEqual(msg['to'], 'b??z')

    def test_print_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertEqual(str(msg),
                         self.headertest_msg.decode(
                            'ascii', 'replace').replace('�', '?'))

    def test_values_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual(msg.values(),
                              ['foo@bar.com',
                               'b??z',
                               'Maintenant je vous pr??sente mon '
                                   'coll??gue, le pouf c??l??bre\n'
                                   '\tJean de Baddie',
                               "g??st"])

    def test_items_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual(msg.items(),
                              [('From', 'foo@bar.com'),
                               ('To', 'b??z'),
                               ('Subject', 'Maintenant je vous pr??sente mon '
                                              'coll??gue, le pouf c??l??bre\n'
                                              '\tJean de Baddie'),
                               ('From', 'g??st')])

    def test_get_all_with_8bit_headers(self):
        msg = email.message_from_bytes(self.headertest_msg)
        self.assertListEqual(msg.get_all('from'),
                              ['foo@bar.com',
                               'g??st'])

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

    # XXX: ultimately the '?' should turn into CTE encoded bytes
    # using 'unknown-8bit' charset.
    non_latin_bin_msg_as7bit = textwrap.dedent("""\
        From: foo@bar.com
        To: b??z
        Subject: Maintenant je vous pr??sente mon coll??gue, le pouf c??l??bre
        \tJean de Baddie
        Mime-Version: 1.0
        Content-Type: text/plain; charset="utf-8"
        Content-Transfer-Encoding: base64

        0JTQsCwg0L7QvdC4INC70LXRgtGP0YIuCg==
        """)

    def test_generator_handles_8bit(self):
        msg = email.message_from_bytes(self.non_latin_bin_msg)
        out = StringIO()
        email.generator.Generator(out).flatten(msg)
        self.assertEqual(out.getvalue(), self.non_latin_bin_msg_as7bit)

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
    maxDiff = None


class TestBytesGeneratorIdempotent(TestIdempotent):

    maxDiff = None

    def _msgobj(self, filename):
        with openfile(filename, 'rb') as fp:
            data = fp.read()
        msg = email.message_from_bytes(data)
        return msg, data

    def _idempotent(self, msg, data):
        # 13 = b'\r'
        linesep = '\r\n' if data[data.index(b'\n')-1] == 13 else '\n'
        b = BytesIO()
        g = email.generator.BytesGenerator(b, maxheaderlen=0)
        g.flatten(msg, linesep=linesep)
        self.assertByteStringsEqual(data, b.getvalue())

    def assertByteStringsEqual(self, str1, str2):
        self.assertListEqual(str1.split(b'\n'), str2.split(b'\n'))



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
        eq(base64mime.body_encode(b''), b'')
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

    def test_header_encode(self):
        eq = self.assertEqual
        he = quoprimime.header_encode
        eq(he(b'hello'), '=?iso-8859-1?q?hello?=')
        eq(he(b'hello', charset='iso-8859-2'), '=?iso-8859-2?q?hello?=')
        eq(he(b'hello\nworld'), '=?iso-8859-1?q?hello=0Aworld?=')
        # Test a non-ASCII character
        eq(he(b'hello\xc7there'), '=?iso-8859-1?q?hello=C7there?=')

    def test_decode(self):
        eq = self.assertEqual
        eq(quoprimime.decode(''), '')
        eq(quoprimime.decode('hello'), 'hello')
        eq(quoprimime.decode('hello', 'X'), 'hello')
        eq(quoprimime.decode('hello\nworld', 'X'), 'helloXworld')

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
        CharsetModule.add_charset('fake', CharsetModule.QP, None)
        c = Charset('fake')
        eq('hello w\xf6rld', c.body_encode('hello w\xf6rld'))

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
            self.assertTrue(len(l) <= 76)

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

    def test_encoded_adjacent_nonencoded(self):
        eq = self.assertEqual
        h = Header()
        h.append('hello', 'iso-8859-1')
        h.append('world')
        s = h.encode()
        eq(s, '=?iso-8859-1?q?hello?= world')
        h = make_header(decode_header(s))
        eq(h.encode(), s)

    def test_whitespace_eater(self):
        eq = self.assertEqual
        s = 'Subject: =?koi8-r?b?8NLP18XSy8EgzsEgxsnOwczYztk=?= =?koi8-r?q?=CA?= zz.'
        parts = decode_header(s)
        eq(parts, [(b'Subject:', None), (b'\xf0\xd2\xcf\xd7\xc5\xd2\xcb\xc1 \xce\xc1 \xc6\xc9\xce\xc1\xcc\xd8\xce\xd9\xca', 'koi8-r'), (b'zz.', None)])
        hdr = make_header(parts)
        eq(hdr.encode(),
           'Subject: =?koi8-r?b?8NLP18XSy8EgzsEgxsnOwczYztnK?= zz.')

    def test_broken_base64_header(self):
        raises = self.assertRaises
        s = 'Subject: =?EUC-KR?B?CSixpLDtKSC/7Liuvsax4iC6uLmwMcijIKHaILzSwd/H0SC8+LCjwLsgv7W/+Mj3I ?='
        raises(errors.HeaderParseError, decode_header, s)



# Test RFC 2231 header parameters (en/de)coding
class TestRFC2231(TestEmailBase):
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
 title*="us-ascii'en'This%20is%20even%20more%20%2A%2A%2Afun%2A%2A%2A%20isn%27t%20it%21"


Hi,

Do you like this message?

-Me
""")

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
 title*="us-ascii'en'This%20is%20even%20more%20%2A%2A%2Afun%2A%2A%2A%20isn%27t%20it%21"


Hi,

Do you like this message?

-Me
""")

    def test_rfc2231_get_content_charset(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_32.txt')
        eq(msg.get_content_charset(), 'us-ascii')

    def test_rfc2231_no_language_or_charset(self):
        m = '''\
Content-Transfer-Encoding: 8bit
Content-Disposition: inline; filename="file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEMP_nsmail.htm"
Content-Type: text/html; NAME*0=file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEM; NAME*1=P_nsmail.htm

'''
        msg = email.message_from_string(m)
        param = msg.get_param('NAME')
        self.assertFalse(isinstance(param, tuple))
        self.assertEqual(
            param,
            'file____C__DOCUMENTS_20AND_20SETTINGS_FABIEN_LOCAL_20SETTINGS_TEMP_nsmail.htm')

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

    def test_rfc2231_single_tick_in_filename(self):
        m = """\
Content-Type: application/x-foo; name*0=\"Frank's\"; name*1=\" Document\"

"""
        msg = email.message_from_string(m)
        param = msg.get_param('name')
        self.assertFalse(isinstance(param, tuple))
        self.assertEqual(param, "Frank's Document")

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

    def test_rfc2231_tick_attack(self):
        m = """\
Content-Type: application/x-foo;
\tname*0=\"us-ascii'en-us'Frank's\"; name*1=\" Document\"

"""
        msg = email.message_from_string(m)
        param = msg.get_param('name')
        self.assertFalse(isinstance(param, tuple))
        self.assertEqual(param, "us-ascii'en-us'Frank's Document")

    def test_rfc2231_no_extended_values(self):
        eq = self.assertEqual
        m = """\
Content-Type: application/x-foo; name=\"Frank's Document\"

"""
        msg = email.message_from_string(m)
        eq(msg.get_param('name'), "Frank's Document")

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



# Tests to ensure that signed parts of an email are completely preserved, as
# required by RFC1847 section 2.1.  Note that these are incomplete, because the
# email package does not currently always preserve the body.  See issue 1670765.
class TestSigned(TestEmailBase):

    def _msg_and_obj(self, filename):
        with openfile(findfile(filename)) as fp:
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



def _testclasses():
    mod = sys.modules[__name__]
    return [getattr(mod, name) for name in dir(mod) if name.startswith('Test')]


def suite():
    suite = unittest.TestSuite()
    for testclass in _testclasses():
        suite.addTest(unittest.makeSuite(testclass))
    return suite


def test_main():
    for testclass in _testclasses():
        run_unittest(testclass)



if __name__ == '__main__':
    unittest.main(defaultTest='suite')
