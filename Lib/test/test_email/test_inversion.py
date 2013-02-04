"""Test the parser and generator are inverses.

Note that this is only strictly true if we are parsing RFC valid messages and
producing RFC valid messages.
"""

import io
import unittest
from email import policy, message_from_bytes
from email.generator import BytesGenerator
from test.test_email import TestEmailBase, parameterize

# This is like textwrap.dedent for bytes, except that it uses \r\n for the line
# separators on the rebuilt string.
def dedent(bstr):
    lines = bstr.splitlines()
    if not lines[0].strip():
        raise ValueError("First line must contain text")
    stripamt = len(lines[0]) - len(lines[0].lstrip())
    return b'\r\n'.join(
        [x[stripamt:] if len(x)>=stripamt else b''
            for x in lines])


@parameterize
class TestInversion(TestEmailBase, unittest.TestCase):

    def msg_as_input(self, msg):
        m = message_from_bytes(msg, policy=policy.SMTP)
        b = io.BytesIO()
        g = BytesGenerator(b)
        g.flatten(m)
        self.assertEqual(b.getvalue(), msg)

    # XXX: spaces are not preserved correctly here yet in the general case.
    msg_params = {
        'header_with_one_space_body': (dedent(b"""\
            From: abc@xyz.com
            X-Status:\x20
            Subject: test

            foo
            """),),

            }
