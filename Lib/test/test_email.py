# Copyright (C) 2001 Python Software Foundation
# email package unit tests

import os
import time
import unittest
import base64
from cStringIO import StringIO
from types import StringType

import email

from email.Parser import Parser, HeaderParser
from email.Generator import Generator, DecodedGenerator
from email.Message import Message
from email.MIMEAudio import MIMEAudio
from email.MIMEText import MIMEText
from email.MIMEImage import MIMEImage
from email.MIMEBase import MIMEBase
from email.MIMEMessage import MIMEMessage
from email import Utils
from email import Errors
from email import Encoders
from email import Iterators

import test_email
from test_support import findfile


NL = '\n'
EMPTYSTRING = ''
SPACE = ' '



def openfile(filename):
    path = os.path.join(os.path.dirname(test_email.__file__), 'data', filename)
    return open(path)



# Base test class
class TestEmailBase(unittest.TestCase):
    def _msgobj(self, filename):
        fp = openfile(filename)
        try:
            msg = email.message_from_file(fp)
        finally:
            fp.close()
        return msg



# Test various aspects of the Message class's API
class TestMessageAPI(TestEmailBase):
    def test_get_all(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_20.txt')
        eq(msg.get_all('cc'), ['ccc@zzz.org', 'ddd@zzz.org', 'eee@zzz.org'])
        eq(msg.get_all('xx', 'n/a'), 'n/a')

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
        eq(value, 'text/plain; charset=us-ascii; boundary="BOUNDARY"')
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
        self.assertRaises(Errors.HeaderParseError,
                          msg.set_boundary, 'BOUNDARY')

    def test_get_decoded_payload(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_10.txt')
        # The outer message is a multipart
        eq(msg.get_payload(decode=1), None)
        # Subpart 1 is 7bit encoded
        eq(msg.get_payload(0).get_payload(decode=1),
           'This is a 7bit encoded message.\n')
        # Subpart 2 is quopri
        eq(msg.get_payload(1).get_payload(decode=1),
           '\xa1This is a Quoted Printable encoded message!\n')
        # Subpart 3 is base64
        eq(msg.get_payload(2).get_payload(decode=1),
           'This is a Base64 encoded message.')
        # Subpart 4 has no Content-Transfer-Encoding: header.
        eq(msg.get_payload(3).get_payload(decode=1),
           'This has no Content-Transfer-Encoding: header.\n')

    def test_decoded_generator(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_07.txt')
        fp = openfile('msg_17.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        s = StringIO()
        g = DecodedGenerator(s)
        g(msg)
        eq(s.getvalue(), text)

    def test__contains__(self):
        msg = Message()
        msg['From'] = 'Me'
        msg['to'] = 'You'
        # Check for case insensitivity
        self.failUnless('from' in msg)
        self.failUnless('From' in msg)
        self.failUnless('FROM' in msg)
        self.failUnless('to' in msg)
        self.failUnless('To' in msg)
        self.failUnless('TO' in msg)

    def test_as_string(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_01.txt')
        fp = openfile('msg_01.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        eq(text, msg.as_string())
        fullrepr = str(msg)
        lines = fullrepr.split('\n')
        self.failUnless(lines[0].startswith('From '))
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

    def test_get_param_funky_continuation_lines(self):
        msg = self._msgobj('msg_22.txt')
        self.assertEqual(msg.get_payload(1).get_param('name'), 'wibble.JPG')

    def test_has_key(self):
        msg = email.message_from_string('Header: exists')
        self.failUnless(msg.has_key('header'))
        self.failUnless(msg.has_key('Header'))
        self.failUnless(msg.has_key('HEADER'))
        self.failIf(msg.has_key('headeri'))



# Test the email.Encoders module
class TestEncoders(unittest.TestCase):
    def test_encode_noop(self):
        eq = self.assertEqual
        msg = MIMEText('hello world', _encoder=Encoders.encode_noop)
        eq(msg.get_payload(), 'hello world\n')
        eq(msg['content-transfer-encoding'], None)

    def test_encode_7bit(self):
        eq = self.assertEqual
        msg = MIMEText('hello world', _encoder=Encoders.encode_7or8bit)
        eq(msg.get_payload(), 'hello world\n')
        eq(msg['content-transfer-encoding'], '7bit')
        msg = MIMEText('hello \x7f world', _encoder=Encoders.encode_7or8bit)
        eq(msg.get_payload(), 'hello \x7f world\n')
        eq(msg['content-transfer-encoding'], '7bit')

    def test_encode_8bit(self):
        eq = self.assertEqual
        msg = MIMEText('hello \x80 world', _encoder=Encoders.encode_7or8bit)
        eq(msg.get_payload(), 'hello \x80 world\n')
        eq(msg['content-transfer-encoding'], '8bit')

    def test_encode_base64(self):
        eq = self.assertEqual
        msg = MIMEText('hello world', _encoder=Encoders.encode_base64)
        eq(msg.get_payload(), 'aGVsbG8gd29ybGQK\n')
        eq(msg['content-transfer-encoding'], 'base64')

    def test_encode_quoted_printable(self):
        eq = self.assertEqual
        msg = MIMEText('hello world', _encoder=Encoders.encode_quopri)
        eq(msg.get_payload(), 'hello=20world\n')
        eq(msg['content-transfer-encoding'], 'quoted-printable')



# Test long header wrapping
class TestLongHeaders(unittest.TestCase):
    def test_header_splitter(self):
        msg = MIMEText('')
        # It'd be great if we could use add_header() here, but that doesn't
        # guarantee an order of the parameters.
        msg['X-Foobar-Spoink-Defrobnit'] = (
            'wasnipoop; giraffes="very-long-necked-animals"; '
            'spooge="yummy"; hippos="gargantuan"; marshmallows="gooey"')
        sfp = StringIO()
        g = Generator(sfp)
        g(msg)
        self.assertEqual(sfp.getvalue(), openfile('msg_18.txt').read())

    def test_no_semis_header_splitter(self):
        msg = Message()
        msg['From'] = 'test@dom.ain'
        refparts = []
        for i in range(10):
            refparts.append('<%d@dom.ain>' % i)
        msg['References'] = SPACE.join(refparts)
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g(msg)
        self.assertEqual(sfp.getvalue(), """\
From: test@dom.ain
References: <0@dom.ain> <1@dom.ain> <2@dom.ain> <3@dom.ain> <4@dom.ain>
\t<5@dom.ain> <6@dom.ain> <7@dom.ain> <8@dom.ain> <9@dom.ain>

Test""")

    def test_no_split_long_header(self):
        msg = Message()
        msg['From'] = 'test@dom.ain'
        refparts = []
        msg['References'] = 'x' * 80
        msg.set_payload('Test')
        sfp = StringIO()
        g = Generator(sfp)
        g(msg)
        self.assertEqual(sfp.getvalue(), """\
From: test@dom.ain
References: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

Test""")



# Test mangling of "From " lines in the body of a message
class TestFromMangling(unittest.TestCase):
    def setUp(self):
        self.msg = Message()
        self.msg['From'] = 'aaa@bbb.org'
        self.msg.add_payload("""\
From the desk of A.A.A.:
Blah blah blah
""")

    def test_mangled_from(self):
        s = StringIO()
        g = Generator(s, mangle_from_=1)
        g(self.msg)
        self.assertEqual(s.getvalue(), """\
From: aaa@bbb.org

>From the desk of A.A.A.:
Blah blah blah
""")

    def test_dont_mangle_from(self):
        s = StringIO()
        g = Generator(s, mangle_from_=0)
        g(self.msg)
        self.assertEqual(s.getvalue(), """\
From: aaa@bbb.org

From the desk of A.A.A.:
Blah blah blah
""")



# Test the basic MIMEAudio class
class TestMIMEAudio(unittest.TestCase):
    def setUp(self):
        # In Python, audiotest.au lives in Lib/test not Lib/test/data
        fp = open(findfile('audiotest.au'))
        try:
            self._audiodata = fp.read()
        finally:
            fp.close()
        self._au = MIMEAudio(self._audiodata)

    def test_guess_minor_type(self):
        self.assertEqual(self._au.get_type(), 'audio/basic')

    def test_encoding(self):
        payload = self._au.get_payload()
        self.assertEqual(base64.decodestring(payload), self._audiodata)

    def checkSetMinor(self):
        au = MIMEAudio(self._audiodata, 'fish')
        self.assertEqual(im.get_type(), 'audio/fish')

    def test_custom_encoder(self):
        eq = self.assertEqual
        def encoder(msg):
            orig = msg.get_payload()
            msg.set_payload(0)
            msg['Content-Transfer-Encoding'] = 'broken64'
        au = MIMEAudio(self._audiodata, _encoder=encoder)
        eq(au.get_payload(), 0)
        eq(au['content-transfer-encoding'], 'broken64')

    def test_add_header(self):
        eq = self.assertEqual
        unless = self.failUnless
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
        fp = openfile('PyBanner048.gif')
        try:
            self._imgdata = fp.read()
        finally:
            fp.close()
        self._im = MIMEImage(self._imgdata)

    def test_guess_minor_type(self):
        self.assertEqual(self._im.get_type(), 'image/gif')

    def test_encoding(self):
        payload = self._im.get_payload()
        self.assertEqual(base64.decodestring(payload), self._imgdata)

    def checkSetMinor(self):
        im = MIMEImage(self._imgdata, 'fish')
        self.assertEqual(im.get_type(), 'image/fish')

    def test_custom_encoder(self):
        eq = self.assertEqual
        def encoder(msg):
            orig = msg.get_payload()
            msg.set_payload(0)
            msg['Content-Transfer-Encoding'] = 'broken64'
        im = MIMEImage(self._imgdata, _encoder=encoder)
        eq(im.get_payload(), 0)
        eq(im['content-transfer-encoding'], 'broken64')

    def test_add_header(self):
        eq = self.assertEqual
        unless = self.failUnless
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



# Test the basic MIMEText class
class TestMIMEText(unittest.TestCase):
    def setUp(self):
        self._msg = MIMEText('hello there')

    def test_types(self):
        eq = self.assertEqual
        unless = self.failUnless
        eq(self._msg.get_type(), 'text/plain')
        eq(self._msg.get_param('charset'), 'us-ascii')
        missing = []
        unless(self._msg.get_param('foobar', missing) is missing)
        unless(self._msg.get_param('charset', missing, header='foobar')
               is missing)

    def test_payload(self):
        self.assertEqual(self._msg.get_payload(), 'hello there\n')
        self.failUnless(not self._msg.is_multipart())



# Test a more complicated multipart/mixed type message
class TestMultipartMixed(unittest.TestCase):
    def setUp(self):
        fp = openfile('PyBanner048.gif')
        try:
            data = fp.read()
        finally:
            fp.close()

        container = MIMEBase('multipart', 'mixed', boundary='BOUNDARY')
        image = MIMEImage(data, name='dingusfish.gif')
        image.add_header('content-disposition', 'attachment',
                         filename='dingusfish.gif')
        intro = MIMEText('''\
Hi there,

This is the dingus fish.
''')
        container.add_payload(intro)
        container.add_payload(image)
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
        unless = self.failUnless
        raises = self.assertRaises
        # tests
        m = self._msg
        unless(m.is_multipart())
        eq(m.get_type(), 'multipart/mixed')
        eq(len(m.get_payload()), 2)
        raises(IndexError, m.get_payload, 2)
        m0 = m.get_payload(0)
        m1 = m.get_payload(1)
        unless(m0 is self._txt)
        unless(m1 is self._im)
        eq(m.get_payload(), [m0, m1])
        unless(not m0.is_multipart())
        unless(not m1.is_multipart())



# Test some badly formatted messages
class TestNonConformant(TestEmailBase):
    def test_parse_missing_minor_type(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_14.txt')
        eq(msg.get_type(), 'text')
        eq(msg.get_main_type(), 'text')
        self.failUnless(msg.get_subtype() is None)

    def test_bogus_boundary(self):
        fp = openfile('msg_15.txt')
        try:
            data = fp.read()
        finally:
            fp.close()
        p = Parser()
        # Note, under a future non-strict parsing mode, this would parse the
        # message into the intended message tree.
        self.assertRaises(Errors.BoundaryError, p.parsestr, data)



# Test RFC 2047 header encoding and decoding
class TestRFC2047(unittest.TestCase):
    def test_iso_8859_1(self):
        eq = self.assertEqual
        s = '=?iso-8859-1?q?this=20is=20some=20text?='
        eq(Utils.decode(s), 'this is some text')
        s = '=?ISO-8859-1?Q?Keld_J=F8rn_Simonsen?='
        eq(Utils.decode(s), u'Keld_J\xf8rn_Simonsen')
        s = '=?ISO-8859-1?B?SWYgeW91IGNhbiByZWFkIHRoaXMgeW8=?=' \
            '=?ISO-8859-2?B?dSB1bmRlcnN0YW5kIHRoZSBleGFtcGxlLg==?='
        eq(Utils.decode(s), 'If you can read this you understand the example.')
        s = '=?iso-8859-8?b?7eXs+SDv4SDp7Oj08A==?='
        eq(Utils.decode(s),
           u'\u05dd\u05d5\u05dc\u05e9 \u05df\u05d1 \u05d9\u05dc\u05d8\u05e4\u05e0')
        s = '=?iso-8859-1?q?this=20is?= =?iso-8859-1?q?some=20text?='
        eq(Utils.decode(s), u'this is some text')

    def test_encode_header(self):
        eq = self.assertEqual
        s = 'this is some text'
        eq(Utils.encode(s), '=?iso-8859-1?q?this=20is=20some=20text?=')
        s = 'Keld_J\xf8rn_Simonsen'
        eq(Utils.encode(s), '=?iso-8859-1?q?Keld_J=F8rn_Simonsen?=')
        s1 = 'If you can read this yo'
        s2 = 'u understand the example.'
        eq(Utils.encode(s1, encoding='b'),
           '=?iso-8859-1?b?SWYgeW91IGNhbiByZWFkIHRoaXMgeW8=?=')
        eq(Utils.encode(s2, charset='iso-8859-2', encoding='b'),
           '=?iso-8859-2?b?dSB1bmRlcnN0YW5kIHRoZSBleGFtcGxlLg==?=')



# Test the MIMEMessage class
class TestMIMEMessage(TestEmailBase):
    def setUp(self):
        fp = openfile('msg_11.txt')
        self._text = fp.read()
        fp.close()

    def test_type_error(self):
        self.assertRaises(TypeError, MIMEMessage, 'a plain string')

    def test_valid_argument(self):
        eq = self.assertEqual
        subject = 'A sub-message'
        m = Message()
        m['Subject'] = subject
        r = MIMEMessage(m)
        eq(r.get_type(), 'message/rfc822')
        self.failUnless(r.get_payload() is m)
        eq(r.get_payload()['subject'], subject)

    def test_generate(self):
        # First craft the message to be encapsulated
        m = Message()
        m['Subject'] = 'An enclosed message'
        m.add_payload('Here is the body of the message.\n')
        r = MIMEMessage(m)
        r['Subject'] = 'The enclosing message'
        s = StringIO()
        g = Generator(s)
        g(r)
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
        eq(msg.get_type(), 'message/rfc822')
        eq(len(msg.get_payload()), 1)
        submsg = msg.get_payload()
        self.failUnless(isinstance(submsg, Message))
        eq(submsg['subject'], 'An enclosed message')
        eq(submsg.get_payload(), 'Here is the body of the message.\n')

    def test_dsn(self):
        eq = self.assertEqual
        unless = self.failUnless
        # msg 16 is a Delivery Status Notification, see RFC XXXX
        msg = self._msgobj('msg_16.txt')
        eq(msg.get_type(), 'multipart/report')
        unless(msg.is_multipart())
        eq(len(msg.get_payload()), 3)
        # Subpart 1 is a text/plain, human readable section
        subpart = msg.get_payload(0)
        eq(subpart.get_type(), 'text/plain')
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
        eq(subpart.get_type(), 'message/delivery-status')
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
        eq(subpart.get_type(), 'message/rfc822')
        subsubpart = subpart.get_payload()
        unless(isinstance(subsubpart, Message))
        eq(subsubpart.get_type(), 'text/plain')
        eq(subsubpart['message-id'],
           '<002001c144a6$8752e060$56104586@oxy.edu>')

    def test_epilogue(self):
        fp = openfile('msg_21.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        msg = Message()
        msg['From'] = 'aperson@dom.ain'
        msg['To'] = 'bperson@dom.ain'
        msg['Subject'] = 'Test'
        msg.preamble = 'MIME message\n'
        msg.epilogue = 'End of MIME message\n'
        msg1 = MIMEText('One')
        msg2 = MIMEText('Two')
        msg.add_header('Content-Type', 'multipart/mixed', boundary='BOUNDARY')
        msg.add_payload(msg1)
        msg.add_payload(msg2)
        sfp = StringIO()
        g = Generator(sfp)
        g(msg)
        self.assertEqual(sfp.getvalue(), text)



# A general test of parser->model->generator idempotency.  IOW, read a message
# in, parse it into a message object tree, then without touching the tree,
# regenerate the plain text.  The original text and the transformed text
# should be identical.  Note: that we ignore the Unix-From since that may
# contain a changed date.
class TestIdempotent(unittest.TestCase):
    def _msgobj(self, filename):
        fp = openfile(filename)
        try:
            data = fp.read()
        finally:
            fp.close()
        msg = email.message_from_string(data)
        return msg, data

    def _idempotent(self, msg, text):
        eq = self.assertEquals
        s = StringIO()
        g = Generator(s, maxheaderlen=0)
        g(msg)
        eq(text, s.getvalue())

    def test_parse_text_message(self):
        eq = self.assertEquals
        msg, text = self._msgobj('msg_01.txt')
        eq(msg.get_type(), 'text/plain')
        eq(msg.get_main_type(), 'text')
        eq(msg.get_subtype(), 'plain')
        eq(msg.get_params()[1], ('charset', 'us-ascii'))
        eq(msg.get_param('charset'), 'us-ascii')
        eq(msg.preamble, None)
        eq(msg.epilogue, None)
        self._idempotent(msg, text)

    def test_parse_untyped_message(self):
        eq = self.assertEquals
        msg, text = self._msgobj('msg_03.txt')
        eq(msg.get_type(), None)
        eq(msg.get_params(), None)
        eq(msg.get_param('charset'), None)
        self._idempotent(msg, text)

    def test_simple_multipart(self):
        msg, text = self._msgobj('msg_04.txt')
        self._idempotent(msg, text)

    def test_MIME_digest(self):
        msg, text = self._msgobj('msg_02.txt')
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

    def test_content_type(self):
        eq = self.assertEquals
        # Get a message object and reset the seek pointer for other tests
        msg, text = self._msgobj('msg_05.txt')
        eq(msg.get_type(), 'multipart/report')
        # Test the Content-Type: parameters
        params = {}
        for pk, pv in msg.get_params():
            params[pk] = pv
        eq(params['report-type'], 'delivery-status')
        eq(params['boundary'], 'D1690A7AC1.996856090/mail.example.com')
        eq(msg.preamble, 'This is a MIME-encapsulated message.\n\n')
        eq(msg.epilogue, '\n\n')
        eq(len(msg.get_payload()), 3)
        # Make sure the subparts are what we expect
        msg1 = msg.get_payload(0)
        eq(msg1.get_type(), 'text/plain')
        eq(msg1.get_payload(), 'Yadda yadda yadda\n')
        msg2 = msg.get_payload(1)
        eq(msg2.get_type(), None)
        eq(msg2.get_payload(), 'Yadda yadda yadda\n')
        msg3 = msg.get_payload(2)
        eq(msg3.get_type(), 'message/rfc822')
        self.failUnless(isinstance(msg3, Message))
        msg4 = msg3.get_payload()
        self.failUnless(isinstance(msg4, Message))
        eq(msg4.get_payload(), 'Yadda yadda yadda\n')

    def test_parser(self):
        eq = self.assertEquals
        msg, text = self._msgobj('msg_06.txt')
        # Check some of the outer headers
        eq(msg.get_type(), 'message/rfc822')
        # Make sure there's exactly one thing in the payload and that's a
        # sub-Message object of type text/plain
        msg1 = msg.get_payload()
        self.failUnless(isinstance(msg1, Message))
        eq(msg1.get_type(), 'text/plain')
        self.failUnless(isinstance(msg1.get_payload(), StringType))
        eq(msg1.get_payload(), '\n')



# Test various other bits of the package's functionality
class TestMiscellaneous(unittest.TestCase):
    def test_message_from_string(self):
        fp = openfile('msg_01.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        msg = email.message_from_string(text)
        s = StringIO()
        # Don't wrap/continue long headers since we're trying to test
        # idempotency.
        g = Generator(s, maxheaderlen=0)
        g(msg)
        self.assertEqual(text, s.getvalue())

    def test_message_from_file(self):
        fp = openfile('msg_01.txt')
        try:
            text = fp.read()
            fp.seek(0)
            msg = email.message_from_file(fp)
            s = StringIO()
            # Don't wrap/continue long headers since we're trying to test
            # idempotency.
            g = Generator(s, maxheaderlen=0)
            g(msg)
            self.assertEqual(text, s.getvalue())
        finally:
            fp.close()

    def test_message_from_string_with_class(self):
        unless = self.failUnless
        fp = openfile('msg_01.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        # Create a subclass
        class MyMessage(Message):
            pass

        msg = email.message_from_string(text, MyMessage)
        unless(isinstance(msg, MyMessage))
        # Try something more complicated
        fp = openfile('msg_02.txt')
        try:
            text = fp.read()
        finally:
            fp.close()
        msg = email.message_from_string(text, MyMessage)
        for subpart in msg.walk():
            unless(isinstance(subpart, MyMessage))

    def test_message_from_file_with_class(self):
        unless = self.failUnless
        # Create a subclass
        class MyMessage(Message):
            pass

        fp = openfile('msg_01.txt')
        try:
            msg = email.message_from_file(fp, MyMessage)
        finally:
            fp.close()
        unless(isinstance(msg, MyMessage))
        # Try something more complicated
        fp = openfile('msg_02.txt')
        try:
            msg = email.message_from_file(fp, MyMessage)
        finally:
            fp.close()
        for subpart in msg.walk():
            unless(isinstance(subpart, MyMessage))

    def test__all__(self):
        module = __import__('email')
        all = module.__all__
        all.sort()
        self.assertEqual(all, ['Encoders', 'Errors', 'Generator', 'Iterators',
                               'MIMEAudio', 'MIMEBase', 'MIMEImage',
                               'MIMEMessage', 'MIMEText', 'Message', 'Parser',
                               'Utils',
                               'message_from_file', 'message_from_string'])

    def test_formatdate(self):
        now = 1005327232.109884
        epoch = time.gmtime(0)[0]
        # When does the epoch start?
        if epoch == 1970:
            # traditional Unix epoch
            matchdate = 'Fri, 09 Nov 2001 17:33:52 -0000'
        elif epoch == 1904:
            # Mac epoch
            matchdate = 'Sat, 09 Nov 1935 16:33:52 -0000'
        else:
            matchdate = "I don't understand your epoch"
        gdate = Utils.formatdate(now)
        ldate = Utils.formatdate(now, localtime=1)
        self.assertEqual(gdate, matchdate)

    def test_formatdate_zoneoffsets(self):
        now = 1005327232.109884
        ldate = Utils.formatdate(now, localtime=1)
        zone = ldate.split()[5]
        offset = int(zone[:3]) * -3600 + int(zone[-2:]) * -60
        if time.daylight and time.localtime(now)[-1]:
            toff = time.altzone
        else:
            toff = time.timezone
        self.assertEqual(offset, toff)

    def test_parsedate_none(self):
        self.assertEqual(Utils.parsedate(''), None)



# Test the iterator/generators
class TestIterators(TestEmailBase):
    def test_body_line_iterator(self):
        eq = self.assertEqual
        # First a simple non-multipart message
        msg = self._msgobj('msg_01.txt')
        it = Iterators.body_line_iterator(msg)
        lines = list(it)
        eq(len(lines), 6)
        eq(EMPTYSTRING.join(lines), msg.get_payload())
        # Now a more complicated multipart
        msg = self._msgobj('msg_02.txt')
        it = Iterators.body_line_iterator(msg)
        lines = list(it)
        eq(len(lines), 43)
        eq(EMPTYSTRING.join(lines), openfile('msg_19.txt').read())

    def test_typed_subpart_iterator(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_04.txt')
        it = Iterators.typed_subpart_iterator(msg, 'text')
        lines = [subpart.get_payload() for subpart in it]
        eq(len(lines), 2)
        eq(EMPTYSTRING.join(lines), """\
a simple kind of mirror
to reflect upon our own
a simple kind of mirror
to reflect upon our own
""")

    def test_typed_subpart_iterator_default_type(self):
        eq = self.assertEqual
        msg = self._msgobj('msg_03.txt')
        it = Iterators.typed_subpart_iterator(msg, 'text', 'plain')
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


class TestParsers(unittest.TestCase):
    def test_header_parser(self):
        eq = self.assertEqual
        # Parse only the headers of a complex multipart MIME document
        p = HeaderParser()
        fp = openfile('msg_02.txt')
        msg = p.parse(fp)
        eq(msg['from'], 'ppp-request@zzz.org')
        eq(msg['to'], 'ppp@zzz.org')
        eq(msg.get_type(), 'multipart/mixed')
        eq(msg.is_multipart(), 0)
        self.failUnless(isinstance(msg.get_payload(), StringType))



def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestMessageAPI))
    suite.addTest(unittest.makeSuite(TestEncoders))
    suite.addTest(unittest.makeSuite(TestLongHeaders))
    suite.addTest(unittest.makeSuite(TestFromMangling))
    suite.addTest(unittest.makeSuite(TestMIMEAudio))
    suite.addTest(unittest.makeSuite(TestMIMEImage))
    suite.addTest(unittest.makeSuite(TestMIMEText))
    suite.addTest(unittest.makeSuite(TestMultipartMixed))
    suite.addTest(unittest.makeSuite(TestNonConformant))
    suite.addTest(unittest.makeSuite(TestRFC2047))
    suite.addTest(unittest.makeSuite(TestMIMEMessage))
    suite.addTest(unittest.makeSuite(TestIdempotent))
    suite.addTest(unittest.makeSuite(TestMiscellaneous))
    suite.addTest(unittest.makeSuite(TestIterators))
    suite.addTest(unittest.makeSuite(TestParsers))
    return suite



if __name__ == '__main__':
    unittest.main(defaultTest='suite')
else:
    from test_support import run_suite
    run_suite(suite())
