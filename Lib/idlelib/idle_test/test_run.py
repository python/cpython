import unittest
from unittest import mock

from test.support import captured_stderr
import idlelib.run as idlerun


class RunTest(unittest.TestCase):
    def test_print_exception_unhashable(self):
        class UnhashableException(Exception):
            def __eq__(self, other):
                return True

        ex1 = UnhashableException('ex1')
        ex2 = UnhashableException('ex2')
        try:
            raise ex2 from ex1
        except UnhashableException:
            try:
                raise ex1
            except UnhashableException:
                with captured_stderr() as output:
                    with mock.patch.object(idlerun,
                                           'cleanup_traceback') as ct:
                        ct.side_effect = lambda t, e: t
                        idlerun.print_exception()

        tb = output.getvalue().strip().splitlines()
        self.assertEqual(11, len(tb))
        self.assertIn('UnhashableException: ex2', tb[3])
        self.assertIn('UnhashableException: ex1', tb[10])


if __name__ == '__main__':
    unittest.main(verbosity=2)
