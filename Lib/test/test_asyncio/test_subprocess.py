import os
import signal
import sys
import unittest
import warnings
from unittest import mock

import asyncio
from asyncio import base_subprocess
from asyncio import subprocess
from test.test_asyncio import utils as test_utils
from test import support

if sys.platform != 'win32':
    from asyncio import unix_events

# Program blocking
PROGRAM_BLOCKED = [sys.executable, '-c', 'import time; time.sleep(3600)']

# Program copying input to output
PROGRAM_CAT = [
    sys.executable, '-c',
    ';'.join(('import sys',
              'data = sys.stdin.buffer.read()',
              'sys.stdout.buffer.write(data)'))]


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class TestSubprocessTransport(base_subprocess.BaseSubprocessTransport):
    def _start(self, *args, **kwargs):
        self._proc = mock.Mock()
        self._proc.stdin = None
        self._proc.stdout = None
        self._proc.stderr = None
        self._proc.pid = -1


class SubprocessTransportTests(test_utils.TestCase):
    def setUp(self):
        super().setUp()
        self.loop = self.new_test_loop()
        self.set_event_loop(self.loop)

    def create_transport(self, waiter=None):
        protocol = mock.Mock()
        protocol.connection_made._is_coroutine = False
        protocol.process_exited._is_coroutine = False
        transport = TestSubprocessTransport(
                        self.loop, protocol, ['test'], False,
                        None, None, None, 0, waiter=waiter)
        return (transport, protocol)

    def test_proc_exited(self):
        waiter = self.loop.create_future()
        transport, protocol = self.create_transport(waiter)
        transport._process_exited(6)
        self.loop.run_until_complete(waiter)

        self.assertEqual(transport.get_returncode(), 6)

        self.assertTrue(protocol.connection_made.called)
        self.assertTrue(protocol.process_exited.called)
        self.assertTrue(protocol.connection_lost.called)
        self.assertEqual(protocol.connection_lost.call_args[0], (None,))

        self.assertFalse(transport.is_closing())
        self.assertIsNone(transport._loop)
        self.assertIsNone(transport._proc)
        self.assertIsNone(transport._protocol)

        # methods must raise ProcessLookupError if the process exited
        self.assertRaises(ProcessLookupError,
                          transport.send_signal, signal.SIGTERM)
        self.assertRaises(ProcessLookupError, transport.terminate)
        self.assertRaises(ProcessLookupError, transport.kill)

        transport.close()

    def test_subprocess_repr(self):
        waiter = self.loop.create_future()
        transport, protocol = self.create_transport(waiter)
        transport._process_exited(6)
        self.loop.run_until_complete(waiter)

        self.assertEqual(
            repr(transport),
            "<TestSubprocessTransport pid=-1 returncode=6>"
        )
        transport._returncode = None
        self.assertEqual(
            repr(transport),
            "<TestSubprocessTransport pid=-1 running>"
        )
        transport._pid = None
        transport._returncode = None
        self.assertEqual(
            repr(transport),
            "<TestSubprocessTransport not started>"
        )
        transport.close()


