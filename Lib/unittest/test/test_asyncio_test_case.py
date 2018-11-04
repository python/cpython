import asyncio
import collections
import unittest


class Test:
    "Keep these TestCase classes out of the main namespace"

    class SingleTestAsyncTestCase(unittest.AsyncioTestCase):
        def test_one(self):
            pass


def run_test_case(test_class, should_fail=False):
    result = unittest.TestResult()
    result.failfast = True
    suite = unittest.makeSuite(test_class)
    suite.run(result)
    if result.testsRun == 0:
        raise AssertionError(f'test {test_class} not run')
    if should_fail and result.wasSuccessful():
        raise AssertionError(f'test {test_class} unexpectedly succeeded')
    elif not should_fail and not result.wasSuccessful():
        raise AssertionError(f'test {test_class} unexpectedly failed')


class EventLoopTracking:
    class Loop(asyncio.SelectorEventLoop):
        def __init__(self, tracker, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.__tracker = tracker

        def run_forever(self):
            super().run_forever()
            self.__tracker.register_call('loop.run_forever', self)

        def close(self):
            super().close()
            self.__tracker.register_call('loop.close', self)

        def stop(self):
            super().stop()
            self.__tracker.register_call('loop.stop', self)

    class Policy(asyncio.DefaultEventLoopPolicy):
        def __init__(self, tracker, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.__tracker = tracker

        def new_event_loop(self):
            loop = self.__tracker.loop_factory(self.__tracker)
            self.__tracker.register_call('policy.new_event_loop', loop)
            return loop

        def set_event_loop(self, loop):
            super().set_event_loop(loop)
            self.__tracker.register_call('policy.set_event_loop', loop)

    def __init__(self):
        self.__saved_loop_policy = asyncio.events._event_loop_policy
        self.loop_factory = self.Loop
        self.call_counter = collections.defaultdict(int)
        self.loop_events = []

    def install(self):
        asyncio.set_event_loop_policy(self.Policy(self))

    def uninstall(self):
        # ALWAYS reset the event loop policy when we are done
        asyncio.set_event_loop_policy(self.__saved_loop_policy)

    def register_call(self, call, obj):
        self.call_counter[call] += 1
        self.loop_events.append((call, obj))


class Test_AsyncioTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.event_loop_tracking = EventLoopTracking()

    def tearDown(self):
        super().tearDown()
        self.event_loop_tracking.uninstall()

    def test_loop_created_per_test(self):
        class TestCase(unittest.AsyncioTestCase):
            async def test_one(self):
                pass

            def test_two(self):
                pass

        self.event_loop_tracking.install()
        run_test_case(TestCase)
        self.assertEqual(
            2, self.event_loop_tracking.call_counter['policy.new_event_loop'])

    def test_loop_policy_saved_and_restored(self):
        with self.subTest('initial state'):
            # first test with a initial policy of `None` to mimic
            # the initial state
            asyncio.set_event_loop_policy(None)
            run_test_case(Test.SingleTestAsyncTestCase)
            self.assertIsNone(asyncio.events._event_loop_policy)

        with self.subTest('custom policy'):
            # next test with a custom policy
            custom_policy = asyncio.DefaultEventLoopPolicy()
            asyncio.set_event_loop_policy(custom_policy)
            run_test_case(Test.SingleTestAsyncTestCase)
            self.assertIs(custom_policy, asyncio.get_event_loop_policy())

    def test_event_loop_closed_and_cleared(self):
        self.event_loop_tracking.install()
        run_test_case(Test.SingleTestAsyncTestCase)
        self.assertEqual(1,
                         self.event_loop_tracking.call_counter['loop.close'])
        self.assertNotEqual(
            0, self.event_loop_tracking.call_counter['policy.set_event_loop'])

        for event, loop in reversed(self.event_loop_tracking.loop_events):
            if event == 'policy.set_event_loop':
                self.assertIsNone(
                    loop, 'final call to set_event_loop should clear loop')
                break

    def test_event_loop_stopped_when_test_fails(self):
        class TestCase(unittest.AsyncioTestCase):
            def test_one(self):
                self.fail()

        self.event_loop_tracking.install()
        run_test_case(TestCase, should_fail=True)
        self.assertEqual(
            self.event_loop_tracking.call_counter['loop.run_forever'],
            self.event_loop_tracking.call_counter['loop.stop'],
        )

    def test_async_and_normal_cleanups_are_run(self):
        class TestCase(Test.SingleTestAsyncTestCase):
            state = {'async_called': False, 'normal_called': False}

            def setUp(self):
                self.addCleanup(self.async_cleanup)
                self.addCleanup(self.normal_cleanup)

            async def async_cleanup(self):
                self.state['async_called'] = True

            def normal_cleanup(self):
                self.state['normal_called'] = True

        run_test_case(TestCase)
        self.assertTrue(TestCase.state['async_called'],
                        'async cleanup not called')
        self.assertTrue(TestCase.state['normal_called'],
                        'normal cleanup not called')

    def test_test_methods_not_run_if_setup_fails(self):
        class TestCase(unittest.AsyncioTestCase):
            state = {'test_called': False}

            def setUp(self):
                raise RuntimeError()

            def test_one(self):
                self.state['test_called'] = True

        run_test_case(TestCase, should_fail=True)
        self.assertFalse(TestCase.state['test_called'],
                         'test method called unexpectedly')

    def test_that_teardown_run_on_test_failure(self):
        class TestCase(unittest.AsyncioTestCase):
            state = {'teardown_called': False}

            def tearDown(self):
                super().tearDown()
                self.state['teardown_called'] = True

            def test_one(self):
                self.fail()

        run_test_case(TestCase, should_fail=True)
        self.assertTrue(TestCase.state['teardown_called'],
                        'tearDown not called')

    def test_that_teardown_not_run_on_setup_failure(self):
        class TestCase(Test.SingleTestAsyncTestCase):
            state = {'teardown_called': False}

            def setUp(self):
                raise RuntimeError()

            def tearDown(self):
                super().tearDown()
                self.state['teardown_called'] = True

        run_test_case(TestCase, should_fail=True)
        self.assertFalse(TestCase.state['teardown_called'],
                         'tearDown called unexpectedly')

    def test_that_test_method_not_run_when_async_setup_failure(self):
        class TestCase(unittest.AsyncioTestCase):
            state = {'test_run': False}

            async def asyncSetUp(self):
                await super().asyncSetUp()
                raise RuntimeError()

            def test_one(self):
                self.state['test_run'] = True

        run_test_case(TestCase, should_fail=True)
        self.assertFalse(TestCase.state['test_run'],
                         'test method called unexpectedly')

    def test_that_async_teardown_run_when_test_method_fails(self):
        class TestCase(unittest.AsyncioTestCase):
            state = {'teardown_called': False}

            async def asyncTearDown(self):
                await super().asyncTearDown()
                self.state['teardown_called'] = True

            def test_one(self):
                self.fail()

        run_test_case(TestCase, should_fail=True)
        self.assertTrue(TestCase.state['teardown_called'],
                        'asyncTearDown method not called')

    def test_that_async_teardown_not_run_when_async_setup_failure(self):
        class TestCase(Test.SingleTestAsyncTestCase):
            state = {'teardown_called': False}

            async def asyncSetUp(self):
                await super().asyncSetUp()
                raise RuntimeError()

            async def asyncTearDown(self):
                await super().asyncTearDown()
                self.state['teardown_called'] = True

        run_test_case(TestCase, should_fail=True)
        self.assertFalse(TestCase.state['teardown_called'],
                         'asyncTearDown called unexpectedly')

    def test_that_do_cleanups_can_be_called_from_within_tests(self):
        class TestCase(unittest.AsyncioTestCase):
            async def test_case(self):
                starting_loop = self.loop
                self.doCleanups()
                self.assertIs(starting_loop, self.loop,
                              'doCleanups altered event loop')

        run_test_case(TestCase)

    def test_unclosed_async_generators(self):
        # XXX - I believe that this tests the closure of asynchronous
        #       generators in AsyncioTestCase._terminateTest but I
        #       can't reliably catch it there in a debugger?!
        async def gen():
            for n in range(0, 10):
                yield n

        class TestCase(unittest.AsyncioTestCase):
            async def test_case(self):
                # Call the async generator without terminating
                # it.  This is what requires the call to
                # shutdown_asyncgens() in _terminateTest().
                # See PEP-525 for details.
                g = gen()
                await g.asend(None)

        run_test_case(TestCase)


if __name__ == "__main__":
    unittest.main()
