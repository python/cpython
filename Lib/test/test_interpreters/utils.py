import contextlib
import os
import os.path
import pickle
import select
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
                stdout = open(w_out, 'w', encoding='utf-8')
                stderr = open(w_err, 'w', encoding='utf-8')
            else:
                stdout = open(w_out, 'w', encoding='utf-8')
        else:
            assert w_err is not None
            stderr = open(w_err, 'w', encoding='utf-8')

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
        self._r_out, self._w_out = os.pipe()
        if combined:
            self._r_err = self._w_err = None
            w_err = self._w_out
        else:
            self._r_err, self._w_err = os.pipe()
            w_err = self._w_err
        self._combined = combined

        self._script = self.WRAPPER.format(
            w_out=self._w_out,
            w_err=w_err,
            wrapped=indent(script, '    '),
        )

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
        for fd in [self._r_out, self._w_out, self._r_err, self._w_err]:
            if fd is not None:
                try:
                    os.close(fd)
                except OSError:
                    if exc.errno != 9:
                        raise  # re-raise
                    # It was closed already.
        self._r_out = self._w_out = self._r_err = self._w_err = None

    def read(self, n=None):
        return self.read_stdout(n)

    def read_stdout(self, n=None):
        try:
            return os.read(self._r_out, n)
        except OSError as exc:
            if exc.errno != 9:
                raise  # re-raise
            # It was closed already.
            return b''

    def read_stderr(self, n=None):
        if self._combined:
            return b''
        try:
            return os.read(self._r_err, n)
        except OSError as exc:
            if exc.errno != 9:
                raise  # re-raise
            # It was closed already.
            return b''


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
            err = captured.finished
            if err is True:
                err = None
        return err, text