class SubprocessMixin:

    def test_stdin_stdout(self):
        args = PROGRAM_CAT

        async def run(data):
            proc = await asyncio.create_subprocess_exec(
                *args,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
            )

            # feed data
            proc.stdin.write(data)
            await proc.stdin.drain()
            proc.stdin.close()

            # get output and exitcode
            data = await proc.stdout.read()
            exitcode = await proc.wait()
            return (exitcode, data)

        task = run(b'some data')
        task = asyncio.wait_for(task, 60.0)
        exitcode, stdout = self.loop.run_until_complete(task)
        self.assertEqual(exitcode, 0)
        self.assertEqual(stdout, b'some data')

    def test_communicate(self):
        args = PROGRAM_CAT

        async def run(data):
            proc = await asyncio.create_subprocess_exec(
                *args,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
            )
            stdout, stderr = await proc.communicate(data)
            return proc.returncode, stdout

        task = run(b'some data')
        task = asyncio.wait_for(task, support.LONG_TIMEOUT)
        exitcode, stdout = self.loop.run_until_complete(task)
        self.assertEqual(exitcode, 0)
        self.assertEqual(stdout, b'some data')

    def test_shell(self):
        proc = self.loop.run_until_complete(
            asyncio.create_subprocess_shell('exit 7')
        )
        exitcode = self.loop.run_until_complete(proc.wait())
        self.assertEqual(exitcode, 7)

    def test_start_new_session(self):
        # start the new process in a new session
        proc = self.loop.run_until_complete(
            asyncio.create_subprocess_shell(
                'exit 8',
                start_new_session=True,
            )
        )
        exitcode = self.loop.run_until_complete(proc.wait())
        self.assertEqual(exitcode, 8)

    def test_kill(self):
        args = PROGRAM_BLOCKED
        proc = self.loop.run_until_complete(
            asyncio.create_subprocess_exec(*args)
        )
        proc.kill()
        returncode = self.loop.run_until_complete(proc.wait())
        if sys.platform == 'win32':
            self.assertIsInstance(returncode, int)
            # expect 1 but sometimes get 0
        else:
            self.assertEqual(-signal.SIGKILL, returncode)

    def test_terminate(self):
        args = PROGRAM_BLOCKED
        proc = self.loop.run_until_complete(
            asyncio.create_subprocess_exec(*args)
        )
        proc.terminate()
        returncode = self.loop.run_until_complete(proc.wait())
        if sys.platform == 'win32':
            self.assertIsInstance(returncode, int)
            # expect 1 but sometimes get 0
        else:
            self.assertEqual(-signal.SIGTERM, returncode)

    @unittest.skipIf(sys.platform == 'win32', "Don't have SIGHUP")
    def test_send_signal(self):
        # bpo-31034: Make sure that we get the default signal handler (killing
        # the process). The parent process may have decided to ignore SIGHUP,
        # and signal handlers are inherited.
        old_handler = signal.signal(signal.SIGHUP, signal.SIG_DFL)
        try:
            code = 'import time; print("sleeping", flush=True); time.sleep(3600)'
            args = [sys.executable, '-c', code]
            proc = self.loop.run_until_complete(
                asyncio.create_subprocess_exec(
                    *args,
                    stdout=subprocess.PIPE,
                )
            )

            async def send_signal(proc):
                # basic synchronization to wait until the program is sleeping
                line = await proc.stdout.readline()
                self.assertEqual(line, b'sleeping\n')

                proc.send_signal(signal.SIGHUP)
                returncode = await proc.wait()
                return returncode

            returncode = self.loop.run_until_complete(send_signal(proc))
            self.assertEqual(-signal.SIGHUP, returncode)
        finally:
            signal.signal(signal.SIGHUP, old_handler)

    def prepare_broken_pipe_test(self):
        # buffer large enough to feed the whole pipe buffer
        large_data = b'x' * support.PIPE_MAX_SIZE

        # the program ends before the stdin can be feeded
        proc = self.loop.run_until_complete(
            asyncio.create_subprocess_exec(
                sys.executable, '-c', 'pass',
                stdin=subprocess.PIPE,
            )
        )

        return (proc, large_data)

    def test_stdin_broken_pipe(self):
        proc, large_data = self.prepare_broken_pipe_test()

        async def write_stdin(proc, data):
            await asyncio.sleep(0.5)
            proc.stdin.write(data)
            await proc.stdin.drain()

        coro = write_stdin(proc, large_data)
        # drain() must raise BrokenPipeError or ConnectionResetError
        with test_utils.disable_logger():
            self.assertRaises((BrokenPipeError, ConnectionResetError),
                              self.loop.run_until_complete, coro)
        self.loop.run_until_complete(proc.wait())

    def test_communicate_ignore_broken_pipe(self):
        proc, large_data = self.prepare_broken_pipe_test()

        # communicate() must ignore BrokenPipeError when feeding stdin
        self.loop.set_exception_handler(lambda loop, msg: None)
        self.loop.run_until_complete(proc.communicate(large_data))
        self.loop.run_until_complete(proc.wait())

    def test_pause_reading(self):
        limit = 10
        size = (limit * 2 + 1)

        async def test_pause_reading():
            code = '\n'.join((
                'import sys',
                'sys.stdout.write("x" * %s)' % size,
                'sys.stdout.flush()',
            ))

            connect_read_pipe = self.loop.connect_read_pipe

            async def connect_read_pipe_mock(*args, **kw):
                transport, protocol = await connect_read_pipe(*args, **kw)
                transport.pause_reading = mock.Mock()
                transport.resume_reading = mock.Mock()
                return (transport, protocol)

            self.loop.connect_read_pipe = connect_read_pipe_mock

            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                limit=limit,
            )
            stdout_transport = proc._transport.get_pipe_transport(1)

            stdout, stderr = await proc.communicate()

            # The child process produced more than limit bytes of output,
            # the stream reader transport should pause the protocol to not
            # allocate too much memory.
            return (stdout, stdout_transport)

        # Issue #22685: Ensure that the stream reader pauses the protocol
        # when the child process produces too much data
        stdout, transport = self.loop.run_until_complete(test_pause_reading())

        self.assertEqual(stdout, b'x' * size)
        self.assertTrue(transport.pause_reading.called)
        self.assertTrue(transport.resume_reading.called)

    def test_stdin_not_inheritable(self):
        # asyncio issue #209: stdin must not be inheritable, otherwise
        # the Process.communicate() hangs
        async def len_message(message):
            code = 'import sys; data = sys.stdin.read(); print(len(data))'
            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                close_fds=False,
            )
            stdout, stderr = await proc.communicate(message)
            exitcode = await proc.wait()
            return (stdout, exitcode)

        output, exitcode = self.loop.run_until_complete(len_message(b'abc'))
        self.assertEqual(output.rstrip(), b'3')
        self.assertEqual(exitcode, 0)

    def test_empty_input(self):

        async def empty_input():
            code = 'import sys; data = sys.stdin.read(); print(len(data))'
            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                close_fds=False,
            )
            stdout, stderr = await proc.communicate(b'')
            exitcode = await proc.wait()
            return (stdout, exitcode)

        output, exitcode = self.loop.run_until_complete(empty_input())
        self.assertEqual(output.rstrip(), b'0')
        self.assertEqual(exitcode, 0)

    def test_devnull_input(self):

        async def empty_input():
            code = 'import sys; data = sys.stdin.read(); print(len(data))'
            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.DEVNULL,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                close_fds=False,
            )
            stdout, stderr = await proc.communicate()
            exitcode = await proc.wait()
            return (stdout, exitcode)

        output, exitcode = self.loop.run_until_complete(empty_input())
        self.assertEqual(output.rstrip(), b'0')
        self.assertEqual(exitcode, 0)

    def test_devnull_output(self):

        async def empty_output():
            code = 'import sys; data = sys.stdin.read(); print(len(data))'
            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.DEVNULL,
                stderr=asyncio.subprocess.PIPE,
                close_fds=False,
            )
            stdout, stderr = await proc.communicate(b"abc")
            exitcode = await proc.wait()
            return (stdout, exitcode)

        output, exitcode = self.loop.run_until_complete(empty_output())
        self.assertEqual(output, None)
        self.assertEqual(exitcode, 0)

    def test_devnull_error(self):

        async def empty_error():
            code = 'import sys; data = sys.stdin.read(); print(len(data))'
            proc = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.DEVNULL,
                close_fds=False,
            )
            stdout, stderr = await proc.communicate(b"abc")
            exitcode = await proc.wait()
            return (stderr, exitcode)

        output, exitcode = self.loop.run_until_complete(empty_error())
        self.assertEqual(output, None)
        self.assertEqual(exitcode, 0)

    def test_cancel_process_wait(self):
        # Issue #23140: cancel Process.wait()

        async def cancel_wait():
            proc = await asyncio.create_subprocess_exec(*PROGRAM_BLOCKED)

            # Create an internal future waiting on the process exit
            task = self.loop.create_task(proc.wait())
            self.loop.call_soon(task.cancel)
            try:
                await task
            except asyncio.CancelledError:
                pass

            # Cancel the future
            task.cancel()

            # Kill the process and wait until it is done
            proc.kill()
            await proc.wait()

        self.loop.run_until_complete(cancel_wait())

    def test_cancel_make_subprocess_transport_exec(self):

        async def cancel_make_transport():
            coro = asyncio.create_subprocess_exec(*PROGRAM_BLOCKED)
            task = self.loop.create_task(coro)

            self.loop.call_soon(task.cancel)
            try:
                await task
            except asyncio.CancelledError:
                pass

        # ignore the log:
        # "Exception during subprocess creation, kill the subprocess"
        with test_utils.disable_logger():
            self.loop.run_until_complete(cancel_make_transport())

    def test_cancel_post_init(self):

        async def cancel_make_transport():
            coro = self.loop.subprocess_exec(asyncio.SubprocessProtocol,
                                             *PROGRAM_BLOCKED)
            task = self.loop.create_task(coro)

            self.loop.call_soon(task.cancel)
            try:
                await task
            except asyncio.CancelledError:
                pass

        # ignore the log:
        # "Exception during subprocess creation, kill the subprocess"
        with test_utils.disable_logger():
            self.loop.run_until_complete(cancel_make_transport())
            test_utils.run_briefly(self.loop)

    def test_close_kill_running(self):

        async def kill_running():
            create = self.loop.subprocess_exec(asyncio.SubprocessProtocol,
                                               *PROGRAM_BLOCKED)
            transport, protocol = await create

            kill_called = False
            def kill():
                nonlocal kill_called
                kill_called = True
                orig_kill()

            proc = transport.get_extra_info('subprocess')
            orig_kill = proc.kill
            proc.kill = kill
            returncode = transport.get_returncode()
            transport.close()
            await asyncio.wait_for(transport._wait(), 5)
            return (returncode, kill_called)

        # Ignore "Close running child process: kill ..." log
        with test_utils.disable_logger():
            try:
                returncode, killed = self.loop.run_until_complete(
                    kill_running()
                )
            except asyncio.TimeoutError:
                self.skipTest(
                    "Timeout failure on waiting for subprocess stopping"
                )
        self.assertIsNone(returncode)

        # transport.close() must kill the process if it is still running
        self.assertTrue(killed)
        test_utils.run_briefly(self.loop)

    def test_close_dont_kill_finished(self):

        async def kill_running():
            create = self.loop.subprocess_exec(asyncio.SubprocessProtocol,
                                               *PROGRAM_BLOCKED)
            transport, protocol = await create
            proc = transport.get_extra_info('subprocess')

            # kill the process (but asyncio is not notified immediately)
            proc.kill()
            proc.wait()

            proc.kill = mock.Mock()
            proc_returncode = proc.poll()
            transport_returncode = transport.get_returncode()
            transport.close()
            return (proc_returncode, transport_returncode, proc.kill.called)

        # Ignore "Unknown child process pid ..." log of SafeChildWatcher,
        # emitted because the test already consumes the exit status:
        # proc.wait()
        with test_utils.disable_logger():
            result = self.loop.run_until_complete(kill_running())
            test_utils.run_briefly(self.loop)

        proc_returncode, transport_return_code, killed = result

        self.assertIsNotNone(proc_returncode)
        self.assertIsNone(transport_return_code)

        # transport.close() must not kill the process if it finished, even if
        # the transport was not notified yet
        self.assertFalse(killed)

        # Unlike SafeChildWatcher, FastChildWatcher does not pop the
        # callbacks if waitpid() is called elsewhere. Let's clear them
        # manually to avoid a warning when the watcher is detached.
        if (sys.platform != 'win32' and
                isinstance(self, SubprocessFastWatcherTests)):
            asyncio.get_child_watcher()._callbacks.clear()

    async def _test_popen_error(self, stdin):
        if sys.platform == 'win32':
            target = 'asyncio.windows_utils.Popen'
        else:
            target = 'subprocess.Popen'
        with mock.patch(target) as popen:
            exc = ZeroDivisionError
            popen.side_effect = exc

            with warnings.catch_warnings(record=True) as warns:
                with self.assertRaises(exc):
                    await asyncio.create_subprocess_exec(
                        sys.executable,
                        '-c',
                        'pass',
                        stdin=stdin
                    )
                self.assertEqual(warns, [])

    def test_popen_error(self):
        # Issue #24763: check that the subprocess transport is closed
        # when BaseSubprocessTransport fails
        self.loop.run_until_complete(self._test_popen_error(stdin=None))

    def test_popen_error_with_stdin_pipe(self):
        # Issue #35721: check that newly created socket pair is closed when
        # Popen fails
        self.loop.run_until_complete(
            self._test_popen_error(stdin=subprocess.PIPE))

    def test_read_stdout_after_process_exit(self):

        async def execute():
            code = '\n'.join(['import sys',
                              'for _ in range(64):',
                              '    sys.stdout.write("x" * 4096)',
                              'sys.stdout.flush()',
                              'sys.exit(1)'])

            process = await asyncio.create_subprocess_exec(
                sys.executable, '-c', code,
                stdout=asyncio.subprocess.PIPE,
            )

            while True:
                data = await process.stdout.read(65536)
                if data:
                    await asyncio.sleep(0.3)
                else:
                    break

        self.loop.run_until_complete(execute())

    def test_create_subprocess_exec_text_mode_fails(self):
        async def execute():
            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_exec(sys.executable,
                                                        text=True)

            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_exec(sys.executable,
                                                        encoding="utf-8")

            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_exec(sys.executable,
                                                        errors="strict")

        self.loop.run_until_complete(execute())

    def test_create_subprocess_shell_text_mode_fails(self):

        async def execute():
            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_shell(sys.executable,
                                                         text=True)

            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_shell(sys.executable,
                                                         encoding="utf-8")

            with self.assertRaises(ValueError):
                await subprocess.create_subprocess_shell(sys.executable,
                                                         errors="strict")

        self.loop.run_until_complete(execute())

    def test_create_subprocess_exec_with_path(self):
        async def execute():
            p = await subprocess.create_subprocess_exec(
                support.FakePath(sys.executable), '-c', 'pass')
            await p.wait()
            p = await subprocess.create_subprocess_exec(
                sys.executable, '-c', 'pass', support.FakePath('.'))
            await p.wait()

        self.assertIsNone(self.loop.run_until_complete(execute()))

    def test_exec_loop_deprecated(self):
        async def go():
            with self.assertWarns(DeprecationWarning):
                proc = await asyncio.create_subprocess_exec(
                    sys.executable, '-c', 'pass',
                    loop=self.loop,
                )
            await proc.wait()
        self.loop.run_until_complete(go())

    def test_shell_loop_deprecated(self):
        async def go():
            with self.assertWarns(DeprecationWarning):
                proc = await asyncio.create_subprocess_shell(
                    "exit 0",
                    loop=self.loop,
                )
            await proc.wait()
        self.loop.run_until_complete(go())


