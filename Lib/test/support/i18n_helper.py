import re
import subprocess
import sys
import unittest
from pathlib import Path
from test.support import REPO_ROOT, TEST_HOME_DIR, requires_subprocess
from test.test_tools import skip_if_missing


pygettext = Path(REPO_ROOT) / 'Tools' / 'i18n' / 'pygettext.py'

msgid_pattern = re.compile(r'msgid(.*?)(?:msgid_plural|msgctxt|msgstr)',
                           re.DOTALL)
msgid_string_pattern = re.compile(r'"((?:\\"|[^"])*)"')


def _generate_po_file(path, *, stdout_only=True):
    res = subprocess.run([sys.executable, pygettext,
                          '--no-location', '-o', '-', path],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         text=True)
    if stdout_only:
        return res.stdout
    return res


def _extract_msgids(po):
    msgids = []
    for msgid in msgid_pattern.findall(po):
        msgid_string = ''.join(msgid_string_pattern.findall(msgid))
        msgid_string = msgid_string.replace(r'\"', '"')
        if msgid_string:
            msgids.append(msgid_string)
    return sorted(msgids)


def _get_snapshot_path(module_name):
    return Path(TEST_HOME_DIR) / 'translationdata' / module_name / 'msgids.txt'


@requires_subprocess()
class TestTranslationsBase(unittest.TestCase):

    def assertMsgidsEqual(self, module):
        '''Assert that msgids extracted from a given module match a
        snapshot.

        '''
        skip_if_missing('i18n')
        res = _generate_po_file(module.__file__, stdout_only=False)
        self.assertEqual(res.returncode, 0)
        self.assertEqual(res.stderr, '')
        msgids = _extract_msgids(res.stdout)
        snapshot_path = _get_snapshot_path(module.__name__)
        snapshot = snapshot_path.read_text().splitlines()
        self.assertListEqual(msgids, snapshot)


def update_translation_snapshots(module):
    contents = _generate_po_file(module.__file__)
    msgids = _extract_msgids(contents)
    snapshot_path = _get_snapshot_path(module.__name__)
    snapshot_path.write_text('\n'.join(msgids))
