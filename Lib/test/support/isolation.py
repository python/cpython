"""Run tests in isolated subprocesses (the test.support.isolation.isolated decorator).

A failure, error or skip that happens in the subprocess is replayed in the
parent process so that the test runner records it.  The original (subprocess)
traceback is attached as the cause of the replayed exception, the same way
:mod:`concurrent.futures` surfaces tracebacks from worker processes.
"""

import functools
import os
import sys
import unittest

# Mark this module's frames as belonging to the test machinery, so that
# unittest strips them from reported tracebacks (see TestResult._clean_tracebacks
# in Lib/unittest/result.py).  Only the original subprocess traceback, attached
# as the cause, is then shown -- not the parent-side replay frames.
__unittest = True

# Environment variable set in the child process so that the decorated test
# method runs its real body instead of spawning yet another subprocess.
_RUN_IN_SUBPROCESS_ENV = '_PYTHON_RUN_IN_SUBPROCESS'

# Environment variable carrying (as JSON) the regrtest-configured test.support
# state, so that the bare subprocess honors -u, -M, -v, etc. like the parent.
_CONFIG_ENV = '_PYTHON_ISOLATED_CONFIG'

# test.support globals set by regrtest (libregrtest/setup.py) that affect how
# tests run and which are skipped at runtime in the subprocess.
_PROPAGATED_CONFIG = (
    'use_resources',                    # -u (is_resource_enabled/requires)
    'max_memuse', 'real_max_memuse',    # -M (bigmemtest)
    'verbose',                          # -v
    'failfast',                         # -f
)

def _child_config():
    import test.support as support
    return {name: getattr(support, name) for name in _PROPAGATED_CONFIG}

def _apply_child_config():
    """Mirror the parent's test.support configuration in the subprocess.

    Called by subprocess_runner before loading the test, so that import-time
    decorators (e.g. requires_resource) and runtime checks see the same -u/-M/-v
    configuration as the parent process.
    """
    import json
    import test.support as support
    data = os.environ.get(_CONFIG_ENV)
    if data:
        for name, value in json.loads(data).items():
            setattr(support, name, value)

# True while running inside the isolated subprocess spawned by @isolated().
# setUp()/tearDown() and the class- and module-level fixtures can test it to
# decide which code to run in the subprocess as opposed to the parent process.
running_isolated = bool(os.environ.get(_RUN_IN_SUBPROCESS_ENV))


class _RemoteTraceback(Exception):
    """Carry a formatted traceback string from the subprocess for display.

    Attached as the ``__cause__`` of the replayed failure/error, so that the
    original traceback is shown by the traceback machinery.
    """
    def __init__(self, tb):
        self.tb = tb

    def __str__(self):
        return self.tb


class _SubprocessTestError(Exception):
    """Replay a subprocess error (as opposed to a failure) in the parent."""


def _remote(detail):
    # Wrap the subprocess traceback the way concurrent.futures does, so it is
    # clearly delimited when shown as the cause.
    return _RemoteTraceback(f'\n"""\n{detail}"""')


def _check_subprocess_support():
    # isolated() always runs the test in a subprocess, so skip (in the parent)
    # on platforms that do not support spawning one.
    import test.support as support
    if not support.has_subprocess_support:
        raise unittest.SkipTest('requires subprocess support')


def _run_in_subprocess(module, qualname):
    """Run module.qualname (a test method or class) in a fresh subprocess.

    Return ``(payload, output, returncode)``, where *payload* is the decoded
    ``{'outcomes': ..., 'durations': ...}`` mapping from the subprocess, or
    ``None`` if it did not run to completion (crash, import error, ...).
    """
    import json
    import subprocess
    import tempfile
    env = dict(os.environ)
    env[_RUN_IN_SUBPROCESS_ENV] = '1'
    env[_CONFIG_ENV] = json.dumps(_child_config())
    fd, result_path = tempfile.mkstemp(suffix='.json')
    os.close(fd)
    try:
        cmd = [sys.executable, '-m', 'test.support.subprocess_runner',
               module, qualname, result_path]
        proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
        try:
            with open(result_path, encoding='utf-8') as f:
                payload = json.load(f)
        except (OSError, ValueError):
            payload = None
    finally:
        try:
            os.unlink(result_path)
        except OSError:
            pass
    return payload, (proc.stdout or '') + (proc.stderr or ''), proc.returncode


def _replay_outcome(test, outcome):
    kind = outcome['kind']
    detail = outcome['detail']
    if kind == 'skipped':
        test.skipTest(detail)  # the detail is the skip reason, not a traceback
    elif kind in ('failure', 'expected_failure'):
        # An expected failure is replayed like a failure: the wrapper carries
        # the @expectedFailure marker (copied by functools.wraps), so the parent
        # records the raised exception as an expectedFailure, not a failure.
        exc = test.failureException('test failed in the subprocess')
        raise exc from _remote(detail)
    else:  # 'error'
        exc = _SubprocessTestError('test failed in the subprocess')
        raise exc from _remote(detail)


def _replay_outcomes(test, outcomes):
    # Replay each subtest outcome in its own subTest() context so that they are
    # reported individually, then replay the whole-test outcome (if any).
    main = []
    for outcome in outcomes:
        if outcome['subtest']:
            with test.subTest(outcome['desc']):
                _replay_outcome(test, outcome)
        else:
            main.append(outcome)
    for outcome in main:
        _replay_outcome(test, outcome)


