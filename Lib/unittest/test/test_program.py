from cStringIO import StringIO

import unittest


class Test_TestProgram(unittest.TestCase):

    # Horrible white box test
    def testNoExit(self):
        result = object()
        test = object()

        class FakeRunner(object):
            def run(self, test):
                self.test = test
                return result

        runner = FakeRunner()

        oldParseArgs = unittest.TestProgram.parseArgs
        def restoreParseArgs():
            unittest.TestProgram.parseArgs = oldParseArgs
        unittest.TestProgram.parseArgs = lambda *args: None
        self.addCleanup(restoreParseArgs)

        def removeTest():
            del unittest.TestProgram.test
        unittest.TestProgram.test = test
        self.addCleanup(removeTest)

        program = unittest.TestProgram(testRunner=runner, exit=False, verbosity=2)

        self.assertEqual(program.result, result)
        self.assertEqual(runner.test, test)
        self.assertEqual(program.verbosity, 2)

    class FooBar(unittest.TestCase):
        def testPass(self):
            assert True
        def testFail(self):
            assert False

    class FooBarLoader(unittest.TestLoader):
        """Test loader that returns a suite containing FooBar."""
        def loadTestsFromModule(self, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(Test_TestProgram.FooBar)])


    def test_NonExit(self):
        program = unittest.main(exit=False,
                                argv=["foobar"],
                                testRunner=unittest.TextTestRunner(stream=StringIO()),
                                testLoader=self.FooBarLoader())
        self.assertTrue(hasattr(program, 'result'))


    def test_Exit(self):
        self.assertRaises(
            SystemExit,
            unittest.main,
            argv=["foobar"],
            testRunner=unittest.TextTestRunner(stream=StringIO()),
            exit=True,
            testLoader=self.FooBarLoader())


    def test_ExitAsDefault(self):
        self.assertRaises(
            SystemExit,
            unittest.main,
            argv=["foobar"],
            testRunner=unittest.TextTestRunner(stream=StringIO()),
            testLoader=self.FooBarLoader())


if __name__ == '__main__':
    unittest.main()
