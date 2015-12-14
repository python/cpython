# Common utility functions used by various script execution tests
#  e.g. test_cmd_line, test_cmd_line_script and test_runpy

import collections
import importlib
import sys
import os
import os.path
import tempfile
import subprocess
import py_compile
import contextlib
import shutil
import zipfile

from importlib.util import source_from_cache
from test.support import make_legacy_pyc, strip_python_stderr, temp_dir


# Cached result of the expensive test performed in the function below.
__cached_interp_requires_environment = None

def _interpreter_requires_environment():
    """
    Returns True if our sys.executable interpreter requires environment
    variables in order to be able to run at all.

    This is designed to be used with @unittest.skipIf() to annotate tests
    that need to use an assert_python*() function to launch an isolated
    mode (-I) or no environment mode (-E) sub-interpreter process.

    A normal build & test does not run into this situation but it can happen
    when trying to run the standard library test suite from an interpreter that
    doesn't have an obvious home with Python's current home finding logic.

    Setting PYTHONHOME is one way to get most of the testsuite to run in that
    situation.  PYTHONPATH or PYTHONUSERSITE are other common environment
    variables that might impact whether or not the interpreter can start.
    """
    global __cached_interp_requires_environment
    if __cached_interp_requires_environment is None:
        # Try running an interpreter with -E to see if it works or not.
        try:
            subprocess.check_call([sys.executable, '-E',
                                   '-c', 'import sys; sys.exit(0)'])
        except subprocess.CalledProcessError:
            __cached_interp_requires_environment = True
        else:
            __cached_interp_requires_environment = False

    return __cached_interp_requires_environment


_PythonRunResult = collections.namedtuple("_PythonRunResult",
                                          ("rc", "out", "err"))


# Executing the interpreter in a subprocess
def run_python_until_end(*args, **env_vars):
    env_required = _interpreter_requires_environment()
    if '__isolated' in env_vars:
        isolated = env_vars.pop('__isolated')
    else:
        isolated = not env_vars and not env_required
    cmd_line = [sys.executable, '-X', 'faulthandler']
    if isolated:
        # isolated mode: ignore Python environment variables, ignore user
        # site-packages, and don't add the current directory to sys.path
        cmd_line.append('-I')
    elif not env_vars and not env_required:
        # ignore Python environment variables
        cmd_line.append('-E')
    # Need to preserve the original environment, for in-place testing of
    # shared library builds.
    env = os.environ.copy()
    # But a special flag that can be set to override -- in this case, the
    # caller is responsible to pass the full environment.
    if env_vars.pop('__cleanenv', None):
        env = {}
    env.update(env_vars)
    cmd_line.extend(args)
    p = subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         env=env)
    try:
        out, err = p.communicate()
    finally:
        subprocess._cleanup()
        p.stdout.close()
        p.stderr.close()
    rc = p.returncode
    err = strip_python_stderr(err)
    return _PythonRunResult(rc, out, err), cmd_line

def _assert_python(expected_success, *args, **env_vars):
    res, cmd_line = run_python_until_end(*args, **env_vars)
    if (res.rc and expected_success) or (not res.rc and not expected_success):
        raise AssertionError(
            "Process return code is %d, command line was: %r, "
            "stderr follows:\n%s" % (res.rc, cmd_line,
                                     res.err.decode('ascii', 'ignore')))
    return res

def assert_python_ok(*args, **env_vars):
    """
    Assert that running the interpreter with `args` and optional environment
    variables `env_vars` succeeds (rc == 0) and return a (return code, stdout,
    stderr) tuple.

    If the __cleanenv keyword is set, env_vars is used as a fresh environment.

    Python is started in isolated mode (command line option -I),
    except if the __isolated keyword is set to False.
    """
    return _assert_python(True, *args, **env_vars)

def assert_python_failure(*args, **env_vars):
    """
    Assert that running the interpreter with `args` and optional environment
    variables `env_vars` fails (rc != 0) and return a (return code, stdout,
    stderr) tuple.

    See assert_python_ok() for more options.
    """
    return _assert_python(False, *args, **env_vars)

