"""Run tests in isolated subprocesses (the test.support.isolation.runInSubprocess decorator).

A failure, error or skip that happens in the subprocess is replayed in the
parent process so that the test runner records it.  The original (subprocess)
traceback is attached as the cause of the replayed exception, the same way
:mod:`concurrent.futures` surfaces tracebacks from worker processes.
"""

import functools
import os
import sys
import unittest

# Let unittest strip this module's frames from tracebacks, so only the original
# subprocess traceback (attached as the cause) is shown, not the replay frames.
__unittest = True

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

def _apply_child_config(config):
    """Set up the child to run the test like a regrtest worker would.

    Mark this process as the subprocess, mirror the parent's -u/-M/-v config,
    then suppress the Windows CRT assertion dialogs, which would block a debug
    build on a modal dialog and hang the parent.
    """
    global runningInSubprocess
    import marshal
    import test.support as support
    runningInSubprocess = True
    for name, value in marshal.loads(bytes.fromhex(config)).items():
        setattr(support, name, value)
    support.suppress_msvcrt_asserts(support.verbose >= 2)

# True inside the subprocess spawned by @runInSubprocess(), set by
# _apply_child_config() before the test is imported.  Fixtures can test it to
# decide what to run in the subprocess as opposed to the parent process.
runningInSubprocess = False


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


def _decode(data):
    # Decode the child output, which is only ever shown as a diagnostic: an
    # undecodable byte must not hide the failure it is part of.
    if not data:
        return ''
    import locale
    encoding = 'utf-8' if sys.flags.utf8_mode else locale.getencoding()
    return data.decode(encoding, 'backslashreplace').replace('\r\n', '\n')


def _remote(detail):
    # Wrap the subprocess traceback the way concurrent.futures does, so it is
    # clearly delimited when shown as the cause.  Return None if the subprocess
    # said nothing (a hung one usually does not), so that "raise ... from None"
    # suppresses an empty cause.
    if not detail:
        return None
    return _RemoteTraceback(f'\n"""\n{detail}"""')


def _check_subprocess_support():
    # runInSubprocess() always runs the test in a subprocess, so skip (in the
    # parent) on platforms that do not support spawning one.
    import test.support as support
    if not support.has_subprocess_support:
        raise unittest.SkipTest('requires subprocess support')


def _child_environ(env):
    # Start from the inherited environment, so that *env* only has to name what
    # the test changes.
    if not env:
        return None
    environ = dict(os.environ)
    for name, value in env.items():
        if value is None:
            environ.pop(name, None)
        else:
            environ[name] = value
    return environ


class _SubprocessTest:
    """A test running in a subprocess, started by _start_test().

    The parent can watch the subprocess (its pid) while the test runs, and
    must wait() for it.
    """

    def __init__(self, proc, result_path):
        self._proc = proc
        self._result_path = result_path

    @property
    def pid(self):
        return self._proc.pid

    def wait(self, timeout=None, tick=None, interval=1.0):
        """Wait for the test to finish, calling *tick* every *interval* seconds.

        Return ``(payload, output, returncode)``, where *payload* is the
        decoded ``{'outcomes': ..., 'durations': ...}`` mapping from the
        subprocess, or ``None`` if it did not run to completion (crash,
        import error, ...).
        """
        import marshal
        import subprocess
        import time
        deadline = None if timeout is None else time.monotonic() + timeout
        try:
            while True:
                step = None if deadline is None else max(
                    0.0, deadline - time.monotonic())
                # Wake up for the next tick, unless the timeout comes first.
                ticking = tick is not None and (step is None or step > interval)
                try:
                    # communicate(), not wait(): a test writing more than a
                    # pipe buffer would block.  Retrying keeps what it read.
                    stdout, stderr = self._proc.communicate(
                        timeout=interval if ticking else step)
                    break
                except subprocess.TimeoutExpired:
                    if ticking:
                        tick()
                        continue
                    # Report the hang rather than leaving the runner stuck.
                    self._proc.kill()
                    stdout, stderr = self._proc.communicate()
                    raise _SubprocessTestError(
                        f'test did not complete in a subprocess '
                        f'within {timeout} seconds'
                    ) from _remote(_decode(stdout) + _decode(stderr))
            try:
                with open(self._result_path, 'rb') as f:
                    payload = marshal.load(f)
            except (OSError, EOFError, ValueError):
                payload = None
            output = _decode(stdout) + _decode(stderr)
            return payload, output, self._proc.returncode
        finally:
            try:
                os.unlink(self._result_path)
            except OSError:
                pass


