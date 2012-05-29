import unittest
import textwrap
import copy
import pickle
import email
import email.message
from email import policy
from email.headerregistry import HeaderRegistry
from test.test_email import TestEmailBase

class TestPickleCopyHeader(TestEmailBase):

    header_factory = HeaderRegistry()

    unstructured = header_factory('subject', 'this is a test')

    def _test_deepcopy(self, name, value):
        header = self.header_factory(name, value)
        h = copy.deepcopy(header)
        self.assertEqual(str(h), str(header))

    def _test_pickle(self, name, value):
        header = self.header_factory(name, value)
        p = pickle.dumps(header)
        h = pickle.loads(p)
        self.assertEqual(str(h), str(header))

    headers = (
        ('subject', 'this is a test'),
        ('from',    'frodo@mordor.net'),
        ('to',      'a: k@b.com, y@z.com;, j@f.com'),
        ('date',    'Tue, 29 May 2012 09:24:26 +1000'),
        )

    for header in headers:
        locals()['test_deepcopy_'+header[0]] = (
            lambda self, header=header:
                self._test_deepcopy(*header))

    for header in headers:
        locals()['test_pickle_'+header[0]] = (
            lambda self, header=header:
                self._test_pickle(*header))


class TestPickleCopyMessage(TestEmailBase):

    msgs = {}

    # Note: there will be no custom header objects in the parsed message.
    msgs['parsed'] = email.message_from_string(textwrap.dedent("""\
        Date: Tue, 29 May 2012 09:24:26 +1000
        From: frodo@mordor.net
        To: bilbo@underhill.org
        Subject: help

        I think I forgot the ring.
        """), policy=policy.default)

    msgs['created'] = email.message.Message(policy=policy.default)
    msgs['created']['Date'] = 'Tue, 29 May 2012 09:24:26 +1000'
    msgs['created']['From'] = 'frodo@mordor.net'
    msgs['created']['To'] = 'bilbo@underhill.org'
    msgs['created']['Subject'] = 'help'
    msgs['created'].set_payload('I think I forgot the ring.')

    def _test_deepcopy(self, msg):
        msg2 = copy.deepcopy(msg)
        self.assertEqual(msg2.as_string(), msg.as_string())

    def _test_pickle(self, msg):
        p = pickle.dumps(msg)
        msg2 = pickle.loads(p)
        self.assertEqual(msg2.as_string(), msg.as_string())

    for name, msg in msgs.items():
        locals()['test_deepcopy_'+name] = (
            lambda self, msg=msg:
                self._test_deepcopy(msg))

    for name, msg in msgs.items():
        locals()['test_pickle_'+name] = (
            lambda self, msg=msg:
                self._test_pickle(msg))


if __name__ == '__main__':
    unittest.main()
