from __future__ import print_function, unicode_literals

import os
import subprocess
import sys
import textwrap
import time
import traceback
from collections import OrderedDict
from fnmatch import fnmatch

import py
import pytest
import six

import tox
import tox.session
from tox import venv
from tox.config import parseconfig
from tox.config.parallel import ENV_VAR_KEY_PRIVATE as PARALLEL_ENV_VAR_KEY_PRIVATE
from tox.config.parallel import ENV_VAR_KEY_PUBLIC as PARALLEL_ENV_VAR_KEY_PUBLIC
from tox.reporter import update_default_reporter
from tox.venv import CreationConfig, VirtualEnv, getdigest

mark_dont_run_on_windows = pytest.mark.skipif(os.name == "nt", reason="non windows test")
mark_dont_run_on_posix = pytest.mark.skipif(os.name == "posix", reason="non posix test")


def pytest_configure():
    if "TOXENV" in os.environ:
        del os.environ["TOXENV"]
    if "HUDSON_URL" in os.environ:
        del os.environ["HUDSON_URL"]


def pytest_addoption(parser):
    parser.addoption(
        "--no-network",
        action="store_true",
        dest="no_network",
        help="don't run tests requiring network",
    )


def pytest_report_header():
    return "tox comes from: {!r}".format(tox.__file__)


@pytest.fixture
def work_in_clean_dir(tmpdir):
    with tmpdir.as_cwd():
        yield


@pytest.fixture(autouse=True)
def check_cwd_not_changed_by_test():
    old = os.getcwd()
    yield
    new = os.getcwd()
    if old != new:
        pytest.fail("test changed cwd: {!r} => {!r}".format(old, new))


@pytest.fixture(autouse=True)
def check_os_environ_stable():
    old = os.environ.copy()

    to_clean = {
        k: os.environ.pop(k, None)
        for k in {
            PARALLEL_ENV_VAR_KEY_PRIVATE,
            PARALLEL_ENV_VAR_KEY_PUBLIC,
            str("TOX_WORK_DIR"),
            str("PYTHONPATH"),
        }
    }

    yield

    for key, value in to_clean.items():
        if value is not None:
            os.environ[key] = value

    new = os.environ
    extra = {k: new[k] for k in set(new) - set(old)}
    miss = {k: old[k] for k in set(old) - set(new)}
    diff = {
        "{} = {} vs {}".format(k, old[k], new[k])
        for k in set(old) & set(new)
        if old[k] != new[k] and not k.startswith("PYTEST_")
    }
    if extra or miss or diff:
        msg = "test changed environ"
        if extra:
            msg += " extra {}".format(extra)
        if miss:
            msg += " miss {}".format(miss)
        if diff:
            msg += " diff {}".format(diff)
        pytest.fail(msg)


@pytest.fixture(name="newconfig")
def create_new_config_file(tmpdir):
    def create_new_config_file_(args, source=None, plugins=(), filename="tox.ini"):
        if source is None:
            source = args
            args = []
        s = textwrap.dedent(source)
        p = tmpdir.join(filename)
        p.write(s)
        tox.session.setup_reporter(args)
        with tmpdir.as_cwd():
            return parseconfig(args, plugins=plugins)

    return create_new_config_file_


@pytest.fixture
def cmd(request, monkeypatch, capfd):
    if request.config.option.no_network:
        pytest.skip("--no-network was specified, test cannot run")
    request.addfinalizer(py.path.local().chdir)

    def run(*argv):
        reset_report()
        with RunResult(argv, capfd) as result:
            _collect_session(result)

            # noinspection PyBroadException
            try:
                tox.session.main([str(x) for x in argv])
                assert False  # this should always exist with SystemExit
            except SystemExit as exception:
                result.ret = exception.code
            except OSError as e:
                traceback.print_exc()
                result.ret = e.errno
            except Exception:
                traceback.print_exc()
                result.ret = 1
        return result

    def _collect_session(result):
        prev_build = tox.session.build_session

        def build_session(config):
            result.session = prev_build(config)
            return result.session

        monkeypatch.setattr(tox.session, "build_session", build_session)

    yield run


