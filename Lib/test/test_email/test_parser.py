import io
import email
import unittest
from email.message import Message, EmailMessage
from email.policy import default
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
        self.assertIsInstance(msg, self.MyMessage)
        self.assertIs(msg.check_policy, self.MyPolicy)

    def test_custom_message_gets_policy_if_possible_from_file(self):
        source_file = io.StringIO("Subject: bogus\n\nmsg\n")
        msg = email.message_from_file(source_file,
                                      self.MyMessage,
                                      policy=self.MyPolicy)
        self.assertIsInstance(msg, self.MyMessage)
        self.assertIs(msg.check_policy, self.MyPolicy)

    # XXX add tests for other functions that take Message arg.


class TestParserBase:

    def test_only_split_on_cr_lf(self):
        # The unicode line splitter splits on unicode linebreaks, which are
        # more numerous than allowed by the email RFCs; make sure we are only
        # splitting on those two.
        for parser in self.parsers:
            with self.subTest(parser=parser.__name__):
                msg = parser(
                    "Next-Line: not\x85broken\r\n"
                    "Null: not\x00broken\r\n"
                    "Vertical-Tab: not\vbroken\r\n"
                    "Form-Feed: not\fbroken\r\n"
                    "File-Separator: not\x1Cbroken\r\n"
                    "Group-Separator: not\x1Dbroken\r\n"
                    "Record-Separator: not\x1Ebroken\r\n"
                    "Line-Separator: not\u2028broken\r\n"
                    "Paragraph-Separator: not\u2029broken\r\n"
                    "\r\n",
                    policy=default,
                )
                self.assertEqual(msg.items(), [
                    ("Next-Line", "not\x85broken"),
                    ("Null", "not\x00broken"),
                    ("Vertical-Tab", "not\vbroken"),
                    ("Form-Feed", "not\fbroken"),
                    ("File-Separator", "not\x1Cbroken"),
                    ("Group-Separator", "not\x1Dbroken"),
                    ("Record-Separator", "not\x1Ebroken"),
                    ("Line-Separator", "not\u2028broken"),
                    ("Paragraph-Separator", "not\u2029broken"),
                ])
                self.assertEqual(msg.get_payload(), "")

    class MyMessage(EmailMessage):
        pass

    def test_custom_message_factory_on_policy(self):
        for parser in self.parsers:
            with self.subTest(parser=parser.__name__):
                MyPolicy = default.clone(message_factory=self.MyMessage)
                msg = parser("To: foo\n\ntest", policy=MyPolicy)
                self.assertIsInstance(msg, self.MyMessage)

    def test_factory_arg_overrides_policy(self):
        for parser in self.parsers:
            with self.subTest(parser=parser.__name__):
                MyPolicy = default.clone(message_factory=self.MyMessage)
                msg = parser("To: foo\n\ntest", Message, policy=MyPolicy)
                self.assertNotIsInstance(msg, self.MyMessage)
                self.assertIsInstance(msg, Message)

# Play some games to get nice output in subTest.  This code could be clearer
# if staticmethod supported __name__.

def message_from_file(s, *args, **kw):
    f = io.StringIO(s)
    return email.message_from_file(f, *args, **kw)

class TestParser(TestParserBase, TestEmailBase):
    parsers = (email.message_from_string, message_from_file)

def message_from_bytes(s, *args, **kw):
    return email.message_from_bytes(s.encode(), *args, **kw)

def message_from_binary_file(s, *args, **kw):
    f = io.BytesIO(s.encode())
    return email.message_from_binary_file(f, *args, **kw)

class TestBytesParser(TestParserBase, TestEmailBase):
    parsers = (message_from_bytes, message_from_binary_file)


if __name__ == '__main__':
    unittest.main()
