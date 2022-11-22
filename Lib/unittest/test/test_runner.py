import io
import os
import sys
import pickle
import subprocess
from test import support

import unittest
from unittest.case import _Outcome

from unittest.test.support import (LoggingResult,
                                   ResultWithNoStartTestRunStopTestRun)


def resultFactory(*_):
    return unittest.TestResult()


def getRunner():
    return unittest.TextTestRunner(resultclass=resultFactory,
                                   stream=io.StringIO())


def runTests(*cases):
    suite = unittest.TestSuite()
    for case in cases:
        tests = unittest.defaultTestLoader.loadTestsFromTestCase(case)
        suite.addTests(tests)

    runner = getRunner()

    # creating a nested suite exposes some potential bugs
    realSuite = unittest.TestSuite()
    realSuite.addTest(suite)
    # adding empty suites to the end exposes potential bugs
    suite.addTest(unittest.TestSuite())
    realSuite.addTest(unittest.TestSuite())
    return runner.run(realSuite)


def cleanup(ordering, blowUp=False):
    if not blowUp:
        ordering.append('cleanup_good')
    else:
        ordering.append('cleanup_exc')
        raise Exception('CleanUpExc')


class TestCM:
    def __init__(self, ordering, enter_result=None):
        self.ordering = ordering
        self.enter_result = enter_result

    def __enter__(self):
        self.ordering.append('enter')
        return self.enter_result

    def __exit__(self, *exc_info):
        self.ordering.append('exit')


class LacksEnterAndExit:
    pass
class LacksEnter:
    def __exit__(self, *exc_info):
        pass
class LacksExit:
    def __enter__(self):
        pass


