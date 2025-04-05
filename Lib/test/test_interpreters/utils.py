from collections import namedtuple
import contextlib
import json
import io
import logging
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
import warnings

from test import support

# We would use test.support.import_helper.import_module(),
# but the indirect import of test.support.os_helper causes refleaks.
try:
    import _interpreters
except ImportError as exc:
    raise unittest.SkipTest(str(exc))
from test.support import interpreters


try:
    import _testinternalcapi
    import _testcapi
except ImportError:
    _testinternalcapi = None
    _testcapi = None

def requires_test_modules(func):
    return unittest.skipIf(_testinternalcapi is None, "test requires _testinternalcapi module")(func)


def _dump_script(text):
    lines = text.splitlines()
    print()
    print('-' * 20)
    for i, line in enumerate(lines, 1):
        print(f' {i:>{len(str(len(lines)))}}  {line}')
    print('-' * 20)


def _close_file(file):
    try:
        if hasattr(file, 'close'):
            file.close()
        else:
            os.close(file)
    except OSError as exc:
        if exc.errno != 9:
            raise  # re-raise
        # It was closed already.


def pack_exception(exc=None):
    captured = _interpreters.capture_exception(exc)
    data = dict(captured.__dict__)
    data['type'] = dict(captured.type.__dict__)
    return json.dumps(data)


def unpack_exception(packed):
    try:
        data = json.loads(packed)
    except json.decoder.JSONDecodeError as e:
        logging.getLogger(__name__).warning('incomplete exception data', exc_info=e)
        print(packed if isinstance(packed, str) else packed.decode('utf-8'))
        return None
    exc = types.SimpleNamespace(**data)
    exc.type = types.SimpleNamespace(**exc.type)
    return exc;


class CapturingResults:

    STDIO = dedent("""\
        with open({w_pipe}, 'wb', buffering=0) as _spipe_{stream}:
            _captured_std{stream} = io.StringIO()
            with contextlib.redirect_std{stream}(_captured_std{stream}):
                #########################
                # begin wrapped script

                {indented}

                # end wrapped script
                #########################
            text = _captured_std{stream}.getvalue()
            _spipe_{stream}.write(text.encode('utf-8'))
        """)[:-1]
    EXC = dedent("""\
        with open({w_pipe}, 'wb', buffering=0) as _spipe_exc:
            try:
                #########################
                # begin wrapped script

                {indented}

                # end wrapped script
                #########################
            except Exception as exc:
                text = _interp_utils.pack_exception(exc)
                _spipe_exc.write(text.encode('utf-8'))
        """)[:-1]

    @classmethod
    def wrap_script(cls, script, *, stdout=True, stderr=False, exc=False):
        script = dedent(script).strip(os.linesep)
        imports = [
            f'import {__name__} as _interp_utils',
        ]
        wrapped = script

        # Handle exc.
        if exc:
            exc = os.pipe()
            r_exc, w_exc = exc
            indented = wrapped.replace('\n', '\n        ')
            wrapped = cls.EXC.format(
                w_pipe=w_exc,
                indented=indented,
            )
        else:
            exc = None

        # Handle stdout.
        if stdout:
            imports.extend([
                'import contextlib, io',
            ])
            stdout = os.pipe()
            r_out, w_out = stdout
            indented = wrapped.replace('\n', '\n        ')
            wrapped = cls.STDIO.format(
                w_pipe=w_out,
                indented=indented,
                stream='out',
            )
        else:
            stdout = None

        # Handle stderr.
        if stderr == 'stdout':
            stderr = None
        elif stderr:
            if not stdout:
                imports.extend([
                    'import contextlib, io',
                ])
            stderr = os.pipe()
            r_err, w_err = stderr
            indented = wrapped.replace('\n', '\n        ')
            wrapped = cls.STDIO.format(
                w_pipe=w_err,
                indented=indented,
                stream='err',
            )
        else:
            stderr = None

        if wrapped == script:
            raise NotImplementedError
        else:
            for line in imports:
                wrapped = f'{line}{os.linesep}{wrapped}'

        results = cls(stdout, stderr, exc)
        return wrapped, results

    def __init__(self, out, err, exc):
        self._rf_out = None
        self._rf_err = None
        self._rf_exc = None
        self._w_out = None
        self._w_err = None
        self._w_exc = None

        if out is not None:
            r_out, w_out = out
            self._rf_out = open(r_out, 'rb', buffering=0)
            self._w_out = w_out

        if err is not None:
            r_err, w_err = err
            self._rf_err = open(r_err, 'rb', buffering=0)
            self._w_err = w_err

        if exc is not None:
            r_exc, w_exc = exc
            self._rf_exc = open(r_exc, 'rb', buffering=0)
            self._w_exc = w_exc

        self._buf_out = b''
        self._buf_err = b''
        self._buf_exc = b''
        self._exc = None

        self._closed = False

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    @property
    def closed(self):
        return self._closed

    def close(self):
        if self._closed:
            return
        self._closed = True

        if self._w_out is not None:
            _close_file(self._w_out)
            self._w_out = None
        if self._w_err is not None:
            _close_file(self._w_err)
            self._w_err = None
        if self._w_exc is not None:
            _close_file(self._w_exc)
            self._w_exc = None

        self._capture()

        if self._rf_out is not None:
            _close_file(self._rf_out)
            self._rf_out = None
        if self._rf_err is not None:
            _close_file(self._rf_err)
            self._rf_err = None
        if self._rf_exc is not None:
            _close_file(self._rf_exc)
            self._rf_exc = None

    def _capture(self):
        # Ideally this is called only after the script finishes
        # (and thus has closed the write end of the pipe.
        if self._rf_out is not None:
            chunk = self._rf_out.read(100)
            while chunk:
                self._buf_out += chunk
                chunk = self._rf_out.read(100)
        if self._rf_err is not None:
            chunk = self._rf_err.read(100)
            while chunk:
                self._buf_err += chunk
                chunk = self._rf_err.read(100)
        if self._rf_exc is not None:
            chunk = self._rf_exc.read(100)
            while chunk:
                self._buf_exc += chunk
                chunk = self._rf_exc.read(100)

    def _unpack_stdout(self):
        return self._buf_out.decode('utf-8')

    def _unpack_stderr(self):
        return self._buf_err.decode('utf-8')

    def _unpack_exc(self):
        if self._exc is not None:
            return self._exc
        if not self._buf_exc:
            return None
        self._exc = unpack_exception(self._buf_exc)
        return self._exc

    def stdout(self):
        if self.closed:
            return self.final().stdout
        self._capture()
        return self._unpack_stdout()

    def stderr(self):
        if self.closed:
            return self.final().stderr
        self._capture()
        return self._unpack_stderr()

    def exc(self):
        if self.closed:
            return self.final().exc
        self._capture()
        return self._unpack_exc()

    def final(self, *, force=False):
        try:
            return self._final
        except AttributeError:
            if not self._closed:
                if not force:
                    raise Exception('no final results available yet')
                else:
                    return CapturedResults.Proxy(self)
            self._final = CapturedResults(
                self._unpack_stdout(),
                self._unpack_stderr(),
                self._unpack_exc(),
            )
            return self._final


