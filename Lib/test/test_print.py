"""Test correct operation of the print function.
"""

import unittest
from test import test_support

import sys
import io
from contextlib import contextmanager

NotDefined = object()

# A dispatch table all 8 combinations of providing
#  sep, end, and file
# I use this machinery so that I'm not just passing default
#  values to print, I'm eiher passing or not passing in the
#  arguments
dispatch = {
    (False, False, False):
     lambda args, sep, end, file: print(*args),
    (False, False, True):
     lambda args, sep, end, file: print(file=file, *args),
    (False, True,  False):
     lambda args, sep, end, file: print(end=end, *args),
    (False, True,  True):
     lambda args, sep, end, file: print(end=end, file=file, *args),
    (True,  False, False):
     lambda args, sep, end, file: print(sep=sep, *args),
    (True,  False, True):
     lambda args, sep, end, file: print(sep=sep, file=file, *args),
    (True,  True,  False):
     lambda args, sep, end, file: print(sep=sep, end=end, *args),
    (True,  True,  True):
     lambda args, sep, end, file: print(sep=sep, end=end, file=file, *args),
    }

@contextmanager
def stdout_redirected(new_stdout):
    save_stdout = sys.stdout
    sys.stdout = new_stdout
    try:
        yield None
    finally:
        sys.stdout = save_stdout

# Class used to test __str__ and print
class ClassWith__str__:
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return self.x

class TestPrint(unittest.TestCase):
    def check(self, expected, args, *,
            sep=NotDefined, end=NotDefined, file=NotDefined):
        # Capture sys.stdout in a StringIO.  Call print with args,
        #  and with sep, end, and file, if they're defined.  Result
        #  must match expected.

        # Look up the actual function to call, based on if sep, end, and file
        #  are defined
        fn = dispatch[(sep is not NotDefined,
                       end is not NotDefined,
                       file is not NotDefined)]

        t = io.StringIO()
        with stdout_redirected(t):
            fn(args, sep, end, file)

        self.assertEqual(t.getvalue(), expected)

    def test_print(self):
        def x(expected, args, *, sep=NotDefined, end=NotDefined):
            # Run the test 2 ways: not using file, and using
            #  file directed to a StringIO

            self.check(expected, args, sep=sep, end=end)

            # When writing to a file, stdout is expected to be empty
            o = io.StringIO()
            self.check('', args, sep=sep, end=end, file=o)

            # And o will contain the expected output
            self.assertEqual(o.getvalue(), expected)

        x('\n', ())
        x('a\n', ('a',))
        x('None\n', (None,))
        x('1 2\n', (1, 2))
        x('1*2\n', (1, 2), sep='*')
        x('1 s', (1, 's'), end='')
        x('a\nb\n', ('a', 'b'), sep='\n')
        x('1.01', (1.0, 1), sep='', end='')
        x('1*a*1.3+', (1, 'a', 1.3), sep='*', end='+')

        x('*\n', (ClassWith__str__('*'),))

def test_main():
    test_support.run_unittest(TestPrint)

if __name__ == "__main__":
    test_main()
