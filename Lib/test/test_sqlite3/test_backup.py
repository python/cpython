import os
import sqlite3 as sqlite
import tempfile
import threading
import time
import unittest

from test import support
from test.support import threading_helper

from .util import memory_database


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
        with memory_database() as bck:
            bck.close()
            with self.assertRaises(sqlite.ProgrammingError):
                self.cx.backup(bck)

    def test_bad_source_closed_connection(self):
        with memory_database() as bck:
            source = sqlite.connect(":memory:")
            source.close()
            with self.assertRaises(sqlite.ProgrammingError):
                source.backup(bck)

    def test_bad_target_in_transaction(self):
        with memory_database() as bck:
            bck.execute('CREATE TABLE bar (key INTEGER)')
            bck.executemany('INSERT INTO bar (key) VALUES (?)', [(3,), (4,)])
            with self.assertRaises(sqlite.OperationalError) as cm:
                self.cx.backup(bck)

    def test_keyword_only_args(self):
        with self.assertRaises(TypeError):
            with memory_database() as bck:
                self.cx.backup(bck, 1)

    def test_simple(self):
        with memory_database() as bck:
            self.cx.backup(bck)
            self.verify_backup(bck)

    def test_progress(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(status)

        with memory_database() as bck:
            self.cx.backup(bck, pages=1, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 2)
        self.assertEqual(journal[0], sqlite.SQLITE_OK)
        self.assertEqual(journal[1], sqlite.SQLITE_DONE)

    def test_progress_all_pages_at_once_1(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(remaining)

        with memory_database() as bck:
            self.cx.backup(bck, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def test_progress_all_pages_at_once_2(self):
        journal = []

        def progress(status, remaining, total):
            journal.append(remaining)

        with memory_database() as bck:
            self.cx.backup(bck, pages=-1, progress=progress)
            self.verify_backup(bck)

        self.assertEqual(len(journal), 1)
        self.assertEqual(journal[0], 0)

    def test_non_callable_progress(self):
        with self.assertRaises(TypeError) as cm:
            with memory_database() as bck:
                self.cx.backup(bck, pages=1, progress='bar')
        self.assertEqual(str(cm.exception), 'progress argument must be a callable')

    def test_modifying_progress(self):
        journal = []

        def progress(status, remaining, total):
            if not journal:
                self.cx.execute('INSERT INTO foo (key) VALUES (?)', (remaining+1000,))
                self.cx.commit()
            journal.append(remaining)

        with memory_database() as bck:
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
            with memory_database() as bck:
                self.cx.backup(bck, progress=progress)
        self.assertEqual(str(err.exception), 'nearly out of space')

    def test_database_source_name(self):
        with memory_database() as bck:
            self.cx.backup(bck, name='main')
        with memory_database() as bck:
            self.cx.backup(bck, name='temp')
        with self.assertRaises(sqlite.OperationalError) as cm:
            with memory_database() as bck:
                self.cx.backup(bck, name='non-existing')
        self.assertIn("unknown database", str(cm.exception))

        self.cx.execute("ATTACH DATABASE ':memory:' AS attached_db")
        self.cx.execute('CREATE TABLE attached_db.foo (key INTEGER)')
        self.cx.executemany('INSERT INTO attached_db.foo (key) VALUES (?)', [(3,), (4,)])
        self.cx.commit()
        with memory_database() as bck:
            self.cx.backup(bck, name='attached_db')
            self.verify_backup(bck)


@threading_helper.requires_working_threading()
class BackupTimeoutTests(unittest.TestCase):
    """Tests for the optional bounded retry window of Connection.backup()."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.src_path = os.path.join(self.tmp.name, "src.db")
        self.dst_path = os.path.join(self.tmp.name, "dst.db")
        self.holder = None
        self._workers = []
        self._conns = []

    def tearDown(self):
        # A worker that outlived its join guard must never have a connection
        # closed underneath it.  Release the lock and give it one more chance
        # to finish; only close connections once no worker is still running.
        for thread in self._workers:
            if thread.is_alive() and self.holder is not None:
                try:
                    self.holder.rollback()
                except sqlite.Error:
                    pass
        for thread in self._workers:
            if thread.is_alive():
                thread.join(support.SHORT_TIMEOUT)
        if any(thread.is_alive() for thread in self._workers):
            return
        if self.holder is not None:
            try:
                self.holder.rollback()
            except sqlite.Error:
                pass
            self.holder.close()
        for cx in self._conns:
            cx.close()

    def _create_source(self, rows=500):
        cx = sqlite.connect(self.src_path)
        cx.execute("CREATE TABLE t(x)")
        cx.executemany("INSERT INTO t VALUES (?)", [(i,) for i in range(rows)])
        cx.commit()
        cx.close()

    def _lock_source(self, rows=500):
        self._create_source(rows)
        self.holder = sqlite.connect(self.src_path, timeout=0.1)
        self.holder.execute("BEGIN EXCLUSIVE")
        self.holder.execute("INSERT INTO t VALUES (9999)")

    def _connections(self):
        # backup() runs in a worker thread (see _run_backup), so allow the
        # connection to be used there.  The lock holder stays in this thread.
        src = sqlite.connect(self.src_path, timeout=0.1, check_same_thread=False)
        dst = sqlite.connect(self.dst_path, check_same_thread=False)
        self._conns.extend((src, dst))
        return src, dst

    def _run_backup(self, src, dst, join_timeout=None, **kwargs):
        """Run backup() in a guarded thread; never let a bug hang the suite."""
        if join_timeout is None:
            join_timeout = support.SHORT_TIMEOUT
        outcome = []

        def worker():
            try:
                src.backup(dst, **kwargs)
                outcome.append(("ok", None))
            except BaseException as exc:
                outcome.append(("exc", exc))

        thread = threading.Thread(target=worker, daemon=True)
        self._workers.append(thread)
        thread.start()
        thread.join(join_timeout)
        first_join_timed_out = thread.is_alive()
        if thread.is_alive():
            # Release the lock so a legitimate retry can finish, then wait
            # again before touching any connection owned by the worker.
            if self.holder is not None:
                try:
                    self.holder.rollback()
                except sqlite.Error:
                    pass
            thread.join(support.SHORT_TIMEOUT)
        if thread.is_alive():
            self.fail("backup() did not return within the guard timeout")
        self.assertEqual(len(outcome), 1)
        if first_join_timed_out:
            self.fail(
                "backup() exceeded its guard timeout; releasing the lock "
                "during cleanup must not turn this into a pass"
            )
        return outcome[0]

    def test_timeout_is_keyword_only(self):
        self._create_source()
        src, dst = self._connections()
        with self.assertRaises(TypeError):
            src.backup(dst, 1, -1, "main", 0.25, -1.0)

    def test_timeout_rejects_invalid_values(self):
        self._create_source()
        for bad in (-2.0, -0.5, float("nan"), float("inf"), float("-inf")):
            with self.subTest(timeout=bad):
                src, dst = self._connections()
                with self.assertRaises(ValueError):
                    src.backup(dst, timeout=bad)

    def test_timeout_minus_one_keeps_default_behaviour(self):
        self._create_source()
        src, dst = self._connections()
        src.backup(dst, timeout=-1.0)
        self.assertEqual(dst.execute("SELECT count(*) FROM t").fetchone()[0], 500)

    def test_does_not_time_out_without_lock_conflict(self):
        # A small timeout must not interrupt a healthy multi-page backup:
        # the retry budget only starts on the first lock conflict.
        self._create_source()
        src, dst = self._connections()
        src.backup(dst, timeout=0.000001, pages=1)
        self.assertEqual(dst.execute("SELECT count(*) FROM t").fetchone()[0], 500)

    def test_bounded_retry_reports_lock_error(self):
        self._lock_source()
        src, dst = self._connections()
        kind, exc = self._run_backup(src, dst, timeout=0.2, sleep=0.01)
        self.assertEqual(kind, "exc")
        self.assertIsInstance(exc, sqlite.OperationalError)
        busy = getattr(sqlite, "SQLITE_BUSY", 5)
        locked = getattr(sqlite, "SQLITE_LOCKED", 6)
        self.assertIn(exc.sqlite_errorcode, (busy, locked))
        self.assertIsInstance(exc.sqlite_errorname, str)

    def test_timeout_zero_returns_on_first_conflict(self):
        self._lock_source()
        src, dst = self._connections()
        callbacks = []

        def progress(status, remaining, total):
            callbacks.append(status)

        kind, exc = self._run_backup(
            src, dst, timeout=0.0, sleep=0.01, progress=progress)
        self.assertEqual(kind, "exc")
        self.assertIsInstance(exc, sqlite.OperationalError)
        # Zero budget allows the first step, then stops on the first conflict.
        self.assertEqual(len(callbacks), 1, callbacks)
        self.assertIn(callbacks[0], (sqlite.SQLITE_BUSY, sqlite.SQLITE_LOCKED))
        busy = getattr(sqlite, "SQLITE_BUSY", 5)
        locked = getattr(sqlite, "SQLITE_LOCKED", 6)
        self.assertIn(exc.sqlite_errorcode, (busy, locked))

    def test_sleep_is_clipped_to_remaining_budget(self):
        # An oversized sleep must not extend the retry window: the sleep is
        # clipped to the remaining budget and the deadline still wins.
        self._lock_source()
        src, dst = self._connections()
        kind, exc = self._run_backup(
            src, dst, join_timeout=4.0, timeout=0.2, sleep=10.0
        )
        self.assertEqual(kind, "exc")
        self.assertIsInstance(exc, sqlite.OperationalError)

    def test_progress_exception_is_preserved(self):
        self._lock_source()
        src, dst = self._connections()

        def progress(status, remaining, total):
            raise SystemError("progress boom")

        kind, exc = self._run_backup(
            src, dst, timeout=0.5, sleep=0.01, progress=progress
        )
        self.assertEqual(kind, "exc")
        self.assertIsInstance(exc, SystemError)
        self.assertEqual(str(exc), "progress boom")

    def test_completed_backup_is_not_reported_as_timeout(self):
        # The deadline is checked only on lock conflicts; reaching SQLITE_DONE
        # succeeds even if the (unstarted) budget would be tiny.
        self._create_source(rows=2000)
        src, dst = self._connections()
        seen = []

        def progress(status, remaining, total):
            seen.append(status)

        src.backup(dst, timeout=0.000001, pages=1, progress=progress)
        self.assertEqual(seen[-1], sqlite.SQLITE_DONE)
        self.assertEqual(dst.execute("SELECT count(*) FROM t").fetchone()[0], 2000)

    def test_connections_reusable_after_timeout(self):
        self._lock_source()
        src, dst = self._connections()
        kind, exc = self._run_backup(src, dst, timeout=0.2, sleep=0.01)
        self.assertEqual(kind, "exc")
        self.holder.rollback()
        src.backup(dst)
        self.assertEqual(dst.execute("SELECT count(*) FROM t").fetchone()[0], 500)

    def test_deadline_counts_progress_callback_time(self):
        # F-01 regression.  The first callback deliberately consumes twice
        # the whole retry budget.  Since the deadline is armed at conflict
        # detection, no second backup_step() may follow.  The evidence is the
        # recorded callback duration, not a guess about step timing.
        self._lock_source()
        src, dst = self._connections()
        timeout = 0.2
        events = []

        def progress(status, remaining, total):
            start = time.monotonic()
            events.append((status, start))
            if status == sqlite.SQLITE_BUSY:
                time.sleep(timeout * 2)
                events.append(("slept", time.monotonic() - start))

        kind, exc = self._run_backup(
            src, dst, join_timeout=8.0, timeout=timeout, sleep=0.001,
            progress=progress)
        self.assertEqual(kind, "exc")
        self.assertIsInstance(exc, sqlite.OperationalError)
        busy = [t for s, t in events if s == sqlite.SQLITE_BUSY]
        self.assertEqual(len(busy), 1, events)
        slept = [d for s, d in events if s == "slept"]
        self.assertEqual(len(slept), 1, events)
        self.assertGreater(slept[0], timeout)

    def test_lock_window_restarts_after_sqlite_ok(self):
        # P-2 regression: BUSY -> OK -> BUSY must open a new consecutive
        # window.  The callback outlives the original budget while the window
        # is closed by OK, so a stale deadline would abort before the second
        # BUSY callback could run.  Uses real SQLite locking only.
        self._create_source(rows=40)
        src, dst = self._connections()
        holder = sqlite.connect(self.src_path, timeout=0.01,
                                 check_same_thread=False)
        self.holder = holder
        holder.execute("BEGIN EXCLUSIVE")
        holder.execute("INSERT INTO t VALUES (9999)")

        timeout = 0.5
        phase = [0]
        busy_times = []
        relocked = []

        def progress(status, remaining, total):
            now = time.monotonic()
            if status == sqlite.SQLITE_BUSY:
                busy_times.append(now)
            if status == sqlite.SQLITE_BUSY and phase[0] == 0:
                phase[0] = 1
                holder.rollback()
            elif status == sqlite.SQLITE_OK and phase[0] == 1:
                phase[0] = 2
                time.sleep(timeout + 0.15)
                holder.execute("BEGIN EXCLUSIVE")
                holder.execute("UPDATE t SET x=x WHERE 0")
                relocked.append(time.monotonic())
            elif status == sqlite.SQLITE_BUSY and phase[0] == 2:
                phase[0] = 3
                time.sleep(0.05)
                holder.rollback()
            if len(busy_times) > 5:
                raise RuntimeError("unexpected repeated lock conflicts")

        kind, exc = self._run_backup(
            src, dst, join_timeout=10.0, timeout=timeout, sleep=0.01,
            pages=1, progress=progress)
        self.assertEqual(kind, "ok")
        self.assertEqual(len(busy_times), 2)
        self.assertEqual(len(relocked), 1)
        self.assertGreater(relocked[0] - busy_times[0], timeout)
        self.assertEqual(dst.execute("SELECT count(*) FROM t").fetchone()[0], 40)


if __name__ == "__main__":
    unittest.main()
