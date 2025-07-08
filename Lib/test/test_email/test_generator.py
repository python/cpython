import io
import textwrap
import unittest
from email import message_from_string, message_from_bytes
from email.message import EmailMessage
from email.generator import Generator, BytesGenerator
from email.headerregistry import Address
from email import policy
import email.errors
from test.test_email import TestEmailBase, parameterize


@parameterize
class TestGeneratorBase:

    policy = policy.default

    def msgmaker(self, msg, policy=None):
        policy = self.policy if policy is None else policy
        return self.msgfunc(msg, policy=policy)

    refold_long_expected = {
        0: textwrap.dedent("""\
            To: whom_it_may_concern@example.com
            From: nobody_you_want_to_know@example.com
            Subject: We the willing led by the unknowing are doing the
             impossible for the ungrateful. We have done so much for so long with so little
             we are now qualified to do anything with nothing.

            None
            """),
        40: textwrap.dedent("""\
            To: whom_it_may_concern@example.com
            From:
             nobody_you_want_to_know@example.com
            Subject: We the willing led by the
             unknowing are doing the impossible for
             the ungrateful. We have done so much
             for so long with so little we are now
             qualified to do anything with nothing.

            None
            """),
        20: textwrap.dedent("""\
            To:
             whom_it_may_concern@example.com
            From:
             nobody_you_want_to_know@example.com
            Subject: We the
             willing led by the
             unknowing are doing
             the impossible for
             the ungrateful. We
             have done so much
             for so long with so
             little we are now
             qualified to do
             anything with
             nothing.

            None
            """),
        }
    refold_long_expected[100] = refold_long_expected[0]

    refold_all_expected = refold_long_expected.copy()
    refold_all_expected[0] = (
            "To: whom_it_may_concern@example.com\n"
            "From: nobody_you_want_to_know@example.com\n"
            "Subject: We the willing led by the unknowing are doing the "
              "impossible for the ungrateful. We have done so much for "
              "so long with so little we are now qualified to do anything "
              "with nothing.\n"
              "\n"
              "None\n")
    refold_all_expected[100] = (
            "To: whom_it_may_concern@example.com\n"
            "From: nobody_you_want_to_know@example.com\n"
            "Subject: We the willing led by the unknowing are doing the "
                "impossible for the ungrateful. We have\n"
              " done so much for so long with so little we are now qualified "
                "to do anything with nothing.\n"
              "\n"
              "None\n")

    length_params = [n for n in refold_long_expected]

    def length_as_maxheaderlen_parameter(self, n):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n, policy=self.policy)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_long_expected[n]))

    def length_as_max_line_length_policy(self, n):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(max_line_length=n))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_long_expected[n]))

    def length_as_maxheaderlen_parm_overrides_policy(self, n):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n,
                          policy=self.policy.clone(max_line_length=10))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_long_expected[n]))

    def length_as_max_line_length_with_refold_none_does_not_fold(self, n):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(refold_source='none',
                                                      max_line_length=n))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_long_expected[0]))

    def length_as_max_line_length_with_refold_all_folds(self, n):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(refold_source='all',
                                                      max_line_length=n))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_all_expected[n]))

    def test_crlf_control_via_policy(self):
        source = "Subject: test\r\n\r\ntest body\r\n"
        expected = source
        msg = self.msgmaker(self.typ(source))
        s = self.ioclass()
        g = self.genclass(s, policy=policy.SMTP)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_flatten_linesep_overrides_policy(self):
        source = "Subject: test\n\ntest body\n"
        expected = source
        msg = self.msgmaker(self.typ(source))
        s = self.ioclass()
        g = self.genclass(s, policy=policy.SMTP)
        g.flatten(msg, linesep='\n')
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_flatten_linesep(self):
        source = 'Subject: one\n two\r three\r\n four\r\n\r\ntest body\r\n'
        msg = self.msgmaker(self.typ(source))
        self.assertEqual(msg['Subject'], 'one two three four')

        expected = 'Subject: one\n two\n three\n four\n\ntest body\n'
        s = self.ioclass()
        g = self.genclass(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

        expected = 'Subject: one two three four\n\ntest body\n'
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(refold_source='all'))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_flatten_control_linesep(self):
        source = 'Subject: one\v two\f three\x1c four\x1d five\x1e six\r\n\r\ntest body\r\n'
        msg = self.msgmaker(self.typ(source))
        self.assertEqual(msg['Subject'], 'one\v two\f three\x1c four\x1d five\x1e six')

        expected = 'Subject: one\v two\f three\x1c four\x1d five\x1e six\n\ntest body\n'
        s = self.ioclass()
        g = self.genclass(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(refold_source='all'))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_set_mangle_from_via_policy(self):
        source = textwrap.dedent("""\
            Subject: test that
             from is mangled in the body!

            From time to time I write a rhyme.
            """)
        variants = (
            (None, True),
            (policy.compat32, True),
            (policy.default, False),
            (policy.default.clone(mangle_from_=True), True),
            )
        for p, mangle in variants:
            expected = source.replace('From ', '>From ') if mangle else source
            with self.subTest(policy=p, mangle_from_=mangle):
                msg = self.msgmaker(self.typ(source))
                s = self.ioclass()
                g = self.genclass(s, policy=p)
                g.flatten(msg)
                self.assertEqual(s.getvalue(), self.typ(expected))

    def test_compat32_max_line_length_does_not_fold_when_none(self):
        msg = self.msgmaker(self.typ(self.refold_long_expected[0]))
        s = self.ioclass()
        g = self.genclass(s, policy=policy.compat32.clone(max_line_length=None))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.refold_long_expected[0]))

    def test_rfc2231_wrapping(self):
        # This is pretty much just to make sure we don't have an infinite
        # loop; I don't expect anyone to hit this in the field.
        msg = self.msgmaker(self.typ(textwrap.dedent("""\
            To: nobody
            Content-Disposition: attachment;
             filename="afilenamelongenoghtowraphere"

            None
            """)))
        expected = textwrap.dedent("""\
            To: nobody
            Content-Disposition: attachment;
             filename*0*=us-ascii''afilename;
             filename*1*=longenoghtowraphere

            None
            """)
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(max_line_length=33))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_rfc2231_wrapping_switches_to_default_len_if_too_narrow(self):
        # This is just to make sure we don't have an infinite loop; I don't
        # expect anyone to hit this in the field, so I'm not bothering to make
        # the result optimal (the encoding isn't needed).
        msg = self.msgmaker(self.typ(textwrap.dedent("""\
            To: nobody
            Content-Disposition: attachment;
             filename="afilenamelongenoghtowraphere"

            None
            """)))
        expected = textwrap.dedent("""\
            To: nobody
            Content-Disposition:
             attachment;
             filename*0*=us-ascii''afilenamelongenoghtowraphere

            None
            """)
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(max_line_length=20))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_keep_encoded_newlines(self):
        msg = self.msgmaker(self.typ(textwrap.dedent("""\
            To: nobody
            Subject: Bad subject=?UTF-8?Q?=0A?=Bcc: injection@example.com

            None
            """)))
        expected = textwrap.dedent("""\
            To: nobody
            Subject: Bad subject=?UTF-8?Q?=0A?=Bcc: injection@example.com

            None
            """)
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(max_line_length=80))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_keep_long_encoded_newlines(self):
        msg = self.msgmaker(self.typ(textwrap.dedent("""\
            To: nobody
            Subject: Bad subject=?UTF-8?Q?=0A?=Bcc: injection@example.com

            None
            """)))
        expected = textwrap.dedent("""\
            To: nobody
            Subject: Bad subject
             =?utf-8?q?=0A?=Bcc:
             injection@example.com

            None
            """)
        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(max_line_length=30))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))


