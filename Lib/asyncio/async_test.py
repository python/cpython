import asyncio
import itertools
import logging
import os
import re
import subprocess
import sys
import time
import unittest
import unittest.mock

import asynctest


class Test:
    class FooTestCase(asynctest.TestCase):
        def runTest(self):
            pass

        def test_foo(self):
            pass

    @asynctest.fail_on(unused_loop=False)
    class LoggingTestCase(asynctest.TestCase):
        def __init__(self, calls):
            super().__init__()
            self.calls = calls

        def setUp(self):
            self.calls.append('setUp')

        def test_basic(self):
            self.events.append('test_basic')

        def tearDown(self):
            self.events.append('tearDown')

    class StartWaitProcessTestCase(asynctest.TestCase):
        @staticmethod
        @asyncio.coroutine
        def start_wait_process(loop):
            process = yield from asyncio.create_subprocess_shell(
                "true", stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                loop=loop)

            try:
                out, err = yield from asyncio.wait_for(
                    process.communicate(), timeout=.1, loop=loop)
            except BaseException:
                process.kill()
                os.waitpid(process.pid, os.WNOHANG)
                raise

        @asyncio.coroutine
        def runTest(self):
            yield from self.start_wait_process(self.loop)


class _TestCase(unittest.TestCase):
    run_methods = ('run', 'debug', )

    def create_default_loop(self):
        default_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(default_loop)
        self.addCleanup(default_loop.close)
        return default_loop