class CapturedResults(namedtuple('CapturedResults', 'stdout stderr exc')):

    class Proxy:
        def __init__(self, capturing):
            self._capturing = capturing
        def _finish(self):
            if self._capturing is None:
                return
            self._final = self._capturing.final()
            self._capturing = None
        def __iter__(self):
            self._finish()
            yield from self._final
        def __len__(self):
            self._finish()
            return len(self._final)
        def __getattr__(self, name):
            self._finish()
            if name.startswith('_'):
                raise AttributeError(name)
            return getattr(self._final, name)

    def raise_if_failed(self):
        if self.exc is not None:
            raise interpreters.ExecutionFailed(self.exc)


def _captured_script(script, *, stdout=True, stderr=False, exc=False):
    return CapturingResults.wrap_script(
        script,
        stdout=stdout,
        stderr=stderr,
        exc=exc,
    )


def clean_up_interpreters():
    for interp in interpreters.list_all():
        if interp.id == 0:  # main
            continue
        try:
            interp.close()
        except _interpreters.InterpreterError:
            pass  # already destroyed


def _run_output(interp, request, init=None):
    script, results = _captured_script(request)
    with results:
        if init:
            interp.prepare_main(init)
        interp.exec(script)
    return results.stdout()


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
        from test.support import os_helper
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

    def _run_string(self, interp, script):
        wrapped, results = _captured_script(script, exc=False)
        #_dump_script(wrapped)
        with results:
            if isinstance(interp, interpreters.Interpreter):
                interp.exec(script)
            else:
                err = _interpreters.run_string(interp, wrapped)
                if err is not None:
                    return None, err
        return results.stdout(), None

    def run_and_capture(self, interp, script):
        text, err = self._run_string(interp, script)
        if err is not None:
            raise interpreters.ExecutionFailed(err)
        else:
            return text

    def interp_exists(self, interpid):
        try:
            _interpreters.whence(interpid)
        except _interpreters.InterpreterNotFoundError:
            return False
        else:
            return True

    @requires_test_modules
    @contextlib.contextmanager
    def interpreter_from_capi(self, config=None, whence=None):
        if config is False:
            if whence is None:
                whence = _interpreters.WHENCE_LEGACY_CAPI
            else:
                assert whence in (_interpreters.WHENCE_LEGACY_CAPI,
                                  _interpreters.WHENCE_UNKNOWN), repr(whence)
            config = None
        elif config is True:
            config = _interpreters.new_config('default')
        elif config is None:
            if whence not in (
                _interpreters.WHENCE_LEGACY_CAPI,
                _interpreters.WHENCE_UNKNOWN,
            ):
                config = _interpreters.new_config('legacy')
        elif isinstance(config, str):
            config = _interpreters.new_config(config)

        if whence is None:
            whence = _interpreters.WHENCE_XI

        interpid = _testinternalcapi.create_interpreter(config, whence=whence)
        try:
            yield interpid
        finally:
            try:
                _testinternalcapi.destroy_interpreter(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass

    @contextlib.contextmanager
    def interpreter_obj_from_capi(self, config='legacy'):
        with self.interpreter_from_capi(config) as interpid:
            interp = interpreters.Interpreter(
                interpid,
                _whence=_interpreters.WHENCE_CAPI,
                _ownsref=False,
            )
            yield interp, interpid

    @contextlib.contextmanager
    def capturing(self, script):
        wrapped, capturing = _captured_script(script, stdout=True, exc=True)
        #_dump_script(wrapped)
        with capturing:
            yield wrapped, capturing.final(force=True)

    @requires_test_modules
    def run_from_capi(self, interpid, script, *, main=False):
        with self.capturing(script) as (wrapped, results):
            rc = _testinternalcapi.exec_interpreter(interpid, wrapped, main=main)
            assert rc == 0, rc
        results.raise_if_failed()
        return results.stdout

    @contextlib.contextmanager
    def _running(self, run_interp, exec_interp):
        token = b'\0'
        r_in, w_in = self.pipe()
        r_out, w_out = self.pipe()

        def close():
            _close_file(r_in)
            _close_file(w_in)
            _close_file(r_out)
            _close_file(w_out)

        # Start running (and wait).
        script = dedent(f"""
            import os
            try:
                # handshake
                token = os.read({r_in}, 1)
                os.write({w_out}, token)
                # Wait for the "done" message.
                os.read({r_in}, 1)
            except BrokenPipeError:
                pass
            except OSError as exc:
                if exc.errno != 9:
                    raise  # re-raise
                # It was closed already.
            """)
        failed = None
        def run():
            nonlocal failed
            try:
                run_interp(script)
            except Exception as exc:
                failed = exc
                close()
        t = threading.Thread(target=run)
        t.start()

        # handshake
        try:
            os.write(w_in, token)
            token2 = os.read(r_out, 1)
            assert token2 == token, (token2, token)
        except OSError:
            t.join()
            if failed is not None:
                raise failed

        # CM __exit__()
        try:
            try:
                yield
            finally:
                # Send "done".
                os.write(w_in, b'\0')
        finally:
            close()
            t.join()
            if failed is not None:
                raise failed

    @contextlib.contextmanager
    def running(self, interp):
        if isinstance(interp, int):
            interpid = interp
            def exec_interp(script):
                exc = _interpreters.exec(interpid, script)
                assert exc is None, exc
            run_interp = exec_interp
        else:
            def run_interp(script):
                text = self.run_and_capture(interp, script)
                assert text == '', repr(text)
            def exec_interp(script):
                interp.exec(script)
        with self._running(run_interp, exec_interp):
            yield

    @requires_test_modules
    @contextlib.contextmanager
    def running_from_capi(self, interpid, *, main=False):
        def run_interp(script):
            text = self.run_from_capi(interpid, script, main=main)
            assert text == '', repr(text)
        def exec_interp(script):
            rc = _testinternalcapi.exec_interpreter(interpid, script)
            assert rc == 0, rc
        with self._running(run_interp, exec_interp):
            yield

    @requires_test_modules
    def run_temp_from_capi(self, script, config='legacy'):
        if config is False:
            # Force using Py_NewInterpreter().
            run_in_interp = (lambda s, c: _testcapi.run_in_subinterp(s))
            config = None
        else:
            run_in_interp = _testinternalcapi.run_in_subinterp_with_config
            if config is True:
                config = 'default'
            if isinstance(config, str):
                config = _interpreters.new_config(config)
        with self.capturing(script) as (wrapped, results):
            rc = run_in_interp(wrapped, config)
            assert rc == 0, rc
        results.raise_if_failed()
        return results.stdout
