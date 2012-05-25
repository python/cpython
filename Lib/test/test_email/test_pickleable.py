import unittest
import textwrap
import copy
import pickle
from email import policy
from email import message_from_string
from email._headerregistry import HeaderRegistry
from test.test_email import TestEmailBase

class TestPickleCopyHeader(TestEmailBase):

    unstructured = HeaderRegistry()('subject', 'this is a test')

    def test_deepcopy_unstructured(self):
        h = copy.deepcopy(self.unstructured)
        self.assertEqual(str(h), str(self.unstructured))

    def test_pickle_unstructured(self):
        p = pickle.dumps(self.unstructured)
        h = pickle.loads(p)
        self.assertEqual(str(h), str(self.unstructured))

    address = HeaderRegistry()('from', 'frodo@mordor.net')

    def test_deepcopy_address(self):
        h = copy.deepcopy(self.address)
        self.assertEqual(str(h), str(self.address))

    def test_pickle_address(self):
        p = pickle.dumps(self.address)
        h = pickle.loads(p)
        self.assertEqual(str(h), str(self.address))


class TestPickleCopyMessage(TestEmailBase):

    testmsg = message_from_string(textwrap.dedent("""\
            From: frodo@mordor.net
            To: bilbo@underhill.org
            Subject: help

            I think I forgot the ring.
            """), policy=policy.default)

    def test_deepcopy(self):
        msg2 = copy.deepcopy(self.testmsg)
        self.assertEqual(msg2.as_string(), self.testmsg.as_string())

    def test_pickle(self):
        p = pickle.dumps(self.testmsg)
        msg2 = pickle.loads(p)
        self.assertEqual(msg2.as_string(), self.testmsg.as_string())



if __name__ == '__main__':
    unittest.main()
