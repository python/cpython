import unittest
import io
import contextlib
import doctest

class Test(unittest.TestCase):
    def test_syntax_error(self):
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            try:
                doctest.run_docstring_examples("""
                                             >>> 1 1
                                             """, globals())
            except Exception as e:
                print('!!!!!', str(e))
        self.assertIn('''
        1 1
        ^^^
    SyntaxError: invalid syntax. Perhaps you forgot a comma?''',
        f.getvalue())