class TestGenerator(TestGeneratorBase, TestEmailBase):

    msgfunc = staticmethod(message_from_string)
    genclass = Generator
    ioclass = io.StringIO
    typ = str

    def test_flatten_unicode_linesep(self):
        source = 'Subject: one\x85 two\u2028 three\u2029 four\r\n\r\ntest body\r\n'
        msg = self.msgmaker(self.typ(source))
        self.assertEqual(msg['Subject'], 'one\x85 two\u2028 three\u2029 four')

        expected = 'Subject: =?utf-8?b?b25lwoUgdHdv4oCoIHRocmVl4oCp?= four\n\ntest body\n'
        s = self.ioclass()
        g = self.genclass(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

        s = self.ioclass()
        g = self.genclass(s, policy=self.policy.clone(refold_source='all'))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(expected))

    def test_verify_generated_headers(self):
        """gh-121650: by default the generator prevents header injection"""
        class LiteralHeader(str):
            name = 'Header'
            def fold(self, **kwargs):
                return self

        for text in (
            'Value\r\nBad Injection\r\n',
            'NoNewLine'
        ):
            with self.subTest(text=text):
                message = message_from_string(
                    "Header: Value\r\n\r\nBody",
                    policy=self.policy,
                )

                del message['Header']
                message['Header'] = LiteralHeader(text)

                with self.assertRaises(email.errors.HeaderWriteError):
                    message.as_string()


