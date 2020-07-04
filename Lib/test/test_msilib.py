""" Test suite for the code in msilib """
import os
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
        db.Close()
        self.assertEqual(
            properties,
            [
                'ProductName', 'ProductCode', 'ProductVersion',
                'Manufacturer', 'ProductLanguage',
            ]
        )
        self.addCleanup(unlink, db_path)

    def test_view_non_ascii(self):
        db, db_path = init_database()
        view = db.OpenView("SELECT 'ß-розпад' FROM Property")
        view.Execute(None)
        record = view.Fetch()
        self.assertEqual(record.GetString(1), 'ß-розпад')
        view.Close()
        db.Close()
        self.addCleanup(unlink, db_path)

    def test_summaryinfo_getproperty_issue1104(self):
        db, db_path = init_database()
        try:
            sum_info = db.GetSummaryInformation(99)
            title = sum_info.GetProperty(msilib.PID_TITLE)
            self.assertEqual(title, b"Installation Database")

            sum_info.SetProperty(msilib.PID_TITLE, "a" * 999)
            title = sum_info.GetProperty(msilib.PID_TITLE)
            self.assertEqual(title, b"a" * 999)

            sum_info.SetProperty(msilib.PID_TITLE, "a" * 1000)
            title = sum_info.GetProperty(msilib.PID_TITLE)
            self.assertEqual(title, b"a" * 1000)

            sum_info.SetProperty(msilib.PID_TITLE, "a" * 1001)
            title = sum_info.GetProperty(msilib.PID_TITLE)
            self.assertEqual(title, b"a" * 1001)
        finally:
            db = None
            sum_info = None
            os.unlink(db_path)

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
        db.Close()
        self.addCleanup(unlink, db_path)

    def test_directory_start_component_keyfile(self):
        db, db_path = init_database()
        self.addCleanup(unlink, db_path)
        self.addCleanup(db.Close)
        self.addCleanup(msilib._directories.clear)
        feature = msilib.Feature(db, 0, 'Feature', 'A feature', 'Python')
        cab = msilib.CAB('CAB')
        dir = msilib.Directory(db, cab, None, TESTFN, 'TARGETDIR',
                               'SourceDir', 0)
        dir.start_component(None, feature, None, 'keyfile')

    def test_getproperty_uninitialized_var(self):
        db, db_path = init_database()
        self.addCleanup(unlink, db_path)
        self.addCleanup(db.Close)
        si = db.GetSummaryInformation(0)
        with self.assertRaises(msilib.MSIError):
            si.GetProperty(-1)

    def test_FCICreate(self):
        filepath = TESTFN + '.txt'
        cabpath = TESTFN + '.cab'
        self.addCleanup(unlink, filepath)
        with open(filepath, 'wb'):
            pass
        self.addCleanup(unlink, cabpath)
        msilib.FCICreate(cabpath, [(filepath, 'test.txt')])
        self.assertTrue(os.path.isfile(cabpath))


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
        self.assertEqual(
            msilib.make_id(".s\x82o?*+rt"), "_.s_o___rt")


if __name__ == '__main__':
    unittest.main()
