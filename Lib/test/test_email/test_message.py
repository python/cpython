import textwrap
import unittest
from email import message_from_bytes, message_from_string, policy
from email.message import EmailMessage, MIMEPart
from test.test_email import TestEmailBase, parameterize


# Helper.
def first(iterable):
    return next(filter(lambda x: x is not None, iterable), None)


class Test(TestEmailBase):

    policy = policy.default

    def test_error_on_setitem_if_max_count_exceeded(self):
        m = self._str_msg("")
        m['To'] = 'abc@xyz'
        with self.assertRaises(ValueError):
            m['To'] = 'xyz@abc'

    def test_rfc2043_auto_decoded_and_emailmessage_used(self):
        m = message_from_string(textwrap.dedent("""\
            Subject: Ayons asperges pour le =?utf-8?q?d=C3=A9jeuner?=
            From: =?utf-8?q?Pep=C3=A9?= Le Pew <pepe@example.com>
            To: "Penelope Pussycat" <"penelope@example.com">
            MIME-Version: 1.0
            Content-Type: text/plain; charset="utf-8"

            sample text
            """), policy=policy.default)
        self.assertEqual(m['subject'], "Ayons asperges pour le déjeuner")
        self.assertEqual(m['from'], "Pepé Le Pew <pepe@example.com>")
        self.assertIsInstance(m, EmailMessage)


