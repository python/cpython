""" Test suite for the code in msilib """
import pathlib
import unittest
from test.support import TESTFN, import_module, unlink
msilib = import_module('msilib')
import msilib.schema


def initialize_db():
    path = pathlib.Path(TESTFN) / 'test.msi'
    db = msilib.init_database(
        path, msilib.schema, 'Python Tests', 'product_code', '1.0', 'PSF',
    )
    return db, path


class MsiTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.db, cls.db_path = initialize_db()

    @classmethod
    def tearDownClass(cls):
        unlink(cls.db_path)

    def test_view_fetch(self):
        properties = []
        view = self.db.OpenView('SELECT Value FROM Property')
        view.Execute(None)
        while True:
            cur_record = view.Fetch()
            if cur_record is None:
                break
            properties.append(cur_record.GetString(0))
        self.assertEqual(
            properties,
            ['ProductName', 'ProductCode', 'ProductVersion',
             'Manufacturer', 'ProductLanguage']
        )


class Test_make_id(unittest.TestCase):
    #http://msdn.microsoft.com/en-us/library/aa369212(v=vs.85).aspx
    """The Identifier data type is a text string. Identifiers may contain the
    ASCII characters A-Z (a-z), digits, underscores (_), or periods (.).
    However, every identifier must begin with either a letter or an
    underscore.
    """

    def test_is_no_change_required(self):
        self.assertEqual(
            msilib.make_id("short"), "short")
        self.assertEqual(
            msilib.make_id("nochangerequired"), "nochangerequired")
        self.assertEqual(
            msilib.make_id("one.dot"), "one.dot")
        self.assertEqual(
            msilib.make_id("_"), "_")
        self.assertEqual(
            msilib.make_id("a"), "a")
        #self.assertEqual(
        #    msilib.make_id(""), "")

    def test_invalid_first_char(self):
        self.assertEqual(
            msilib.make_id("9.short"), "_9.short")
        self.assertEqual(
            msilib.make_id(".short"), "_.short")

    def test_invalid_any_char(self):
        self.assertEqual(
            msilib.make_id(".s\x82ort"), "_.s_ort")
        self.assertEqual    (
            msilib.make_id(".s\x82o?*+rt"), "_.s_o___rt")


if __name__ == '__main__':
    unittest.main()
