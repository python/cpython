import argparse
import re
import subprocess
import sys
import unittest

from pathlib import Path


i18n_tools = Path(__file__).parents[3] / 'Tools' / 'i18n'
pygettext = i18n_tools / 'pygettext.py'
snapshot_path = Path(__file__).parents[0] / 'data' / 'msgids.txt'

msgid_pattern = re.compile(r'msgid(.*?)(?:msgid_plural|msgctxt|msgstr)', re.DOTALL)
msgid_string_pattern = re.compile(r'"((?:\\"|[^"])*)"')


class TestTranslations(unittest.TestCase):

    def test_translations(self):
        # Test messages extracted from the argparse module against a snapshot
        res = generate_po_file(stdout_only=False)
        self.assertEqual(res.returncode, 0)
        self.assertEqual(res.stderr, '')
        msgids = extract_msgids(res.stdout)
        snapshot = snapshot_path.read_text().splitlines()
        self.assertListEqual(msgids, snapshot)


def generate_po_file(*, stdout_only=True):
    res = subprocess.run([sys.executable, pygettext,
                          '--no-location', '-o', '-', argparse.__file__],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if stdout_only:
        return res.stdout
    return res


def extract_msgids(po):
    msgids = []
    for msgid in msgid_pattern.findall(po):
        msgid_string = ''.join(msgid_string_pattern.findall(msgid))
        msgid_string = msgid_string.replace(r'\"', '"')
        if msgid_string:
            msgids.append(msgid_string)
    return sorted(msgids)


def update_translation_snapshots():
    contents = generate_po_file()
    msgids = extract_msgids(contents)
    snapshot_path.write_text('\n'.join(msgids))


if __name__ == '__main__':
    # To regenerate translation snapshots
    if len(sys.argv) > 1 and sys.argv[1] == '--snapshot-update':
        update_translation_snapshots()
        sys.exit(0)
    unittest.main()
