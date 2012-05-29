import unittest
from email import policy
from test.test_email import TestEmailBase


class Test(TestEmailBase):

    policy = policy.default

    def test_error_on_setitem_if_max_count_exceeded(self):
        m = self._str_msg("")
        m['To'] = 'abc@xyz'
        with self.assertRaises(ValueError):
            m['To'] = 'xyz@abc'


if __name__ == '__main__':
    unittest.main()