class TestBytesGenerator(TestGeneratorBase, TestEmailBase):

    msgfunc = staticmethod(message_from_bytes)
    genclass = BytesGenerator
    ioclass = io.BytesIO
    typ = lambda self, x: x.encode('ascii')

    def test_defaults_handle_spaces_between_encoded_words_when_folded(self):
        source = ("Уведомление о принятии в работу обращения для"
                  " подключения услуги")
        expected = ('Subject: =?utf-8?b?0KPQstC10LTQvtC80LvQtdC90LjQtSDQviDQv9GA0LjQvdGP0YLQuNC4?=\n'
                    ' =?utf-8?b?INCyINGA0LDQsdC+0YLRgyDQvtCx0YDQsNGJ0LXQvdC40Y8g0LTQu9GPINC/0L4=?=\n'
                    ' =?utf-8?b?0LTQutC70Y7Rh9C10L3QuNGPINGD0YHQu9GD0LPQuA==?=\n\n').encode('ascii')
        msg = EmailMessage()
        msg['Subject'] = source
        s = io.BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_defaults_handle_spaces_when_encoded_words_is_folded_in_middle(self):
        source = ('A very long long long long long long long long long long long long '
                  'long long long long long long long long long long long súmmäry')
        expected = ('Subject: A very long long long long long long long long long long long long\n'
                    ' long long long long long long long long long long long =?utf-8?q?s=C3=BAmm?=\n'
                    ' =?utf-8?q?=C3=A4ry?=\n\n').encode('ascii')
        msg = EmailMessage()
        msg['Subject'] = source
        s = io.BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_defaults_handle_spaces_at_start_of_subject(self):
        source = " Уведомление"
        expected = b"Subject:  =?utf-8?b?0KPQstC10LTQvtC80LvQtdC90LjQtQ==?=\n\n"
        msg = EmailMessage()
        msg['Subject'] = source
        s = io.BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_defaults_handle_spaces_at_start_of_continuation_line(self):
        source = " ф ффффффффффффффффффф ф ф"
        expected = (b"Subject:  "
                    b"=?utf-8?b?0YQg0YTRhNGE0YTRhNGE0YTRhNGE0YTRhNGE0YTRhNGE0YTRhNGE0YQ=?=\n"
                    b" =?utf-8?b?INGEINGE?=\n\n")
        msg = EmailMessage()
        msg['Subject'] = source
        s = io.BytesIO()
        g = BytesGenerator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_cte_type_7bit_handles_unknown_8bit(self):
        source = ("Subject: Maintenant je vous présente mon "
                 "collègue\n\n").encode('utf-8')
        expected = ('Subject: Maintenant je vous =?unknown-8bit?q?'
                    'pr=C3=A9sente_mon_coll=C3=A8gue?=\n\n').encode('ascii')
        msg = message_from_bytes(source)
        s = io.BytesIO()
        g = BytesGenerator(s, policy=self.policy.clone(cte_type='7bit'))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_cte_type_7bit_transforms_8bit_cte(self):
        source = textwrap.dedent("""\
            From: foo@bar.com
            To: Dinsdale
            Subject: Nudge nudge, wink, wink
            Mime-Version: 1.0
            Content-Type: text/plain; charset="latin-1"
            Content-Transfer-Encoding: 8bit

            oh là là, know what I mean, know what I mean?
            """).encode('latin1')
        msg = message_from_bytes(source)
        expected =  textwrap.dedent("""\
            From: foo@bar.com
            To: Dinsdale
            Subject: Nudge nudge, wink, wink
            Mime-Version: 1.0
            Content-Type: text/plain; charset="iso-8859-1"
            Content-Transfer-Encoding: quoted-printable

            oh l=E0 l=E0, know what I mean, know what I mean?
            """).encode('ascii')
        s = io.BytesIO()
        g = BytesGenerator(s, policy=self.policy.clone(cte_type='7bit',
                                                       linesep='\n'))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_smtputf8_policy(self):
        msg = EmailMessage()
        msg['From'] = "Páolo <főo@bar.com>"
        msg['To'] = 'Dinsdale'
        msg['Subject'] = 'Nudge nudge, wink, wink \u1F609'
        msg.set_content("oh là là, know what I mean, know what I mean?")
        expected = textwrap.dedent("""\
            From: Páolo <főo@bar.com>
            To: Dinsdale
            Subject: Nudge nudge, wink, wink \u1F609
            Content-Type: text/plain; charset="utf-8"
            Content-Transfer-Encoding: 8bit
            MIME-Version: 1.0

            oh là là, know what I mean, know what I mean?
            """).encode('utf-8').replace(b'\n', b'\r\n')
        s = io.BytesIO()
        g = BytesGenerator(s, policy=policy.SMTPUTF8)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)

    def test_smtp_policy(self):
        msg = EmailMessage()
        msg["From"] = Address(addr_spec="foo@bar.com", display_name="Páolo")
        msg["To"] = Address(addr_spec="bar@foo.com", display_name="Dinsdale")
        msg["Subject"] = "Nudge nudge, wink, wink"
        msg.set_content("oh boy, know what I mean, know what I mean?")
        expected = textwrap.dedent("""\
            From: =?utf-8?q?P=C3=A1olo?= <foo@bar.com>
            To: Dinsdale <bar@foo.com>
            Subject: Nudge nudge, wink, wink
            Content-Type: text/plain; charset="utf-8"
            Content-Transfer-Encoding: 7bit
            MIME-Version: 1.0

            oh boy, know what I mean, know what I mean?
            """).encode().replace(b"\n", b"\r\n")
        s = io.BytesIO()
        g = BytesGenerator(s, policy=policy.SMTP)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), expected)


if __name__ == '__main__':
    unittest.main()