class Test_TestCase(_TestCase):
    def test_init_and_close_loop_for_test(self):
        default_loop = self.create_default_loop()

        @asynctest.lenient
        class LoopTest(asynctest.TestCase):
            failing = False

            def runTest(self):
                try:
                    self.assertIsNotNone(self.loop)
                    self.assertFalse(self.loop.close.called)
                except Exception:
                    self.failing = True
                    raise

            def runFailingTest(self):
                self.runTest()
                raise SystemError()

        for method, test in itertools.product(self.run_methods, ('runTest', 'runFailingTest', )):
            with self.subTest(method=method, test=test), \
                    unittest.mock.patch('asyncio.new_event_loop') as mock:
                mock_loop = unittest.mock.Mock(asyncio.AbstractEventLoop)
                mock_loop.run_until_complete.side_effect = default_loop.run_until_complete

                if sys.version_info >= (3, 6):
                    mock_loop.shutdown_asyncgens = asynctest.CoroutineMock()

                mock.return_value = mock_loop

                case = LoopTest(test)
                try:
                    getattr(case, method)()
                except SystemError:
                    pass

                mock.assert_called_with()
                mock_loop.close.assert_called_with()

                if sys.version_info >= (3, 6):
                    mock_loop.shutdown_asyncgens.assert_awaited()

                # If failing is True, one of the assertions in runTest failed
                self.assertFalse(case.failing)
                # Also, ensure we didn't override the original loop
                self.assertIs(default_loop, asyncio.get_event_loop())

    def test_default_loop_is_not_created_when_unused(self):
        policy = asyncio.get_event_loop_policy()

        @asynctest.fail_on(unused_loop=False)
        class Dummy_TestCase(Test.FooTestCase):
            pass

        for method in self.run_methods:
            with self.subTest(method=method):
                with unittest.mock.patch.object(policy,
                                                "get_event_loop") as mock:
                    case = Dummy_TestCase()
                    getattr(case, method)()

                self.assertFalse(mock.called)

    def test_update_default_loop_works(self):
        a_loop = asyncio.new_event_loop()
        self.addCleanup(a_loop.close)

        class Update_Default_Loop_TestCase(asynctest.TestCase):
            @asynctest.fail_on(unused_loop=False)
            def runTest(self):
                self.assertIs(self.loop, asyncio.get_event_loop())
                asyncio.set_event_loop(a_loop)
                self.assertIs(a_loop, asyncio.get_event_loop())

        for method in self.run_methods:
            with self.subTest(method=method):
                case = Update_Default_Loop_TestCase()
                result = getattr(case, method)()

                if result:
                    self.assertTrue(result.wasSuccessful())

                self.assertIs(a_loop, asyncio.get_event_loop())

    def test_use_default_loop(self):
        default_loop = self.create_default_loop()

        class Using_Default_Loop_TestCase(asynctest.TestCase):
            use_default_loop = True

            @asynctest.fail_on(unused_loop=False)
            def runTest(self):
                self.assertIs(default_loop, self.loop)

        for method in self.run_methods:
            with self.subTest(method=method):
                case = Using_Default_Loop_TestCase()
                result = getattr(case, method)()

                # assert that the original loop is reset after the test
                self.assertIs(default_loop, asyncio.get_event_loop())

                if result:
                    self.assertTrue(result.wasSuccessful())

            self.assertFalse(default_loop.is_closed())

    def test_forbid_get_event_loop(self):
        default_loop = self.create_default_loop()

        # Changed in python 3.6: get_event_loop() returns the running loop in
        # a callback or a coroutine, so forbid_get_event_loop should only be
        # forbidden where the loop is not running.
        @asynctest.lenient
        class Forbid_get_event_loop_TestCase(asynctest.TestCase):
            forbid_get_event_loop = True

            def runTest(self):
                with self.assertRaises(AssertionError):
                    asyncio.get_event_loop()

        for method in self.run_methods:
            with self.subTest(method=method):
                case = Forbid_get_event_loop_TestCase()
                result = getattr(case, method)()

                # assert that the original loop is reset after the test
                self.assertIs(default_loop, asyncio.get_event_loop())

                if result:
                    self.assertTrue(result.wasSuccessful())

    def test_coroutinefunction_executed(self):
        class CoroutineFunctionTest(asynctest.TestCase):
            ran = False

            async def noop(self):
                pass

            @asyncio.coroutine
            def runTest(self):
                self.ran = True
                yield from self.noop()

        class NativeCoroutineFunctionTest(CoroutineFunctionTest):
            async def runTest(self):
                self.ran = True
                await self.noop()

        for method in self.run_methods:
            with self.subTest(method=method):
                for case in (CoroutineFunctionTest(),
                             NativeCoroutineFunctionTest()):
                    with self.subTest(case=case):
                        case.ran = False
                        getattr(case, method)()
                        self.assertTrue(case.ran)

    def test_coroutine_returned_executed(self):
        class CoroutineTest(asynctest.TestCase):
            ran = False

            @asyncio.coroutine
            def set_ran(self):
                self.ran = True

            def runTest(self):
                return self.set_ran()

        class NativeCoroutineTest(CoroutineTest):
            async def set_ran(self):
                self.ran = True

            def runTest(self):
                return self.set_ran()

        for method in self.run_methods:
            with self.subTest(method=method):
                for case in (CoroutineTest(), NativeCoroutineTest()):
                    with self.subTest(case=case):
                        case.ran = False
                        getattr(case, method)()
                        self.assertTrue(case.ran)

    def test_fails_when_future_has_scheduled_calls(self):
        class CruftyTest(asynctest.TestCase):
            @asynctest.fail_on(active_handles=True, unused_loop=False)
            def runTest(instance):
                instance.loop.call_later(5, lambda: None)

        with self.subTest(method='debug'):
            with self.assertRaisesRegex(AssertionError, 'unfinished work'):
                CruftyTest().debug()
        with self.subTest(method='run'):
            result = CruftyTest().run()
            self.assertEqual(1, len(result.failures))

    def test_setup_teardown_may_be_coroutines(self):
        @asynctest.fail_on(unused_loop=False)
        class WithSetupFunction(Test.FooTestCase):
            ran = False

            def setUp(self):
                WithSetupFunction.ran = True

        @asynctest.fail_on(unused_loop=False)
        class WithSetupCoroutine(Test.FooTestCase):
            ran = False

            @asyncio.coroutine
            def setUp(self):
                WithSetupCoroutine.ran = True

        @asynctest.fail_on(unused_loop=False)
        class WithTearDownFunction(Test.FooTestCase):
            ran = False

            def tearDown(self):
                WithTearDownFunction.ran = True

        @asynctest.fail_on(unused_loop=False)
        class WithTearDownCoroutine(Test.FooTestCase):
            ran = False

            def tearDown(self):
                WithTearDownCoroutine.ran = True

        for method in self.run_methods:
            for call_mode in (WithSetupFunction, WithSetupCoroutine,
                              WithTearDownFunction, WithTearDownCoroutine):
                with self.subTest(method=method, case=call_mode.__name__):
                    case = call_mode()
                    call_mode.ran = False

                    getattr(case, method)()
                    self.assertTrue(case.ran)

    def test_cleanup_functions_can_be_coroutines(self):
        cleanup_normal_called = False
        cleanup_normal_called_too_soon = False
        cleanup_coro_called = False

        def cleanup_normal():
            nonlocal cleanup_normal_called
            cleanup_normal_called = True

        @asyncio.coroutine
        def cleanup_coro():
            nonlocal cleanup_coro_called
            cleanup_coro_called = True

        @asynctest.fail_on(unused_loop=False)
        class TestCase(Test.FooTestCase):
            def setUp(self):
                nonlocal cleanup_normal_called, cleanup_normal_called_too_soon
                nonlocal cleanup_coro_called

                cleanup_normal_called = cleanup_coro_called = False

                self.addCleanup(cleanup_normal)
                cleanup_normal_called_too_soon = cleanup_normal_called

                self.addCleanup(cleanup_coro)

        for method in self.run_methods:
            with self.subTest(method=method):
                getattr(TestCase(), method)()
                self.assertTrue(cleanup_normal_called)
                self.assertTrue(cleanup_coro_called)

    def test_loop_uses_TestSelector(self):
        @asynctest.fail_on(unused_loop=False)
        class CheckLoopTest(asynctest.TestCase):
            def runTest(self):
                # TestSelector is used
                self.assertIsInstance(self.loop._selector,
                                      asynctest.selector.TestSelector)

                # And wraps the original selector
                self.assertIsNotNone(self.loop._selector._selector)

        for method in self.run_methods:
            with self.subTest(method=method):
                case = CheckLoopTest()
                outcome = getattr(case, method)()

                if outcome:
                    self.assertTrue(outcome.wasSuccessful())