def _start_test(module, qualname, options=(), env=None):
    """Start module.qualname (a test method or class) in a fresh subprocess.

    Return a _SubprocessTest.  Its wait() is what removes the temporary file
    the subprocess writes its result to.
    """
    import marshal
    import subprocess
    import tempfile
    fd, result_path = tempfile.mkstemp(suffix='.json')
    os.close(fd)
    try:
        # Pass the config on the command line, not in the environment, so that
        # the test cannot pass it on to the processes it spawns itself, and so
        # that it survives the -E and -I options.  Use marshal, not json: it is
        # built in, so the child imports nothing that the test would not see in
        # a normal test run.
        cmd = [sys.executable, *options, '-m', 'test.support.subprocess_runner',
               module, qualname, result_path,
               marshal.dumps(_child_config()).hex()]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, env=_child_environ(env))
    except BaseException:
        try:
            os.unlink(result_path)
        except OSError:
            pass
        raise
    return _SubprocessTest(proc, result_path)



def _replay_outcome(test, outcome):
    kind = outcome['kind']
    detail = outcome['detail']
    if kind == 'skipped':
        test.skipTest(detail)  # the detail is the skip reason, not a traceback
    elif kind in ('failure', 'expected_failure'):
        # Replay an expected failure like a failure: the wrapper keeps the
        # @expectedFailure marker (via functools.wraps), so the parent records
        # the raised exception as an expectedFailure.
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


def _check_returncode(returncode, output, what):
    # The subprocess writes its result before exiting, so a non-zero exit code
    # means it died afterwards, during finalization, unnoticed by the result.
    if returncode:
        exc = _SubprocessTestError(
            f'the subprocess exited with code {returncode} '
            f'after running the {what}')
        raise exc from _remote(output)


def _replay_test(test, payload, output, returncode):
    """Reproduce in *test* the result that _SubprocessTest.wait() returned."""
    if payload is None:
        exc = _SubprocessTestError(
            f'test did not complete in a subprocess (exit code {returncode})')
        raise exc from _remote(output)
    # The parent measures the test method's own duration (the real cost of the
    # isolated run, subprocess startup included), so nothing to forward here.
    # Replay the outcomes first: a failure of the test itself is more useful.
    _replay_outcomes(test, payload['outcomes'])
    _check_returncode(returncode, output, 'test')


def _isolate_method(func, options, env, timeout):
    @functools.wraps(func)
    def wrapper(self, /, *args, **kwargs):
        if runningInSubprocess:
            # Already running in the subprocess: run the real test.
            return func(self, *args, **kwargs)
        _check_subprocess_support()
        cls = type(self)
        qualname = f'{cls.__qualname__}.{func.__name__}'
        proc = _start_test(cls.__module__, qualname, options, env)
        _replay_test(self, *proc.wait(timeout))
    return wrapper