class RunResult:
    def __init__(self, args, capfd):
        self.args = args
        self.ret = None
        self.duration = None
        self.out = None
        self.err = None
        self.session = None
        self.capfd = capfd

    def __enter__(self):
        self._start = time.time()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.duration = time.time() - self._start
        self.out, self.err = self.capfd.readouterr()

    def _read(self, out, pos):
        out.buffer.seek(pos)
        return out.buffer.read().decode(out.encoding, errors=out.errors)

    @property
    def outlines(self):
        out = [] if self.out is None else self.out.splitlines()
        err = [] if self.err is None else self.err.splitlines()
        return err + out

    def __repr__(self):
        res = "RunResult(ret={}, args={!r}, out=\n{}\n, err=\n{})".format(
            self.ret,
            self.args,
            self.out,
            self.err,
        )
        if six.PY2:
            return res.encode("UTF-8")
        else:
            return res

    def output(self):
        return "{}\n{}\n{}".format(self.ret, self.err, self.out)

    def assert_success(self, is_run_test_env=True):
        msg = self.output()
        assert self.ret == 0, msg
        if is_run_test_env:
            assert any("  congratulations :)" == line for line in reversed(self.outlines)), msg

    def assert_fail(self, is_run_test_env=True):
        msg = self.output()
        assert self.ret, msg
        if is_run_test_env:
            assert not any("  congratulations :)" == line for line in reversed(self.outlines)), msg


class ReportExpectMock:
    def __init__(self):
        from tox import reporter

        self.instance = reporter._INSTANCE
        self.clear()
        self._index = -1

    def clear(self):
        self._index = -1
        if not six.PY2:
            self.instance.reported_lines.clear()
        else:
            del self.instance.reported_lines[:]

    def getnext(self, cat):
        __tracebackhide__ = True
        newindex = self._index + 1
        while newindex < len(self.instance.reported_lines):
            call = self.instance.reported_lines[newindex]
            lcat = call[0]
            if fnmatch(lcat, cat):
                self._index = newindex
                return call
            newindex += 1
        raise LookupError(
            "looking for {!r}, no reports found at >={:d} in {!r}".format(
                cat,
                self._index + 1,
                self.instance.reported_lines,
            ),
        )

    def expect(self, cat, messagepattern="*", invert=False):
        __tracebackhide__ = True
        if not messagepattern.startswith("*"):
            messagepattern = "*{}".format(messagepattern)
        while self._index < len(self.instance.reported_lines):
            try:
                call = self.getnext(cat)
            except LookupError:
                break
            for lmsg in call[1:]:
                lmsg = str(lmsg).replace("\n", " ")
                if fnmatch(lmsg, messagepattern):
                    if invert:
                        raise AssertionError(
                            "found {}({!r}), didn't expect it".format(cat, messagepattern),
                        )
                    return
        if not invert:
            raise AssertionError(
                "looking for {}({!r}), no reports found at >={:d} in {!r}".format(
                    cat,
                    messagepattern,
                    self._index + 1,
                    self.instance.reported_lines,
                ),
            )

    def not_expect(self, cat, messagepattern="*"):
        return self.expect(cat, messagepattern, invert=True)


class pcallMock:
    def __init__(self, args, cwd, env, stdout, stderr, shell):
        self.arg0 = args[0]
        self.args = args
        self.cwd = cwd
        self.env = env
        self.stdout = stdout
        self.stderr = stderr
        self.shell = shell
        self.pid = os.getpid()
        self.returncode = 0

    @staticmethod
    def communicate():
        return "", ""

    def wait(self):
        pass


@pytest.fixture(name="mocksession")
def create_mocksession(request):
    config = request.getfixturevalue("newconfig")([], "")

    class MockSession(tox.session.Session):
        def __init__(self, config):
            self.logging_levels(config.option.quiet_level, config.option.verbose_level)
            super(MockSession, self).__init__(config, popen=self.popen)
            self._pcalls = []
            self.report = ReportExpectMock()

        def _clearmocks(self):
            if not six.PY2:
                self._pcalls.clear()
            else:
                del self._pcalls[:]
            self.report.clear()

        def popen(self, args, cwd, shell=None, stdout=None, stderr=None, env=None, **_):
            process_call_mock = pcallMock(args, cwd, env, stdout, stderr, shell)
            self._pcalls.append(process_call_mock)
            return process_call_mock

        def new_config(self, config):
            self.logging_levels(config.option.quiet_level, config.option.verbose_level)
            self.config = config
            self.venv_dict.clear()
            self.existing_venvs.clear()

        def logging_levels(self, quiet, verbose):
            update_default_reporter(quiet, verbose)
            if hasattr(self, "config"):
                self.config.option.quiet_level = quiet
                self.config.option.verbose_level = verbose

    return MockSession(config)


