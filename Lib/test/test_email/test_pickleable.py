import unittest
import textwrap
import copy
import pickle
import email
import email.message
from email import policy
from email.headerregistry import HeaderRegistry
from test.test_email import TestEmailBase, Parameterized

class TestPickleCopyHeader(TestEmailBase, metaclass=Parameterized):

    header_factory = HeaderRegistry()

    unstructured = header_factory('subject', 'this is a test')

    header_params = {
        'subject': ('subject', 'this is a test'),
        'from':    ('from',    'frodo@mordor.net'),
        'to':      ('to',      'a: k@b.com, y@z.com;, j@f.com'),
        'date':    ('date',    'Tue, 29 May 2012 09:24:26 +1000'),
        }

    def header_as_deepcopy(self, name, value):
        header = self.header_factory(name, value)
        h = copy.deepcopy(header)
        self.assertEqual(str(h), str(header))

    def header_as_pickle(self, name, value):
        header = self.header_factory(name, value)
        p = pickle.dumps(header)
        h = pickle.loads(p)
        self.assertEqual(str(h), str(header))


class TestPickleCopyMessage(TestEmailBase, metaclass=Parameterized):

    # Message objects are a sequence, so we have to make them a one-tuple in
    # msg_params so they get passed to the parameterized test method as a
    # single argument instead of as a list of headers.
    msg_params = {}

    # Note: there will be no custom header objects in the parsed message.
    msg_params['parsed'] = (email.message_from_string(textwrap.dedent("""\
        Date: Tue, 29 May 2012 09:24:26 +1000
        From: frodo@mordor.net
        To: bilbo@underhill.org
        Subject: help

        I think I forgot the ring.
        """), policy=policy.default),)

    msg_params['created'] = (email.message.Message(policy=policy.default),)
    msg_params['created'][0]['Date'] = 'Tue, 29 May 2012 09:24:26 +1000'
    msg_params['created'][0]['From'] = 'frodo@mordor.net'
    msg_params['created'][0]['To'] = 'bilbo@underhill.org'
    msg_params['created'][0]['Subject'] = 'help'
    msg_params['created'][0].set_payload('I think I forgot the ring.')

    def msg_as_deepcopy(self, msg):
        msg2 = copy.deepcopy(msg)
        self.assertEqual(msg2.as_string(), msg.as_string())

    def msg_as_pickle(self, msg):
        p = pickle.dumps(msg)
        msg2 = pickle.loads(p)
        self.assertEqual(msg2.as_string(), msg.as_string())


if __name__ == '__main__':
    unittest.main()
