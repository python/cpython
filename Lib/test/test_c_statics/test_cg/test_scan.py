import unittest

from test.test_c_statics.cg import info
from test.test_c_statics.cg.scan import iter_statics


class IterStaticsTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_typical(self):
        found = list(iter_statics('spam.c'))

        self.assertEqual(found, [
            info.StaticVar('spam.c', None, 'var1', 'int'),
            # ...
            ])