@pytest.fixture
def newmocksession(mocksession, newconfig):
    def newmocksession_(args, source, plugins=()):
        config = newconfig(args, source, plugins=plugins)
        mocksession._reset(config, mocksession.popen)
        return mocksession

    return newmocksession_


def getdecoded(out):
    try:
        return out.decode("utf-8")
    except UnicodeDecodeError:
        return "INTERNAL not-utf8-decodeable, truncated string:\n{}".format(py.io.saferepr(out))


@pytest.fixture
def initproj(tmpdir):
    """Create a factory function for creating example projects.

    Constructed folder/file hierarchy examples:

    with `src_root` other than `.`:

      tmpdir/
          name/                  # base
            src_root/            # src_root
                name/            # package_dir
                    __init__.py
                name.egg-info/   # created later on package build
            setup.py

    with `src_root` given as `.`:

      tmpdir/
          name/                  # base, src_root
            name/                # package_dir
                __init__.py
            name.egg-info/       # created later on package build
            setup.py
    """

    def initproj_(nameversion, filedefs=None, src_root=".", add_missing_setup_py=True):
        if filedefs is None:
            filedefs = {}
        if not src_root:
            src_root = "."
        if isinstance(nameversion, six.string_types):
            parts = nameversion.rsplit(str("-"), 1)
            if len(parts) == 1:
                parts.append("0.1")
            name, version = parts
        else:
            name, version = nameversion
        base = tmpdir.join(name)
        src_root_path = _path_join(base, src_root)
        assert base == src_root_path or src_root_path.relto(
            base,
        ), "`src_root` must be the constructed project folder or its direct or indirect subfolder"

        base.ensure(dir=1)
        create_files(base, filedefs)
        if not _filedefs_contains(base, filedefs, "setup.py") and add_missing_setup_py:
            create_files(
                base,
                {
                    "setup.py": """
                from setuptools import setup, find_packages
                setup(
                    name='{name}',
                    description='{name} project',
                    version='{version}',
                    license='MIT',
                    platforms=['unix', 'win32'],
                    packages=find_packages('{src_root}'),
                    package_dir={{'':'{src_root}'}},
                )
            """.format(
                        **locals()
                    ),
                },
            )
        if not _filedefs_contains(base, filedefs, src_root_path.join(name)):
            create_files(
                src_root_path,
                {
                    name: {
                        "__init__.py": textwrap.dedent(
                            '''
                """ module {} """
                __version__ = {!r}''',
                        )
                        .strip()
                        .format(name, version),
                    },
                },
            )
        manifestlines = [
            "include {}".format(p.relto(base)) for p in base.visit(lambda x: x.check(file=1))
        ]
        create_files(base, {"MANIFEST.in": "\n".join(manifestlines)})
        base.chdir()
        return base

    with py.path.local().as_cwd():
        yield initproj_


def _path_parts(path):
    path = path and str(path)  # py.path.local support
    parts = []
    while path:
        folder, name = os.path.split(path)
        if folder == path:  # root folder
            folder, name = name, folder
        if name:
            parts.append(name)
        path = folder
    parts.reverse()
    return parts


def _path_join(base, *args):
    # workaround for a py.path.local bug on Windows (`path.join('/x', abs=1)`
    # should be py.path.local('X:\\x') where `X` is the current drive, when in
    # fact it comes out as py.path.local('\\x'))
    return py.path.local(base.join(*args, abs=1))


def _filedefs_contains(base, filedefs, path):
    """
    whether `filedefs` defines a file/folder with the given `path`

    `path`, if relative, will be interpreted relative to the `base` folder, and
    whether relative or not, must refer to either the `base` folder or one of
    its direct or indirect children. The base folder itself is considered
    created if the filedefs structure is not empty.

    """
    unknown = object()
    base = py.path.local(base)
    path = _path_join(base, path)

    path_rel_parts = _path_parts(path.relto(base))
    for part in path_rel_parts:
        if not isinstance(filedefs, dict):
            return False
        filedefs = filedefs.get(part, unknown)
        if filedefs is unknown:
            return False
    return path_rel_parts or path == base and filedefs


