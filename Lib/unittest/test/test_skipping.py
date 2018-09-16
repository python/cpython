import unittest

from unittest.test.support import LoggingResult


class Test_TestSkipping(unittest.TestCase):

    def test_skipping(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        test.run(result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

        # Try letting setUp skip the test now.
        class Foo(unittest.TestCase):
            def setUp(self):
                self.skipTest("testing")
            def test_nothing(self): pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_nothing")
        test.run(result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(result.testsRun, 1)

    def test_skipping_subtests(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                with self.subTest(a=1):
                    with self.subTest(b=2):
                        self.skipTest("skip 1")
                    self.skipTest("skip 2")
                self.skipTest("skip 3")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        test.run(result)
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

    def test_skipping_decorators(self):
        op_table = ((unittest.skipUnless, False, True),
                    (unittest.skipIf, True, False))
        for deco, do_skip, dont_skip in op_table:
            class Foo(unittest.TestCase):
                @deco(do_skip, "testing")
                def test_skip(self): pass

                @deco(dont_skip, "testing")
                def test_dont_skip(self): pass
            test_do_skip = Foo("test_skip")
            test_dont_skip = Foo("test_dont_skip")
            suite = unittest.TestSuite([test_do_skip, test_dont_skip])
            events = []
            result = LoggingResult(events)
            suite.run(result)
            self.assertEqual(len(result.skipped), 1)
            expected = ['startTest', 'addSkip', 'stopTest',
                        'startTest', 'addSuccess', 'stopTest']
            self.assertEqual(events, expected)
            self.assertEqual(result.testsRun, 2)
            self.assertEqual(result.skipped, [(test_do_skip, "testing")])
            self.assertTrue(result.wasSuccessful())

    def test_skip_class(self):
        @unittest.skip("testing")
        class Foo(unittest.TestCase):
            def test_1(self):
                record.append(1)
        record = []
        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        suite.run(result)
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
        suite.run(result)
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(record, [])

    def test_expected_failure(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                self.fail("help me!")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertEqual(result.expectedFailures[0][0], test)
        self.assertTrue(result.wasSuccessful())

    def test_expected_failure_with_wrapped_class(self):
        @unittest.expectedFailure
        class Foo(unittest.TestCase):
            def test_1(self):
                self.assertTrue(False)

        events = []
        result = LoggingResult(events)
        test = Foo("test_1")
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertEqual(result.expectedFailures[0][0], test)
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
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertEqual(result.expectedFailures[0][0], test)
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
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addSubTestSuccess',
                          'addExpectedFailure', 'stopTest'])
        self.assertEqual(len(result.expectedFailures), 1)
        self.assertIs(result.expectedFailures[0][0], test)
        self.assertTrue(result.wasSuccessful())

    def test_unexpected_success(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addUnexpectedSuccess', 'stopTest'])
        self.assertFalse(result.failures)
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
        test.run(result)
        self.assertEqual(events,
                         ['startTest',
                          'addSubTestSuccess', 'addSubTestSuccess',
                          'addUnexpectedSuccess', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(result.unexpectedSuccesses, [test])
        self.assertFalse(result.wasSuccessful())

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
        suite.run(result)
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
        suite.run(result)
        self.assertEqual(result.skipped, [(test, "testing")])

    def test_skip_without_reason(self):
        class Foo(unittest.TestCase):
            @unittest.skip
            def test_1(self):
                pass

        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        suite.run(result)
        self.assertEqual(result.skipped, [(test, "")])

if __name__ == "__main__":
    unittest.main()