def _isolate_class(cls, options, env, timeout):
    # Unwrap to the plain functions so the replacements can call them with the
    # runtime cls; a bound classmethod would freeze the decoration-time class
    # and a subclass would run the fixtures bound to the base class.
    orig_setUpClass = cls.setUpClass.__func__
    orig_tearDownClass = cls.tearDownClass.__func__
    # Hook the _call*() indirections rather than setUp(), tearDown() and the
    # test methods themselves, to cover what a subclass adds or overrides too.
    orig_callSetUp = cls._callSetUp
    orig_callTearDown = cls._callTearDown
    orig_callTestMethod = cls._callTestMethod
    orig_addDuration = cls._addDuration

    def setUpClass(cls):
        if runningInSubprocess:
            orig_setUpClass(cls)
            return
        _check_subprocess_support()
        # Run the whole class in a single subprocess and stash the outcomes
        # for the test methods to replay.
        proc = _start_test(cls.__module__, cls.__qualname__, options, env)
        payload, output, returncode = proc.wait(timeout)
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
        # Report the crash from tearDownClass(), after replaying the outcomes.
        cls._isolated_exit = (returncode, output)

    def tearDownClass(cls):
        if runningInSubprocess:
            orig_tearDownClass(cls)
            return
        cls._isolated_outcomes = None
        cls._isolated_durations = None
        # Missing if an overriding setUpClass() bypassed the subprocess.
        exited = getattr(cls, '_isolated_exit', None)
        cls._isolated_exit = None
        if exited is not None:
            _check_returncode(*exited, 'class')

    def _callSetUp(self):
        # In the parent the real test does not run, so neither should setUp().
        if runningInSubprocess:
            orig_callSetUp(self)

    def _callTearDown(self):
        if runningInSubprocess:
            orig_callTearDown(self)

    def _callTestMethod(self, method):
        if runningInSubprocess:
            orig_callTestMethod(self, method)
            return
        by_id = getattr(type(self), '_isolated_outcomes', None)
        if by_id is None:
            raise _SubprocessTestError(
                f'{type(self).__name__} did not run in a subprocess; '
                f'an overriding setUpClass() must call super().setUpClass()')
        _replay_outcomes(self, by_id.get(self.id(), []))

    def _addDuration(self, result, elapsed):
        # In the parent, report the subprocess timing rather than the (instant)
        # replay time; subprocess startup is paid once, in setUpClass.
        if not runningInSubprocess:
            durations = getattr(type(self), '_isolated_durations', None) or {}
            elapsed = durations.get(self.id(), elapsed)
        orig_addDuration(self, result, elapsed)

    cls.setUpClass = classmethod(setUpClass)
    cls.tearDownClass = classmethod(tearDownClass)
    cls._callSetUp = _callSetUp
    cls._callTearDown = _callTearDown
    cls._callTestMethod = _callTestMethod
    cls._addDuration = _addDuration
    return cls


def runInSubprocess(*, options=(), env=None, timeout=None):
    """Decorator to run a test method or class in a fresh subprocess.

    The decorated test runs in a separate, fresh Python process, so it does not
    share global or interpreter state with the rest of the test run.  When a
    :class:`~unittest.TestCase` subclass is decorated, the whole class runs in a
    single subprocess and its ``setUpClass()``/``setUpModule()`` fixtures run
    once there; when a method is decorated, only that method runs in a
    subprocess.  Decorated methods must take no extra arguments.

    *options* is a sequence of interpreter command line options for the
    subprocess, and *env* is a mapping of environment variables to set in it,
    on top of the inherited environment; a value of ``None`` unsets a variable.
    Note that ``-E`` and ``-I`` make the subprocess ignore the ``PYTHON*``
    variables, including ``PYTHONPATH``.

    *timeout* is the number of seconds to wait for the subprocess; the test is
    reported as an error if it does not complete in time.  By default there is
    no timeout, and a hung test is left to the timeout of the test runner.

    A failure, error or skip of the whole test is reported for the test, and
    individual subtests (:meth:`~unittest.TestCase.subTest`) that fail or are
    skipped are reported individually.  The original subprocess traceback is
    shown as the cause of a reported failure or error.  Use
    :data:`runningInSubprocess` in fixtures to choose what to run in the subprocess.

    The test is skipped on platforms without subprocess support, since it must
    spawn one.
    """
    def decorator(obj):
        if isinstance(obj, type) and issubclass(obj, unittest.TestCase):
            return _isolate_class(obj, options, env, timeout)
        return _isolate_method(obj, options, env, timeout)
    return decorator
