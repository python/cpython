"""Regression tests for the Sphinx extensions used by the Docs job."""

from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent / 'extensions'))

import pyspecific
import suspicious


class IssueRoleTests(unittest.TestCase):
    def test_roundup_link(self):
        nodes, messages = pyspecific.issue_role('issue', '', '12345', 1, None)
        self.assertEqual(messages, [])
        self.assertEqual(nodes[0].astext(), 'bpo-12345')
        self.assertEqual(nodes[0]['refuri'], 'https://bugs.python.org/issue12345')

    def test_github_link(self):
        nodes, messages = pyspecific.gh_issue_role('gh', '', '133767', 1, None)
        self.assertEqual(messages, [])
        self.assertEqual(nodes[0].astext(), 'gh-133767')
        self.assertEqual(nodes[0]['refuri'],
                         'https://github.com/python/cpython/issues/133767')


class SuspiciousLoggingTests(unittest.TestCase):
    def setUp(self):
        self.builder = object.__new__(suspicious.CheckSuspiciousMarkupBuilder)
        self.builder.logger = mock.Mock()
        self.builder.app = SimpleNamespace(statuscode=0)
        self.builder.docname = 'test-document'
        self.builder.any_issue = False
        self.builder.write_log_entry = mock.Mock()

    def test_detected_markup_still_fails(self):
        self.builder.report_issue('Leaked :role markup', 12, ':role')
        self.assertEqual(self.builder.app.statuscode, 1)
        self.builder.logger.info.assert_called_once_with('')
        self.builder.logger.warning.assert_called_once_with(
            '[test-document:12] ":role" found in "Leaked :role markup"')
        self.builder.write_log_entry.assert_called_once_with(
            12, ':role', 'Leaked :role markup')

    def test_unused_ignore_rules_still_warn(self):
        self.builder.rules = [suspicious.Rule('test-document', None, '::', 'example')]
        self.builder.finish()
        self.builder.logger.warning.assert_called_once_with(
            'Found 1/1 unused rules:')


if __name__ == '__main__':
    unittest.main()
