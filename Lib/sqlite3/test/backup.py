import sqlite3 as sqlite
from tempfile import NamedTemporaryFile
import unittest

class BackupTests(unittest.TestCase):
    def setUp(self):
        cx = self.cx = sqlite.connect(":memory:")
        cx.execute('CREATE TABLE foo (key INTEGER)')
        cx.executemany('INSERT INTO foo (key) VALUES (?)', [(3,), (4,)])
        cx.commit()

    def tearDown(self):
        self.cx.close()

    def testBackup(self, bckfn):
        cx = sqlite.connect(bckfn)
        result = cx.execute("SELECT key FROM foo ORDER BY key").fetchall()
        self.assertEqual(result[0][0], 3)
        self.assertEqual(result[1][0], 4)

    def CheckSimple(self):
        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name)
            self.testBackup(bckfn.name)

    def CheckProgress(self):
        journal = []

        def progress(remaining, total):
            journal.append(remaining)

        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, pages=1, progress=progress)
            self.testBackup(bckfn.name)

        self.assertEqual(len(journal), 2)
        self.assertEqual(journal[0], 1)
        self.assertEqual(journal[1], 0)

    def CheckProgressAllPagesAtOnce_0(self):
        journal = []

        def progress(remaining, total):
            journal.append(remaining)

        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, progress=progress)
            self.testBackup(bckfn.name)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def CheckProgressAllPagesAtOnce_1(self):
        journal = []

        def progress(remaining, total):
            journal.append(remaining)

        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, pages=-1, progress=progress)
            self.testBackup(bckfn.name)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def CheckNonCallableProgress(self):
        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            with self.assertRaises(TypeError) as err:
                self.cx.backup(bckfn.name, pages=1, progress='bar')
            self.assertEqual(str(err.exception), 'progress argument must be a callable')

    def CheckModifyingProgress(self):
        journal = []

        def progress(remaining, total):
            if not journal:
                self.cx.execute('INSERT INTO foo (key) VALUES (?)', (remaining+1000,))
                self.cx.commit()
            journal.append(remaining)

        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, pages=1, progress=progress)
            self.testBackup(bckfn.name)

            cx = sqlite.connect(bckfn.name)
            result = cx.execute("SELECT key FROM foo"
                                " WHERE key >= 1000"
                                " ORDER BY key").fetchall()
            self.assertEqual(result[0][0], 1001)

        self.assertEqual(len(journal), 3)
        self.assertEqual(journal[0], 1)
        self.assertEqual(journal[1], 1)
        self.assertEqual(journal[2], 0)

    def CheckFailingProgress(self):
        def progress(remaining, total):
            raise SystemError('nearly out of space')

        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            with self.assertRaises(SystemError) as err:
                self.cx.backup(bckfn.name, progress=progress)
            self.assertEqual(str(err.exception), 'nearly out of space')

    def CheckDatabaseSourceName(self):
        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, name='main')
            self.cx.backup(bckfn.name, name='temp')
            with self.assertRaises(sqlite.OperationalError):
                self.cx.backup(bckfn.name, name='non-existing')
        self.cx.execute("ATTACH DATABASE ':memory:' AS attached_db")
        self.cx.execute('CREATE TABLE attached_db.foo (key INTEGER)')
        self.cx.executemany('INSERT INTO attached_db.foo (key) VALUES (?)', [(3,), (4,)])
        self.cx.commit()
        with NamedTemporaryFile(suffix='.sqlite') as bckfn:
            self.cx.backup(bckfn.name, name='attached_db')
            self.testBackup(bckfn.name)

def suite():
    return unittest.TestSuite(unittest.makeSuite(BackupTests, "Check"))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
