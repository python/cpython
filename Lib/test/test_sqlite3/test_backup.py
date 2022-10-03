import sqlite3 as sqlite
import unittest


class BackupTests(unittest.TestCase):
    def setUp(self):
        cx = self.cx = sqlite.connect(":memory:")
        cx.execute('CREATE TABLE foo (key INTEGER)')
        cx.executemany('INSERT INTO foo (key) VALUES (?)', [(3,), (4,)])
        cx.commit()

    def tearDown(self):
        self.cx.close()

    def verify_backup(self, bckcx):
        result = bckcx.execute("SELECT key FROM foo ORDER BY key").fetchall()
        self.assertEqual(result[0][0], 3)
        self.assertEqual(result[1][0], 4)

    def test_bad_target(self):
        with self.assertRaises(TypeError):
            self.cx.backup(None)
        with self.assertRaises(TypeError):
            self.cx.backup()

    def test_bad_target_filename(self):
        with self.assertRaises(TypeError):
            self.cx.backup('some_file_name.db')

    def test_bad_target_same_connection(self):
        with self.assertRaises(ValueError):
            self.cx.backup(self.cx)

    def test_bad_target_closed_connection(self):
        bck = sqlite.connect(':memory:')
        bck.close()
        with self.assertRaises(sqlite.ProgrammingError):
            self.cx.backup(bck)

    def test_bad_source_closed_connection(self):
        bck = sqlite.connect(':memory:')
        source = sqlite.connect(":memory:")
        source.close()
        with self.assertRaises(sqlite.ProgrammingError):
            source.backup(bck)

    def test_bad_target_in_transaction(self):
        bck = sqlite.connect(':memory:')
        bck.execute('CREATE TABLE bar (key INTEGER)')
        bck.executemany('INSERT INTO bar (key) VALUES (?)', [(3,), (4,)])
        with self.assertRaises(sqlite.OperationalError) as cm:
            self.cx.backup(bck)
        if sqlite.sqlite_version_info < (3, 8, 8):
            self.assertEqual(str(cm.exception), 'target is in transaction')

    def test_keyword_only_args(self):
        with self.assertRaises(TypeError):
            with sqlite.connect(':memory:') as bck:
                self.cx.backup(bck, 1)

    def test_simple(self):
        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck)
            self.verify_backup(bck)

    def test_progress(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(status)

        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, pages=1, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 2)
        self.assertEqual(journal[0], sqlite.SQLITE_OK)
        self.assertEqual(journal[1], sqlite.SQLITE_DONE)

    def test_progress_all_pages_at_once_1(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(remaining)

        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def test_progress_all_pages_at_once_2(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(remaining)

        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, pages=-1, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def test_non_callable_progress(self):
        with self.assertRaises(TypeError) as cm:
            with sqlite.connect(':memory:') as bck:
                self.cx.backup(bck, pages=1, progress='bar')
        self.assertEqual(str(cm.exception), 'progress argument must be a callable')

    def test_modifying_progress(self):
        journal = []

        def progress(status, remaining, total):
            if not journal:
                self.cx.execute('INSERT INTO foo (key) VALUES (?)', (remaining+1000,))
                self.cx.commit()
            journal.append(remaining)

        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, pages=1, progress=progress)
            self.verify_backup(bck)

            result = bck.execute("SELECT key FROM foo"
                                 " WHERE key >= 1000"
                                 " ORDER BY key").fetchall()
            self.assertEqual(result[0][0], 1001)

        self.assertEqual(len(journal), 3)
        self.assertEqual(journal[0], 1)
        self.assertEqual(journal[1], 1)
        self.assertEqual(journal[2], 0)

    def test_failing_progress(self):
        def progress(status, remaining, total):
            raise SystemError('nearly out of space')

        with self.assertRaises(SystemError) as err:
            with sqlite.connect(':memory:') as bck:
                self.cx.backup(bck, progress=progress)
        self.assertEqual(str(err.exception), 'nearly out of space')

    def test_database_source_name(self):
        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, name='main')
        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, name='temp')
        with self.assertRaises(sqlite.OperationalError) as cm:
            with sqlite.connect(':memory:') as bck:
                self.cx.backup(bck, name='non-existing')
        self.assertIn("unknown database", str(cm.exception))

        self.cx.execute("ATTACH DATABASE ':memory:' AS attached_db")
        self.cx.execute('CREATE TABLE attached_db.foo (key INTEGER)')
        self.cx.executemany('INSERT INTO attached_db.foo (key) VALUES (?)', [(3,), (4,)])
        self.cx.commit()
        with sqlite.connect(':memory:') as bck:
            self.cx.backup(bck, name='attached_db')
            self.verify_backup(bck)


if __name__ == "__main__":
    unittest.main()