@parameterize
class TestEmailMessageBase:

    policy = policy.default

    # The first argument is a triple (related, html, plain) of indices into the
    # list returned by 'walk' called on a Message constructed from the third.
    # The indices indicate which part should match the corresponding part-type
    # when passed to get_body (ie: the "first" part of that type in the
    # message).  The second argument is a list of indices into the 'walk' list
    # of the attachments that should be returned by a call to
    # 'iter_attachments'.  The third argument is a list of indices into 'walk'
    # that should be returned by a call to 'iter_parts'.  Note that the first
    # item returned by 'walk' is the Message itself.

    message_params = {

        'empty_message': (
            (None, None, 0),
            (),
            (),
            ""),

        'non_mime_plain': (
            (None, None, 0),
            (),
            (),
            textwrap.dedent("""\
                To: foo@example.com

                simple text body
                """)),

        'mime_non_text': (
            (None, None, None),
            (),
            (),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: image/jpg

                bogus body.
                """)),

        'plain_html_alternative': (
            (None, 2, 1),
            (),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/alternative; boundary="==="

                preamble

                --===
                Content-Type: text/plain

                simple body

                --===
                Content-Type: text/html

                <p>simple body</p>
                --===--
                """)),

        'plain_html_mixed': (
            (None, 2, 1),
            (),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                preamble

                --===
                Content-Type: text/plain

                simple body

                --===
                Content-Type: text/html

                <p>simple body</p>

                --===--
                """)),

        'plain_html_attachment_mixed': (
            (None, None, 1),
            (2,),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: text/plain

                simple body

                --===
                Content-Type: text/html
                Content-Disposition: attachment

                <p>simple body</p>

                --===--
                """)),

        'html_text_attachment_mixed': (
            (None, 2, None),
            (1,),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: text/plain
                Content-Disposition: AtTaChment

                simple body

                --===
                Content-Type: text/html

                <p>simple body</p>

                --===--
                """)),

        'html_text_attachment_inline_mixed': (
            (None, 2, 1),
            (),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: text/plain
                Content-Disposition: InLine

                simple body

                --===
                Content-Type: text/html
                Content-Disposition: inline

                <p>simple body</p>

                --===--
                """)),

        # RFC 2387
        'related': (
            (0, 1, None),
            (2,),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/related; boundary="==="; type=text/html

                --===
                Content-Type: text/html

                <p>simple body</p>

                --===
                Content-Type: image/jpg
                Content-ID: <image1>

                bogus data

                --===--
                """)),

        # This message structure will probably never be seen in the wild, but
        # it proves we distinguish between text parts based on 'start'.  The
        # content would not, of course, actually work :)
        'related_with_start': (
            (0, 2, None),
            (1,),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/related; boundary="==="; type=text/html;
                 start="<body>"

                --===
                Content-Type: text/html
                Content-ID: <include>

                useless text

                --===
                Content-Type: text/html
                Content-ID: <body>

                <p>simple body</p>
                <!--#include file="<include>"-->

                --===--
                """)),


        'mixed_alternative_plain_related': (
            (3, 4, 2),
            (6, 7),
            (1, 6, 7),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: multipart/alternative; boundary="+++"

                --+++
                Content-Type: text/plain

                simple body

                --+++
                Content-Type: multipart/related; boundary="___"

                --___
                Content-Type: text/html

                <p>simple body</p>

                --___
                Content-Type: image/jpg
                Content-ID: <image1@cid>

                bogus jpg body

                --___--

                --+++--

                --===
                Content-Type: image/jpg
                Content-Disposition: attachment

                bogus jpg body

                --===
                Content-Type: image/jpg
                Content-Disposition: AttacHmenT

                another bogus jpg body

                --===--
                """)),

        # This structure suggested by Stephen J. Turnbull...may not exist/be
        # supported in the wild, but we want to support it.
        'mixed_related_alternative_plain_html': (
            (1, 4, 3),
            (6, 7),
            (1, 6, 7),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: multipart/related; boundary="+++"

                --+++
                Content-Type: multipart/alternative; boundary="___"

                --___
                Content-Type: text/plain

                simple body

                --___
                Content-Type: text/html

                <p>simple body</p>

                --___--

                --+++
                Content-Type: image/jpg
                Content-ID: <image1@cid>

                bogus jpg body

                --+++--

                --===
                Content-Type: image/jpg
                Content-Disposition: attachment

                bogus jpg body

                --===
                Content-Type: image/jpg
                Content-Disposition: attachment

                another bogus jpg body

                --===--
                """)),

        # Same thing, but proving we only look at the root part, which is the
        # first one if there isn't any start parameter.  That is, this is a
        # broken related.
        'mixed_related_alternative_plain_html_wrong_order': (
            (1, None, None),
            (6, 7),
            (1, 6, 7),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: multipart/related; boundary="+++"

                --+++
                Content-Type: image/jpg
                Content-ID: <image1@cid>

                bogus jpg body

                --+++
                Content-Type: multipart/alternative; boundary="___"

                --___
                Content-Type: text/plain

                simple body

                --___
                Content-Type: text/html

                <p>simple body</p>

                --___--

                --+++--

                --===
                Content-Type: image/jpg
                Content-Disposition: attachment

                bogus jpg body

                --===
                Content-Type: image/jpg
                Content-Disposition: attachment

                another bogus jpg body

                --===--
                """)),

        'message_rfc822': (
            (None, None, None),
            (),
            (),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: message/rfc822

                To: bar@example.com
                From: robot@examp.com

                this is a message body.
                """)),

        'mixed_text_message_rfc822': (
            (None, None, 1),
            (2,),
            (1, 2),
            textwrap.dedent("""\
                To: foo@example.com
                MIME-Version: 1.0
                Content-Type: multipart/mixed; boundary="==="

                --===
                Content-Type: text/plain

                Your message has bounced, sir.

                --===
                Content-Type: message/rfc822

                To: bar@example.com
                From: robot@examp.com

                this is a message body.

                --===--
                """)),

         }

    def message_as_get_body(self, body_parts, attachments, parts, msg):
        m = self._str_msg(msg)
        allparts = list(m.walk())
        expected = [None if n is None else allparts[n] for n in body_parts]
        related = 0; html = 1; plain = 2
        self.assertEqual(m.get_body(), first(expected))
        self.assertEqual(m.get_body(preferencelist=(
                                        'related', 'html', 'plain')),
                         first(expected))
        self.assertEqual(m.get_body(preferencelist=('related', 'html')),
                         first(expected[related:html+1]))
        self.assertEqual(m.get_body(preferencelist=('related', 'plain')),
                         first([expected[related], expected[plain]]))
        self.assertEqual(m.get_body(preferencelist=('html', 'plain')),
                         first(expected[html:plain+1]))
        self.assertEqual(m.get_body(preferencelist=['related']),
                         expected[related])
        self.assertEqual(m.get_body(preferencelist=['html']), expected[html])
        self.assertEqual(m.get_body(preferencelist=['plain']), expected[plain])
        self.assertEqual(m.get_body(preferencelist=('plain', 'html')),
                         first(expected[plain:html-1:-1]))
        self.assertEqual(m.get_body(preferencelist=('plain', 'related')),
                         first([expected[plain], expected[related]]))
        self.assertEqual(m.get_body(preferencelist=('html', 'related')),
                         first(expected[html::-1]))
        self.assertEqual(m.get_body(preferencelist=('plain', 'html', 'related')),
                         first(expected[::-1]))
        self.assertEqual(m.get_body(preferencelist=('html', 'plain', 'related')),
                         first([expected[html],
                                expected[plain],
                                expected[related]]))

    def message_as_iter_attachment(self, body_parts, attachments, parts, msg):
        m = self._str_msg(msg)
        allparts = list(m.walk())
        attachments = [allparts[n] for n in attachments]
        self.assertEqual(list(m.iter_attachments()), attachments)

    def message_as_iter_parts(self, body_parts, attachments, parts, msg):
        def _is_multipart_msg(msg):
            return 'Content-Type: multipart' in msg

        m = self._str_msg(msg)
        allparts = list(m.walk())
        parts = [allparts[n] for n in parts]
        iter_parts = list(m.iter_parts()) if _is_multipart_msg(msg) else []
        self.assertEqual(iter_parts, parts)

    class _TestContentManager:
        def get_content(self, msg, *args, **kw):
            return msg, args, kw
        def set_content(self, msg, *args, **kw):
            self.msg = msg
            self.args = args
            self.kw = kw

    def test_get_content_with_cm(self):
        m = self._str_msg('')
        cm = self._TestContentManager()
        self.assertEqual(m.get_content(content_manager=cm), (m, (), {}))
        msg, args, kw = m.get_content('foo', content_manager=cm, bar=1, k=2)
        self.assertEqual(msg, m)
        self.assertEqual(args, ('foo',))
        self.assertEqual(kw, dict(bar=1, k=2))

    def test_get_content_default_cm_comes_from_policy(self):
        p = policy.default.clone(content_manager=self._TestContentManager())
        m = self._str_msg('', policy=p)
        self.assertEqual(m.get_content(), (m, (), {}))
        msg, args, kw = m.get_content('foo', bar=1, k=2)
        self.assertEqual(msg, m)
        self.assertEqual(args, ('foo',))
        self.assertEqual(kw, dict(bar=1, k=2))

    def test_set_content_with_cm(self):
        m = self._str_msg('')
        cm = self._TestContentManager()
        m.set_content(content_manager=cm)
        self.assertEqual(cm.msg, m)
        self.assertEqual(cm.args, ())
        self.assertEqual(cm.kw, {})
        m.set_content('foo', content_manager=cm, bar=1, k=2)
        self.assertEqual(cm.msg, m)
        self.assertEqual(cm.args, ('foo',))
        self.assertEqual(cm.kw, dict(bar=1, k=2))

    def test_set_content_default_cm_comes_from_policy(self):
        cm = self._TestContentManager()
        p = policy.default.clone(content_manager=cm)
        m = self._str_msg('', policy=p)
        m.set_content()
        self.assertEqual(cm.msg, m)
        self.assertEqual(cm.args, ())
        self.assertEqual(cm.kw, {})
        m.set_content('foo', bar=1, k=2)
        self.assertEqual(cm.msg, m)
        self.assertEqual(cm.args, ('foo',))
        self.assertEqual(cm.kw, dict(bar=1, k=2))

    # outcome is whether xxx_method should raise ValueError error when called
    # on multipart/subtype.  Blank outcome means it depends on xxx (add
    # succeeds, make raises).  Note: 'none' means there are content-type
    # headers but payload is None...this happening in practice would be very
    # unusual, so treating it as if there were content seems reasonable.
    #    method          subtype           outcome
    subtype_params = (
        ('related',      'no_content',     'succeeds'),
        ('related',      'none',           'succeeds'),
        ('related',      'plain',          'succeeds'),
        ('related',      'related',        ''),
        ('related',      'alternative',    'raises'),
        ('related',      'mixed',          'raises'),
        ('alternative',  'no_content',     'succeeds'),
        ('alternative',  'none',           'succeeds'),
        ('alternative',  'plain',          'succeeds'),
        ('alternative',  'related',        'succeeds'),
        ('alternative',  'alternative',    ''),
        ('alternative',  'mixed',          'raises'),
        ('mixed',        'no_content',     'succeeds'),
        ('mixed',        'none',           'succeeds'),
        ('mixed',        'plain',          'succeeds'),
        ('mixed',        'related',        'succeeds'),
        ('mixed',        'alternative',    'succeeds'),
        ('mixed',        'mixed',          ''),
        )

    def _make_subtype_test_message(self, subtype):
        m = self.message()
        payload = None
        msg_headers =  [
            ('To', 'foo@bar.com'),
            ('From', 'bar@foo.com'),
            ]
        if subtype != 'no_content':
            ('content-shadow', 'Logrus'),
        msg_headers.append(('X-Random-Header', 'Corwin'))
        if subtype == 'text':
            payload = ''
            msg_headers.append(('Content-Type', 'text/plain'))
            m.set_payload('')
        elif subtype != 'no_content':
            payload = []
            msg_headers.append(('Content-Type', 'multipart/' + subtype))
        msg_headers.append(('X-Trump', 'Random'))
        m.set_payload(payload)
        for name, value in msg_headers:
            m[name] = value
        return m, msg_headers, payload

    def _check_disallowed_subtype_raises(self, m, method_name, subtype, method):
        with self.assertRaises(ValueError) as ar:
            getattr(m, method)()
        exc_text = str(ar.exception)
        self.assertIn(subtype, exc_text)
        self.assertIn(method_name, exc_text)

    def _check_make_multipart(self, m, msg_headers, payload):
        count = 0
        for name, value in msg_headers:
            if not name.lower().startswith('content-'):
                self.assertEqual(m[name], value)
                count += 1
        self.assertEqual(len(m), count+1) # +1 for new Content-Type
        part = next(m.iter_parts())
        count = 0
        for name, value in msg_headers:
            if name.lower().startswith('content-'):
                self.assertEqual(part[name], value)
                count += 1
        self.assertEqual(len(part), count)
        self.assertEqual(part.get_payload(), payload)

    def subtype_as_make(self, method, subtype, outcome):
        m, msg_headers, payload = self._make_subtype_test_message(subtype)
        make_method = 'make_' + method
        if outcome in ('', 'raises'):
            self._check_disallowed_subtype_raises(m, method, subtype, make_method)
            return
        getattr(m, make_method)()
        self.assertEqual(m.get_content_maintype(), 'multipart')
        self.assertEqual(m.get_content_subtype(), method)
        if subtype == 'no_content':
            self.assertEqual(len(m.get_payload()), 0)
            self.assertEqual(m.items(),
                             msg_headers + [('Content-Type',
                                             'multipart/'+method)])
        else:
            self.assertEqual(len(m.get_payload()), 1)
            self._check_make_multipart(m, msg_headers, payload)

    def subtype_as_make_with_boundary(self, method, subtype, outcome):
        # Doing all variation is a bit of overkill...
        m = self.message()
        if outcome in ('', 'raises'):
            m['Content-Type'] = 'multipart/' + subtype
            with self.assertRaises(ValueError) as cm:
                getattr(m, 'make_' + method)()
            return
        if subtype == 'plain':
            m['Content-Type'] = 'text/plain'
        elif subtype != 'no_content':
            m['Content-Type'] = 'multipart/' + subtype
        getattr(m, 'make_' + method)(boundary="abc")
        self.assertTrue(m.is_multipart())
        self.assertEqual(m.get_boundary(), 'abc')

    def test_policy_on_part_made_by_make_comes_from_message(self):
        for method in ('make_related', 'make_alternative', 'make_mixed'):
            m = self.message(policy=self.policy.clone(content_manager='foo'))
            m['Content-Type'] = 'text/plain'
            getattr(m, method)()
            self.assertEqual(m.get_payload(0).policy.content_manager, 'foo')

    class _TestSetContentManager:
        def set_content(self, msg, content, *args, **kw):
            msg['Content-Type'] = 'text/plain'
            msg.set_payload(content)

    def subtype_as_add(self, method, subtype, outcome):
        m, msg_headers, payload = self._make_subtype_test_message(subtype)
        cm = self._TestSetContentManager()
        add_method = 'add_attachment' if method=='mixed' else 'add_' + method
        if outcome == 'raises':
            self._check_disallowed_subtype_raises(m, method, subtype, add_method)
            return
        getattr(m, add_method)('test', content_manager=cm)
        self.assertEqual(m.get_content_maintype(), 'multipart')
        self.assertEqual(m.get_content_subtype(), method)
        if method == subtype or subtype == 'no_content':
            self.assertEqual(len(m.get_payload()), 1)
            for name, value in msg_headers:
                self.assertEqual(m[name], value)
            part = m.get_payload()[0]
        else:
            self.assertEqual(len(m.get_payload()), 2)
            self._check_make_multipart(m, msg_headers, payload)
            part = m.get_payload()[1]
        self.assertEqual(part.get_content_type(), 'text/plain')
        self.assertEqual(part.get_payload(), 'test')
        if method=='mixed':
            self.assertEqual(part['Content-Disposition'], 'attachment')
        elif method=='related':
            self.assertEqual(part['Content-Disposition'], 'inline')
        else:
            # Otherwise we don't guess.
            self.assertIsNone(part['Content-Disposition'])

    class _TestSetRaisingContentManager:
        class CustomError(Exception):
            pass
        def set_content(self, msg, content, *args, **kw):
            raise self.CustomError('test')

    def test_default_content_manager_for_add_comes_from_policy(self):
        cm = self._TestSetRaisingContentManager()
        m = self.message(policy=self.policy.clone(content_manager=cm))
        for method in ('add_related', 'add_alternative', 'add_attachment'):
            with self.assertRaises(self._TestSetRaisingContentManager.CustomError) as ar:
                getattr(m, method)('')
            self.assertEqual(str(ar.exception), 'test')

    def message_as_clear(self, body_parts, attachments, parts, msg):
        m = self._str_msg(msg)
        m.clear()
        self.assertEqual(len(m), 0)
        self.assertEqual(list(m.items()), [])
        self.assertIsNone(m.get_payload())
        self.assertEqual(list(m.iter_parts()), [])

    def message_as_clear_content(self, body_parts, attachments, parts, msg):
        m = self._str_msg(msg)
        expected_headers = [h for h in m.keys()
                            if not h.lower().startswith('content-')]
        m.clear_content()
        self.assertEqual(list(m.keys()), expected_headers)
        self.assertIsNone(m.get_payload())
        self.assertEqual(list(m.iter_parts()), [])

    def test_is_attachment(self):
        m = self._make_message()
        self.assertFalse(m.is_attachment())
        m['Content-Disposition'] = 'inline'
        self.assertFalse(m.is_attachment())
        m.replace_header('Content-Disposition', 'attachment')
        self.assertTrue(m.is_attachment())
        m.replace_header('Content-Disposition', 'AtTachMent')
        self.assertTrue(m.is_attachment())
        m.set_param('filename', 'abc.png', 'Content-Disposition')
        self.assertTrue(m.is_attachment())

    def test_iter_attachments_mutation(self):
        # We had a bug where iter_attachments was mutating the list.
        m = self._make_message()
        m.set_content('arbitrary text as main part')
        m.add_related('more text as a related part')
        m.add_related('yet more text as a second "attachment"')
        orig = m.get_payload().copy()
        self.assertEqual(len(list(m.iter_attachments())), 2)
        self.assertEqual(m.get_payload(), orig)

    get_payload_surrogate_params = {

        'good_surrogateescape': (
            "String that can be encod\udcc3\udcabd with surrogateescape",
            b'String that can be encod\xc3\xabd with surrogateescape'
            ),

        'string_with_utf8': (
            "String with utf-8 charactër",
            b'String with utf-8 charact\xebr'
            ),

        'surrogate_and_utf8': (
            "String that cannot be ëncod\udcc3\udcabd with surrogateescape",
             b'String that cannot be \xebncod\\udcc3\\udcabd with surrogateescape'
            ),

        'out_of_range_surrogate': (
            "String with \udfff cannot be encoded with surrogateescape",
             b'String with \\udfff cannot be encoded with surrogateescape'
            ),
    }

    def get_payload_surrogate_as_gh_94606(self, msg, expected):
        """test for GH issue 94606"""
        m = self._str_msg(msg)
        payload = m.get_payload(decode=True)
        self.assertEqual(expected, payload)


