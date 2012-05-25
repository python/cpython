import io
import textwrap
import unittest
from email import message_from_string, message_from_bytes
from email.generator import Generator, BytesGenerator
from email import policy
from test.test_email import TestEmailBase

# XXX: move generator tests from test_email into here at some point.


class TestGeneratorBase():

    policy = policy.compat32

    long_subject = {
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
            From:\x20
             nobody_you_want_to_know@example.com
            Subject: We the willing led by the
             unknowing are doing the
             impossible for the ungrateful. We have
             done so much for so long with so little
             we are now qualified to do anything
             with nothing.

            None
            """),
        20: textwrap.dedent("""\
            To:\x20
             whom_it_may_concern@example.com
            From:\x20
             nobody_you_want_to_know@example.com
            Subject: We the
             willing led by the
             unknowing are doing
             the
             impossible for the
             ungrateful. We have
             done so much for so
             long with so little
             we are now
             qualified to do
             anything with
             nothing.

            None
            """),
        }
    long_subject[100] = long_subject[0]

    def maxheaderlen_parameter_test(self, n):
        msg = self.msgmaker(self.typ(self.long_subject[0]))
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.long_subject[n]))

    def test_maxheaderlen_parameter_0(self):
        self.maxheaderlen_parameter_test(0)

    def test_maxheaderlen_parameter_100(self):
        self.maxheaderlen_parameter_test(100)

    def test_maxheaderlen_parameter_40(self):
        self.maxheaderlen_parameter_test(40)

    def test_maxheaderlen_parameter_20(self):
        self.maxheaderlen_parameter_test(20)

    def maxheaderlen_policy_test(self, n):
        msg = self.msgmaker(self.typ(self.long_subject[0]))
        s = self.ioclass()
        g = self.genclass(s, policy=policy.default.clone(max_line_length=n))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.long_subject[n]))

    def test_maxheaderlen_policy_0(self):
        self.maxheaderlen_policy_test(0)

    def test_maxheaderlen_policy_100(self):
        self.maxheaderlen_policy_test(100)

    def test_maxheaderlen_policy_40(self):
        self.maxheaderlen_policy_test(40)

    def test_maxheaderlen_policy_20(self):
        self.maxheaderlen_policy_test(20)

    def maxheaderlen_parm_overrides_policy_test(self, n):
        msg = self.msgmaker(self.typ(self.long_subject[0]))
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n,
                          policy=policy.default.clone(max_line_length=10))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.typ(self.long_subject[n]))

    def test_maxheaderlen_parm_overrides_policy_0(self):
        self.maxheaderlen_parm_overrides_policy_test(0)

    def test_maxheaderlen_parm_overrides_policy_100(self):
        self.maxheaderlen_parm_overrides_policy_test(100)

    def test_maxheaderlen_parm_overrides_policy_40(self):
        self.maxheaderlen_parm_overrides_policy_test(40)

    def test_maxheaderlen_parm_overrides_policy_20(self):
        self.maxheaderlen_parm_overrides_policy_test(20)

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

    genclass = Generator
    ioclass = io.StringIO
    typ = str

    def msgmaker(self, msg, policy=None):
        policy = self.policy if policy is None else policy
        return message_from_string(msg, policy=policy)


class TestBytesGenerator(TestGeneratorBase, TestEmailBase):

    genclass = BytesGenerator
    ioclass = io.BytesIO
    typ = lambda self, x: x.encode('ascii')

    def msgmaker(self, msg, policy=None):
        policy = self.policy if policy is None else policy
        return message_from_bytes(msg, policy=policy)

    def test_cte_type_7bit_handles_unknown_8bit(self):
        source = ("Subject: Maintenant je vous présente mon "
                 "collègue\n\n").encode('utf-8')
        expected = ('Subject: =?unknown-8bit?q?Maintenant_je_vous_pr=C3=A9sente_mon_'
                    'coll=C3=A8gue?=\n\n').encode('ascii')
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