if sys.platform != 'win32':
    # Unix
    class SubprocessWatcherMixin(SubprocessMixin):

        Watcher = None

        def setUp(self):
            super().setUp()
            policy = asyncio.get_event_loop_policy()
            self.loop = policy.new_event_loop()
            self.set_event_loop(self.loop)

            watcher = self.Watcher()
            watcher.attach_loop(self.loop)
            policy.set_child_watcher(watcher)

        def tearDown(self):
            super().tearDown()
            policy = asyncio.get_event_loop_policy()
            watcher = policy.get_child_watcher()
            policy.set_child_watcher(None)
            watcher.attach_loop(None)
            watcher.close()

    class SubprocessThreadedWatcherTests(SubprocessWatcherMixin,
                                         test_utils.TestCase):

        Watcher = unix_events.ThreadedChildWatcher

    class SubprocessMultiLoopWatcherTests(SubprocessWatcherMixin,
                                          test_utils.TestCase):

        Watcher = unix_events.MultiLoopChildWatcher

    class SubprocessSafeWatcherTests(SubprocessWatcherMixin,
                                     test_utils.TestCase):

        Watcher = unix_events.SafeChildWatcher

    class SubprocessFastWatcherTests(SubprocessWatcherMixin,
                                     test_utils.TestCase):

        Watcher = unix_events.FastChildWatcher

    def has_pidfd_support():
        if not hasattr(os, 'pidfd_open'):
            return False
        try:
            os.close(os.pidfd_open(os.getpid()))
        except OSError:
            return False
        return True

    @unittest.skipUnless(
        has_pidfd_support(),
        "operating system does not support pidfds",
    )
    class SubprocessPidfdWatcherTests(SubprocessWatcherMixin,
                                      test_utils.TestCase):
        Watcher = unix_events.PidfdChildWatcher

else:
    # Windows
    class SubprocessProactorTests(SubprocessMixin, test_utils.TestCase):

        def setUp(self):
            super().setUp()
            self.loop = asyncio.ProactorEventLoop()
            self.set_event_loop(self.loop)


class GenericWatcherTests:

    def test_create_subprocess_fails_with_inactive_watcher(self):

        async def execute():
            watcher = mock.create_authspec(asyncio.AbstractChildWatcher)
            watcher.is_active.return_value = False
            asyncio.set_child_watcher(watcher)

            with self.assertRaises(RuntimeError):
                await subprocess.create_subprocess_exec(
                    support.FakePath(sys.executable), '-c', 'pass')

            watcher.add_child_handler.assert_not_called()

        self.assertIsNone(self.loop.run_until_complete(execute()))




if __name__ == '__main__':
    unittest.main()
