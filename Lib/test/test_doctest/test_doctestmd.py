"""Test that doctest processes strings where examples are enclosed in
markdown-style codeblocks.
"""

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

mdfile = r'''
# A Markdown file

Some documentation in an md file which contains codeblocks like this:

```pycon
>>> 1 == 1
True
```

and another one like this:

```pycon
>>> 1 + 2
3
```

and sometimes with multiple cases in one codeblock
```pycon
>>> 2 == 2
True

>>> 2 + 2
4
```

It would be nice if doctest could test them too ...
'''

class TestMarkdownDocstring(unittest.TestCase):
    """Test DocTestFinder processes docstrings including markup codeblocks"""

    def test_DocTestFinder_codeblocks(self):
        """A single codeblock in the docstring"""

        # DocTestFinder returns a list of tests, we only need the first
        test = doctest.DocTestFinder().find(dummyfunction_codeblocks)[0]
        results = doctest.DocTestRunner().run(test)
        self.assertEqual(results, (0,1))

    def test_DocTestFinder_multiplecodeblocks(self):
        """Multiple codeblocks in the docstring"""

        # DocTestFinder returns a list of tests, we only need the first
        test = doctest.DocTestFinder().find(dummyfunction_multiplecodeblocks)[0]
        results = doctest.DocTestRunner().run(test)
        self.assertEqual(results, (0,2))

class TestMarkdownFile(unittest.TestCase):
    """Test DocTestParser processes markdown files"""

    def test_DocTestParser_getdoctest(self):
        parser = doctest.DocTestParser()
        tests = parser.get_doctest(
            mdfile,
            globs=dict(),
            name="mdfile",
            filename=None,
            lineno=None,
        )
        results = doctest.DocTestRunner().run(tests)
        self.assertEqual(results, (0,4))