def spawn_python(*args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw):
    """Run a Python subprocess with the given arguments.

    kw is extra keyword args to pass to subprocess.Popen. Returns a Popen
    object.
    """
    cmd_line = [sys.executable, '-E']
    cmd_line.extend(args)
    # Under Fedora (?), GNU readline can output junk on stderr when initialized,
    # depending on the TERM setting.  Setting TERM=vt100 is supposed to disable
    # that.  References:
    # - http://reinout.vanrees.org/weblog/2009/08/14/readline-invisible-character-hack.html
    # - http://stackoverflow.com/questions/15760712/python-readline-module-prints-escape-character-during-import
    # - http://lists.gnu.org/archive/html/bug-readline/2007-08/msg00004.html
    env = kw.setdefault('env', dict(os.environ))
    env['TERM'] = 'vt100'
    return subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                            stdout=stdout, stderr=stderr,
                            **kw)

def kill_python(p):
    """Run the given Popen process until completion and return stdout."""
    p.stdin.close()
    data = p.stdout.read()
    p.stdout.close()
    # try to cleanup the child so we don't appear to leak when running
    # with regrtest -R.
    p.wait()
    subprocess._cleanup()
    return data

def make_script(script_dir, script_basename, source, omit_suffix=False):
    script_filename = script_basename
    if not omit_suffix:
        script_filename += os.extsep + 'py'
    script_name = os.path.join(script_dir, script_filename)
    # The script should be encoded to UTF-8, the default string encoding
    script_file = open(script_name, 'w', encoding='utf-8')
    script_file.write(source)
    script_file.close()
    importlib.invalidate_caches()
    return script_name

def make_zip_script(zip_dir, zip_basename, script_name, name_in_zip=None):
    zip_filename = zip_basename+os.extsep+'zip'
    zip_name = os.path.join(zip_dir, zip_filename)
    zip_file = zipfile.ZipFile(zip_name, 'w')
    if name_in_zip is None:
        parts = script_name.split(os.sep)
        if len(parts) >= 2 and parts[-2] == '__pycache__':
            legacy_pyc = make_legacy_pyc(source_from_cache(script_name))
            name_in_zip = os.path.basename(legacy_pyc)
            script_name = legacy_pyc
        else:
            name_in_zip = os.path.basename(script_name)
    zip_file.write(script_name, name_in_zip)
    zip_file.close()
    #if test.support.verbose:
    #    zip_file = zipfile.ZipFile(zip_name, 'r')
    #    print 'Contents of %r:' % zip_name
    #    zip_file.printdir()
    #    zip_file.close()
    return zip_name, os.path.join(zip_name, name_in_zip)

def make_pkg(pkg_dir, init_source=''):
    os.mkdir(pkg_dir)
    make_script(pkg_dir, '__init__', init_source)

def make_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                 source, depth=1, compiled=False):
    unlink = []
    init_name = make_script(zip_dir, '__init__', '')
    unlink.append(init_name)
    init_basename = os.path.basename(init_name)
    script_name = make_script(zip_dir, script_basename, source)
    unlink.append(script_name)
    if compiled:
        init_name = py_compile.compile(init_name, doraise=True)
        script_name = py_compile.compile(script_name, doraise=True)
        unlink.extend((init_name, script_name))
    pkg_names = [os.sep.join([pkg_name]*i) for i in range(1, depth+1)]
    script_name_in_zip = os.path.join(pkg_names[-1], os.path.basename(script_name))
    zip_filename = zip_basename+os.extsep+'zip'
    zip_name = os.path.join(zip_dir, zip_filename)
    zip_file = zipfile.ZipFile(zip_name, 'w')
    for name in pkg_names:
        init_name_in_zip = os.path.join(name, init_basename)
        zip_file.write(init_name, init_name_in_zip)
    zip_file.write(script_name, script_name_in_zip)
    zip_file.close()
    for name in unlink:
        os.unlink(name)
    #if test.support.verbose:
    #    zip_file = zipfile.ZipFile(zip_name, 'r')
    #    print 'Contents of %r:' % zip_name
    #    zip_file.printdir()
    #    zip_file.close()
    return zip_name, os.path.join(zip_name, script_name_in_zip)
