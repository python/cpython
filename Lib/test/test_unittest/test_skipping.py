import unittest

from test.support import force_not_colorized
from test.test_unittest.support import LoggingResult


class Test_TestSkipping(unittest.TestCase):

    def test_skipping(self):
        class Foo(unittest.TestCase):
            def defaultTestResult(self):
                return LoggingResult(events)
            def test_skip_me(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

        events = []
        result = test.run()
        self.assertEqual(events, ['startTestRun', 'startTest', 'addSkip',
                                  'stopTest', 'stopTestRun'])
        self.assertEqual(result.skipped, [(test, "skip")])
        self.assertEqual(result.testsRun, 1)

        # Try letting setUp skip the test now.
        class Foo(unittest.TestCase):
            def defaultTestResult(self):
                return LoggingResult(events)
            def setUp(self):
                self.skipTest("testing")
            def test_nothing(self): pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_nothing")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(result.testsRun, 1)

        events = []
        result = test.run()
        self.assertEqual(events, ['startTestRun', 'startTest', 'addSkip',
                                  'stopTest', 'stopTestRun'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(result.testsRun, 1)

    def test_skipping_subtests(self):
        class Foo(unittest.TestCase):
            def defaultTestResult(self):
                return LoggingResult(events)
            def test_skip_me(self):
                with self.subTest(a=1):
                    with self.subTest(b=2):
                        self.skipTest("skip 1")
                    self.skipTest("skip 2")
                self.skipTest("skip 3")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'addSkip',
                                  'addSkip', 'stopTest'])
        self.assertEqual(len(result.skipped), 3)
        subtest, msg = result.skipped[0]
        self.assertEqual(msg, "skip 1")
        self.assertIsInstance(subtest, unittest.TestCase)
        self.assertIsNot(subtest, test)
        subtest, msg = result.skipped[1]
        self.assertEqual(msg, "skip 2")
        self.assertIsInstance(subtest, unittest.TestCase)
        self.assertIsNot(subtest, test)
        self.assertEqual(result.skipped[2], (test, "skip 3"))

        events = []
        result = test.run()
        self.assertEqual(events,
                         ['startTestRun', 'startTest', 'addSkip', 'addSkip',
                          'addSkip', 'stopTest', 'stopTestRun'])
        self.assertEqual([msg for subtest, msg in result.skipped],
                         ['skip 1', 'skip 2', 'skip 3'])

    def test_skipping_decorators(self):
        op_table = ((unittest.skipUnless, False, True),
                    (unittest.skipIf, True, False))
        for deco, do_skip, dont_skip in op_table:
            class Foo(unittest.TestCase):
                def defaultTestResult(self):
                    return LoggingResult(events)

                @deco(do_skip, "testing")
                def test_skip(self): pass

                @deco(dont_skip, "testing")
                def test_dont_skip(self): pass
            test_do_skip = Foo("test_skip")
            test_dont_skip = Foo("test_dont_skip")

            suite = unittest.TestSuite([test_do_skip, test_dont_skip])
            events = []
            result = LoggingResult(events)
            self.assertIs(suite.run(result), result)
            self.assertEqual(len(result.skipped), 1)
            expected = ['startTest', 'addSkip', 'stopTest',
                        'startTest', 'addSuccess', 'stopTest']
            self.assertEqual(events, expected)
            self.assertEqual(result.testsRun, 2)
            self.assertEqual(result.skipped, [(test_do_skip, "testing")])
            self.assertTrue(result.wasSuccessful())

            events = []
            result = test_do_skip.run()
            self.assertEqual(events, ['startTestRun', 'startTest', 'addSkip',
                                      'stopTest', 'stopTestRun'])
            self.assertEqual(result.skipped, [(test_do_skip, "testing")])

            events = []
            result = test_dont_skip.run()
            self.assertEqual(events, ['startTestRun', 'startTest', 'addSuccess',
                                      'stopTest', 'stopTestRun'])
            self.assertEqual(result.skipped, [])

    def test_skip_class(self):
        @unittest.skip("testing")
        class Foo(unittest.TestCase):
            def defaultTestResult(self):
                return LoggingResult(events)
            def test_1(self):
                record.append(1)
        events = []
        record = []
        result = LoggingResult(events)
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        self.assertIs(suite.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(record, [])

        events = []
        result = test.run()
        self.assertEqual(events, ['startTestRun', 'startTest', 'addSkip',
                                  'stopTest', 'stopTestRun'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(record, [])

    def test_skip_non_unittest_class(self):
        @unittest.skip("testing")
        class Mixin:
            def test_1(self):
                record.append(1)
        class Foo(Mixin, unittest.TestCase):
            pass
        record = []
        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        self.assertIs(suite.run(result), result)
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(record, [])

    def test_skip_in_setup(self):
        class Foo(unittest.TestCase):
            def setUp(self):
                self.skipTest("skip")
            def test_skip_me(self):
                self.fail("shouldn't come here")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

    def test_skip_in_cleanup(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                pass
            def tearDown(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

    def test_failure_and_skip_in_cleanup(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                self.fail("fail")
            def tearDown(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addFailure', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

    def test_skipping_and_fail_in_cleanup(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                self.skipTest("skip")
            def tearDown(self):
                self.fail("fail")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        self.assertIs(test.run(result), result)
        self.assertEqual(events, ['startTest', 'addSkip', 'addFailure', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

    def test_expected_failure(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                self.fail("help me!")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(result.expectedFailures[0][0], test)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertTrue(result.wasSuccessful())

    def test_expected_failure_with_wrapped_class(self):
        @unittest.expectedFailure
        class Foo(unittest.TestCase):
            def test_1(self):
                self.assertTrue(False)

        events = []
        result = LoggingResult(events)
        test = Foo("test_1")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(result.expectedFailures[0][0], test)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertTrue(result.wasSuccessful())

    def test_expected_failure_with_wrapped_subclass(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                self.assertTrue(False)

        @unittest.expectedFailure
        class Bar(Foo):
            pass

        events = []
        result = LoggingResult(events)
        test = Bar("test_1")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(result.expectedFailures[0][0], test)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertTrue(result.wasSuccessful())

    def test_expected_failure_subtests(self):
        # A failure in any subtest counts as the expected failure of the
        # whole test.
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                with self.subTest():
                    # This one succeeds
                    pass
                with self.subTest():
                    self.fail("help me!")
                with self.subTest():
                    # This one doesn't get executed
                    self.fail("shouldn't come here")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addSubTestSuccess',
                          'addExpectedFailure', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(len(result.expectedFailures), 1)
        self.assertIs(result.expectedFailures[0][0], test)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertTrue(result.wasSuccessful())

    @force_not_colorized
    def test_expected_failure_and_fail_in_cleanup(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                self.fail("help me!")
            def tearDown(self):
                self.fail("bad tearDown")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addFailure', 'stopTest'])
        self.assertEqual(len(result.failures), 1)
        self.assertIn('AssertionError: bad tearDown', result.failures[0][1])
        self.assertFalse(result.expectedFailures)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertFalse(result.wasSuccessful())

    def test_expected_failure_and_skip_in_cleanup(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                self.fail("help me!")
            def tearDown(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addSkip', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertFalse(result.expectedFailures)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertEqual(result.skipped, [(test, "skip")])
        self.assertTrue(result.wasSuccessful())

    def test_unexpected_success(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addUnexpectedSuccess', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertFalse(result.expectedFailures)
        self.assertEqual(result.unexpectedSuccesses, [test])
        self.assertFalse(result.wasSuccessful())

    def test_unexpected_success_subtests(self):
        # Success in all subtests counts as the unexpected success of
        # the whole test.
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                with self.subTest():
                    # This one succeeds
                    pass
                with self.subTest():
                    # So does this one
                    pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest',
                          'addSubTestSuccess', 'addSubTestSuccess',
                          'addUnexpectedSuccess', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertFalse(result.expectedFailures)
        self.assertEqual(result.unexpectedSuccesses, [test])
        self.assertFalse(result.wasSuccessful())

    @force_not_colorized
    def test_unexpected_success_and_fail_in_cleanup(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                pass
            def tearDown(self):
                self.fail("bad tearDown")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addFailure', 'stopTest'])
        self.assertEqual(len(result.failures), 1)
        self.assertIn('AssertionError: bad tearDown', result.failures[0][1])
        self.assertFalse(result.expectedFailures)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertFalse(result.wasSuccessful())

    def test_unexpected_success_and_skip_in_cleanup(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                pass
            def tearDown(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        self.assertIs(test.run(result), result)
        self.assertEqual(events,
                         ['startTest', 'addSkip', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertFalse(result.expectedFailures)
        self.assertFalse(result.unexpectedSuccesses)
        self.assertEqual(result.skipped, [(test, "skip")])
        self.assertTrue(result.wasSuccessful())

    def test_skip_doesnt_run_setup(self):
        class Foo(unittest.TestCase):
            wasSetUp = False
            wasTornDown = False
            def setUp(self):
                Foo.wasSetUp = True
            def tornDown(self):
                Foo.wasTornDown = True
            @unittest.skip('testing')
            def test_1(self):
                pass

        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        self.assertIs(suite.run(result), result)
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertFalse(Foo.wasSetUp)
        self.assertFalse(Foo.wasTornDown)

    def test_decorated_skip(self):
        def decorator(func):
            def inner(*a):
                return func(*a)
            return inner

        class Foo(unittest.TestCase):
            @decorator
            @unittest.skip('testing')
            def test_1(self):
                pass

        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        self.assertIs(suite.run(result), result)
        self.assertEqual(result.skipped, [(test, "testing")])

    def test_skip_without_reason(self):
        class Foo(unittest.TestCase):
            @unittest.skip
            def test_1(self):
                pass

        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        self.assertIs(suite.run(result), result)
        self.assertEqual(result.skipped, [(test, "")])

    def test_debug_skipping(self):
        class Foo(unittest.TestCase):
            def setUp(self):
                events.append("setUp")
            def tearDown(self):
                events.append("tearDown")
            def test1(self):
                self.skipTest('skipping exception')
                events.append("test1")
            @unittest.skip("skipping decorator")
            def test2(self):
                events.append("test2")

        events = []
        test = Foo("test1")
        with self.assertRaises(unittest.SkipTest) as cm:
            test.debug()
        self.assertIn("skipping exception", str(cm.exception))
        self.assertEqual(events, ["setUp"])

        events = []
        test = Foo("test2")
        with self.assertRaises(unittest.SkipTest) as cm:
            test.debug()
        self.assertIn("skipping decorator", str(cm.exception))
        self.assertEqual(events, [])

    def test_debug_skipping_class(self):
        @unittest.skip("testing")
        class Foo(unittest.TestCase):
            def setUp(self):
                events.append("setUp")
            def tearDown(self):
                events.append("tearDown")
            def test(self):
                events.append("test")

        events = []
        test = Foo("test")
        with self.assertRaises(unittest.SkipTest) as cm:
            test.debug()
        self.assertIn("testing", str(cm.exception))
        self.assertEqual(events, [])

    def test_debug_skipping_subtests(self):
        class Foo(unittest.TestCase):
            def setUp(self):
                events.append("setUp")
            def tearDown(self):
                events.append("tearDown")
            def test(self):
                with self.subTest(a=1):
                    events.append('subtest')
                    self.skipTest("skip subtest")
                    events.append('end subtest')
                events.append('end test')

        events = []
        result = LoggingResult(events)
        test = Foo("test")
        with self.assertRaises(unittest.SkipTest) as cm:
            test.debug()
        self.assertIn("skip subtest", str(cm.exception))
        self.assertEqual(events, ['setUp', 'subtest'])


if __name__ == "__main__":
    unittest.main()
