import io
import email
import unittest
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


if __name__ == '__main__':
    unittest.main()