class TestCleanUp(unittest.TestCase):
    def testCleanUp(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')
        self.assertEqual(test._cleanups, [])

        cleanups = []

        def cleanup1(*args, **kwargs):
            cleanups.append((1, args, kwargs))

        def cleanup2(*args, **kwargs):
            cleanups.append((2, args, kwargs))

        test.addCleanup(cleanup1, 1, 2, 3, four='hello', five='goodbye')
        test.addCleanup(cleanup2)

        self.assertEqual(test._cleanups,
                         [(cleanup1, (1, 2, 3), dict(four='hello', five='goodbye')),
                          (cleanup2, (), {})])

        self.assertTrue(test.doCleanups())
        self.assertEqual(cleanups, [(2, (), {}), (1, (1, 2, 3), dict(four='hello', five='goodbye'))])

    def testCleanUpWithErrors(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')
        result = unittest.TestResult()
        outcome = test._outcome = _Outcome(result=result)

        CleanUpExc = Exception('foo')
        exc2 = Exception('bar')
        def cleanup1():
            raise CleanUpExc

        def cleanup2():
            raise exc2

        test.addCleanup(cleanup1)
        test.addCleanup(cleanup2)

        self.assertFalse(test.doCleanups())
        self.assertFalse(outcome.success)

        (_, msg2), (_, msg1) = result.errors
        self.assertIn('in cleanup1', msg1)
        self.assertIn('raise CleanUpExc', msg1)
        self.assertIn('Exception: foo', msg1)
        self.assertIn('in cleanup2', msg2)
        self.assertIn('raise exc2', msg2)
        self.assertIn('Exception: bar', msg2)

    def testCleanupInRun(self):
        blowUp = False
        ordering = []

        class TestableTest(unittest.TestCase):
            def setUp(self):
                ordering.append('setUp')
                test.addCleanup(cleanup2)
                if blowUp:
                    raise Exception('foo')

            def testNothing(self):
                ordering.append('test')
                test.addCleanup(cleanup3)

            def tearDown(self):
                ordering.append('tearDown')

        test = TestableTest('testNothing')

        def cleanup1():
            ordering.append('cleanup1')
        def cleanup2():
            ordering.append('cleanup2')
        def cleanup3():
            ordering.append('cleanup3')
        test.addCleanup(cleanup1)

        def success(some_test):
            self.assertEqual(some_test, test)
            ordering.append('success')

        result = unittest.TestResult()
        result.addSuccess = success

        test.run(result)
        self.assertEqual(ordering, ['setUp', 'test', 'tearDown', 'cleanup3',
                                    'cleanup2', 'cleanup1', 'success'])

        blowUp = True
        ordering = []
        test = TestableTest('testNothing')
        test.addCleanup(cleanup1)
        test.run(result)
        self.assertEqual(ordering, ['setUp', 'cleanup2', 'cleanup1'])

    def testTestCaseDebugExecutesCleanups(self):
        ordering = []

        class TestableTest(unittest.TestCase):
            def setUp(self):
                ordering.append('setUp')
                self.addCleanup(cleanup1)

            def testNothing(self):
                ordering.append('test')
                self.addCleanup(cleanup3)

            def tearDown(self):
                ordering.append('tearDown')
                test.addCleanup(cleanup4)

        test = TestableTest('testNothing')

        def cleanup1():
            ordering.append('cleanup1')
            test.addCleanup(cleanup2)
        def cleanup2():
            ordering.append('cleanup2')
        def cleanup3():
            ordering.append('cleanup3')
        def cleanup4():
            ordering.append('cleanup4')

        test.debug()
        self.assertEqual(ordering, ['setUp', 'test', 'tearDown', 'cleanup4',
                                    'cleanup3', 'cleanup1', 'cleanup2'])


    def test_enterContext(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')
        cleanups = []

        test.addCleanup(cleanups.append, 'cleanup1')
        cm = TestCM(cleanups, 42)
        self.assertEqual(test.enterContext(cm), 42)
        test.addCleanup(cleanups.append, 'cleanup2')

        self.assertTrue(test.doCleanups())
        self.assertEqual(cleanups, ['enter', 'cleanup2', 'exit', 'cleanup1'])

    def test_enterContext_arg_errors(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')

        with self.assertRaisesRegex(TypeError, 'the context manager'):
            test.enterContext(LacksEnterAndExit())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            test.enterContext(LacksEnter())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            test.enterContext(LacksExit())

        self.assertEqual(test._cleanups, [])


class TestClassCleanup(unittest.TestCase):
    def test_addClassCleanUp(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass
        test = TestableTest('testNothing')
        self.assertEqual(test._class_cleanups, [])
        class_cleanups = []

        def class_cleanup1(*args, **kwargs):
            class_cleanups.append((3, args, kwargs))

        def class_cleanup2(*args, **kwargs):
            class_cleanups.append((4, args, kwargs))

        TestableTest.addClassCleanup(class_cleanup1, 1, 2, 3,
                                     four='hello', five='goodbye')
        TestableTest.addClassCleanup(class_cleanup2)

        self.assertEqual(test._class_cleanups,
                         [(class_cleanup1, (1, 2, 3),
                           dict(four='hello', five='goodbye')),
                          (class_cleanup2, (), {})])

        TestableTest.doClassCleanups()
        self.assertEqual(class_cleanups, [(4, (), {}), (3, (1, 2, 3),
                                          dict(four='hello', five='goodbye'))])

    def test_run_class_cleanUp(self):
        ordering = []
        blowUp = True

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering)
                if blowUp:
                    raise Exception()
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        runTests(TestableTest)
        self.assertEqual(ordering, ['setUpClass', 'cleanup_good'])

        ordering = []
        blowUp = False
        runTests(TestableTest)
        self.assertEqual(ordering,
                         ['setUpClass', 'test', 'tearDownClass', 'cleanup_good'])

    def test_run_class_cleanUp_without_tearDownClass(self):
        ordering = []
        blowUp = True

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering)
                if blowUp:
                    raise Exception()
            def testNothing(self):
                ordering.append('test')
            @classmethod
            @property
            def tearDownClass(cls):
                raise AttributeError

        runTests(TestableTest)
        self.assertEqual(ordering, ['setUpClass', 'cleanup_good'])

        ordering = []
        blowUp = False
        runTests(TestableTest)
        self.assertEqual(ordering,
                         ['setUpClass', 'test', 'cleanup_good'])

    def test_debug_executes_classCleanUp(self):
        ordering = []
        blowUp = False

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering, blowUp=blowUp)
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        suite.debug()
        self.assertEqual(ordering,
                         ['setUpClass', 'test', 'tearDownClass', 'cleanup_good'])

        ordering = []
        blowUp = True
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'test', 'tearDownClass', 'cleanup_exc'])

    def test_debug_executes_classCleanUp_when_teardown_exception(self):
        ordering = []
        blowUp = False

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering, blowUp=blowUp)
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')
                raise Exception('TearDownClassExc')

        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'TearDownClassExc')
        self.assertEqual(ordering, ['setUpClass', 'test', 'tearDownClass'])
        self.assertTrue(TestableTest._class_cleanups)
        TestableTest._class_cleanups.clear()

        ordering = []
        blowUp = True
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'TearDownClassExc')
        self.assertEqual(ordering, ['setUpClass', 'test', 'tearDownClass'])
        self.assertTrue(TestableTest._class_cleanups)
        TestableTest._class_cleanups.clear()

    def test_doClassCleanups_with_errors_addClassCleanUp(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        def cleanup1():
            raise Exception('cleanup1')

        def cleanup2():
            raise Exception('cleanup2')

        TestableTest.addClassCleanup(cleanup1)
        TestableTest.addClassCleanup(cleanup2)
        with self.assertRaises(Exception) as e:
            TestableTest.doClassCleanups()
            self.assertEqual(e, 'cleanup1')

    def test_with_errors_addCleanUp(self):
        ordering = []
        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering)
            def setUp(self):
                ordering.append('setUp')
                self.addCleanup(cleanup, ordering, blowUp=True)
            def testNothing(self):
                pass
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'setUp', 'cleanup_exc',
                          'tearDownClass', 'cleanup_good'])

    def test_run_with_errors_addClassCleanUp(self):
        ordering = []
        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering, blowUp=True)
            def setUp(self):
                ordering.append('setUp')
                self.addCleanup(cleanup, ordering)
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'setUp', 'test', 'cleanup_good',
                          'tearDownClass', 'cleanup_exc'])

    def test_with_errors_in_addClassCleanup_and_setUps(self):
        ordering = []
        class_blow_up = False
        method_blow_up = False

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering, blowUp=True)
                if class_blow_up:
                    raise Exception('ClassExc')
            def setUp(self):
                ordering.append('setUp')
                if method_blow_up:
                    raise Exception('MethodExc')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'setUp', 'test',
                          'tearDownClass', 'cleanup_exc'])

        ordering = []
        class_blow_up = True
        method_blow_up = False
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: ClassExc')
        self.assertEqual(result.errors[1][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'cleanup_exc'])

        ordering = []
        class_blow_up = False
        method_blow_up = True
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: MethodExc')
        self.assertEqual(result.errors[1][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'setUp', 'tearDownClass',
                          'cleanup_exc'])

    def test_with_errors_in_tearDownClass(self):
        ordering = []
        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering)
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')
                raise Exception('TearDownExc')

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: TearDownExc')
        self.assertEqual(ordering,
                         ['setUpClass', 'test', 'tearDownClass', 'cleanup_good'])

    def test_enterClassContext(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        cleanups = []

        TestableTest.addClassCleanup(cleanups.append, 'cleanup1')
        cm = TestCM(cleanups, 42)
        self.assertEqual(TestableTest.enterClassContext(cm), 42)
        TestableTest.addClassCleanup(cleanups.append, 'cleanup2')

        TestableTest.doClassCleanups()
        self.assertEqual(cleanups, ['enter', 'cleanup2', 'exit', 'cleanup1'])

    def test_enterClassContext_arg_errors(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        with self.assertRaisesRegex(TypeError, 'the context manager'):
            TestableTest.enterClassContext(LacksEnterAndExit())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            TestableTest.enterClassContext(LacksEnter())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            TestableTest.enterClassContext(LacksExit())

        self.assertEqual(TestableTest._class_cleanups, [])

    def test_run_nested_test(self):
        ordering = []

        class InnerTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('inner setup')
                cls.addClassCleanup(ordering.append, 'inner cleanup')
            def test(self):
                ordering.append('inner test')

        class OuterTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('outer setup')
                cls.addClassCleanup(ordering.append, 'outer cleanup')
            def test(self):
                ordering.append('start outer test')
                runTests(InnerTest)
                ordering.append('end outer test')

        runTests(OuterTest)
        self.assertEqual(ordering, [
                'outer setup', 'start outer test',
                'inner setup', 'inner test', 'inner cleanup',
                'end outer test', 'outer cleanup'])


class TestModuleCleanUp(unittest.TestCase):
    def test_add_and_do_ModuleCleanup(self):
        module_cleanups = []

        def module_cleanup1(*args, **kwargs):
            module_cleanups.append((3, args, kwargs))

        def module_cleanup2(*args, **kwargs):
            module_cleanups.append((4, args, kwargs))

        class Module(object):
            unittest.addModuleCleanup(module_cleanup1, 1, 2, 3,
                                      four='hello', five='goodbye')
            unittest.addModuleCleanup(module_cleanup2)

        self.assertEqual(unittest.case._module_cleanups,
                         [(module_cleanup1, (1, 2, 3),
                           dict(four='hello', five='goodbye')),
                          (module_cleanup2, (), {})])

        unittest.case.doModuleCleanups()
        self.assertEqual(module_cleanups, [(4, (), {}), (3, (1, 2, 3),
                                          dict(four='hello', five='goodbye'))])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_doModuleCleanup_with_errors_in_addModuleCleanup(self):
        module_cleanups = []

        def module_cleanup_good(*args, **kwargs):
            module_cleanups.append((3, args, kwargs))

        def module_cleanup_bad(*args, **kwargs):
            raise Exception('CleanUpExc')

        class Module(object):
            unittest.addModuleCleanup(module_cleanup_good, 1, 2, 3,
                                      four='hello', five='goodbye')
            unittest.addModuleCleanup(module_cleanup_bad)
        self.assertEqual(unittest.case._module_cleanups,
                         [(module_cleanup_good, (1, 2, 3),
                           dict(four='hello', five='goodbye')),
                          (module_cleanup_bad, (), {})])
        with self.assertRaises(Exception) as e:
            unittest.case.doModuleCleanups()
        self.assertEqual(str(e.exception), 'CleanUpExc')
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_addModuleCleanup_arg_errors(self):
        cleanups = []
        def cleanup(*args, **kwargs):
            cleanups.append((args, kwargs))

        class Module(object):
            unittest.addModuleCleanup(cleanup, 1, 2, function='hello')
            with self.assertRaises(TypeError):
                unittest.addModuleCleanup(function=cleanup, arg='hello')
            with self.assertRaises(TypeError):
                unittest.addModuleCleanup()
        unittest.case.doModuleCleanups()
        self.assertEqual(cleanups,
                         [((1, 2), {'function': 'hello'})])

    def test_run_module_cleanUp(self):
        blowUp = True
        ordering = []
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)
                if blowUp:
                    raise Exception('setUpModule Exc')
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        result = runTests(TestableTest)
        self.assertEqual(ordering, ['setUpModule', 'cleanup_good'])
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: setUpModule Exc')

        ordering = []
        blowUp = False
        runTests(TestableTest)
        self.assertEqual(ordering,
                         ['setUpModule', 'setUpClass', 'test', 'tearDownClass',
                          'tearDownModule', 'cleanup_good'])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_run_multiple_module_cleanUp(self):
        blowUp = True
        blowUp2 = False
        ordering = []
        class Module1(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)
                if blowUp:
                    raise Exception()
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class Module2(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule2')
                unittest.addModuleCleanup(cleanup, ordering)
                if blowUp2:
                    raise Exception()
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule2')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        class TestableTest2(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass2')
            def testNothing(self):
                ordering.append('test2')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass2')

        TestableTest.__module__ = 'Module1'
        sys.modules['Module1'] = Module1
        TestableTest2.__module__ = 'Module2'
        sys.modules['Module2'] = Module2
        runTests(TestableTest, TestableTest2)
        self.assertEqual(ordering, ['setUpModule', 'cleanup_good',
                                    'setUpModule2', 'setUpClass2', 'test2',
                                    'tearDownClass2', 'tearDownModule2',
                                    'cleanup_good'])
        ordering = []
        blowUp = False
        blowUp2 = True
        runTests(TestableTest, TestableTest2)
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'tearDownModule',
                                    'cleanup_good', 'setUpModule2',
                                    'cleanup_good'])

        ordering = []
        blowUp = False
        blowUp2 = False
        runTests(TestableTest, TestableTest2)
        self.assertEqual(ordering,
                         ['setUpModule', 'setUpClass', 'test', 'tearDownClass',
                          'tearDownModule', 'cleanup_good', 'setUpModule2',
                          'setUpClass2', 'test2', 'tearDownClass2',
                          'tearDownModule2', 'cleanup_good'])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_run_module_cleanUp_without_teardown(self):
        ordering = []
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        runTests(TestableTest)
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'cleanup_good'])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_run_module_cleanUp_when_teardown_exception(self):
        ordering = []
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')
                raise Exception('CleanUpExc')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'tearDownModule',
                                    'cleanup_good'])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_debug_module_executes_cleanUp(self):
        ordering = []
        blowUp = False
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering, blowUp=blowUp)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        suite.debug()
        self.assertEqual(ordering,
                         ['setUpModule', 'setUpClass', 'test', 'tearDownClass',
                          'tearDownModule', 'cleanup_good'])
        self.assertEqual(unittest.case._module_cleanups, [])

        ordering = []
        blowUp = True
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'CleanUpExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'tearDownModule', 'cleanup_exc'])
        self.assertEqual(unittest.case._module_cleanups, [])

    def test_debug_module_cleanUp_when_teardown_exception(self):
        ordering = []
        blowUp = False
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering, blowUp=blowUp)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')
                raise Exception('TearDownModuleExc')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'TearDownModuleExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'tearDownModule'])
        self.assertTrue(unittest.case._module_cleanups)
        unittest.case._module_cleanups.clear()

        ordering = []
        blowUp = True
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestableTest)
        with self.assertRaises(Exception) as cm:
            suite.debug()
        self.assertEqual(str(cm.exception), 'TearDownModuleExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'test',
                                    'tearDownClass', 'tearDownModule'])
        self.assertTrue(unittest.case._module_cleanups)
        unittest.case._module_cleanups.clear()

    def test_addClassCleanup_arg_errors(self):
        cleanups = []
        def cleanup(*args, **kwargs):
            cleanups.append((args, kwargs))

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                cls.addClassCleanup(cleanup, 1, 2, function=3, cls=4)
                with self.assertRaises(TypeError):
                    cls.addClassCleanup(function=cleanup, arg='hello')
            def testNothing(self):
                pass

        with self.assertRaises(TypeError):
            TestableTest.addClassCleanup()
        with self.assertRaises(TypeError):
            unittest.TestCase.addCleanup(cls=TestableTest(), function=cleanup)
        runTests(TestableTest)
        self.assertEqual(cleanups,
                         [((1, 2), {'function': 3, 'cls': 4})])

    def test_addCleanup_arg_errors(self):
        cleanups = []
        def cleanup(*args, **kwargs):
            cleanups.append((args, kwargs))

        class TestableTest(unittest.TestCase):
            def setUp(self2):
                self2.addCleanup(cleanup, 1, 2, function=3, self=4)
                with self.assertRaises(TypeError):
                    self2.addCleanup(function=cleanup, arg='hello')
            def testNothing(self):
                pass

        with self.assertRaises(TypeError):
            TestableTest().addCleanup()
        with self.assertRaises(TypeError):
            unittest.TestCase.addCleanup(self=TestableTest(), function=cleanup)
        runTests(TestableTest)
        self.assertEqual(cleanups,
                         [((1, 2), {'function': 3, 'self': 4})])

    def test_with_errors_in_addClassCleanup(self):
        ordering = []

        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                cls.addClassCleanup(cleanup, ordering, blowUp=True)
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpModule', 'setUpClass', 'test', 'tearDownClass',
                          'cleanup_exc', 'tearDownModule', 'cleanup_good'])

    def test_with_errors_in_addCleanup(self):
        ordering = []
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            def setUp(self):
                ordering.append('setUp')
                self.addCleanup(cleanup, ordering, blowUp=True)
            def testNothing(self):
                ordering.append('test')
            def tearDown(self):
                ordering.append('tearDown')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpModule', 'setUp', 'test', 'tearDown',
                          'cleanup_exc', 'tearDownModule', 'cleanup_good'])

    def test_with_errors_in_addModuleCleanup_and_setUps(self):
        ordering = []
        module_blow_up = False
        class_blow_up = False
        method_blow_up = False
        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup, ordering, blowUp=True)
                if module_blow_up:
                    raise Exception('ModuleExc')
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                ordering.append('setUpClass')
                if class_blow_up:
                    raise Exception('ClassExc')
            def setUp(self):
                ordering.append('setUp')
                if method_blow_up:
                    raise Exception('MethodExc')
            def testNothing(self):
                ordering.append('test')
            @classmethod
            def tearDownClass(cls):
                ordering.append('tearDownClass')

        TestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module

        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering,
                         ['setUpModule', 'setUpClass', 'setUp', 'test',
                          'tearDownClass', 'tearDownModule',
                          'cleanup_exc'])

        ordering = []
        module_blow_up = True
        class_blow_up = False
        method_blow_up = False
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: ModuleExc')
        self.assertEqual(result.errors[1][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering, ['setUpModule', 'cleanup_exc'])

        ordering = []
        module_blow_up = False
        class_blow_up = True
        method_blow_up = False
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: ClassExc')
        self.assertEqual(result.errors[1][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass',
                                    'tearDownModule', 'cleanup_exc'])

        ordering = []
        module_blow_up = False
        class_blow_up = False
        method_blow_up = True
        result = runTests(TestableTest)
        self.assertEqual(result.errors[0][1].splitlines()[-1],
                         'Exception: MethodExc')
        self.assertEqual(result.errors[1][1].splitlines()[-1],
                         'Exception: CleanUpExc')
        self.assertEqual(ordering, ['setUpModule', 'setUpClass', 'setUp',
                                    'tearDownClass', 'tearDownModule',
                                    'cleanup_exc'])

    def test_module_cleanUp_with_multiple_classes(self):
        ordering =[]
        def cleanup1():
            ordering.append('cleanup1')

        def cleanup2():
            ordering.append('cleanup2')

        def cleanup3():
            ordering.append('cleanup3')

        class Module(object):
            @staticmethod
            def setUpModule():
                ordering.append('setUpModule')
                unittest.addModuleCleanup(cleanup1)
            @staticmethod
            def tearDownModule():
                ordering.append('tearDownModule')

        class TestableTest(unittest.TestCase):
            def setUp(self):
                ordering.append('setUp')
                self.addCleanup(cleanup2)
            def testNothing(self):
                ordering.append('test')
            def tearDown(self):
                ordering.append('tearDown')

        class OtherTestableTest(unittest.TestCase):
            def setUp(self):
                ordering.append('setUp2')
                self.addCleanup(cleanup3)
            def testNothing(self):
                ordering.append('test2')
            def tearDown(self):
                ordering.append('tearDown2')

        TestableTest.__module__ = 'Module'
        OtherTestableTest.__module__ = 'Module'
        sys.modules['Module'] = Module
        runTests(TestableTest, OtherTestableTest)
        self.assertEqual(ordering,
                         ['setUpModule', 'setUp', 'test', 'tearDown',
                          'cleanup2',  'setUp2', 'test2', 'tearDown2',
                          'cleanup3', 'tearDownModule', 'cleanup1'])

    def test_enterModuleContext(self):
        cleanups = []

        unittest.addModuleCleanup(cleanups.append, 'cleanup1')
        cm = TestCM(cleanups, 42)
        self.assertEqual(unittest.enterModuleContext(cm), 42)
        unittest.addModuleCleanup(cleanups.append, 'cleanup2')

        unittest.case.doModuleCleanups()
        self.assertEqual(cleanups, ['enter', 'cleanup2', 'exit', 'cleanup1'])

    def test_enterModuleContext_arg_errors(self):
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        with self.assertRaisesRegex(TypeError, 'the context manager'):
            unittest.enterModuleContext(LacksEnterAndExit())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            unittest.enterModuleContext(LacksEnter())
        with self.assertRaisesRegex(TypeError, 'the context manager'):
            unittest.enterModuleContext(LacksExit())

        self.assertEqual(unittest.case._module_cleanups, [])


class Test_TextTestRunner(unittest.TestCase):
    """Tests for TextTestRunner."""

    def setUp(self):
        # clean the environment from pre-existing PYTHONWARNINGS to make
        # test_warnings results consistent
        self.pythonwarnings = os.environ.get('PYTHONWARNINGS')
        if self.pythonwarnings:
            del os.environ['PYTHONWARNINGS']

    def tearDown(self):
        # bring back pre-existing PYTHONWARNINGS if present
        if self.pythonwarnings:
            os.environ['PYTHONWARNINGS'] = self.pythonwarnings

    def test_init(self):
        runner = unittest.TextTestRunner()
        self.assertFalse(runner.failfast)
        self.assertFalse(runner.buffer)
        self.assertEqual(runner.verbosity, 1)
        self.assertEqual(runner.warnings, None)
        self.assertTrue(runner.descriptions)
        self.assertEqual(runner.resultclass, unittest.TextTestResult)
        self.assertFalse(runner.tb_locals)

    def test_multiple_inheritance(self):
        class AResult(unittest.TestResult):
            def __init__(self, stream, descriptions, verbosity):
                super(AResult, self).__init__(stream, descriptions, verbosity)

        class ATextResult(unittest.TextTestResult, AResult):
            pass

        # This used to raise an exception due to TextTestResult not passing
        # on arguments in its __init__ super call
        ATextResult(None, None, 1)

    def testBufferAndFailfast(self):
        class Test(unittest.TestCase):
            def testFoo(self):
                pass
        result = unittest.TestResult()
        runner = unittest.TextTestRunner(stream=io.StringIO(), failfast=True,
                                         buffer=True)
        # Use our result object
        runner._makeResult = lambda: result
        runner.run(Test('testFoo'))

        self.assertTrue(result.failfast)
        self.assertTrue(result.buffer)

    def test_locals(self):
        runner = unittest.TextTestRunner(stream=io.StringIO(), tb_locals=True)
        result = runner.run(unittest.TestSuite())
        self.assertEqual(True, result.tb_locals)

    def testRunnerRegistersResult(self):
        class Test(unittest.TestCase):
            def testFoo(self):
                pass
        originalRegisterResult = unittest.runner.registerResult
        def cleanup():
            unittest.runner.registerResult = originalRegisterResult
        self.addCleanup(cleanup)

        result = unittest.TestResult()
        runner = unittest.TextTestRunner(stream=io.StringIO())
        # Use our result object
        runner._makeResult = lambda: result

        self.wasRegistered = 0
        def fakeRegisterResult(thisResult):
            self.wasRegistered += 1
            self.assertEqual(thisResult, result)
        unittest.runner.registerResult = fakeRegisterResult

        runner.run(unittest.TestSuite())
        self.assertEqual(self.wasRegistered, 1)

    def test_works_with_result_without_startTestRun_stopTestRun(self):
        class OldTextResult(ResultWithNoStartTestRunStopTestRun):
            separator2 = ''
            def printErrors(self):
                pass

        class Runner(unittest.TextTestRunner):
            def __init__(self):
                super(Runner, self).__init__(io.StringIO())

            def _makeResult(self):
                return OldTextResult()

        runner = Runner()
        runner.run(unittest.TestSuite())

    def test_startTestRun_stopTestRun_called(self):
        class LoggingTextResult(LoggingResult):
            separator2 = ''
            def printErrors(self):
                pass

        class LoggingRunner(unittest.TextTestRunner):
            def __init__(self, events):
                super(LoggingRunner, self).__init__(io.StringIO())
                self._events = events

            def _makeResult(self):
                return LoggingTextResult(self._events)

        events = []
        runner = LoggingRunner(events)
        runner.run(unittest.TestSuite())
        expected = ['startTestRun', 'stopTestRun']
        self.assertEqual(events, expected)

    def test_pickle_unpickle(self):
        # Issue #7197: a TextTestRunner should be (un)pickleable. This is
        # required by test_multiprocessing under Windows (in verbose mode).
        stream = io.StringIO("foo")
        runner = unittest.TextTestRunner(stream)
        for protocol in range(2, pickle.HIGHEST_PROTOCOL + 1):
            s = pickle.dumps(runner, protocol)
            obj = pickle.loads(s)
            # StringIO objects never compare equal, a cheap test instead.
            self.assertEqual(obj.stream.getvalue(), stream.getvalue())

    def test_resultclass(self):
        def MockResultClass(*args):
            return args
        STREAM = object()
        DESCRIPTIONS = object()
        VERBOSITY = object()
        runner = unittest.TextTestRunner(STREAM, DESCRIPTIONS, VERBOSITY,
                                         resultclass=MockResultClass)
        self.assertEqual(runner.resultclass, MockResultClass)

        expectedresult = (runner.stream, DESCRIPTIONS, VERBOSITY)
        self.assertEqual(runner._makeResult(), expectedresult)

    @support.requires_subprocess()
    def test_warnings(self):
        """
        Check that warnings argument of TextTestRunner correctly affects the
        behavior of the warnings.
        """
        # see #10535 and the _test_warnings file for more information

        def get_parse_out_err(p):
            return [b.splitlines() for b in p.communicate()]
        opts = dict(stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    cwd=os.path.dirname(__file__))
        ae_msg = b'Please use assertEqual instead.'
        at_msg = b'Please use assertTrue instead.'

        # no args -> all the warnings are printed, unittest warnings only once
        p = subprocess.Popen([sys.executable, '-E', '_test_warnings.py'], **opts)
        with p:
            out, err = get_parse_out_err(p)
        self.assertIn(b'OK', err)
        # check that the total number of warnings in the output is correct
        self.assertEqual(len(out), 12)
        # check that the numbers of the different kind of warnings is correct
        for msg in [b'dw', b'iw', b'uw']:
            self.assertEqual(out.count(msg), 3)
        for msg in [ae_msg, at_msg, b'rw']:
            self.assertEqual(out.count(msg), 1)

        args_list = (
            # passing 'ignore' as warnings arg -> no warnings
            [sys.executable, '_test_warnings.py', 'ignore'],
            # -W doesn't affect the result if the arg is passed
            [sys.executable, '-Wa', '_test_warnings.py', 'ignore'],
            # -W affects the result if the arg is not passed
            [sys.executable, '-Wi', '_test_warnings.py']
        )
        # in all these cases no warnings are printed
        for args in args_list:
            p = subprocess.Popen(args, **opts)
            with p:
                out, err = get_parse_out_err(p)
            self.assertIn(b'OK', err)
            self.assertEqual(len(out), 0)


        # passing 'always' as warnings arg -> all the warnings printed,
        #                                     unittest warnings only once
        p = subprocess.Popen([sys.executable, '_test_warnings.py', 'always'],
                             **opts)
        with p:
            out, err = get_parse_out_err(p)
        self.assertIn(b'OK', err)
        self.assertEqual(len(out), 14)
        for msg in [b'dw', b'iw', b'uw', b'rw']:
            self.assertEqual(out.count(msg), 3)
        for msg in [ae_msg, at_msg]:
            self.assertEqual(out.count(msg), 1)

    def testStdErrLookedUpAtInstantiationTime(self):
        # see issue 10786
        old_stderr = sys.stderr
        f = io.StringIO()
        sys.stderr = f
        try:
            runner = unittest.TextTestRunner()
            self.assertTrue(runner.stream.stream is f)
        finally:
            sys.stderr = old_stderr

    def testSpecifiedStreamUsed(self):
        # see issue 10786
        f = io.StringIO()
        runner = unittest.TextTestRunner(f)
        self.assertTrue(runner.stream.stream is f)


if __name__ == "__main__":
    unittest.main()