def _raise_fixture_outcome(outcome):
    # Reproduce a setUpClass()/setUpModule() failure or skip from the
    # subprocess in a parent-process fixture, so it applies to every test.
    if outcome['kind'] == 'skipped':
        raise unittest.SkipTest(outcome['detail'])
    exc = _SubprocessTestError('class failed in the subprocess')
    raise exc from _remote(outcome['detail'])


def _isolate_method(func):
    @functools.wraps(func)
    def wrapper(self, /, *args, **kwargs):
        if running_isolated:
            # Already running in the subprocess: run the real test.
            return func(self, *args, **kwargs)
        _check_subprocess_support()
        cls = type(self)
        qualname = f'{cls.__qualname__}.{func.__name__}'
        payload, output, returncode = _run_in_subprocess(cls.__module__,
                                                         qualname)
        if payload is None:
            exc = _SubprocessTestError(
                f'test did not complete in a subprocess (exit code {returncode})')
            raise exc from _remote(output)
        # The parent measures this method's own duration (the real cost of the
        # isolated run, subprocess startup included), so nothing to forward here.
        _replay_outcomes(self, payload['outcomes'])
    return wrapper


def _isolate_class(cls):
    # Unwrap to the plain functions: the replacements below call them with the
    # runtime cls, so a subclass of an isolated class runs the fixtures bound to
    # itself (a bound classmethod would freeze the decoration-time class).
    orig_setUpClass = cls.setUpClass.__func__
    orig_tearDownClass = cls.tearDownClass.__func__
    orig_setUp = cls.setUp
    orig_tearDown = cls.tearDown
    orig_addDuration = getattr(cls, '_addDuration', None)

    def setUpClass(cls):
        if running_isolated:
            orig_setUpClass(cls)
            return
        _check_subprocess_support()
        # Run the whole class in a single subprocess and stash the outcomes
        # for the wrapped test methods to replay.
        payload, output, returncode = _run_in_subprocess(cls.__module__,
                                                         cls.__qualname__)
        if payload is None:
            exc = _SubprocessTestError(
                f'class did not complete in a subprocess (exit code {returncode})')
            raise exc from _remote(output)
        by_id = {}
        for outcome in payload['outcomes']:
            if outcome['fixture']:
                # A setUpClass()/setUpModule() failure or skip: apply it to the
                # whole class by raising it here, in the parent's setUpClass().
                _raise_fixture_outcome(outcome)
            by_id.setdefault(outcome['id'], []).append(outcome)
        cls._isolated_outcomes = by_id
        cls._isolated_durations = dict(payload.get('durations', ()))

    def tearDownClass(cls):
        if running_isolated:
            orig_tearDownClass(cls)
        else:
            cls._isolated_outcomes = None
            cls._isolated_durations = None

    def setUp(self):
        # In the parent the real test does not run, so neither should setUp().
        if running_isolated:
            orig_setUp(self)

    def tearDown(self):
        if running_isolated:
            orig_tearDown(self)

    def _addDuration(self, result, elapsed):
        # In the parent, report the per-test duration measured in the subprocess
        # rather than the replay time (subprocess startup is paid once, in
        # setUpClass).
        if not running_isolated:
            durations = getattr(type(self), '_isolated_durations', None) or {}
            elapsed = durations.get(self.id(), elapsed)
        orig_addDuration(self, result, elapsed)

    def replay(self):
        by_id = getattr(type(self), '_isolated_outcomes', None) or {}
        _replay_outcomes(self, by_id.get(self.id(), []))

    cls.setUpClass = classmethod(setUpClass)
    cls.tearDownClass = classmethod(tearDownClass)
    cls.setUp = setUp
    cls.tearDown = tearDown
    if orig_addDuration is not None:
        cls._addDuration = _addDuration
    for name in unittest.TestLoader().getTestCaseNames(cls):
        method = getattr(cls, name)
        @functools.wraps(method)
        def wrapper(self, /, *args, __func=method, **kwargs):
            if running_isolated:
                return __func(self, *args, **kwargs)
            replay(self)
        setattr(cls, name, wrapper)
    return cls


def isolated():
    """Decorator to run a test method or class in isolation from the rest.

    The decorated test runs in a separate, fresh Python process, so it does not
    share global or interpreter state with the rest of the test run.  When a
    :class:`~unittest.TestCase` subclass is decorated, the whole class runs in a
    single subprocess and its ``setUpClass()``/``setUpModule()`` fixtures run
    once there; when a method is decorated, only that method runs in a
    subprocess.  Decorated methods must take no extra arguments.

    A failure, error or skip of the whole test is reported for the test, and
    individual subtests (:meth:`~unittest.TestCase.subTest`) that fail or are
    skipped are reported individually.  The original subprocess traceback is
    shown as the cause of a reported failure or error.  Use
    :data:`running_isolated` in fixtures to choose what to run in the subprocess.

    The test is skipped on platforms without subprocess support, since it must
    spawn one.
    """
    def decorator(obj):
        if isinstance(obj, type) and issubclass(obj, unittest.TestCase):
            return _isolate_class(obj)
        return _isolate_method(obj)
    return decorator
