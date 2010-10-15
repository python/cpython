"""
Tests for the html module functions.
"""

import html
import unittest
from test.support import run_unittest


class HtmlTests(unittest.TestCase):
    def test_escape(self):
        self.assertEqual(
            html.escape('\'<script>"&foo;"</script>\''),
            '&#x27;&lt;script&gt;&quot;&amp;foo;&quot;&lt;/script&gt;&#x27;')
        self.assertEqual(
            html.escape('\'<script>"&foo;"</script>\'', False),
            '\'&lt;script&gt;"&amp;foo;"&lt;/script&gt;\'')


def test_main():
    run_unittest(HtmlTests)

if __name__ == '__main__':
    test_main()
