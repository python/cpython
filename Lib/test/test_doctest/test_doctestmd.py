import unittest
import doctest

def dummyfunction_codeblocks():
    """
    A dummy function that uses codeblocks in the examples.

    It is used like this:
    ```
    >>> 1 == 1
    True
    ```
    """
    pass

def dummyfunction_multiplecodeblocks():
    """
    A dummy function that uses codeblocks in the examples.

    It is used like this:
    ```
    >>> 1 == 1
    True
    ```

    Or like this:
    ```
    >>> 1 + 2
    3
    ```
    """
    pass

class TestMarkdownDocstring(unittest.TestCase):
    def test_DoctestRunner_codeblocks(self):
        test = doctest.DocTestFinder().find(dummyfunction_codeblocks)[0]
        results = doctest.DocTestRunner().run(test)
        self.assertEqual(results, (0,1))

    def test_DoctestRunner_multiplecodeblocks(self):
        test = doctest.DocTestFinder().find(dummyfunction_multiplecodeblocks)[0]
        results = doctest.DocTestRunner().run(test)
        self.assertEqual(results, (0,2))
