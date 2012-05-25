import io
import email
import textwrap
import unittest
from email._policybase import Compat32
from email import errors
from email.message import Message
from test.test_email import TestEmailBase


class TestCustomMessage(TestEmailBase):

    class MyMessage(Message):
        def __init__(self, policy):
            self.check_policy = policy
            super().__init__()

    MyPolicy = TestEmailBase.policy.clone(linesep='boo')

    def test_custom_message_gets_policy_if_possible_from_string(self):
        msg = email.message_from_string("Subject: bogus\n\nmsg\n",
                                        self.MyMessage,
                                        policy=self.MyPolicy)
        self.assertTrue(isinstance(msg, self.MyMessage))
        self.assertIs(msg.check_policy, self.MyPolicy)

    def test_custom_message_gets_policy_if_possible_from_file(self):
        source_file = io.StringIO("Subject: bogus\n\nmsg\n")
        msg = email.message_from_file(source_file,
                                      self.MyMessage,
                                      policy=self.MyPolicy)
        self.assertTrue(isinstance(msg, self.MyMessage))
        self.assertIs(msg.check_policy, self.MyPolicy)

    # XXX add tests for other functions that take Message arg.


class TestMessageDefectDetectionBase:

    dup_boundary_msg = textwrap.dedent("""\
        Subject: XX
        From: xx@xx.dk
        To: XX
        Mime-version: 1.0
        Content-type: multipart/mixed;
           boundary="MS_Mac_OE_3071477847_720252_MIME_Part"

        --MS_Mac_OE_3071477847_720252_MIME_Part
        Content-type: multipart/alternative;
           boundary="MS_Mac_OE_3071477847_720252_MIME_Part"

        --MS_Mac_OE_3071477847_720252_MIME_Part
        Content-type: text/plain; charset="ISO-8859-1"
        Content-transfer-encoding: quoted-printable

        text

        --MS_Mac_OE_3071477847_720252_MIME_Part
        Content-type: text/html; charset="ISO-8859-1"
        Content-transfer-encoding: quoted-printable

        <HTML></HTML>

        --MS_Mac_OE_3071477847_720252_MIME_Part--

        --MS_Mac_OE_3071477847_720252_MIME_Part
        Content-type: image/gif; name="xx.gif";
        Content-disposition: attachment
        Content-transfer-encoding: base64

        Some removed base64 encoded chars.

        --MS_Mac_OE_3071477847_720252_MIME_Part--

        """)

    def test_same_boundary_inner_outer(self):
        # XXX better would be to actually detect the duplicate.
        msg = self._str_msg(self.dup_boundary_msg)
        inner = msg.get_payload(0)
        self.assertTrue(hasattr(inner, 'defects'))
        self.assertEqual(len(self.get_defects(inner)), 1)
        self.assertTrue(isinstance(self.get_defects(inner)[0],
                                   errors.StartBoundaryNotFoundDefect))

    def test_same_boundary_inner_outer_raises_on_defect(self):
        with self.assertRaises(errors.StartBoundaryNotFoundDefect):
            self._str_msg(self.dup_boundary_msg,
                policy=self.policy.clone(raise_on_defect=True))

    no_boundary_msg = textwrap.dedent("""\
        Date: Fri, 6 Apr 2001 09:23:06 -0800 (GMT-0800)
        From: foobar
        Subject: broken mail
        MIME-Version: 1.0
        Content-Type: multipart/report; report-type=delivery-status;

        --JAB03225.986577786/zinfandel.lacita.com

        One part

        --JAB03225.986577786/zinfandel.lacita.com
        Content-Type: message/delivery-status

        Header: Another part

        --JAB03225.986577786/zinfandel.lacita.com--
        """)

    def test_multipart_no_boundary(self):
        msg = self._str_msg(self.no_boundary_msg)
        self.assertTrue(isinstance(msg.get_payload(), str))
        self.assertEqual(len(self.get_defects(msg)), 2)
        self.assertTrue(isinstance(self.get_defects(msg)[0],
                                   errors.NoBoundaryInMultipartDefect))
        self.assertTrue(isinstance(self.get_defects(msg)[1],
                                   errors.MultipartInvariantViolationDefect))

    def test_multipart_no_boundary_raise_on_defect(self):
        with self.assertRaises(errors.NoBoundaryInMultipartDefect):
            self._str_msg(self.no_boundary_msg,
                policy=self.policy.clone(raise_on_defect=True))

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

    def test_multipart_invalid_cte(self):
        msg = self._str_msg(
            self.multipart_msg.format("\nContent-Transfer-Encoding: base64"))
        self.assertEqual(len(self.get_defects(msg)), 1)
        self.assertIsInstance(self.get_defects(msg)[0],
            errors.InvalidMultipartContentTransferEncodingDefect)

    def test_multipart_invalid_cte_raise_on_defect(self):
        with self.assertRaises(
                errors.InvalidMultipartContentTransferEncodingDefect):
            self._str_msg(
                self.multipart_msg.format(
                    "\nContent-Transfer-Encoding: base64"),
                policy=self.policy.clone(raise_on_defect=True))

    def test_multipart_no_cte_no_defect(self):
        msg = self._str_msg(self.multipart_msg.format(''))
        self.assertEqual(len(self.get_defects(msg)), 0)

    def test_multipart_valid_cte_no_defect(self):
        for cte in ('7bit', '8bit', 'BINary'):
            msg = self._str_msg(
                self.multipart_msg.format("\nContent-Transfer-Encoding: "+cte))
            self.assertEqual(len(self.get_defects(msg)), 0, "cte="+cte)

    lying_multipart_msg = textwrap.dedent("""\
        From: "Allison Dunlap" <xxx@example.com>
        To: yyy@example.com
        Subject: 64423
        Date: Sun, 11 Jul 2004 16:09:27 -0300
        MIME-Version: 1.0
        Content-Type: multipart/alternative;

        Blah blah blah
        """)

    def test_lying_multipart(self):
        msg = self._str_msg(self.lying_multipart_msg)
        self.assertTrue(hasattr(msg, 'defects'))
        self.assertEqual(len(self.get_defects(msg)), 2)
        self.assertTrue(isinstance(self.get_defects(msg)[0],
                                   errors.NoBoundaryInMultipartDefect))
        self.assertTrue(isinstance(self.get_defects(msg)[1],
                                   errors.MultipartInvariantViolationDefect))

    def test_lying_multipart_raise_on_defect(self):
        with self.assertRaises(errors.NoBoundaryInMultipartDefect):
            self._str_msg(self.lying_multipart_msg,
                policy=self.policy.clone(raise_on_defect=True))

    missing_start_boundary_msg = textwrap.dedent("""\
        Content-Type: multipart/mixed; boundary="AAA"
        From: Mail Delivery Subsystem <xxx@example.com>
        To: yyy@example.com

        --AAA

        Stuff

        --AAA
        Content-Type: message/rfc822

        From: webmaster@python.org
        To: zzz@example.com
        Content-Type: multipart/mixed; boundary="BBB"

        --BBB--

        --AAA--

        """)

    def test_missing_start_boundary(self):
        # The message structure is:
        #
        # multipart/mixed
        #    text/plain
        #    message/rfc822
        #        multipart/mixed [*]
        #
        # [*] This message is missing its start boundary
        outer = self._str_msg(self.missing_start_boundary_msg)
        bad = outer.get_payload(1).get_payload(0)
        self.assertEqual(len(self.get_defects(bad)), 1)
        self.assertTrue(isinstance(self.get_defects(bad)[0],
                                   errors.StartBoundaryNotFoundDefect))

    def test_missing_start_boundary_raise_on_defect(self):
        with self.assertRaises(errors.StartBoundaryNotFoundDefect):
            self._str_msg(self.missing_start_boundary_msg,
                          policy=self.policy.clone(raise_on_defect=True))

    def test_first_line_is_continuation_header(self):
        msg = self._str_msg(' Line 1\nLine 2\nLine 3')
        self.assertEqual(msg.keys(), [])
        self.assertEqual(msg.get_payload(), 'Line 2\nLine 3')
        self.assertEqual(len(self.get_defects(msg)), 1)
        self.assertTrue(isinstance(self.get_defects(msg)[0],
                                   errors.FirstHeaderLineIsContinuationDefect))
        self.assertEqual(self.get_defects(msg)[0].line, ' Line 1\n')

    def test_first_line_is_continuation_header_raise_on_defect(self):
        with self.assertRaises(errors.FirstHeaderLineIsContinuationDefect):
            self._str_msg(' Line 1\nLine 2\nLine 3',
                          policy=self.policy.clone(raise_on_defect=True))


class TestMessageDefectDetection(TestMessageDefectDetectionBase, TestEmailBase):

    def get_defects(self, obj):
        return obj.defects


class TestMessageDefectDetectionCapture(TestMessageDefectDetectionBase,
                                        TestEmailBase):

    class CapturePolicy(Compat32):
        captured = None
        def register_defect(self, obj, defect):
            self.captured.append(defect)

    def setUp(self):
        self.policy = self.CapturePolicy(captured=list())

    def get_defects(self, obj):
        return self.policy.captured


if __name__ == '__main__':
    unittest.main()
