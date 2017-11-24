""" Test suite for the code in msilib """
import os.path
import unittest
from test.support import TESTFN, import_module, unlink
msilib = import_module('msilib')
import msilib.schema


def init_database():
    path = TESTFN + '.msi'
    db = msilib.init_database(
        path,
        msilib.schema,
        'Python Tests',
        'product_code',
        '1.0',
        'PSF',
    )
    return db, path


class MsiDatabaseTestCase(unittest.TestCase):

    def test_view_fetch_returns_none(self):
        db, db_path = init_database()
        properties = []
        view = db.OpenView('SELECT Property, Value FROM Property')
        view.Execute(None)
        while True:
            record = view.Fetch()
            if record is None:
                break
            properties.append(record.GetString(1))
        view.Close()
        self.assertEqual(
            properties,
            [
                'ProductName', 'ProductCode', 'ProductVersion',
                'Manufacturer', 'ProductLanguage',
            ]
        )
        self.addCleanup(unlink, db_path)

    def test_database_open_failed(self):
        with self.assertRaises(msilib.MSIError) as cm:
            msilib.OpenDatabase('non-existent.msi', msilib.MSIDBOPEN_READONLY)
        self.assertEqual(str(cm.exception), 'open failed')

    def test_database_create_failed(self):
        db_path = os.path.join(TESTFN, 'test.msi')
        with self.assertRaises(msilib.MSIError) as cm:
            msilib.OpenDatabase(db_path, msilib.MSIDBOPEN_CREATE)
        self.assertEqual(str(cm.exception), 'create failed')

    def test_get_property_vt_empty(self):
        db, db_path = init_database()
        summary = db.GetSummaryInformation(0)
        self.assertIsNone(summary.GetProperty(msilib.PID_SECURITY))
        del db
        self.addCleanup(unlink, db_path)


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