@unittest.skipIf(sys.platform == "win32", "Tests specific to Unix")
class Test_TestCase_and_ChildWatcher(_TestCase):
    def test_watched_process_is_awaited(self):
        for method in self.run_methods:
            with self.subTest(method=method):
                case = Test.StartWaitProcessTestCase()
                outcome = getattr(case, method)()

                if outcome:
                    self.assertTrue(outcome.wasSuccessful())

    def test_original_watcher_works_outside_loop(self):
        default_loop = self.create_default_loop()

        # check if we can spawn and wait a subprocess before an after a test
        for method in self.run_methods:
            with self.subTest(method=method):
                coro = Test.StartWaitProcessTestCase.start_wait_process(
                    default_loop)
                default_loop.run_until_complete(coro)

                case = Test.StartWaitProcessTestCase()
                outcome = getattr(case, method)()

                if outcome:
                    self.assertTrue(outcome.wasSuccessful())

                coro = Test.StartWaitProcessTestCase.start_wait_process(
                    default_loop)
                default_loop.run_until_complete(coro)


class Test_ClockedTestCase(asynctest.ClockedTestCase):
    took_n_seconds = re.compile(r'took \d+\.\d{3} seconds')

    @asyncio.coroutine
    def advance(self, seconds):
        debug = self.loop.get_debug()
        try:
            self.loop.set_debug(True)
            with self.assertLogs(level=logging.WARNING) as log:
                yield from self.loop.create_task(super().advance(seconds))

            self.assertTrue(any(filter(self.took_n_seconds.search,
                                       log.output)))
        finally:
            self.loop.set_debug(debug)

    @asyncio.coroutine
    def test_advance(self):
        f = asyncio.Future(loop=self.loop)
        g = asyncio.Future(loop=self.loop)
        started_wall_clock = time.monotonic()
        started_loop_clock = self.loop.time()
        self.loop.call_later(1, f.set_result, None)
        self.loop.call_later(2, g.set_result, None)
        self.assertFalse(f.done())
        yield from self.advance(1)
        self.assertTrue(f.done())
        self.assertFalse(g.done())
        yield from self.advance(9)
        self.assertTrue(g)
        finished_wall_clock = time.monotonic()
        finished_loop_clock = self.loop.time()
        self.assertLess(
            finished_wall_clock - started_wall_clock,
            finished_loop_clock - started_loop_clock)

    def test_advance_with_run_until_complete(self):
        f = asyncio.Future(loop=self.loop)
        started_wall_clock = time.monotonic()
        started_loop_clock = self.loop.time()
        self.loop.call_later(1, f.set_result, None)
        self.loop.run_until_complete(self.advance(1))
        self.assertTrue(f.done())
        finished_wall_clock = time.monotonic()
        finished_loop_clock = self.loop.time()
        self.assertLess(
            finished_wall_clock - started_wall_clock,
            finished_loop_clock - started_loop_clock)

    @asyncio.coroutine
    def test_negative_advance(self):
        with self.assertRaisesRegex(ValueError, 'back in time'):
            yield from self.advance(-1)
        self.assertEqual(self.loop.time(), 0)

    @asyncio.coroutine
    def test_callbacks_are_called_on_time(self):
        def record(call_time):
            call_time.append(self.loop.time())

        call_time = []
        self.loop.call_later(0, record, call_time)
        self.loop.call_later(1, record, call_time)
        self.loop.call_later(2, record, call_time)
        self.loop.call_later(5, record, call_time)
        yield from self.advance(3)
        expected = list(range(3))
        self.assertEqual(call_time, expected)
        yield from self.advance(2)
        expected.append(5)
        self.assertEqual(call_time, expected)