def create_files(base, filedefs):
    for key, value in filedefs.items():
        if isinstance(value, dict):
            create_files(base.ensure(key, dir=1), value)
        elif isinstance(value, six.string_types):
            s = textwrap.dedent(value)
            base.join(key).write(s)


@pytest.fixture()
def mock_venv(monkeypatch):
    """This creates a mock virtual environment (e.g. will inherit the current interpreter).
    Note: because we inherit, to keep things sane you must call the py environment and only that;
    and cannot install any packages."""

    # first ensure we have a clean python path
    monkeypatch.delenv(str("PYTHONPATH"), raising=False)

    # object to collect some data during the execution
    class Result(object):
        def __init__(self, session):
            self.popens = popen_list
            self.session = session

    res = OrderedDict()

    # convince tox that the current running virtual environment is already the env we would create
    class ProxyCurrentPython:
        @classmethod
        def readconfig(cls, path):
            if path.dirname.endswith("{}py".format(os.sep)):
                return CreationConfig(
                    base_resolved_python_sha256=getdigest(sys.executable),
                    base_resolved_python_path=sys.executable,
                    tox_version=tox.__version__,
                    sitepackages=False,
                    usedevelop=False,
                    deps=[],
                    alwayscopy=False,
                )
            elif path.dirname.endswith("{}.package".format(os.sep)):
                return CreationConfig(
                    base_resolved_python_sha256=getdigest(sys.executable),
                    base_resolved_python_path=sys.executable,
                    tox_version=tox.__version__,
                    sitepackages=False,
                    usedevelop=False,
                    deps=[(getdigest(""), "setuptools >= 35.0.2"), (getdigest(""), "wheel")],
                    alwayscopy=False,
                )
            assert False  # pragma: no cover

    monkeypatch.setattr(CreationConfig, "readconfig", ProxyCurrentPython.readconfig)

    # provide as Python the current python executable
    def venv_lookup(venv, name):
        assert name == "python"
        venv.envconfig.envdir = py.path.local(sys.executable).join("..", "..")
        return sys.executable

    monkeypatch.setattr(VirtualEnv, "_venv_lookup", venv_lookup)

    # don't allow overriding the tox config data for the host Python
    def finish_venv(self):
        return

    monkeypatch.setattr(VirtualEnv, "finish", finish_venv)

    # we lie that it's an environment with no packages in it
    @tox.hookimpl
    def tox_runenvreport(venv, action):
        return []

    monkeypatch.setattr(venv, "tox_runenvreport", tox_runenvreport)

    # intercept the build session to save it and we intercept the popen invocations
    # collect all popen calls
    popen_list = []

    def popen(cmd, **kwargs):
        # we don't want to perform installation of new packages,
        # just replace with an always ok cmd
        if "pip" in cmd and "install" in cmd:
            cmd = ["python", "-c", "print({!r})".format(cmd)]
        ret = None
        try:
            ret = subprocess.Popen(cmd, **kwargs)
        except tox.exception.InvocationError as exception:  # pragma: no cover
            ret = exception  # pragma: no cover
        finally:
            popen_list.append((kwargs.get("env"), ret, cmd))
        return ret

    def build_session(config):
        session = tox.session.Session(config, popen=popen)
        res[id(session)] = Result(session)
        return session

    monkeypatch.setattr(tox.session, "build_session", build_session)
    return res


@pytest.fixture(scope="session")
def current_tox_py():
    """generate the current (test runners) python versions key
    e.g. py37 when running under Python 3.7"""
    return "{}{}{}".format("pypy" if tox.INFO.IS_PYPY else "py", *sys.version_info)


def pytest_runtest_setup(item):
    reset_report()


def pytest_runtest_teardown(item):
    reset_report()


def pytest_pyfunc_call(pyfuncitem):
    reset_report()


def reset_report(quiet=0, verbose=0):
    from tox.reporter import _INSTANCE

    _INSTANCE._reset(quiet_level=quiet, verbose_level=verbose)
