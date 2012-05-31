import io
import textwrap
import unittest
from email import message_from_string, message_from_bytes
from email.generator import Generator, BytesGenerator
from email import policy
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
        # From is wrapped because wrapped it fits in 40.
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
        # Neither to nor from fit even if put on a new line,
        # so we leave them sticking out on the first line.
        20: textwrap.dedent("""\
            To: whom_it_may_concern@example.com
            From: nobody_you_want_to_know@example.com
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


class TestGenerator(TestGeneratorBase, TestEmailBase):

    msgfunc = staticmethod(message_from_string)
    genclass = Generator
    ioclass = io.StringIO
    typ = str


class TestBytesGenerator(TestGeneratorBase, TestEmailBase):

    msgfunc = staticmethod(message_from_bytes)
    genclass = BytesGenerator
    ioclass = io.BytesIO
    typ = lambda self, x: x.encode('ascii')

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


if __name__ == '__main__':
    unittest.main()