class Test_ClockedTestCase_setUp(asynctest.ClockedTestCase):
    def setUp(self):
        # Deactivate debug mode if enabled, because it will warn us that the
        # advance() coroutine took 1s.
        debug = self.loop.get_debug()
        self.loop.set_debug(False)
        self.addCleanup(self.loop.set_debug, debug)

    @asyncio.coroutine
    def test_setUp(self):
        yield from self.advance(1)
        self.assertEqual(1, self.loop.time())


class Test_ClockedTestCase_async_setUp(asynctest.ClockedTestCase):
    @asyncio.coroutine
    def setUp(self):
        # Deactivate debug mode if enabled, because it will warn us that the
        # advance() coroutine took 1s.
        debug = self.loop.get_debug()
        self.loop.set_debug(False)
        self.addCleanup(self.loop.set_debug, debug)

        yield from self.advance(1)

    @asyncio.coroutine
    def test_setUp(self):
        self.assertEqual(1, self.loop.time())


@unittest.mock.patch.dict("asynctest._fail_on.DEFAULTS",
                          values={"foo": False, "bar": True},
                          clear=True)
class Test_fail_on_decorator(unittest.TestCase):
    def checks(self, obj, fatal=True):
        if isinstance(obj, asynctest.TestCase):
            case = obj
        else:
            case = obj.__self__

        try:
            return getattr(obj, asynctest._fail_on._FAIL_ON_ATTR).get_checks(case)
        except AttributeError:
            if fatal:
                self.fail("{!r} not decorated".format(obj))

    def assert_not_decorated(self, obj):
        if self.checks(obj, False) is not None:
            self.fail("{!r} is decorated".format(obj))

    def assert_checks_equal(self, obj, **kwargs):
        self.assertEqual(self.checks(obj), kwargs)

    def assert_checks(self, obj, **kwargs):
        checks = self.checks(obj)
        for check in kwargs:
            if kwargs[check] != checks[check]:
                self.fail("check '{}' {} (expected: {})".format(
                    check,
                    "enabled" if checks[check] else "disabled",
                    "enabled" if kwargs[check] else "disabled")
                )

    def test_check_arguments(self):
        message = "got an unexpected keyword argument 'not_existing'"
        with self.assertRaisesRegex(TypeError, message):
            asynctest.fail_on(foo=True, not_existing=True)

    def test_decorate_method(self):
        class TestCase(asynctest.TestCase):
            @asynctest.fail_on(foo=True)
            def test_foo(self):
                pass

        instance = TestCase()
        self.assert_checks_equal(instance.test_foo, foo=True, bar=True)

    def test_decorate_class(self):
        @asynctest.fail_on(foo=True)
        class TestCase(asynctest.TestCase):
            def test_foo(self):
                pass

            @asynctest.fail_on(bar=False)
            def test_bar(self):
                pass

            @asynctest.fail_on(foo=False, bar=False)
            def test_baz(self):
                pass

        instance = TestCase()
        with self.subTest("class is decorated"):
            self.assert_checks_equal(instance, foo=True, bar=True)

        with self.subTest("method without decoration is not decorated"):
            self.assert_not_decorated(instance.test_foo)

        with self.subTest("method decorated inherits of class params"):
            self.assert_checks_equal(instance.test_bar, foo=True, bar=False)

        with self.subTest("method decorated overrides class params"):
            self.assert_checks_equal(instance.test_baz, foo=False, bar=False)

    def test_decorate_subclass_doesnt_affect_base_class(self):
        class TestCase(asynctest.TestCase):
            pass

        @asynctest.fail_on(foo=True)
        class SubTestCase(TestCase):
            pass

        self.assert_not_decorated(TestCase())
        self.assert_checks(SubTestCase(), foo=True)

    def test_decorate_subclass_inherits_parent_params(self):
        @asynctest.fail_on(foo=True)
        class TestCase(asynctest.TestCase):
            pass

        @asynctest.fail_on(bar=False)
        class SubTestCase(TestCase):
            pass

        @asynctest.fail_on(foo=False, bar=False)
        class OverridingTestCase(TestCase):
            pass

        self.assert_checks_equal(TestCase(), foo=True, bar=True)
        self.assert_checks_equal(SubTestCase(), foo=True, bar=False)
        self.assert_checks_equal(OverridingTestCase(), foo=False, bar=False)

    def test_strict_decorator(self):
        @asynctest.strict
        class TestCase(asynctest.TestCase):
            pass

        self.assert_checks_equal(TestCase(), foo=True, bar=True)

        @asynctest.strict()
        class TestCase(asynctest.TestCase):
            pass

        self.assert_checks_equal(TestCase(), foo=True, bar=True)

    def test_lenient_decorator(self):
        @asynctest.lenient
        class TestCase(asynctest.TestCase):
            pass

        self.assert_checks_equal(TestCase(), foo=False, bar=False)

        @asynctest.lenient()
        class TestCase(asynctest.TestCase):
            pass

        self.assert_checks_equal(TestCase(), foo=False, bar=False)