class TestEmailMessage(TestEmailMessageBase, TestEmailBase):
    message = EmailMessage

    def test_set_content_adds_MIME_Version(self):
        m = self._str_msg('')
        cm = self._TestContentManager()
        self.assertNotIn('MIME-Version', m)
        m.set_content(content_manager=cm)
        self.assertEqual(m['MIME-Version'], '1.0')

    class _MIME_Version_adding_CM:
        def set_content(self, msg, *args, **kw):
            msg['MIME-Version'] = '1.0'

    def test_set_content_does_not_duplicate_MIME_Version(self):
        m = self._str_msg('')
        cm = self._MIME_Version_adding_CM()
        self.assertNotIn('MIME-Version', m)
        m.set_content(content_manager=cm)
        self.assertEqual(m['MIME-Version'], '1.0')

    def test_as_string_uses_max_header_length_by_default(self):
        m = self._str_msg('Subject: long line' + ' ab'*50 + '\n\n')
        self.assertEqual(len(m.as_string().strip().splitlines()), 3)

    def test_as_string_allows_maxheaderlen(self):
        m = self._str_msg('Subject: long line' + ' ab'*50 + '\n\n')
        self.assertEqual(len(m.as_string(maxheaderlen=0).strip().splitlines()),
                         1)
        self.assertEqual(len(m.as_string(maxheaderlen=34).strip().splitlines()),
                         6)

    def test_as_string_unixform(self):
        m = self._str_msg('test')
        m.set_unixfrom('From foo@bar Thu Jan  1 00:00:00 1970')
        self.assertEqual(m.as_string(unixfrom=True),
                        'From foo@bar Thu Jan  1 00:00:00 1970\n\ntest')
        self.assertEqual(m.as_string(unixfrom=False), '\ntest')

    def test_str_defaults_to_policy_max_line_length(self):
        m = self._str_msg('Subject: long line' + ' ab'*50 + '\n\n')
        self.assertEqual(len(str(m).strip().splitlines()), 3)

    def test_str_defaults_to_utf8(self):
        m = EmailMessage()
        m['Subject'] = 'unicöde'
        self.assertEqual(str(m), 'Subject: unicöde\n\n')

    def test_folding_with_utf8_encoding_1(self):
        # bpo-36520
        #
        # Fold a line that contains UTF-8 words before
        # and after the whitespace fold point, where the
        # line length limit is reached within an ASCII
        # word.

        m = EmailMessage()
        m['Subject'] = 'Hello Wörld! Hello Wörld! '            \
                       'Hello Wörld! Hello Wörld!Hello Wörld!'
        self.assertEqual(bytes(m),
                         b'Subject: Hello =?utf-8?q?W=C3=B6rld!_Hello_W'
                         b'=C3=B6rld!_Hello_W=C3=B6rld!?=\n'
                         b' Hello =?utf-8?q?W=C3=B6rld!Hello_W=C3=B6rld!?=\n\n')


    def test_folding_with_utf8_encoding_2(self):
        # bpo-36520
        #
        # Fold a line that contains UTF-8 words before
        # and after the whitespace fold point, where the
        # line length limit is reached at the end of an
        # encoded word.

        m = EmailMessage()
        m['Subject'] = 'Hello Wörld! Hello Wörld! '                \
                       'Hello Wörlds123! Hello Wörld!Hello Wörld!'
        self.assertEqual(bytes(m),
                         b'Subject: Hello =?utf-8?q?W=C3=B6rld!_Hello_W'
                         b'=C3=B6rld!_Hello_W=C3=B6rlds123!?=\n'
                         b' Hello =?utf-8?q?W=C3=B6rld!Hello_W=C3=B6rld!?=\n\n')

    def test_folding_with_utf8_encoding_3(self):
        # bpo-36520
        #
        # Fold a line that contains UTF-8 words before
        # and after the whitespace fold point, where the
        # line length limit is reached at the end of the
        # first word.

        m = EmailMessage()
        m['Subject'] = 'Hello-Wörld!-Hello-Wörld!-Hello-Wörlds123! ' \
                       'Hello Wörld!Hello Wörld!'
        self.assertEqual(bytes(m), \
                         b'Subject: =?utf-8?q?Hello-W=C3=B6rld!-Hello-W'
                         b'=C3=B6rld!-Hello-W=C3=B6rlds123!?=\n'
                         b' Hello =?utf-8?q?W=C3=B6rld!Hello_W=C3=B6rld!?=\n\n')

    def test_folding_with_utf8_encoding_4(self):
        # bpo-36520
        #
        # Fold a line that contains UTF-8 words before
        # and after the fold point, where the first
        # word is UTF-8 and the fold point is within
        # the word.

        m = EmailMessage()
        m['Subject'] = 'Hello-Wörld!-Hello-Wörld!-Hello-Wörlds123!-Hello' \
                       ' Wörld!Hello Wörld!'
        self.assertEqual(bytes(m),
                         b'Subject: =?utf-8?q?Hello-W=C3=B6rld!-Hello-W'
                         b'=C3=B6rld!-Hello-W=C3=B6rlds123!?=\n'
                         b' =?utf-8?q?-Hello_W=C3=B6rld!Hello_W=C3=B6rld!?=\n\n')

    def test_folding_with_utf8_encoding_5(self):
        # bpo-36520
        #
        # Fold a line that contains a UTF-8 word after
        # the fold point.

        m = EmailMessage()
        m['Subject'] = '123456789 123456789 123456789 123456789 123456789' \
                       ' 123456789 123456789 Hello Wörld!'
        self.assertEqual(bytes(m),
                         b'Subject: 123456789 123456789 123456789 123456789'
                         b' 123456789 123456789 123456789\n'
                         b' Hello =?utf-8?q?W=C3=B6rld!?=\n\n')

    def test_folding_with_utf8_encoding_6(self):
        # bpo-36520
        #
        # Fold a line that contains a UTF-8 word before
        # the fold point and ASCII words after

        m = EmailMessage()
        m['Subject'] = '123456789 123456789 123456789 123456789 Hello Wörld!' \
                       ' 123456789 123456789 123456789 123456789 123456789'   \
                       ' 123456789'
        self.assertEqual(bytes(m),
                         b'Subject: 123456789 123456789 123456789 123456789'
                         b' Hello =?utf-8?q?W=C3=B6rld!?=\n 123456789 '
                         b'123456789 123456789 123456789 123456789 '
                         b'123456789\n\n')

    def test_folding_with_utf8_encoding_7(self):
        # bpo-36520
        #
        # Fold a line twice that contains UTF-8 words before
        # and after the first fold point, and ASCII words
        # after the second fold point.

        m = EmailMessage()
        m['Subject'] = '123456789 123456789 Hello Wörld! Hello Wörld! '       \
                       '123456789-123456789 123456789 Hello Wörld! 123456789' \
                       ' 123456789'
        self.assertEqual(bytes(m),
                         b'Subject: 123456789 123456789 Hello =?utf-8?q?'
                         b'W=C3=B6rld!_Hello_W=C3=B6rld!?=\n'
                         b' 123456789-123456789 123456789 Hello '
                         b'=?utf-8?q?W=C3=B6rld!?= 123456789\n 123456789\n\n')

    def test_folding_with_utf8_encoding_8(self):
        # bpo-36520
        #
        # Fold a line twice that contains UTF-8 words before
        # the first fold point, and ASCII words after the
        # first fold point, and UTF-8 words after the second
        # fold point.

        m = EmailMessage()
        m['Subject'] = '123456789 123456789 Hello Wörld! Hello Wörld! '       \
                       '123456789 123456789 123456789 123456789 123456789 '   \
                       '123456789-123456789 123456789 Hello Wörld! 123456789' \
                       ' 123456789'
        self.assertEqual(bytes(m),
                         b'Subject: 123456789 123456789 Hello '
                         b'=?utf-8?q?W=C3=B6rld!_Hello_W=C3=B6rld!?=\n 123456789 '
                         b'123456789 123456789 123456789 123456789 '
                         b'123456789-123456789\n 123456789 Hello '
                         b'=?utf-8?q?W=C3=B6rld!?= 123456789 123456789\n\n')

    def test_folding_with_short_nospace_1(self):
        # bpo-36520
        #
        # Fold a line that contains a long whitespace after
        # the fold point.

        m = EmailMessage(policy.default)
        m['Message-ID'] = '123456789' * 3
        parsed_msg = message_from_bytes(m.as_bytes(), policy=policy.default)
        self.assertEqual(parsed_msg['Message-ID'], m['Message-ID'])

    def test_folding_with_long_nospace_default_policy_1(self):
        # Fixed: https://github.com/python/cpython/issues/124452
        #
        # When the value is too long, it should be converted back
        # to its original form without any modifications.

        m = EmailMessage(policy.default)
        message = '123456789' * 10
        m['Message-ID'] = message
        self.assertEqual(m.as_bytes(),
                         f'Message-ID:\n {message}\n\n'.encode())
        parsed_msg = message_from_bytes(m.as_bytes(), policy=policy.default)
        self.assertEqual(parsed_msg['Message-ID'], m['Message-ID'])

    def test_folding_with_long_nospace_compat32_policy_1(self):
        m = EmailMessage(policy.compat32)
        message = '123456789' * 10
        m['Message-ID'] = message
        parsed_msg = message_from_bytes(m.as_bytes(), policy=policy.default)
        self.assertEqual(parsed_msg['Message-ID'], m['Message-ID'])

    def test_folding_with_long_nospace_smtp_policy_1(self):
        m = EmailMessage(policy.SMTP)
        message = '123456789' * 10
        m['Message-ID'] = message
        parsed_msg = message_from_bytes(m.as_bytes(), policy=policy.default)
        self.assertEqual(parsed_msg['Message-ID'], m['Message-ID'])

    def test_folding_with_long_nospace_http_policy_1(self):
        m = EmailMessage(policy.HTTP)
        message = '123456789' * 10
        m['Message-ID'] = message
        parsed_msg = message_from_bytes(m.as_bytes(), policy=policy.default)
        self.assertEqual(parsed_msg['Message-ID'], m['Message-ID'])

    def test_invalid_header_names(self):
        invalid_headers = [
            ('Invalid Header', 'contains space'),
            ('Tab\tHeader', 'contains tab'),
            ('Colon:Header', 'contains colon'),
            ('', 'Empty name'),
            (' LeadingSpace', 'starts with space'),
            ('TrailingSpace ', 'ends with space'),
            ('Header\x7F', 'Non-ASCII character'),
            ('Header\x80', 'Extended ASCII'),
        ]
        for email_policy in (policy.default, policy.compat32):
            for setter in (EmailMessage.__setitem__, EmailMessage.add_header):
                for name, value in invalid_headers:
                    self.do_test_invalid_header_names(email_policy, setter, name, value)

    def do_test_invalid_header_names(self, policy, setter, name, value):
        with self.subTest(policy=policy, setter=setter, name=name, value=value):
            message = EmailMessage(policy=policy)
            pattern = r'(?i)(?=.*invalid)(?=.*header)(?=.*name)'
            with self.assertRaisesRegex(ValueError, pattern) as cm:
                 setter(message, name, value)
            self.assertIn(f"{name!r}", str(cm.exception))

    def test_get_body_malformed(self):
        """test for bpo-42892"""
        msg = textwrap.dedent("""\
            Message-ID: <674392CA.4347091@email.au>
            Date: Wed, 08 Nov 2017 08:50:22 +0700
            From: Foo Bar <email@email.au>
            MIME-Version: 1.0
            To: email@email.com <email@email.com>
            Subject: Python Email
            Content-Type: multipart/mixed;
            boundary="------------879045806563892972123996"
            X-Global-filter:Messagescannedforspamandviruses:passedalltests

            This is a multi-part message in MIME format.
            --------------879045806563892972123996
            Content-Type: text/plain; charset=ISO-8859-1; format=flowed
            Content-Transfer-Encoding: 7bit

            Your message is ready to be sent with the following file or link
            attachments:
            XU89 - 08.11.2017
            """)
        m = self._str_msg(msg)
        # In bpo-42892, this would raise
        # AttributeError: 'str' object has no attribute 'is_attachment'
        m.get_body()


class TestMIMEPart(TestEmailMessageBase, TestEmailBase):
    # Doing the full test run here may seem a bit redundant, since the two
    # classes are almost identical.  But what if they drift apart?  So we do
    # the full tests so that any future drift doesn't introduce bugs.
    message = MIMEPart

    def test_set_content_does_not_add_MIME_Version(self):
        m = self._str_msg('')
        cm = self._TestContentManager()
        self.assertNotIn('MIME-Version', m)
        m.set_content(content_manager=cm)
        self.assertNotIn('MIME-Version', m)

    def test_string_payload_with_multipart_content_type(self):
        msg = message_from_string(textwrap.dedent("""\
        Content-Type: multipart/mixed; charset="utf-8"

        sample text
        """), policy=policy.default)
        attachments = msg.iter_attachments()
        self.assertEqual(list(attachments), [])


if __name__ == '__main__':
    unittest.main()
