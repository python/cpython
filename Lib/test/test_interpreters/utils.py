import contextlib
import os
import os.path
import subprocess
import sys
import tempfile
import threading
from textwrap import dedent
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
        interp.exec_sync(script)
        return rpipe.read()


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interp.exec_sync(dedent(f"""
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

    def tearDown(self):
        clean_up_interpreters()