fail_on_defaults = {"default": True, "optional": False}


@unittest.mock.patch.dict("asynctest._fail_on.DEFAULTS",
                          values=fail_on_defaults, clear=True)
class Test_fail_on(_TestCase):
    def setUp(self):
        self.mocks = {}
        for method in fail_on_defaults:
            mock = self.mocks[method] = unittest.mock.Mock()
            setattr(asynctest._fail_on._fail_on, method, mock)

        self.addCleanup(lambda: [delattr(asynctest._fail_on._fail_on, method)
                                 for method in fail_on_defaults])

    def tearDown(self):
        self.mocks.clear()

    def assert_checked(self, *args):
        for check in args:
            if not self.mocks[check].called:
                self.fail("'{}' unexpectedly unchecked".format(check))

    def assert_not_checked(self, *args):
        for check in args:
            if self.mocks[check].called:
                self.fail("'{}' unexpectedly checked".format(check))

    def test_default_checks(self):
        for method in self.run_methods:
            with self.subTest(method=method):
                case = Test.FooTestCase()
                getattr(case, method)()

                self.assert_checked("default")
                self.assert_not_checked("optional")
                self.mocks["default"].assert_called_with(case)

    def test_checks_on_decorated_class(self):
        @asynctest.fail_on(optional=True)
        class Dummy_TestCase(Test.FooTestCase):
            pass

        for method in self.run_methods:
            with self.subTest(method=method):
                getattr(Dummy_TestCase(), method)()
                self.assert_checked("default", "optional")

    def test_check_after_tearDown(self):
        self.mocks['default'].side_effect = AssertionError

        class Dummy_TestCase(asynctest.TestCase):
            def tearDown(self):
                self.tearDown_called = True

            def runTest(self):
                pass

        with self.subTest(method="debug"):
            case = Dummy_TestCase()
            with self.assertRaises(AssertionError):
                case.debug()

            self.assertTrue(case.tearDown_called)

        case = Dummy_TestCase()
        result = case.run()
        self.assertEqual(1, len(result.failures))
        self.assertTrue(case.tearDown_called)

    def test_before_test_called_before_user_setup(self):
        mock = unittest.mock.Mock()
        setattr(asynctest._fail_on._fail_on, "before_test_default", mock)
        self.addCleanup(delattr, asynctest._fail_on._fail_on,
                        "before_test_default")

        class TestCase(asynctest.TestCase):
            def setUp(self):
                self.assertTrue(mock.called)

            def runTest(self):
                pass

        for method in self.run_methods:
            with self.subTest(method=method):
                case = Test.FooTestCase()
                getattr(case, method)()

                self.assert_checked("default")
                self.assertTrue(mock.called)
                mock.assert_called_with(case)

    def test_non_existing_before_test_wont_fail(self):
        # set something not callable for default, nothing for optional, the
        # test must not fail
        setattr(asynctest._fail_on._fail_on, "before_test_default", None)
        self.addCleanup(delattr, asynctest._fail_on._fail_on,
                        "before_test_default")

        @asynctest.fail_on(default=True, optional=True)
        class TestCase(asynctest.TestCase):
            def runTest(self):
                pass

        for method in self.run_methods:
            with self.subTest(method=method):
                getattr(TestCase(), method)()
                self.assert_checked("default", "optional")

    def test_before_test_called_for_enabled_checks_only(self):
        for method in map(lambda m: "before_test_" + m, fail_on_defaults):
            mock = self.mocks[method] = unittest.mock.Mock()
            setattr(asynctest._fail_on._fail_on, method, mock)

        self.addCleanup(lambda: [delattr(asynctest._fail_on._fail_on,
                                         "before_test_" + method)
                                 for method in fail_on_defaults])

        for method in self.run_methods:
            with self.subTest(method=method):
                getattr(Test.FooTestCase(), method)()
                self.assertTrue(self.mocks["before_test_default"].called)
                self.assertFalse(self.mocks["before_test_optional"].called)


