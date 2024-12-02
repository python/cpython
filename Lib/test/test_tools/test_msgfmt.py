import unittest
import os
import tempfile
from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("i18n")
with imports_under_tool("i18n"):
    from msgfmt import make


class TestMsgfmt(unittest.TestCase):

    def setUp(self):
        self.test_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.test_dir.cleanup)

    def create_po_file(self, content):
        po_file_path = os.path.join(self.test_dir.name, "test.po")
        with open(po_file_path, "w", encoding="utf-8") as f:
            f.write(content)
        return po_file_path

    def test_make_creates_mo_file(self):
        po_content = """
        msgid ""
        msgstr ""
        "Content-Type: text/plain; charset=UTF-8\\n"

        msgid "Hello"
        msgstr "Bonjour"
        """
        po_file = self.create_po_file(po_content)
        mo_file = os.path.splitext(po_file)[0] + ".mo"

        make(po_file, mo_file)

        self.assertTrue(os.path.exists(mo_file))

    def test_make_handles_fuzzy(self):
        po_content = """
        msgid ""
        msgstr ""
        "Content-Type: text/plain; charset=UTF-8\\n"

        #, fuzzy
        msgid "Hello"
        msgstr "Bonjour"
        """
        po_file = self.create_po_file(po_content)
        mo_file = os.path.splitext(po_file)[0] + ".mo"

        make(po_file, mo_file)

        self.assertTrue(os.path.exists(mo_file))

    def test_make_invalid_po_file(self):
        po_content = """
        msgid "Hello"
        msgstr "Bonjour"
        msgid_plural "Hellos"
        """
        po_file = self.create_po_file(po_content)
        mo_file = os.path.splitext(po_file)[0] + ".mo"

        with self.assertRaises(SystemExit):
            make(po_file, mo_file)


if __name__ == "__main__":
    unittest.main()
