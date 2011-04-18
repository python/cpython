import io
import textwrap
import unittest
from email import message_from_string, message_from_bytes
from email.generator import Generator, BytesGenerator
from email import policy
from test.test_email import TestEmailBase

# XXX: move generator tests from test_email into here at some point.


class TestGeneratorBase():

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
        msg = self.msgmaker(self.long_subject[0])
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.long_subject[n])

    def test_maxheaderlen_parameter_0(self):
        self.maxheaderlen_parameter_test(0)

    def test_maxheaderlen_parameter_100(self):
        self.maxheaderlen_parameter_test(100)

    def test_maxheaderlen_parameter_40(self):
        self.maxheaderlen_parameter_test(40)

    def test_maxheaderlen_parameter_20(self):
        self.maxheaderlen_parameter_test(20)

    def maxheaderlen_policy_test(self, n):
        msg = self.msgmaker(self.long_subject[0])
        s = self.ioclass()
        g = self.genclass(s, policy=policy.default.clone(max_line_length=n))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.long_subject[n])

    def test_maxheaderlen_policy_0(self):
        self.maxheaderlen_policy_test(0)

    def test_maxheaderlen_policy_100(self):
        self.maxheaderlen_policy_test(100)

    def test_maxheaderlen_policy_40(self):
        self.maxheaderlen_policy_test(40)

    def test_maxheaderlen_policy_20(self):
        self.maxheaderlen_policy_test(20)

    def maxheaderlen_parm_overrides_policy_test(self, n):
        msg = self.msgmaker(self.long_subject[0])
        s = self.ioclass()
        g = self.genclass(s, maxheaderlen=n,
                          policy=policy.default.clone(max_line_length=10))
        g.flatten(msg)
        self.assertEqual(s.getvalue(), self.long_subject[n])

    def test_maxheaderlen_parm_overrides_policy_0(self):
        self.maxheaderlen_parm_overrides_policy_test(0)

    def test_maxheaderlen_parm_overrides_policy_100(self):
        self.maxheaderlen_parm_overrides_policy_test(100)

    def test_maxheaderlen_parm_overrides_policy_40(self):
        self.maxheaderlen_parm_overrides_policy_test(40)

    def test_maxheaderlen_parm_overrides_policy_20(self):
        self.maxheaderlen_parm_overrides_policy_test(20)


class TestGenerator(TestGeneratorBase, TestEmailBase):

    msgmaker = staticmethod(message_from_string)
    genclass = Generator
    ioclass = io.StringIO


class TestBytesGenerator(TestGeneratorBase, TestEmailBase):

    msgmaker = staticmethod(message_from_bytes)
    genclass = BytesGenerator
    ioclass = io.BytesIO
    long_subject = {key: x.encode('ascii')
        for key, x in TestGeneratorBase.long_subject.items()}


if __name__ == '__main__':
    unittest.main()