@unittest.mock.patch.dict(
    "asynctest._fail_on.DEFAULTS", clear=True,
    unused_loop=asynctest._fail_on.DEFAULTS['unused_loop'])
class Test_fail_on_unused_loop(_TestCase):
    @asynctest.fail_on(unused_loop=True)
    class WithCheckTestCase(Test.FooTestCase):
        pass

    def test_fails_when_loop_didnt_run(self):
        with self.assertRaisesRegex(AssertionError,
                                    'Loop did not run during the test'):
            self.WithCheckTestCase().debug()

        result = self.WithCheckTestCase().run()
        self.assertEqual(1, len(result.failures))

    def test_fails_when_loop_didnt_run_using_default_loop(self):
        class TestCase(self.WithCheckTestCase):
            use_default_loop = True

        default_loop = self.create_default_loop()

        with self.assertRaisesRegex(AssertionError,
                                    'Loop did not run during the test'):
            TestCase().debug()

        result = TestCase().run()
        self.assertEqual(1, len(result.failures))

        default_loop.run_until_complete(asyncio.sleep(0, loop=default_loop))

        with self.assertRaisesRegex(AssertionError,
                                    'Loop did not run during the test'):
            TestCase().debug()

        default_loop.run_until_complete(asyncio.sleep(0, loop=default_loop))

        result = TestCase().run()
        self.assertEqual(1, len(result.failures))

    def test_passes_when_ignore_loop_or_loop_run(self):
        @asynctest.fail_on(unused_loop=False)
        class IgnoreLoopClassTest(Test.FooTestCase):
            pass

        @asynctest.fail_on(unused_loop=True)
        class WithCoroutineTest(asynctest.TestCase):
            @asyncio.coroutine
            def runTest(self):
                yield from []

        @asynctest.fail_on(unused_loop=True)
        class WithFunctionCallingLoopTest(asynctest.TestCase):
            def runTest(self):
                fut = asyncio.Future()
                self.loop.call_soon(fut.set_result, None)
                self.loop.run_until_complete(fut)

        for test in (IgnoreLoopClassTest, WithCoroutineTest,
                     WithFunctionCallingLoopTest):
            with self.subTest(test=test):
                test().debug()

                result = test().run()
                self.assertEqual(0, len(result.failures))

    def test_fails_when_loop_ran_only_during_setup(self):
        for test_use_default_loop in (False, True):
            with self.subTest(use_default_loop=test_use_default_loop):
                if test_use_default_loop:
                    self.create_default_loop()

                class TestCase(self.WithCheckTestCase):
                    use_default_loop = test_use_default_loop

                    def setUp(self):
                        self.loop.run_until_complete(
                            asyncio.sleep(0, loop=self.loop))

                with self.assertRaisesRegex(
                        AssertionError, 'Loop did not run during the test'):
                    TestCase().debug()

                result = TestCase().run()
                self.assertEqual(1, len(result.failures))

    def test_fails_when_loop_ran_only_during_cleanup(self):
        for test_use_default_loop in (False, True):
            with self.subTest(use_default_loop=test_use_default_loop):
                if test_use_default_loop:
                    self.create_default_loop()

                class TestCase(self.WithCheckTestCase):
                    use_default_loop = test_use_default_loop

                    def setUp(self):
                        self.addCleanup(asyncio.coroutine(lambda: None))

                with self.assertRaisesRegex(
                        AssertionError, 'Loop did not run during the test'):
                    TestCase().debug()

                result = TestCase().run()
                self.assertEqual(1, len(result.failures))


