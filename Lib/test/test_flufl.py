import __future__
import unittest
import sys
from test import support


@support.skip_if_new_parser("Not supported by pegen yet")
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
        self.assertEqual(cm.exception.text, '2 != 3\n')
        self.assertEqual(cm.exception.filename, '<FLUFL test>')
        self.assertEqual(cm.exception.lineno, 2)
        self.assertEqual(cm.exception.offset, 4)

    def test_guido_as_bdfl(self):
        code = '2 {0} 3'
        compile(code.format('!='), '<BDFL test>', 'exec')
        with self.assertRaises(SyntaxError) as cm:
            compile(code.format('<>'), '<FLUFL test>', 'exec')
        self.assertRegex(str(cm.exception), "invalid syntax")
        self.assertEqual(cm.exception.text, '2 <> 3\n')
        self.assertEqual(cm.exception.filename, '<FLUFL test>')
        self.assertEqual(cm.exception.lineno, 1)
        self.assertEqual(cm.exception.offset, 4)


if __name__ == '__main__':
    unittest.main()
