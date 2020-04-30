import __future__
import unittest
from test import support


class FLUFLTests(unittest.TestCase):

    def test_barry_as_bdfl(self):
        code = "from __future__ import barry_as_FLUFL\n2 {0} 3"
        compile(code.format('<>'), '<BDFL test>', 'exec',
                __future__.CO_FUTURE_BARRY_AS_BDFL)
        with self.assertRaises(SyntaxError) as cm:
            compile(code.format('!='), '<FLUFL test>', 'exec',
                    __future__.CO_FUTURE_BARRY_AS_BDFL)
        self.assertRegex(str(cm.exception),
                         "with Barry as BDFL, use '<>' instead of '!='")
        self.assertIn('2 != 3', cm.exception.text)
        self.assertEqual(cm.exception.filename, '<FLUFL test>')

        self.assertTrue(cm.exception.lineno, 2)
        # The old parser reports the end of the token and the new
        # parser reports the start of the token
        self.assertEqual(cm.exception.offset, 4 if support.use_old_parser() else 3)

    def test_guido_as_bdfl(self):
        code = '2 {0} 3'
        compile(code.format('!='), '<BDFL test>', 'exec')
        with self.assertRaises(SyntaxError) as cm:
            compile(code.format('<>'), '<FLUFL test>', 'exec')
        self.assertRegex(str(cm.exception), "invalid syntax")
        self.assertIn('2 <> 3', cm.exception.text)
        self.assertEqual(cm.exception.filename, '<FLUFL test>')
        self.assertEqual(cm.exception.lineno, 1)
        # The old parser reports the end of the token and the new
        # parser reports the start of the token
        self.assertEqual(cm.exception.offset, 4 if support.use_old_parser() else 3)


if __name__ == '__main__':
    unittest.main()