class Test_assertAsyncRaises(asynctest.TestCase):
    class CustomException(Exception):
        def __str__(self):
            return "CustomException Message"

    @property
    def raising(self):
        awaitable = asyncio.Future()
        awaitable.set_exception(self.CustomException())
        return awaitable

    @property
    def finishing(self):
        awaitable = asyncio.Future()
        awaitable.set_result(None)
        return awaitable

    @asyncio.coroutine
    def test_assertAsyncRaises(self):
        with self.subTest("exception raised"):
            yield from self.assertAsyncRaises(self.CustomException,
                                              self.raising)

        with self.subTest("no exception raised"):
            with self.assertRaises(AssertionError):
                yield from self.assertAsyncRaises(self.CustomException,
                                                  self.finishing)

    @asyncio.coroutine
    def test_assertAsyncRaisesRegex(self):
        regex = "CustomException"
        with self.subTest("exception raised"):
            yield from self.assertAsyncRaisesRegex(self.CustomException,
                                                   regex, self.raising)

        with self.subTest("no exception raised"):
            with self.assertRaises(AssertionError):
                yield from self.assertAsyncRaisesRegex(self.CustomException,
                                                       regex, self.finishing)


class Test_assertAsyncWarns(asynctest.TestCase):
    class CustomWarning(Warning):
        pass

    @classmethod
    @asyncio.coroutine
    def warns(cls):
        import warnings
        warnings.warn("asynctest warning message", cls.CustomWarning)

    @staticmethod
    def doesnt_warn():
        awaitable = asyncio.Future()
        awaitable.set_result(None)
        return awaitable

    @asyncio.coroutine
    def test_assertAsyncWarns(self):
        with self.subTest("warning triggered"):
            yield from self.assertAsyncWarns(self.CustomWarning, self.warns())

        with self.subTest("warning not triggered"):
            with self.assertRaises(AssertionError):
                yield from self.assertAsyncWarns(self.CustomWarning,
                                                 self.doesnt_warn())

    @asyncio.coroutine
    def test_assertAsyncWarnsRegex(self):
        regex = "asynctest warning"
        with self.subTest("warning triggered"):
            yield from self.assertAsyncWarnsRegex(self.CustomWarning, regex,
                                                  self.warns())

        with self.subTest("warning not triggered"):
            with self.assertRaises(AssertionError):
                yield from self.assertAsyncWarnsRegex(
                    self.CustomWarning, regex, self.doesnt_warn())


if __name__ == "__main__":
    unittest.main()
    
