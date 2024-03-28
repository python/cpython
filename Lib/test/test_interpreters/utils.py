import contextlib
import os
import os.path
import pickle
import queue
#import select
import subprocess
import sys
import tempfile
from textwrap import dedent, indent
import threading
import types
import unittest

from test import support
from test.support import os_helper

from test.support import interpreters


try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None
else:
    run_in_interpreter = _testinternalcapi.run_in_subinterp_with_config

def requires__testinternalcapi(func):
    return unittest.skipIf(_testinternalcapi is None, "test requires _testinternalcapi module")(func)


def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w', encoding='utf-8') as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r, encoding='utf-8')


def clean_up_interpreters():
    for interp in interpreters.list_all():
        if interp.id == 0:  # main
            continue
        try:
            interp.close()
        except RuntimeError:
            pass  # already destroyed


def _run_output(interp, request, init=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        if init:
            interp.prepare_main(init)
        interp.exec(script)
        return rpipe.read()


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interp.exec(dedent(f"""
            # wait for "signal"
            with open({r}) as rpipe:
                rpipe.read()
            """))

    t = threading.Thread(target=run)
    t.start()

    yield

    with open(w, 'w') as spipe:
        spipe.write('done')
    t.join()


@contextlib.contextmanager
def fd_maybe_closed():
    try:
        yield
    except OSError as exc:
        if exc.errno != 9:
            raise  # re-raise
        # The file descriptor has closed.


class PipeEnd:

    def __init__(self, fd):
        self._fd = fd
        self._closed = False
        self._lock = threading.Lock()

    def __del__(self):
        self.close()

    def __str__(self):
        return str(self._fd)

    def __repr__(self):
        return f'{type_self.__name__}({self._fd!r})'

    def __index__(self):
        return self._fd

    def close(self):
        with self._lock:
            if self._closed:
                return
            self._closed = True
            with fd_maybe_closed():
                self._close()

    @contextlib.contextmanager
    def _maybe_closed(self):
        assert self._lock.locked()
        with fd_maybe_closed():
            yield
            return
        # It was closed already.
        if not self._closed:
            self._closed = True
            with fd_maybe_closed():
                self._close()

    def _close(self):
        os.close(self._fd)


class ReadPipe(PipeEnd):

    def __init__(self, fd):
        super().__init__(fd)

        self._requests = queue.Queue(1)
        self._looplock = threading.Lock()
        self._thread = threading.Thread(target=self._handle_read_requests)
        self._thread.start()
        self._buffer = bytearray()
        self._bufferlock = threading.Lock()

    def _handle_read_requests(self):
        while True:
            try:
                req = self._requests.get()
            except queue.ShutDown:
                # The pipe is closed.
                break
            finally:
                # It was locked in either self.close() or self.read().
                assert self._lock.locked()

            # The loop lock was taken for us in self._read().
            try:
                if req is None:
                    # Read all available bytes.
                    buf = bytearray()
                    data = os.read(self._fd, 8147)  # a prime number
                    while data:
                        buf += data
                        data = os.read(self._fd, 8147)
                    with self._bufferlock:
                        self._buffer += buf
                else:
                    assert req >= 0, repr(req)
                    with self._bufferlock:
                        missing = req - len(self._buffer)
                    if missing > 0:
                        with self._maybe_closed():
                            data = os.read(self._fd, missing)
                            # The rest is skipped if its already closed.
                            with self._bufferlock:
                                self._buffer += data
            finally:
                self._looplock.release()

    def read(self, n=None, timeout=-1):
        if n == 0:
            return b''
        elif n is not None and n < 0:
            # This will fail.
            os.read(self._fd, n)
            raise OSError('invalid argument')

        with self._lock:
            # The looo will release it after handling the request.
            self._looplock.acquire()
            try:
                self._requests.put_nowait(n)
            except BaseException:
                self._looplock.release()
                raise  # re-raise
            # Wait for the request to finish.
            if self._looplock.acquire(timeout=timeout):
                self._looplock.release()
            # Return (up to) the requested bytes.
            with self._bufferlock:
                data = self._buffer[:n]
                self._buffer = self._buffer[n:]
            return bytes(data)

    def _close(self):
        # Ideally the write end is already closed.
        self._requests.shutdown()
        with self._bufferlock:
            # Throw away any leftover bytes.
            self._buffer = b''
        super()._close()


class WritePipe(PipeEnd):

    def write(self, n=None):
        try:
            with self._maybe_closed():
                return os.write(self._fd, n)
        except BrokenPipeError:
            # The read end was already closed.
            self.close()


def create_pipe():
    r, w = os.pipe()
    return ReadPipe(r), WritePipe(w)


class CapturingScript:
    """
    Embeds a script in a new script that captures stdout/stderr
    and uses pipes to expose them.
    """

    WRAPPER = dedent("""
        import sys
        w_out = {w_out}
        w_err = {w_err}
        stdout, stderr = orig = (sys.stdout, sys.stderr)
        if w_out is not None:
            if w_err == w_out:
                stdout = stderr = open(w_out, 'w', encoding='utf-8')
            elif w_err is not None:
                stdout = open(w_out, 'w', encoding='utf-8', closefd=False)
                stderr = open(w_err, 'w', encoding='utf-8', closefd=False)
            else:
                stdout = open(w_out, 'w', encoding='utf-8', closefd=False)
        else:
            assert w_err is not None
            stderr = open(w_err, 'w', encoding='utf-8', closefd=False)

        sys.stdout = stdout
        sys.stderr = stderr
        try:
            #########################
            # begin wrapped script

            {wrapped}

            # end wrapped script
            #########################
        finally:
            sys.stdout, sys.stderr = orig
        """)

    def __init__(self, script, *, combined=True):
        self._r_out, self._w_out = create_pipe()
        if combined:
            self._r_err = self._w_err = None
            w_err = self._w_out
        else:
            self._r_err, self._w_err = create_pipe()
            w_err = self._w_err
        self._combined = combined

        self._script = self.WRAPPER.format(
            w_out=self._w_out,
            w_err=w_err,
            wrapped=indent(script, '    '),
        )

        self._buf_stdout = None
        self._buf_stderr = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    @property
    def script(self):
        return self._script

    def close(self):
        if self._w_out is not None:
            assert self._r_out is not None
            self._w_out.close()
            self._w_out = None
            self._buf_stdout = self._r_out.read()
            self._r_out.close()
            self._r_out = None
        else:
            assert self._r_out is None

        if self._combined:
            assert self._w_err is None
            assert self._r_err is None
        elif self._w_err is not None:
            assert self._r_err is not None
            self._w_err.close()
            self._w_err = None
            self._buf_stderr = self._r_err.read()
            self._r_err.close()
            self._r_err = None
        else:
            assert self._r_err is None

    def read(self, n=None):
        return self.read_stdout(n)

    def read_stdout(self, n=None):
        if self._r_out is not None:
            data = self._r_out.read(n)
        elif self._buf_stdout is None:
            data = b''
        elif n is None or n == len(self._buf_stdout):
            data = self._buf_stdout
            self._buf_stdout = None
        else:
            data = self._buf_stdout[:n]
            self._buf_stdout = self._buf_stdout[n:]
        return data

    def read_stderr(self, n=None):
        if self._combined:
            return b''
        if self._r_err is not None:
            data = self._r_err.read(n)
        elif self._buf_stderr is None:
            data = b''
        elif n is None or n == len(self._buf_stderr):
            data = self._buf_stderr
            self._buf_stderr = None
        else:
            data = self._buf_stderr[:n]
            self._buf_stderr = self._buf_stderr[n:]
        return data


class WatchedScript:
    """
    Embeds a script in a new script that identifies when the script finishes.
    Captures any uncaught exception, and uses a pipe to expose it.
    """

    WRAPPER = dedent("""
        import os, traceback, json
        w_done = {w_done}
        try:
            os.write(w_done, b'\0')  # started
        except OSError:
            # It was canceled.
            pass
        else:
            try:
                #########################
                # begin wrapped script

                {wrapped}

                # end wrapped script
                #########################
            except Exception as exc:
                text = json.dumps(dict(
                    type=dict(
                        __name__=type(exc).__name__,
                        __qualname__=type(exc).__qualname__,
                        __module__=type(exc).__module__,
                    ),
                    msg=str(exc),
                    formatted=traceback.format_exception_only(exc),
                    errdisplay=traceback.format_exception(exc),
                ))
                try:
                    os.write(w_done, text.encode('utf-8'))
                except BrokenPipeError:
                    # It was closed already.
                    pass
                except OSError:
                    if exc.errno != 9:
                        raise  # re-raise
                    # It was closed already.
            finally:
                try:
                    os.close({w_done})
                except OSError:
                    if exc.errno != 9:
                        raise  # re-raise
                    # It was closed already.
        """)

    def __init__(self, script):
        self._started = False
        self._finished = False
        self._r_done, self._w_done = os.pipe()

        wrapper = self.WRAPPER.format(
            w_done=self._w_done,
            wrapped=indent(script, '    '),
        )
        t = threading.Thread(target=self._watch)
        t.start()

        self._script = wrapper
        self._thread = t

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    @property
    def script(self):
        return self._script

    def close(self):
        for fd in [self._r_done, self._w_done]:
            if fd is not None:
                try:
                    os.close(fd)
                except OSError:
                    if exc.errno != 9:
                        raise  # re-raise
                    # It was closed already.
        self._r_done = None
        self._w_done = None

    @property
    def finished(self):
        if isinstance(self._finished, dict):
            exc = types.SimpleNamespace(**self._finished)
            exc.type = types.SimpleNamespace(**exc.type)
            return exc
        return self._finished

    def _watch(self):
        r_fd = self._r_done

        assert not self._started
        try:
            ch0 = os.read(r_fd, 1)
        except OSError as exc:
            if exc.errno != 9:
                raise  # re-raise
            # It was closed already.
            return
        if ch0 == b'':
            # The write end of the pipe has closed.
            return
        assert ch0 == b'\0', repr(ch0)
        self._started = True

        assert not self._finished
        data = b''
        try:
            chunk = os.read(r_fd, 100)
            self._finished = True
            while chunk:
                data += chunk
                chunk = os.read(r_fd, 100)
            os.close(r_fd)
        except OSError as exc:
            if exc.errno != 9:
                raise  # re-raise
            # It was closed already.

        if data:
            self._finished = json.loads(data)

    # It may be useful to implement the concurrent.futures.Future API
    # on this class.


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()

    def pipe(self):
        def ensure_closed(fd):
            try:
                os.close(fd)
            except OSError:
                pass
        r, w = os.pipe()
        self.addCleanup(lambda: ensure_closed(r))
        self.addCleanup(lambda: ensure_closed(w))
        return r, w

    def temp_dir(self):
        tempdir = tempfile.mkdtemp()
        tempdir = os.path.realpath(tempdir)
        self.addCleanup(lambda: os_helper.rmtree(tempdir))
        return tempdir

    @contextlib.contextmanager
    def captured_thread_exception(self):
        ctx = types.SimpleNamespace(caught=None)
        def excepthook(args):
            ctx.caught = args
        orig_excepthook = threading.excepthook
        threading.excepthook = excepthook
        try:
            yield ctx
        finally:
            threading.excepthook = orig_excepthook

    def make_script(self, filename, dirname=None, text=None):
        if text:
            text = dedent(text)
        if dirname is None:
            dirname = self.temp_dir()
        filename = os.path.join(dirname, filename)

        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, 'w', encoding='utf-8') as outfile:
            outfile.write(text or '')
        return filename

    def make_module(self, name, pathentry=None, text=None):
        if text:
            text = dedent(text)
        if pathentry is None:
            pathentry = self.temp_dir()
        else:
            os.makedirs(pathentry, exist_ok=True)
        *subnames, basename = name.split('.')

        dirname = pathentry
        for subname in subnames:
            dirname = os.path.join(dirname, subname)
            if os.path.isdir(dirname):
                pass
            elif os.path.exists(dirname):
                raise Exception(dirname)
            else:
                os.mkdir(dirname)
            initfile = os.path.join(dirname, '__init__.py')
            if not os.path.exists(initfile):
                with open(initfile, 'w'):
                    pass
        filename = os.path.join(dirname, basename + '.py')

        with open(filename, 'w', encoding='utf-8') as outfile:
            outfile.write(text or '')
        return filename

    @support.requires_subprocess()
    def run_python(self, *argv):
        proc = subprocess.run(
            [sys.executable, *argv],
            capture_output=True,
            text=True,
        )
        return proc.returncode, proc.stdout, proc.stderr

    def assert_python_ok(self, *argv):
        exitcode, stdout, stderr = self.run_python(*argv)
        self.assertNotEqual(exitcode, 1)
        return stdout, stderr

    def assert_python_failure(self, *argv):
        exitcode, stdout, stderr = self.run_python(*argv)
        self.assertNotEqual(exitcode, 0)
        return stdout, stderr

    def assert_ns_equal(self, ns1, ns2, msg=None):
        # This is mostly copied from TestCase.assertDictEqual.
        self.assertEqual(type(ns1), type(ns2))
        if ns1 == ns2:
            return

        import difflib
        import pprint
        from unittest.util import _common_shorten_repr
        standardMsg = '%s != %s' % _common_shorten_repr(ns1, ns2)
        diff = ('\n' + '\n'.join(difflib.ndiff(
                       pprint.pformat(vars(ns1)).splitlines(),
                       pprint.pformat(vars(ns2)).splitlines())))
        diff = f'namespace({diff})'
        standardMsg = self._truncateMessage(standardMsg, diff)
        self.fail(self._formatMessage(msg, standardMsg))

    @requires__testinternalcapi
    def run_external(self, script, config='legacy'):
        with CapturingScript(script, combined=False) as captured:
            with WatchedScript(captured.script) as watched:
                rc = run_in_interpreter(watched.script, config)
                assert rc == 0, rc
                text = watched.read(100).decode('utf-8')
            err = watched.finished
            if err is True:
                err = None
        return err, text
