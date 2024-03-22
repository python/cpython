import contextlib
import os
import os.path
import pickle
import subprocess
import sys
import tempfile
from textwrap import dedent
import threading
import types
import unittest

from test import support
from test.support import os_helper

from test.support import interpreters


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


class UnmanagedInterpreter:

    @classmethod
    def start(cls, create_pipe, *, legacy=True):
        r_in, _ = inpipe = create_pipe()
        r_out, w_out = outpipe = create_pipe()

        script = dedent(f"""
            import pickle, os, traceback, _testcapi

            # Send the interpreter ID.
            interpid = _testcapi.get_current_interpid()
            os.write({w_out}, pickle.dumps(interpid))

            # Run exec requests until "done".
            script = b''
            while True:
                ch = os.read({r_in}, 1)
                if ch == b'\0':
                    if not script:
                        # done!
                        break

                    # Run the provided script.
                    try:
                        exec(script)
                    except Exception as exc:
                        traceback.print_exc()
                        err = traceback.format_exception_only(exc)
                        os.write(w_out, err.encode('utf-8'))
                    os.write(w_out, b'\0')
                    script = b''
                else:
                    script += ch
            """)
        def run():
            try:
                if legacy:
                    rc = support.run_in_subinterp(script)
                else:
                    rc = support.run_in_subinterp_with_config(script)
                assert rc == 0, rc
            except BaseException:
                os.write(w_out, b'\0')
                raise  # re-raise
        t = threading.Thread(target=run)
        t.start()
        try:
            # Get the interpreter ID.
            data = os.read(r_out, 10)
            assert len(data) < 10, repr(data) 
            assert data != b'\0'
            interpid = pickle.loads(data)

            return cls(interpid, t, inpipe, outpipe)
        except BaseException:
            os.write(w_out, b'\0')
            raise  # re-raise

    def __init__(self, id, thread, inpipe, outpipe):
        self._id = id
        self._thread = thread
        self._r_in, self._w_in = inpipe
        self._r_out, self._w_out = outpipe

    def __repr__(self):
        return f'<{type(self).__name__} {self._id}>'

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.shutdown()

    @property
    def id(self):
        return self._id

    def exec(self, code):
        if isinstance(code, str):
            code = code.encode('utf-8')
        elif not isinstance(code, bytes):
            raise TypeError(f'expected str/bytes, got {code!r}')
        if not code:
            raise ValueError('empty code')
        os.write(self._w_in, code)
        os.write(self._w_in, b'\0')

        out = b''
        while True:
            ch = os.read(self._r_out, 1)
            if ch == b'\0':
                break
            out += ch
        return out.decode('utf-8')

    def shutdown(self, *, wait=True):
        t = self._thread
        self._thread = None
        if t is not None:
            os.write(self._w_in, b'\0')
            if wait:
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

    def unmanaged_interpreter(self, *, legacy=True):
        return UnmanagedInterpreter.start(self.pipe, legacy=legacy)
#    @contextlib.contextmanager
#    def unmanaged_interpreter(self, *, legacy=True):
#        r_id, w_id = self.pipe()
#        r_done, w_done = self.pipe()
#        script = dedent(f"""
#            import marshal, os, _testcapi
#
#            # Send the interpreter ID.
#            interpid = _testcapi.get_current_interpid()
#            data = marshal.dumps(interpid)
#            os.write({w_id}, data)
#
#            # Wait for "done".
#            os.read({r_done}, 1)
#            """)
#        rc = None
#        def task():
#            nonlocal rc
#            if legacy:
#                rc = support.run_in_subinterp(script)
#            else:
#                rc = support.run_in_subinterp_with_config(script)
#        t = threading.Thread(target=task)
#        t.start()
#        try:
#            # Get the interpreter ID.
#            data = os.read(r_id, 10)
#            assert len(data) < 10, repr(data) 
#            yield marshal.loads(data)
#        finally:
#            # Send "done".
#            os.write(w_done, b'\0')
#            t.join()
#        self.assertEqual(rc, 0)
