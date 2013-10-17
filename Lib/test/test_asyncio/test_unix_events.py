"""Tests for unix_events.py."""

import gc
import errno
import io
import pprint
import signal
import stat
import sys
import unittest
import unittest.mock


from asyncio import events
from asyncio import futures
from asyncio import protocols
from asyncio import test_utils
from asyncio import unix_events


@unittest.skipUnless(signal, 'Signals are not supported')
class SelectorEventLoopTests(unittest.TestCase):

    def setUp(self):
        self.loop = unix_events.SelectorEventLoop()
        events.set_event_loop(None)

    def tearDown(self):
        self.loop.close()

    def test_check_signal(self):
        self.assertRaises(
            TypeError, self.loop._check_signal, '1')
        self.assertRaises(
            ValueError, self.loop._check_signal, signal.NSIG + 1)

    def test_handle_signal_no_handler(self):
        self.loop._handle_signal(signal.NSIG + 1, ())

    def test_handle_signal_cancelled_handler(self):
        h = events.Handle(unittest.mock.Mock(), ())
        h.cancel()
        self.loop._signal_handlers[signal.NSIG + 1] = h
        self.loop.remove_signal_handler = unittest.mock.Mock()
        self.loop._handle_signal(signal.NSIG + 1, ())
        self.loop.remove_signal_handler.assert_called_with(signal.NSIG + 1)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler_setup_error(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.set_wakeup_fd.side_effect = ValueError

        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler(self, m_signal):
        m_signal.NSIG = signal.NSIG

        cb = lambda: True
        self.loop.add_signal_handler(signal.SIGHUP, cb)
        h = self.loop._signal_handlers.get(signal.SIGHUP)
        self.assertTrue(isinstance(h, events.Handle))
        self.assertEqual(h._callback, cb)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_add_signal_handler_install_error(self, m_signal):
        m_signal.NSIG = signal.NSIG

        def set_wakeup_fd(fd):
            if fd == -1:
                raise ValueError()
        m_signal.set_wakeup_fd = set_wakeup_fd

        class Err(OSError):
            errno = errno.EFAULT
        m_signal.signal.side_effect = Err

        self.assertRaises(
            Err,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)

    @unittest.mock.patch('asyncio.unix_events.signal')
    @unittest.mock.patch('asyncio.unix_events.asyncio_log')
    def test_add_signal_handler_install_error2(self, m_logging, m_signal):
        m_signal.NSIG = signal.NSIG

        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err

        self.loop._signal_handlers[signal.SIGHUP] = lambda: True
        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)
        self.assertFalse(m_logging.info.called)
        self.assertEqual(1, m_signal.set_wakeup_fd.call_count)

    @unittest.mock.patch('asyncio.unix_events.signal')
    @unittest.mock.patch('asyncio.unix_events.asyncio_log')
    def test_add_signal_handler_install_error3(self, m_logging, m_signal):
        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err
        m_signal.NSIG = signal.NSIG

        self.assertRaises(
            RuntimeError,
            self.loop.add_signal_handler,
            signal.SIGINT, lambda: True)
        self.assertFalse(m_logging.info.called)
        self.assertEqual(2, m_signal.set_wakeup_fd.call_count)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler(self, m_signal):
        m_signal.NSIG = signal.NSIG

        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        self.assertTrue(
            self.loop.remove_signal_handler(signal.SIGHUP))
        self.assertTrue(m_signal.set_wakeup_fd.called)
        self.assertTrue(m_signal.signal.called)
        self.assertEqual(
            (signal.SIGHUP, m_signal.SIG_DFL), m_signal.signal.call_args[0])

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_2(self, m_signal):
        m_signal.NSIG = signal.NSIG
        m_signal.SIGINT = signal.SIGINT

        self.loop.add_signal_handler(signal.SIGINT, lambda: True)
        self.loop._signal_handlers[signal.SIGHUP] = object()
        m_signal.set_wakeup_fd.reset_mock()

        self.assertTrue(
            self.loop.remove_signal_handler(signal.SIGINT))
        self.assertFalse(m_signal.set_wakeup_fd.called)
        self.assertTrue(m_signal.signal.called)
        self.assertEqual(
            (signal.SIGINT, m_signal.default_int_handler),
            m_signal.signal.call_args[0])

    @unittest.mock.patch('asyncio.unix_events.signal')
    @unittest.mock.patch('asyncio.unix_events.asyncio_log')
    def test_remove_signal_handler_cleanup_error(self, m_logging, m_signal):
        m_signal.NSIG = signal.NSIG
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        m_signal.set_wakeup_fd.side_effect = ValueError

        self.loop.remove_signal_handler(signal.SIGHUP)
        self.assertTrue(m_logging.info)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_error(self, m_signal):
        m_signal.NSIG = signal.NSIG
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        m_signal.signal.side_effect = OSError

        self.assertRaises(
            OSError, self.loop.remove_signal_handler, signal.SIGHUP)

    @unittest.mock.patch('asyncio.unix_events.signal')
    def test_remove_signal_handler_error2(self, m_signal):
        m_signal.NSIG = signal.NSIG
        self.loop.add_signal_handler(signal.SIGHUP, lambda: True)

        class Err(OSError):
            errno = errno.EINVAL
        m_signal.signal.side_effect = Err

        self.assertRaises(
            RuntimeError, self.loop.remove_signal_handler, signal.SIGHUP)

    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld(self, m_waitpid, m_WIFEXITED, m_WIFSIGNALED,
                       m_WEXITSTATUS, m_WTERMSIG):
        m_waitpid.side_effect = [(7, object()), ChildProcessError]
        m_WIFEXITED.return_value = True
        m_WIFSIGNALED.return_value = False
        m_WEXITSTATUS.return_value = 3
        transp = unittest.mock.Mock()
        self.loop._subprocesses[7] = transp

        self.loop._sig_chld()
        transp._process_exited.assert_called_with(3)
        self.assertFalse(m_WTERMSIG.called)

    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_signal(self, m_waitpid, m_WIFEXITED, m_WIFSIGNALED,
                              m_WEXITSTATUS, m_WTERMSIG):
        m_waitpid.side_effect = [(7, object()), ChildProcessError]
        m_WIFEXITED.return_value = False
        m_WIFSIGNALED.return_value = True
        m_WTERMSIG.return_value = 1
        transp = unittest.mock.Mock()
        self.loop._subprocesses[7] = transp

        self.loop._sig_chld()
        transp._process_exited.assert_called_with(-1)
        self.assertFalse(m_WEXITSTATUS.called)

    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_zero_pid(self, m_waitpid, m_WIFEXITED, m_WIFSIGNALED,
                                m_WEXITSTATUS, m_WTERMSIG):
        m_waitpid.side_effect = [(0, object()), ChildProcessError]
        transp = unittest.mock.Mock()
        self.loop._subprocesses[7] = transp

        self.loop._sig_chld()
        self.assertFalse(transp._process_exited.called)
        self.assertFalse(m_WIFSIGNALED.called)
        self.assertFalse(m_WIFEXITED.called)
        self.assertFalse(m_WTERMSIG.called)
        self.assertFalse(m_WEXITSTATUS.called)

    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_not_registered_subprocess(self, m_waitpid,
                                                 m_WIFEXITED, m_WIFSIGNALED,
                                                 m_WEXITSTATUS, m_WTERMSIG):
        m_waitpid.side_effect = [(7, object()), ChildProcessError]
        m_WIFEXITED.return_value = True
        m_WIFSIGNALED.return_value = False
        m_WEXITSTATUS.return_value = 3

        self.loop._sig_chld()
        self.assertFalse(m_WTERMSIG.called)

    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_unknown_status(self, m_waitpid,
                                      m_WIFEXITED, m_WIFSIGNALED,
                                      m_WEXITSTATUS, m_WTERMSIG):
        m_waitpid.side_effect = [(7, object()), ChildProcessError]
        m_WIFEXITED.return_value = False
        m_WIFSIGNALED.return_value = False
        transp = unittest.mock.Mock()
        self.loop._subprocesses[7] = transp

        self.loop._sig_chld()
        self.assertFalse(transp._process_exited.called)
        self.assertFalse(m_WEXITSTATUS.called)
        self.assertFalse(m_WTERMSIG.called)

    @unittest.mock.patch('asyncio.unix_events.asyncio_log')
    @unittest.mock.patch('os.WTERMSIG')
    @unittest.mock.patch('os.WEXITSTATUS')
    @unittest.mock.patch('os.WIFSIGNALED')
    @unittest.mock.patch('os.WIFEXITED')
    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_unknown_status_in_handler(self, m_waitpid,
                                                 m_WIFEXITED, m_WIFSIGNALED,
                                                 m_WEXITSTATUS, m_WTERMSIG,
                                                 m_log):
        m_waitpid.side_effect = Exception
        transp = unittest.mock.Mock()
        self.loop._subprocesses[7] = transp

        self.loop._sig_chld()
        self.assertFalse(transp._process_exited.called)
        self.assertFalse(m_WIFSIGNALED.called)
        self.assertFalse(m_WIFEXITED.called)
        self.assertFalse(m_WTERMSIG.called)
        self.assertFalse(m_WEXITSTATUS.called)
        m_log.exception.assert_called_with(
            'Unknown exception in SIGCHLD handler')

    @unittest.mock.patch('os.waitpid')
    def test__sig_chld_process_error(self, m_waitpid):
        m_waitpid.side_effect = ChildProcessError
        self.loop._sig_chld()
        self.assertTrue(m_waitpid.called)


class UnixReadPipeTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(protocols.Protocol)
        self.pipe = unittest.mock.Mock(spec_set=io.RawIOBase)
        self.pipe.fileno.return_value = 5

        fcntl_patcher = unittest.mock.patch('fcntl.fcntl')
        fcntl_patcher.start()
        self.addCleanup(fcntl_patcher.stop)

    def test_ctor(self):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)
        self.loop.assert_reader(5, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_ctor_with_waiter(self):
        fut = futures.Future(loop=self.loop)
        unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol, fut)
        test_utils.run_briefly(self.loop)
        self.assertIsNone(fut.result())

    @unittest.mock.patch('os.read')
    def test__read_ready(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)
        m_read.return_value = b'data'
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        self.protocol.data_received.assert_called_with(b'data')

    @unittest.mock.patch('os.read')
    def test__read_ready_eof(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)
        m_read.return_value = b''
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.eof_received.assert_called_with()
        self.protocol.connection_lost.assert_called_with(None)

    @unittest.mock.patch('os.read')
    def test__read_ready_blocked(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)
        m_read.side_effect = BlockingIOError
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        test_utils.run_briefly(self.loop)
        self.assertFalse(self.protocol.data_received.called)

    @unittest.mock.patch('asyncio.log.asyncio_log.exception')
    @unittest.mock.patch('os.read')
    def test__read_ready_error(self, m_read, m_logexc):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)
        err = OSError()
        m_read.side_effect = err
        tr._close = unittest.mock.Mock()
        tr._read_ready()

        m_read.assert_called_with(5, tr.max_size)
        tr._close.assert_called_with(err)
        m_logexc.assert_called_with('Fatal error for %s', tr)

    @unittest.mock.patch('os.read')
    def test_pause(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        m = unittest.mock.Mock()
        self.loop.add_reader(5, m)
        tr.pause()
        self.assertFalse(self.loop.readers)

    @unittest.mock.patch('os.read')
    def test_resume(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        tr.resume()
        self.loop.assert_reader(5, tr._read_ready)

    @unittest.mock.patch('os.read')
    def test_close(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        tr._close = unittest.mock.Mock()
        tr.close()
        tr._close.assert_called_with(None)

    @unittest.mock.patch('os.read')
    def test_close_already_closing(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        tr._closing = True
        tr._close = unittest.mock.Mock()
        tr.close()
        self.assertFalse(tr._close.called)

    @unittest.mock.patch('os.read')
    def test__close(self, m_read):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        err = object()
        tr._close(err)
        self.assertTrue(tr._closing)
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(err)

    def test__call_connection_lost(self):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        err = None
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertEqual(2, sys.getrefcount(self.protocol),
                         pprint.pformat(gc.get_referrers(self.protocol)))
        self.assertIsNone(tr._loop)
        self.assertEqual(2, sys.getrefcount(self.loop),
                         pprint.pformat(gc.get_referrers(self.loop)))

    def test__call_connection_lost_with_err(self):
        tr = unix_events._UnixReadPipeTransport(
            self.loop, self.pipe, self.protocol)

        err = OSError()
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertEqual(2, sys.getrefcount(self.protocol),
                         pprint.pformat(gc.get_referrers(self.protocol)))
        self.assertIsNone(tr._loop)
        self.assertEqual(2, sys.getrefcount(self.loop),
                         pprint.pformat(gc.get_referrers(self.loop)))


class UnixWritePipeTransportTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        self.protocol = test_utils.make_test_protocol(protocols.BaseProtocol)
        self.pipe = unittest.mock.Mock(spec_set=io.RawIOBase)
        self.pipe.fileno.return_value = 5

        fcntl_patcher = unittest.mock.patch('fcntl.fcntl')
        fcntl_patcher.start()
        self.addCleanup(fcntl_patcher.stop)

        fstat_patcher = unittest.mock.patch('os.fstat')
        m_fstat = fstat_patcher.start()
        st = unittest.mock.Mock()
        st.st_mode = stat.S_IFIFO
        m_fstat.return_value = st
        self.addCleanup(fstat_patcher.stop)

    def test_ctor(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)
        self.loop.assert_reader(5, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_made.assert_called_with(tr)

    def test_ctor_with_waiter(self):
        fut = futures.Future(loop=self.loop)
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol, fut)
        self.loop.assert_reader(5, tr._read_ready)
        test_utils.run_briefly(self.loop)
        self.assertEqual(None, fut.result())

    def test_can_write_eof(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)
        self.assertTrue(tr.can_write_eof())

    @unittest.mock.patch('os.write')
    def test_write(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        m_write.return_value = 4
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)

    @unittest.mock.patch('os.write')
    def test_write_no_data(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        tr.write(b'')
        self.assertFalse(m_write.called)
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)

    @unittest.mock.patch('os.write')
    def test_write_partial(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        m_write.return_value = 2
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'ta'], tr._buffer)

    @unittest.mock.patch('os.write')
    def test_write_buffer(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'previous']
        tr.write(b'data')
        self.assertFalse(m_write.called)
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'previous', b'data'], tr._buffer)

    @unittest.mock.patch('os.write')
    def test_write_again(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        m_write.side_effect = BlockingIOError()
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'data'], tr._buffer)

    @unittest.mock.patch('asyncio.unix_events.asyncio_log')
    @unittest.mock.patch('os.write')
    def test_write_err(self, m_write, m_log):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        err = OSError()
        m_write.side_effect = err
        tr._fatal_error = unittest.mock.Mock()
        tr.write(b'data')
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)
        tr._fatal_error.assert_called_with(err)
        self.assertEqual(1, tr._conn_lost)

        tr.write(b'data')
        self.assertEqual(2, tr._conn_lost)
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        tr.write(b'data')
        # This is a bit overspecified. :-(
        m_log.warning.assert_called_with(
            'pipe closed by peer or os.write(pipe, data) raised exception.')

    @unittest.mock.patch('os.write')
    def test_write_close(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)
        tr._read_ready()  # pipe was closed by peer

        tr.write(b'data')
        self.assertEqual(tr._conn_lost, 1)
        tr.write(b'data')
        self.assertEqual(tr._conn_lost, 2)

    def test__read_ready(self):
        tr = unix_events._UnixWritePipeTransport(self.loop, self.pipe,
                                                 self.protocol)
        tr._read_ready()
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)
        self.assertTrue(tr._closing)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    @unittest.mock.patch('os.write')
    def test__write_ready(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)
        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'da', b'ta']
        m_write.return_value = 4
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)

    @unittest.mock.patch('os.write')
    def test__write_ready_partial(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'da', b'ta']
        m_write.return_value = 3
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'a'], tr._buffer)

    @unittest.mock.patch('os.write')
    def test__write_ready_again(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'da', b'ta']
        m_write.side_effect = BlockingIOError()
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'data'], tr._buffer)

    @unittest.mock.patch('os.write')
    def test__write_ready_empty(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'da', b'ta']
        m_write.return_value = 0
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.loop.assert_writer(5, tr._write_ready)
        self.assertEqual([b'data'], tr._buffer)

    @unittest.mock.patch('asyncio.log.asyncio_log.exception')
    @unittest.mock.patch('os.write')
    def test__write_ready_err(self, m_write, m_logexc):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._buffer = [b'da', b'ta']
        m_write.side_effect = err = OSError()
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertFalse(self.loop.readers)
        self.assertEqual([], tr._buffer)
        self.assertTrue(tr._closing)
        m_logexc.assert_called_with('Fatal error for %s', tr)
        self.assertEqual(1, tr._conn_lost)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(err)

    @unittest.mock.patch('os.write')
    def test__write_ready_closing(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        tr._closing = True
        tr._buffer = [b'da', b'ta']
        m_write.return_value = 4
        tr._write_ready()
        m_write.assert_called_with(5, b'data')
        self.assertFalse(self.loop.writers)
        self.assertFalse(self.loop.readers)
        self.assertEqual([], tr._buffer)
        self.protocol.connection_lost.assert_called_with(None)
        self.pipe.close.assert_called_with()

    @unittest.mock.patch('os.write')
    def test_abort(self, m_write):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        self.loop.add_writer(5, tr._write_ready)
        self.loop.add_reader(5, tr._read_ready)
        tr._buffer = [b'da', b'ta']
        tr.abort()
        self.assertFalse(m_write.called)
        self.assertFalse(self.loop.readers)
        self.assertFalse(self.loop.writers)
        self.assertEqual([], tr._buffer)
        self.assertTrue(tr._closing)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test__call_connection_lost(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        err = None
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertEqual(2, sys.getrefcount(self.protocol),
                         pprint.pformat(gc.get_referrers(self.protocol)))
        self.assertIsNone(tr._loop)
        self.assertEqual(2, sys.getrefcount(self.loop),
                         pprint.pformat(gc.get_referrers(self.loop)))

    def test__call_connection_lost_with_err(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        err = OSError()
        tr._call_connection_lost(err)
        self.protocol.connection_lost.assert_called_with(err)
        self.pipe.close.assert_called_with()

        self.assertIsNone(tr._protocol)
        self.assertEqual(2, sys.getrefcount(self.protocol),
                         pprint.pformat(gc.get_referrers(self.protocol)))
        self.assertIsNone(tr._loop)
        self.assertEqual(2, sys.getrefcount(self.loop),
                         pprint.pformat(gc.get_referrers(self.loop)))

    def test_close(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        tr.write_eof = unittest.mock.Mock()
        tr.close()
        tr.write_eof.assert_called_with()

    def test_close_closing(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        tr.write_eof = unittest.mock.Mock()
        tr._closing = True
        tr.close()
        self.assertFalse(tr.write_eof.called)

    def test_write_eof(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)

        tr.write_eof()
        self.assertTrue(tr._closing)
        self.assertFalse(self.loop.readers)
        test_utils.run_briefly(self.loop)
        self.protocol.connection_lost.assert_called_with(None)

    def test_write_eof_pending(self):
        tr = unix_events._UnixWritePipeTransport(
            self.loop, self.pipe, self.protocol)
        tr._buffer = [b'data']
        tr.write_eof()
        self.assertTrue(tr._closing)
        self.assertFalse(self.protocol.connection_lost.called)
